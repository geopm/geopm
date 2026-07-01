#!/usr/bin/env python3
#
#  Copyright (c) 2015 - 2026 Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#

import math
import unittest
from unittest import mock
from argparse import ArgumentParser
from argparse import Namespace

# Patch dlopen to allow the tests to run when there is no build
with mock.patch('cffi.FFI.dlopen', return_value=mock.MagicMock()):
    from geopmdpy import heatmap_agent
    from geopmdpy.heatmap_agent import HeatmapAgent
    from geopmdpy.session import get_parser


class TestHeatmapAgent(unittest.TestCase):
    def setUp(self):
        self._test_name = 'TestHeatmapAgent'

    def test_help_nonempty(self):
        self.assertIsInstance(HeatmapAgent().help(), str)
        self.assertTrue(HeatmapAgent().help())

    def test_update_parser_adds_options(self):
        agent = HeatmapAgent()
        parser = agent.update_parser(get_parser())
        args = parser.parse_args([])
        self.assertEqual('component_heatmap.png', args.heatmap_out)
        self.assertIsNone(args.fom)
        self.assertFalse(args.no_plot)
        self.assertIsNone(args.replot_csv)
        # The agent lowers the default sampling period.
        self.assertEqual(heatmap_agent._PERIOD_DEFAULT, args.period)
        args = parser.parse_args(['--heatmap-out', 'hm.png', '--fom', 'f.csv',
                                  '--no-plot', '--replot-csv', 'trace.csv'])
        self.assertEqual('hm.png', args.heatmap_out)
        self.assertEqual('f.csv', args.fom)
        self.assertTrue(args.no_plot)
        self.assertEqual('trace.csv', args.replot_csv)

    def test_update_parser_corrects_signal_config_help(self):
        # The agent supplies a default signal set, so the -i/--signal-config
        # help must not claim standard input is the default.
        agent = HeatmapAgent()
        parser = agent.update_parser(get_parser())
        help_text = next(action.help for action in parser._actions
                         if action.dest == 'config_path')
        self.assertNotIn('standard input', help_text)
        self.assertIn('built-in signal set', help_text)

    def test_update_args_sets_options(self):
        agent = HeatmapAgent()
        agent.update_args(Namespace(heatmap_out='hm.png', fom='f.csv',
                                    no_plot=True, replot_csv=None))
        self.assertEqual('hm.png', agent._out_path)
        self.assertEqual('f.csv', agent._fom_path)
        self.assertTrue(agent._no_plot)

    def test_signal_config_override_selects_available(self):
        # Only granted candidate signals appear, prefixed by TIME, and every
        # domain index is requested with the '*' wildcard.
        names = ['TIME', 'CPU_POWER', 'DRAM_POWER', 'CPU_FREQUENCY_STATUS']
        agent = HeatmapAgent()
        with mock.patch.object(heatmap_agent.pio, 'signal_names',
                               return_value=names):
            override = agent.signal_config_override()
        self.assertEqual('TIME board 0\n'
                         'CPU_POWER package *\n'
                         'CPU_FREQUENCY_STATUS package *\n'
                         'DRAM_POWER package *\n', override)

    def test_signal_config_override_no_signals_raises(self):
        agent = HeatmapAgent()
        with mock.patch.object(heatmap_agent.pio, 'signal_names',
                               return_value=['SOME_OTHER_SIGNAL']):
            with self.assertRaises(RuntimeError):
                agent.signal_config_override()

    def test_discover_signals_expands_all_domains(self):
        # Each granted candidate is expanded to one row per domain index, with
        # the domain index appended to the label when more than one exists.
        names = ['CPU_POWER', 'GPU_POWER']

        def fake_num_domain(domain):
            return {'package': 2, 'gpu': 3}.get(domain, 1)

        with mock.patch.object(heatmap_agent.pio, 'signal_names',
                               return_value=names), \
                mock.patch.object(heatmap_agent.topo, 'num_domain',
                                  side_effect=fake_num_domain):
            signals = HeatmapAgent._discover_signals()
        self.assertEqual(
            [('CPU_POWER', 'package', 0, 'CPU power pkg0'),
             ('CPU_POWER', 'package', 1, 'CPU power pkg1'),
             ('GPU_POWER', 'gpu', 0, 'GPU power gpu0'),
             ('GPU_POWER', 'gpu', 1, 'GPU power gpu1'),
             ('GPU_POWER', 'gpu', 2, 'GPU power gpu2')],
            signals)

    def test_ipc_derivation_and_trace(self):
        names = ['TIME', 'CPU_POWER', 'CPU_INSTRUCTIONS_RETIRED',
                 'CPU_CYCLES_THREAD']
        # push_signal hands out sequential handles mapped back to signal names.
        handle_name = {}

        def fake_push(name, domain, index):
            handle = len(handle_name)
            handle_name[handle] = name
            return handle

        # Two periods of samples per signal; counters advance by 200 instr /
        # 100 cycles between periods -> IPC == 2.0 on the second period.
        sample_values = {
            'TIME': [0.0, 0.05],
            'CPU_POWER': [50.0, 60.0],
            'CPU_INSTRUCTIONS_RETIRED': [1000.0, 1200.0],
            'CPU_CYCLES_THREAD': [500.0, 600.0],
        }
        cursor = {}

        def fake_sample(handle):
            name = handle_name[handle]
            index = cursor.get(handle, 0)
            cursor[handle] = index + 1
            return sample_values[name][index]

        agent = HeatmapAgent()
        agent.update_args(Namespace(heatmap_out='x.png', fom=None,
                                    no_plot=True, replot_csv=None))
        with mock.patch.object(heatmap_agent.pio, 'signal_names',
                               return_value=names), \
                mock.patch.object(heatmap_agent.pio, 'control_names',
                                  return_value=[]), \
                mock.patch.object(heatmap_agent.topo, 'num_domain',
                                  return_value=1), \
                mock.patch.object(heatmap_agent.pio, 'push_signal',
                                  side_effect=fake_push), \
                mock.patch.object(heatmap_agent.pio, 'sample',
                                  side_effect=fake_sample):
            agent.run_begin()
            self.assertEqual(['IPC-package-0'], agent.header_names())
            agent.update_loop()  # first period: no delta yet
            agent.update_loop()  # second period: IPC = 200 / 100

        self.assertEqual([2.0], agent._ipc[-1])
        self.assertEqual(['2.0000'], agent.trace_out())
        ipc_rows = agent._ipc_rows()
        self.assertEqual(1, len(ipc_rows))
        self.assertEqual('CPU IPC', ipc_rows[0][0])
        self.assertEqual(2.0, ipc_rows[0][1][-1])

    def test_ipc_is_per_package(self):
        names = ['TIME', 'CPU_POWER', 'CPU_INSTRUCTIONS_RETIRED',
                 'CPU_CYCLES_THREAD']
        handle_name = {}

        def fake_push(name, domain, index):
            handle = len(handle_name)
            handle_name[handle] = (name, index)
            return handle

        # package 0: 200 instr / 100 cycles -> IPC 2.0; package 1: 300 / 100.
        per_pkg = {
            ('CPU_POWER', 0): [50.0, 60.0],
            ('CPU_POWER', 1): [55.0, 65.0],
            ('CPU_INSTRUCTIONS_RETIRED', 0): [1000.0, 1200.0],
            ('CPU_INSTRUCTIONS_RETIRED', 1): [2000.0, 2300.0],
            ('CPU_CYCLES_THREAD', 0): [500.0, 600.0],
            ('CPU_CYCLES_THREAD', 1): [700.0, 800.0],
            ('TIME', 0): [0.0, 0.05],
        }
        cursor = {}

        def fake_sample(handle):
            key = handle_name[handle]
            index = cursor.get(handle, 0)
            cursor[handle] = index + 1
            return per_pkg[key][index]

        agent = HeatmapAgent()
        agent.update_args(Namespace(heatmap_out='x.png', fom=None,
                                    no_plot=True, replot_csv=None))
        with mock.patch.object(heatmap_agent.pio, 'signal_names',
                               return_value=names), \
                mock.patch.object(heatmap_agent.pio, 'control_names',
                                  return_value=[]), \
                mock.patch.object(heatmap_agent.topo, 'num_domain',
                                  return_value=2), \
                mock.patch.object(heatmap_agent.pio, 'push_signal',
                                  side_effect=fake_push), \
                mock.patch.object(heatmap_agent.pio, 'sample',
                                  side_effect=fake_sample):
            agent.run_begin()
            self.assertEqual(['IPC-package-0', 'IPC-package-1'],
                             agent.header_names())
            agent.update_loop()  # first period: no delta yet
            agent.update_loop()  # second period

        self.assertEqual([2.0, 3.0], agent._ipc[-1])
        ipc_rows = agent._ipc_rows()
        self.assertEqual(['CPU IPC pkg0', 'CPU IPC pkg1'],
                         [label for label, _ in ipc_rows])
        self.assertEqual(2.0, ipc_rows[0][1][-1])
        self.assertEqual(3.0, ipc_rows[1][1][-1])

    def test_header_names_before_run_begin(self):
        # Regression: the session calls header_names() BEFORE run_begin(), so
        # the IPC columns must resolve without relying on run_begin, otherwise
        # the agent-derived IPC columns never reach the -o trace.
        names = ['TIME', 'CPU_POWER', 'CPU_INSTRUCTIONS_RETIRED',
                 'CPU_CYCLES_THREAD']
        agent = HeatmapAgent()
        with mock.patch.object(heatmap_agent.pio, 'signal_names',
                               return_value=names), \
                mock.patch.object(heatmap_agent.topo, 'num_domain',
                                  return_value=2):
            # No run_begin() call here, mirroring the session's ordering.
            self.assertEqual(['IPC-package-0', 'IPC-package-1'],
                             agent.header_names())

    def test_run_begin_enables_fixed_counters(self):
        # The fixed counters backing IPC can be left disabled on the platform,
        # so run_begin must enable them (snapshotting first for auto-revert)
        # for every enable control and every domain index.
        names = ['TIME', 'CPU_POWER', 'CPU_INSTRUCTIONS_RETIRED',
                 'CPU_CYCLES_THREAD']
        agent = HeatmapAgent()
        with mock.patch.object(heatmap_agent.pio, 'signal_names',
                               return_value=names), \
                mock.patch.object(heatmap_agent.pio, 'control_names',
                                  return_value=list(
                                      heatmap_agent._IPC_ENABLE_CONTROLS)), \
                mock.patch.object(heatmap_agent.pio, 'control_domain_type',
                                  return_value='cpu'), \
                mock.patch.object(heatmap_agent.topo, 'num_domain',
                                  side_effect=lambda d: 2 if d == 'cpu' else 1), \
                mock.patch.object(heatmap_agent.pio, 'push_signal',
                                  return_value=0), \
                mock.patch.object(heatmap_agent.pio, 'save_control') as save, \
                mock.patch.object(heatmap_agent.pio, 'write_control') as write:
            agent.run_begin()

        save.assert_called_once()
        # Every enable control written to both CPU domain indices with value 1.
        written = {(c.args[0], c.args[2]) for c in write.call_args_list}
        expected = {(name, idx)
                    for name in heatmap_agent._IPC_ENABLE_CONTROLS
                    for idx in (0, 1)}
        self.assertEqual(expected, written)
        self.assertTrue(all(c.args[3] == 1.0 for c in write.call_args_list))
        self.assertTrue(agent._counters_enabled)

    def test_run_begin_without_enable_controls_skips(self):
        # When the enable controls are absent (e.g. no MSR access) run_begin
        # must not attempt any control write and must not snapshot controls.
        names = ['TIME', 'CPU_POWER', 'CPU_INSTRUCTIONS_RETIRED',
                 'CPU_CYCLES_THREAD']
        agent = HeatmapAgent()
        with mock.patch.object(heatmap_agent.pio, 'signal_names',
                               return_value=names), \
                mock.patch.object(heatmap_agent.pio, 'control_names',
                                  return_value=[]), \
                mock.patch.object(heatmap_agent.topo, 'num_domain',
                                  return_value=1), \
                mock.patch.object(heatmap_agent.pio, 'push_signal',
                                  return_value=0), \
                mock.patch.object(heatmap_agent.pio, 'save_control') as save, \
                mock.patch.object(heatmap_agent.pio, 'write_control') as write:
            agent.run_begin()

        save.assert_not_called()
        write.assert_not_called()
        self.assertFalse(agent._counters_enabled)

    def test_sanitize_unit_ignores_out_of_range(self):
        # Unbounded signals pass through unchanged.
        self.assertEqual(1234.0, heatmap_agent._sanitize_unit(1234.0, False))
        # Bounded signals keep in-range values, including the endpoints.
        self.assertEqual(0.0, heatmap_agent._sanitize_unit(0.0, True))
        self.assertEqual(0.5, heatmap_agent._sanitize_unit(0.5, True))
        self.assertEqual(1.0, heatmap_agent._sanitize_unit(1.0, True))
        # Out-of-range readings for bounded signals become NaN (ignored).
        self.assertTrue(math.isnan(heatmap_agent._sanitize_unit(10090.0, True)))
        self.assertTrue(math.isnan(heatmap_agent._sanitize_unit(-0.1, True)))

    def test_update_loop_ignores_out_of_range_activity(self):
        # A GPU_UTILIZATION reading outside [0, 1] is stored as NaN so it is
        # dropped by the normalization instead of clipped.
        names = ['TIME', 'GPU_UTILIZATION']
        handle_name = {}

        def fake_push(name, domain, index):
            handle = len(handle_name)
            handle_name[handle] = name
            return handle

        values = {'TIME': [0.0], 'GPU_UTILIZATION': [10090.0]}
        cursor = {}

        def fake_sample(handle):
            name = handle_name[handle]
            index = cursor.get(handle, 0)
            cursor[handle] = index + 1
            return values[name][index]

        agent = HeatmapAgent()
        with mock.patch.object(heatmap_agent.pio, 'signal_names',
                               return_value=names), \
                mock.patch.object(heatmap_agent.pio, 'control_names',
                                  return_value=[]), \
                mock.patch.object(heatmap_agent.topo, 'num_domain',
                                  return_value=1), \
                mock.patch.object(heatmap_agent.pio, 'push_signal',
                                  side_effect=fake_push), \
                mock.patch.object(heatmap_agent.pio, 'sample',
                                  side_effect=fake_sample):
            agent.run_begin()
            agent.update_loop()

        self.assertTrue(math.isnan(agent._rows[-1][0]))

    def test_delta_ipc_clips_spikes(self):
        # A tiny positive cycle delta must not yield an unbounded IPC spike.
        self.assertEqual(heatmap_agent._IPC_MAX,
                         heatmap_agent._delta_ipc(1e9, 1.0))
        self.assertEqual(2.0, heatmap_agent._delta_ipc(200.0, 100.0))
        self.assertTrue(heatmap_agent._delta_ipc(200.0, 0.0) != # NaN guard
                        heatmap_agent._delta_ipc(200.0, 0.0))

    def test_replot_csv_renders_and_exits(self):
        agent = HeatmapAgent()
        loaded = ([0.0, 0.05], ['CPU power'], [[50.0], [60.0]], None)
        with mock.patch.object(heatmap_agent, '_load_trace',
                               return_value=loaded) as load_mock, \
                mock.patch.object(heatmap_agent, '_render_heatmap') as render_mock:
            with self.assertRaises(SystemExit) as ctx:
                agent.update_args(Namespace(heatmap_out='hm.png', fom='f.csv',
                                            no_plot=False, replot_csv='trace.csv'))
        self.assertEqual(0, ctx.exception.code)
        load_mock.assert_called_once_with('trace.csv')
        render_mock.assert_called_once_with([0.0, 0.05], ['CPU power'],
                                            [[50.0], [60.0]], None, 'f.csv',
                                            'hm.png')

    def test_match_column_prefers_domain_suffix(self):
        columns = ['TIME', 'CPU_POWER-package-0', 'DRAM_POWER']
        self.assertEqual('CPU_POWER-package-0',
                         heatmap_agent._match_column(columns, 'CPU_POWER',
                                                     'package', 0))
        # Board-domain signals are written without a suffix.
        self.assertEqual('DRAM_POWER',
                         heatmap_agent._match_column(columns, 'DRAM_POWER',
                                                     'package', 0))
        self.assertIsNone(heatmap_agent._match_column(columns, 'GPU_POWER',
                                                      'board', 0))

    def test_signal_columns_orders_all_indices(self):
        columns = ['TIME', 'CPU_POWER-package-1', 'CPU_POWER-package-0',
                   'GPU_POWER-gpu-0', 'GPU_POWER-gpu-1', 'DRAM_POWER']
        self.assertEqual(
            [('package', 0, 'CPU_POWER-package-0'),
             ('package', 1, 'CPU_POWER-package-1')],
            heatmap_agent._signal_columns(columns, 'CPU_POWER'))
        self.assertEqual(
            [('gpu', 0, 'GPU_POWER-gpu-0'), ('gpu', 1, 'GPU_POWER-gpu-1')],
            heatmap_agent._signal_columns(columns, 'GPU_POWER'))
        # Board-domain signals are written without a suffix.
        self.assertEqual([('board', 0, 'DRAM_POWER')],
                         heatmap_agent._signal_columns(columns, 'DRAM_POWER'))

    def test_header_and_trace_without_counters(self):
        names = ['TIME', 'CPU_POWER']
        agent = HeatmapAgent()
        with mock.patch.object(heatmap_agent.pio, 'signal_names',
                               return_value=names), \
                mock.patch.object(heatmap_agent.topo, 'num_domain',
                                  return_value=1), \
                mock.patch.object(heatmap_agent.pio, 'push_signal',
                                  side_effect=lambda *a: 0), \
                mock.patch.object(heatmap_agent.pio, 'sample',
                                  return_value=1.0):
            agent.run_begin()
            self.assertEqual([], agent.header_names())
            self.assertEqual([], agent.trace_out())


if __name__ == '__main__':
    unittest.main()
