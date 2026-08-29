#!/usr/bin/env python3
#
#  Copyright (c) 2015 - 2026 Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#

import unittest
from unittest import mock
from argparse import ArgumentParser
from argparse import Namespace

# Patch dlopen to allow the tests to run when there is no build
with mock.patch('cffi.FFI.dlopen', return_value=mock.MagicMock()):
    from geopmdpy import cpu_activity_agent
    from geopmdpy.cpu_activity_agent import CPUActivityAgent

# Domain type values used by the mocked topo module.  The exact numbers do
# not matter as long as the board/package/core domains are distinct and the
# coarser domains are numbered smaller (as in GEOPM).
_DOMAIN_BOARD = 0
_DOMAIN_PACKAGE = 1
_DOMAIN_CORE = 2

# Static frequency characterization shared by most tests.
_CORE_MIN = 1.0e9
_CORE_MAX = 3.0e9
_UNCORE_MIN = 1.0e9
_UNCORE_MAX = 2.4e9

# Default uncore bandwidth characterization pairs (FREQ:BW).
_BW_PAIRS = ['1.0e9:1.0e11', '2.0e9:2.0e11']


class TestCPUActivityAgent(unittest.TestCase):
    def setUp(self):
        self._test_name = 'TestCPUActivityAgent'

        self._signal_names = {
            'MSR::CPU_SCALABILITY_RATIO',
            'MSR::QM_CTR_SCALED_RATE',
            'CPU_FREQUENCY_STATUS',
            'CPU_UNCORE_FREQUENCY_STATUS',
            'TIME',
        }
        # control_domain_type() and signal_domain_type() both resolve to the
        # core domain so the agent's core control domain is DOMAIN_CORE.
        self._control_domain = _DOMAIN_CORE
        self._signal_domain = _DOMAIN_CORE
        # Number of domains reported by topo.num_domain() per domain type.
        self._domain_counts = {_DOMAIN_CORE: 1, _DOMAIN_PACKAGE: 1}
        self._read_values = {
            'CPU_FREQUENCY_MIN_AVAIL': _CORE_MIN,
            'CPU_FREQUENCY_MAX_AVAIL': _CORE_MAX,
            'CPU_UNCORE_FREQUENCY_MIN_CONTROL': _UNCORE_MIN,
            'CPU_UNCORE_FREQUENCY_MAX_CONTROL': _UNCORE_MAX,
        }
        # Values returned by pio.sample(), keyed by the token returned from
        # push_signal()/push_control() ("<name>:<domain_idx>").
        self._sample_values = {}

        self._patch(mock.patch.object(cpu_activity_agent.topo, 'DOMAIN_BOARD', _DOMAIN_BOARD))
        self._patch(mock.patch.object(cpu_activity_agent.topo, 'DOMAIN_PACKAGE', _DOMAIN_PACKAGE))
        self._patch(mock.patch.object(cpu_activity_agent.topo, 'DOMAIN_CORE', _DOMAIN_CORE))

        self._num_domain = self._patch(
            mock.patch.object(cpu_activity_agent.topo, 'num_domain',
                              side_effect=self._num_domain_impl))
        self._domain_name = self._patch(
            mock.patch.object(cpu_activity_agent.topo, 'domain_name',
                              return_value='core'))

        self._signal_names_mock = self._patch(
            mock.patch.object(cpu_activity_agent.pio, 'signal_names',
                              side_effect=lambda: set(self._signal_names)))
        self._control_domain_type = self._patch(
            mock.patch.object(cpu_activity_agent.pio, 'control_domain_type',
                              side_effect=lambda name: self._control_domain))
        self._signal_domain_type = self._patch(
            mock.patch.object(cpu_activity_agent.pio, 'signal_domain_type',
                              side_effect=lambda name: self._signal_domain))
        self._push_signal = self._patch(
            mock.patch.object(cpu_activity_agent.pio, 'push_signal',
                              side_effect=lambda name, dom, idx: f'{name}:{idx}'))
        self._push_control = self._patch(
            mock.patch.object(cpu_activity_agent.pio, 'push_control',
                              side_effect=lambda name, dom, idx: f'{name}:{idx}'))
        self._read_signal = self._patch(
            mock.patch.object(cpu_activity_agent.pio, 'read_signal',
                              side_effect=lambda name, dom, idx: self._read_values[name]))
        self._sample = self._patch(
            mock.patch.object(cpu_activity_agent.pio, 'sample',
                              side_effect=lambda token: self._sample_values[token]))
        self._adjust = self._patch(
            mock.patch.object(cpu_activity_agent.pio, 'adjust'))
        self._write_batch = self._patch(
            mock.patch.object(cpu_activity_agent.pio, 'write_batch'))
        self._write_control = self._patch(
            mock.patch.object(cpu_activity_agent.pio, 'write_control'))
        self._save_control = self._patch(
            mock.patch.object(cpu_activity_agent.pio, 'save_control'))

    def _patch(self, patcher):
        started = patcher.start()
        self.addCleanup(patcher.stop)
        return started

    def _num_domain_impl(self, domain):
        return self._domain_counts.get(domain, 1)

    def _make_agent(self, phi=0.5, cpu_freq_efficient=None,
                    cpu_uncore_freq_efficient=None, cpu_uncore_bandwidth=None,
                    hi_res=False):
        agent = CPUActivityAgent()
        agent.update_args(Namespace(
            phi=phi,
            cpu_freq_efficient=cpu_freq_efficient,
            cpu_uncore_freq_efficient=cpu_uncore_freq_efficient,
            cpu_uncore_bandwidth=cpu_uncore_bandwidth,
            hi_res=hi_res))
        return agent

    def _adjust_values(self):
        return [call.args[1] for call in self._adjust.call_args_list]

    # ---- argument handling ------------------------------------------------

    def test_update_parser_adds_args(self):
        agent = CPUActivityAgent()
        parser = agent.update_parser(ArgumentParser())
        args = parser.parse_args([
            '--phi', '0.7',
            '--cpu-freq-efficient', '1.5e9',
            '--cpu-uncore-freq-efficient', '1.2e9',
            '--cpu-uncore-bandwidth', '1.0e9:1.0e11',
            '--cpu-uncore-bandwidth', '2.0e9:2.0e11'])
        self.assertAlmostEqual(0.7, args.phi)
        self.assertAlmostEqual(1.5e9, args.cpu_freq_efficient)
        self.assertAlmostEqual(1.2e9, args.cpu_uncore_freq_efficient)
        self.assertEqual(['1.0e9:1.0e11', '2.0e9:2.0e11'],
                         args.cpu_uncore_bandwidth)

    def test_update_parser_defaults(self):
        agent = CPUActivityAgent()
        parser = agent.update_parser(ArgumentParser())
        args = parser.parse_args([])
        self.assertAlmostEqual(0.5, args.phi)
        self.assertIsNone(args.cpu_freq_efficient)
        self.assertIsNone(args.cpu_uncore_freq_efficient)
        self.assertIsNone(args.cpu_uncore_bandwidth)

    def test_update_parser_hi_res(self):
        agent = CPUActivityAgent()
        parser = agent.update_parser(ArgumentParser())
        self.assertFalse(parser.parse_args([]).hi_res)
        self.assertTrue(parser.parse_args(['--hi-res']).hi_res)

    def test_update_args_valid_phi(self):
        for phi in (0.0, 0.25, 0.5, 0.75, 1.0):
            agent = self._make_agent(phi=phi)
            self.assertAlmostEqual(phi, agent._phi)

    def test_update_args_phi_out_of_range_high(self):
        with self.assertRaisesRegex(RuntimeError, 'phi value out of range'):
            self._make_agent(phi=1.5)

    def test_update_args_phi_out_of_range_low(self):
        with self.assertRaisesRegex(RuntimeError, 'phi value out of range'):
            self._make_agent(phi=-0.1)

    def test_update_args_phi_nan(self):
        with self.assertRaisesRegex(RuntimeError, 'phi value out of range'):
            self._make_agent(phi=float('nan'))

    def test_update_args_bandwidth_builds_map(self):
        agent = self._make_agent(cpu_uncore_bandwidth=list(_BW_PAIRS))
        self.assertTrue(agent._uncore_enabled)
        self.assertEqual({1.0e9: 1.0e11, 2.0e9: 2.0e11},
                         agent._uncore_bandwidth_map)
        self.assertEqual([1.0e9, 2.0e9], agent._uncore_bandwidth_freqs)

    def test_update_args_no_bandwidth_core_only(self):
        agent = self._make_agent()
        self.assertFalse(agent._uncore_enabled)
        self.assertEqual({}, agent._uncore_bandwidth_map)

    def test_update_args_bandwidth_malformed(self):
        for bad in (['no_colon'], ['1.0e9:bad'], ['bad:1.0e11'],
                    ['1.0e9:2.0e11:3'], ['-1.0e9:2.0e11'], ['1.0e9:0']):
            with self.assertRaisesRegex(RuntimeError, 'cpu-uncore-bandwidth'):
                self._make_agent(cpu_uncore_bandwidth=bad)

    def test_update_args_efficient_negative(self):
        with self.assertRaisesRegex(RuntimeError, 'cpu-freq-efficient'):
            self._make_agent(cpu_freq_efficient=-1.0e9)
        with self.assertRaisesRegex(RuntimeError, 'cpu-uncore-freq-efficient'):
            self._make_agent(cpu_uncore_freq_efficient=-1.0e9)

    def test_help_nonempty(self):
        self.assertIsInstance(CPUActivityAgent().help(), str)
        self.assertTrue(CPUActivityAgent().help())

    # ---- signal_config_override -------------------------------------------

    def test_signal_config_override(self):
        agent = self._make_agent()
        override = agent.signal_config_override()
        self.assertIn('TIME board 0', override)
        self.assertIn('MSR::CPU_SCALABILITY_RATIO board 0', override)
        self.assertIn('MSR::QM_CTR_SCALED_RATE board 0', override)
        self.assertIn('CPU_FREQUENCY_STATUS board 0', override)
        self.assertIn('CPU_UNCORE_FREQUENCY_STATUS board 0', override)
        self.assertTrue(override.endswith('\n'))

    def test_signal_config_override_hi_res(self):
        agent = self._make_agent(hi_res=True)
        override = agent.signal_config_override()
        # TIME stays board-level; other signals use the wildcard domain/index.
        self.assertIn('TIME board 0', override)
        self.assertIn('MSR::CPU_SCALABILITY_RATIO * *', override)
        self.assertIn('MSR::QM_CTR_SCALED_RATE * *', override)
        self.assertIn('CPU_FREQUENCY_STATUS * *', override)
        self.assertIn('CPU_UNCORE_FREQUENCY_STATUS * *', override)

    def test_signal_config_override_subset(self):
        self._signal_names = {'MSR::CPU_SCALABILITY_RATIO', 'TIME'}
        agent = self._make_agent()
        override = agent.signal_config_override()
        self.assertIn('MSR::CPU_SCALABILITY_RATIO board 0', override)
        self.assertNotIn('MSR::QM_CTR_SCALED_RATE', override)
        self.assertNotIn('CPU_UNCORE_FREQUENCY_STATUS', override)

    # ---- run_begin --------------------------------------------------------

    def test_run_begin_no_core(self):
        self._domain_counts[_DOMAIN_CORE] = 0
        agent = self._make_agent()
        with self.assertRaisesRegex(RuntimeError, 'at least one CPU core'):
            agent.run_begin()

    def test_run_begin_core_only(self):
        agent = self._make_agent()
        agent.run_begin()
        # Core signal and controls pushed.
        pushed_signals = [c.args[0] for c in self._push_signal.call_args_list]
        pushed_controls = [c.args[0] for c in self._push_control.call_args_list]
        self.assertIn('MSR::CPU_SCALABILITY_RATIO', pushed_signals)
        self.assertIn('CPU_FREQUENCY_MIN_CONTROL', pushed_controls)
        self.assertIn('CPU_FREQUENCY_MAX_CONTROL', pushed_controls)
        # No uncore signals/controls, no RMID/QM event writes.
        self.assertNotIn('MSR::QM_CTR_SCALED_RATE', pushed_signals)
        self.assertNotIn('CPU_UNCORE_FREQUENCY_MIN_CONTROL', pushed_controls)
        self._write_control.assert_not_called()
        self._save_control.assert_called_once()
        self.assertAlmostEqual(_CORE_MIN, agent._freq_core_efficient)

    def test_run_begin_core_efficient_arg(self):
        agent = self._make_agent(cpu_freq_efficient=1.8e9)
        agent.run_begin()
        self.assertAlmostEqual(1.8e9, agent._freq_core_efficient)

    def test_run_begin_core_efficient_out_of_range(self):
        agent = self._make_agent(cpu_freq_efficient=5.0e9)
        with self.assertRaisesRegex(RuntimeError,
                                    'core efficient frequency out of range'):
            agent.run_begin()

    def test_run_begin_uncore_enabled(self):
        agent = self._make_agent(cpu_uncore_bandwidth=list(_BW_PAIRS))
        agent.run_begin()
        pushed_signals = [c.args[0] for c in self._push_signal.call_args_list]
        pushed_controls = [c.args[0] for c in self._push_control.call_args_list]
        self.assertIn('MSR::QM_CTR_SCALED_RATE', pushed_signals)
        self.assertIn('CPU_UNCORE_FREQUENCY_STATUS', pushed_signals)
        self.assertIn('CPU_UNCORE_FREQUENCY_MIN_CONTROL', pushed_controls)
        self.assertIn('CPU_UNCORE_FREQUENCY_MAX_CONTROL', pushed_controls)
        # RMID / QM event-selection controls written for bandwidth monitoring.
        written = [c.args[0] for c in self._write_control.call_args_list]
        self.assertIn('MSR::PQR_ASSOC:RMID', written)
        self.assertIn('MSR::QM_EVTSEL:RMID', written)
        self.assertIn('MSR::QM_EVTSEL:EVENT_ID', written)
        self.assertAlmostEqual(_UNCORE_MIN, agent._freq_uncore_efficient)

    def test_run_begin_uncore_efficient_out_of_range(self):
        agent = self._make_agent(cpu_uncore_freq_efficient=5.0e9,
                                 cpu_uncore_bandwidth=list(_BW_PAIRS))
        with self.assertRaisesRegex(RuntimeError,
                                    'uncore efficient frequency out of range'):
            agent.run_begin()

    # ---- update_loop: core ------------------------------------------------

    def _set_core_samples(self, values):
        """values: list of scalability per core domain index."""
        for idx, val in enumerate(values):
            self._sample_values[f'MSR::CPU_SCALABILITY_RATIO:{idx}'] = val

    def test_update_loop_core_phi_neutral(self):
        agent = self._make_agent(phi=0.5)
        agent.run_begin()
        self._set_core_samples([0.5])
        agent.update_loop()
        # eff=1e9, max=3e9, range=2e9; req = 1e9 + 2e9*0.5 = 2e9
        for value in self._adjust_values():
            self.assertAlmostEqual(2.0e9, value, delta=1.0)
        self._write_batch.assert_called_once()

    def test_update_loop_core_scalability_nan(self):
        agent = self._make_agent(phi=0.5)
        agent.run_begin()
        self._set_core_samples([float('nan')])
        agent.update_loop()
        # NaN scalability -> 1.0; req = 1e9 + 2e9 = 3e9 (== max)
        for value in self._adjust_values():
            self.assertAlmostEqual(3.0e9, value, delta=1.0)

    def test_update_loop_core_energy_bias(self):
        agent = self._make_agent(phi=0.75)
        agent.run_begin()
        self._set_core_samples([1.0])
        agent.update_loop()
        # resolved_max = 3e9 - 2e9*0.5 = 2e9; eff = 1e9; range = 1e9
        # req = 1e9 + 1e9*1.0 = 2e9
        for value in self._adjust_values():
            self.assertAlmostEqual(2.0e9, value, delta=1.0)

    def test_update_loop_core_performance_bias(self):
        agent = self._make_agent(phi=0.25)
        agent.run_begin()
        self._set_core_samples([0.0])
        agent.update_loop()
        # resolved_eff = 1e9 + 2e9*0.5 = 2e9; max = 3e9
        # req = 2e9 + range*0.0 = 2e9
        for value in self._adjust_values():
            self.assertAlmostEqual(2.0e9, value, delta=1.0)

    def test_update_loop_core_clamp_counts_clip(self):
        agent = self._make_agent(phi=0.5)
        agent.run_begin()
        self._set_core_samples([2.0])
        agent.update_loop()
        # req = 1e9 + 2e9*2.0 = 5e9 -> clamped to max 3e9
        for value in self._adjust_values():
            self.assertAlmostEqual(3.0e9, value, delta=1.0)
        self.assertEqual(1, agent._core_frequency_clipped)

    def test_update_loop_core_no_write_when_unchanged(self):
        agent = self._make_agent(phi=0.5)
        agent.run_begin()
        self._set_core_samples([0.5])
        agent.update_loop()
        first = self._adjust.call_count
        self._write_batch.reset_mock()
        agent.update_loop()
        self.assertEqual(first, self._adjust.call_count)
        self._write_batch.assert_not_called()

    def test_update_loop_multi_core(self):
        self._domain_counts[_DOMAIN_CORE] = 2
        agent = self._make_agent(phi=0.5)
        agent.run_begin()
        self._set_core_samples([0.5, 0.5])
        agent.update_loop()
        # Two core domains, each writes min and max -> 4 adjusts.
        self.assertEqual(4, self._adjust.call_count)
        self.assertEqual(2, agent._core_frequency_requests)

    # ---- update_loop: uncore ----------------------------------------------

    def _set_uncore_samples(self, status_values, qm_values):
        for idx, val in enumerate(status_values):
            self._sample_values[f'CPU_UNCORE_FREQUENCY_STATUS:{idx}'] = val
        for idx, val in enumerate(qm_values):
            self._sample_values[f'MSR::QM_CTR_SCALED_RATE:{idx}'] = val

    def test_update_loop_uncore_bandwidth_exact_key(self):
        agent = self._make_agent(phi=0.5, cpu_uncore_bandwidth=list(_BW_PAIRS))
        agent.run_begin()
        self._set_core_samples([0.0])
        # status 2.0e9 -> exact key -> max_bw 2.0e11; qm 1.0e11 -> scal 0.5
        self._set_uncore_samples([2.0e9], [1.0e11])
        agent.update_loop()
        # uncore eff=1e9, max=2.4e9, range=1.4e9; req = 1e9 + 1.4e9*0.5 = 1.7e9
        uncore_values = [c.args[1] for c in self._adjust.call_args_list
                         if c.args[0].startswith('CPU_UNCORE_FREQUENCY')]
        self.assertTrue(uncore_values)
        for value in uncore_values:
            self.assertAlmostEqual(1.7e9, value, delta=1.0)

    def test_update_loop_uncore_bandwidth_between_keys(self):
        agent = self._make_agent(phi=0.5, cpu_uncore_bandwidth=list(_BW_PAIRS))
        agent.run_begin()
        self._set_core_samples([0.0])
        # status 1.5e9 -> largest key <= 1.5e9 is 1.0e9 -> max_bw 1.0e11
        # qm 0.5e11 -> scal 0.5 -> req = 1e9 + 1.4e9*0.5 = 1.7e9
        self._set_uncore_samples([1.5e9], [0.5e11])
        agent.update_loop()
        uncore_values = [c.args[1] for c in self._adjust.call_args_list
                         if c.args[0].startswith('CPU_UNCORE_FREQUENCY')]
        for value in uncore_values:
            self.assertAlmostEqual(1.7e9, value, delta=1.0)

    def test_update_loop_uncore_clamp_counts_clip(self):
        agent = self._make_agent(phi=0.5, cpu_uncore_bandwidth=list(_BW_PAIRS))
        agent.run_begin()
        self._set_core_samples([0.0])
        # qm hugely exceeds max_bw -> scalability > 1 -> request clamped to max
        self._set_uncore_samples([2.0e9], [1.0e12])
        agent.update_loop()
        uncore_values = [c.args[1] for c in self._adjust.call_args_list
                         if c.args[0].startswith('CPU_UNCORE_FREQUENCY')]
        for value in uncore_values:
            self.assertAlmostEqual(_UNCORE_MAX, value, delta=1.0)
        self.assertEqual(1, agent._uncore_frequency_clipped)

    def test_update_loop_core_only_leaves_uncore_untouched(self):
        agent = self._make_agent()
        agent.run_begin()
        self._set_core_samples([0.5])
        agent.update_loop()
        self.assertFalse(agent._uncore_enabled)
        uncore_values = [c.args[1] for c in self._adjust.call_args_list
                         if c.args[0].startswith('CPU_UNCORE_FREQUENCY')]
        self.assertEqual([], uncore_values)


if __name__ == '__main__':
    unittest.main()
