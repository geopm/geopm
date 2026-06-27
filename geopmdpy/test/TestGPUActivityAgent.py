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
    from geopmdpy import gpu_activity_agent
    from geopmdpy.gpu_activity_agent import GPUActivityAgent
    from geopmdpy.session import get_parser

# Domain type values used by the mocked topo module.  The exact numbers
# do not matter as long as the GPU/GPU_CHIP domains are coarser-numbered
# (smaller) consistently and distinct from the board domain.
_DOMAIN_BOARD = 0
_DOMAIN_GPU = 8
_DOMAIN_GPU_CHIP = 9

_FE_CONSTCONFIG = gpu_activity_agent._FE_CONSTCONFIG
_FE_SIG_NAME = gpu_activity_agent._FE_SIG_NAME

# Static frequency characterization shared by most tests.
_FREQ_MIN = 0.5e9
_FREQ_MAX = 1.5e9
_FREQ_EFFICIENT = 1.0e9


class TestGPUActivityAgent(unittest.TestCase):
    def setUp(self):
        self._test_name = 'TestGPUActivityAgent'

        # Configurable inputs that individual tests may override before
        # calling run_begin()/update_loop().
        self._signal_names = {
            'GPU_CORE_ACTIVITY',
            'GPU_UTILIZATION',
            'GPU_CORE_FREQUENCY_STATUS',
            'DRM::IDLE_RESIDENCY',
            'TIME',
            _FE_CONSTCONFIG,
        }
        self._signal_domain = _DOMAIN_GPU
        self._control_domain = _DOMAIN_GPU
        # Number of domains reported by topo.num_domain() per domain type.
        self._domain_counts = {_DOMAIN_GPU: 1, _DOMAIN_GPU_CHIP: 1}
        self._read_values = {
            'GPU_CORE_FREQUENCY_MIN_AVAIL': _FREQ_MIN,
            'GPU_CORE_FREQUENCY_MAX_AVAIL': _FREQ_MAX,
            _FE_CONSTCONFIG: _FREQ_EFFICIENT,
            _FE_SIG_NAME: _FREQ_EFFICIENT,
        }
        # Values returned by pio.sample(), keyed by the token returned
        # from push_signal()/push_control() ("<name>:<domain_idx>").
        self._sample_values = {}

        # Patch the topo domain-type constants to real integers (under the
        # dlopen mock they would otherwise be MagicMock attributes).
        self._patch(mock.patch.object(gpu_activity_agent.topo, 'DOMAIN_BOARD', _DOMAIN_BOARD))
        self._patch(mock.patch.object(gpu_activity_agent.topo, 'DOMAIN_GPU', _DOMAIN_GPU))
        self._patch(mock.patch.object(gpu_activity_agent.topo, 'DOMAIN_GPU_CHIP', _DOMAIN_GPU_CHIP))

        self._num_domain = self._patch(
            mock.patch.object(gpu_activity_agent.topo, 'num_domain',
                              side_effect=self._num_domain_impl))
        self._domain_name = self._patch(
            mock.patch.object(gpu_activity_agent.topo, 'domain_name',
                              return_value='gpu'))

        self._signal_names_mock = self._patch(
            mock.patch.object(gpu_activity_agent.pio, 'signal_names',
                              side_effect=lambda: set(self._signal_names)))
        self._control_domain_type = self._patch(
            mock.patch.object(gpu_activity_agent.pio, 'control_domain_type',
                              side_effect=lambda name: self._control_domain))
        self._signal_domain_type = self._patch(
            mock.patch.object(gpu_activity_agent.pio, 'signal_domain_type',
                              side_effect=lambda name: self._signal_domain))
        self._push_signal = self._patch(
            mock.patch.object(gpu_activity_agent.pio, 'push_signal',
                              side_effect=lambda name, dom, idx: f'{name}:{idx}'))
        self._push_control = self._patch(
            mock.patch.object(gpu_activity_agent.pio, 'push_control',
                              side_effect=lambda name, dom, idx: f'{name}:{idx}'))
        self._read_signal = self._patch(
            mock.patch.object(gpu_activity_agent.pio, 'read_signal',
                              side_effect=lambda name, dom, idx: self._read_values[name]))
        self._sample = self._patch(
            mock.patch.object(gpu_activity_agent.pio, 'sample',
                              side_effect=lambda token: self._sample_values[token]))
        self._adjust = self._patch(
            mock.patch.object(gpu_activity_agent.pio, 'adjust'))
        self._write_batch = self._patch(
            mock.patch.object(gpu_activity_agent.pio, 'write_batch'))
        self._save_control = self._patch(
            mock.patch.object(gpu_activity_agent.pio, 'save_control'))

    def _patch(self, patcher):
        started = patcher.start()
        self.addCleanup(patcher.stop)
        return started

    def _num_domain_impl(self, domain):
        return self._domain_counts.get(domain, 1)

    # ---- argument handling ------------------------------------------------

    def test_update_parser_adds_phi(self):
        agent = GPUActivityAgent()
        parser = agent.update_parser(ArgumentParser())
        args = parser.parse_args(['--phi', '0.7'])
        self.assertAlmostEqual(0.7, args.phi)

    def test_update_parser_phi_default(self):
        agent = GPUActivityAgent()
        parser = agent.update_parser(ArgumentParser())
        args = parser.parse_args([])
        self.assertAlmostEqual(0.5, args.phi)

    def test_update_parser_hi_res(self):
        agent = GPUActivityAgent()
        parser = agent.update_parser(ArgumentParser())
        self.assertFalse(parser.parse_args([]).hi_res)
        self.assertTrue(parser.parse_args(['--hi-res']).hi_res)

    def test_update_args_valid(self):
        agent = GPUActivityAgent()
        for phi in (0.0, 0.25, 0.5, 0.75, 1.0):
            agent.update_args(Namespace(phi=phi))
            self.assertAlmostEqual(phi, agent._phi)

    def test_update_args_out_of_range_high(self):
        agent = GPUActivityAgent()
        with self.assertRaisesRegex(RuntimeError, 'out of range'):
            agent.update_args(Namespace(phi=1.5))

    def test_update_args_out_of_range_low(self):
        agent = GPUActivityAgent()
        with self.assertRaisesRegex(RuntimeError, 'out of range'):
            agent.update_args(Namespace(phi=-0.1))

    def test_update_args_nan(self):
        agent = GPUActivityAgent()
        with self.assertRaisesRegex(RuntimeError, 'out of range'):
            agent.update_args(Namespace(phi=float('nan')))

    def test_update_parser_overrides_default_period(self):
        # The session default of 100 ms is replaced with the 20 ms period
        # used by the C++ gpu_activity agent.
        agent = GPUActivityAgent()
        parser = agent.update_parser(get_parser())
        self.assertAlmostEqual(0.02, parser.parse_args([]).period)

    def test_update_parser_preserves_explicit_period(self):
        # An explicit -p value is left untouched, even when it equals the
        # session's original 100 ms default.
        agent = GPUActivityAgent()
        parser = agent.update_parser(get_parser())
        self.assertAlmostEqual(0.1, parser.parse_args(['-p', '0.1']).period)
        self.assertAlmostEqual(0.05, parser.parse_args(['--period', '0.05']).period)

    def test_update_parser_corrects_signal_config_help(self):
        # The agent supplies a default signal set, so the -i/--signal-config
        # help must not claim standard input is the default.
        agent = GPUActivityAgent()
        parser = agent.update_parser(get_parser())
        help_text = next(action.help for action in parser._actions
                         if action.dest == 'config_path')
        self.assertNotIn('standard input', help_text)
        self.assertIn("built-in signal set", help_text)

    def test_help_nonempty(self):
        self.assertIn('gpu_activity', GPUActivityAgent().help())

    # ---- signal_config_override ------------------------------------------

    def test_signal_config_override_levelzero(self):
        self._signal_names = {'GPU_CORE_ACTIVITY', 'GPU_UTILIZATION', 'TIME'}
        override = GPUActivityAgent().signal_config_override()
        self.assertEqual('TIME board 0\n'
                         'GPU_CORE_ACTIVITY board 0\n'
                         'GPU_UTILIZATION board 0\n', override)

    def test_signal_config_override_drm_idle(self):
        self._signal_names = {'DRM::IDLE_RESIDENCY', 'TIME'}
        override = GPUActivityAgent().signal_config_override()
        self.assertEqual('TIME board 0\n'
                         'DRM::IDLE_RESIDENCY board 0\n', override)

    def test_signal_config_override_hi_res(self):
        self._signal_names = {'GPU_CORE_FREQUENCY_STATUS', 'GPU_CORE_ACTIVITY',
                              'GPU_UTILIZATION', 'TIME'}
        agent = GPUActivityAgent()
        agent.update_args(Namespace(phi=0.5, hi_res=True))
        override = agent.signal_config_override()
        # TIME stays at board 0; other signals use the wildcard domain/index.
        self.assertEqual('TIME board 0\n'
                         'GPU_CORE_FREQUENCY_STATUS * *\n'
                         'GPU_CORE_ACTIVITY * *\n'
                         'GPU_UTILIZATION * *\n', override)

    # ---- run_begin --------------------------------------------------------

    def test_run_begin_no_gpu(self):
        self._domain_counts[_DOMAIN_GPU] = 0
        with self.assertRaisesRegex(RuntimeError, 'requires at least one GPU'):
            GPUActivityAgent().run_begin()

    def test_run_begin_no_activity_signal(self):
        self._signal_names = {'GPU_CORE_FREQUENCY_STATUS', 'TIME'}
        with self.assertRaisesRegex(RuntimeError, 'no GPU activity signal'):
            GPUActivityAgent().run_begin()

    def test_run_begin_invalid_domain(self):
        # All required signals/controls resolve to the board domain.
        self._signal_domain = _DOMAIN_BOARD
        self._control_domain = _DOMAIN_BOARD
        with self.assertRaisesRegex(RuntimeError, 'GPU or GPU_CHIP domain'):
            GPUActivityAgent().run_begin()

    def test_run_begin_levelzero(self):
        agent = GPUActivityAgent()
        agent.run_begin()
        self.assertEqual('levelzero', agent._activity_source)
        self.assertEqual(_DOMAIN_GPU, agent._agent_domain)
        self.assertEqual(_FREQ_MIN, agent._freq_gpu_min)
        self.assertEqual(_FREQ_MAX, agent._freq_gpu_max)
        self.assertEqual(_FREQ_EFFICIENT, agent._freq_gpu_efficient)
        self._save_control.assert_called_once()
        pushed = {call.args[0] for call in self._push_signal.call_args_list}
        self.assertIn('GPU_CORE_ACTIVITY', pushed)
        self.assertIn('GPU_UTILIZATION', pushed)
        self.assertNotIn('DRM::IDLE_RESIDENCY', pushed)
        controls = {call.args[0] for call in self._push_control.call_args_list}
        self.assertEqual({'GPU_CORE_FREQUENCY_MIN_CONTROL',
                          'GPU_CORE_FREQUENCY_MAX_CONTROL'}, controls)

    def test_run_begin_levelzero_no_utilization(self):
        # DCGM exposes GPU_CORE_ACTIVITY without GPU_UTILIZATION.
        self._signal_names = {'GPU_CORE_ACTIVITY', 'GPU_CORE_FREQUENCY_STATUS',
                              'TIME', _FE_CONSTCONFIG}
        agent = GPUActivityAgent()
        agent.run_begin()
        self.assertEqual('levelzero', agent._activity_source)
        self.assertFalse(agent._has_utilization)
        pushed = {call.args[0] for call in self._push_signal.call_args_list}
        self.assertIn('GPU_CORE_ACTIVITY', pushed)
        self.assertNotIn('GPU_UTILIZATION', pushed)

    def test_run_begin_drm_idle(self):
        self._signal_names = {'DRM::IDLE_RESIDENCY', 'GPU_CORE_FREQUENCY_STATUS',
                              'TIME', _FE_CONSTCONFIG}
        agent = GPUActivityAgent()
        agent.run_begin()
        self.assertEqual('drm_idle', agent._activity_source)
        pushed = {call.args[0] for call in self._push_signal.call_args_list}
        self.assertIn('DRM::IDLE_RESIDENCY', pushed)
        self.assertIn('TIME', pushed)
        self.assertNotIn('GPU_CORE_ACTIVITY', pushed)
        self.assertIsNotNone(agent._time_idx)

    def test_run_begin_efficient_from_levelzero_signal(self):
        self._signal_names = {'GPU_CORE_ACTIVITY', 'GPU_UTILIZATION', _FE_SIG_NAME}
        self._read_values[_FE_SIG_NAME] = 1.1e9
        agent = GPUActivityAgent()
        agent.run_begin()
        self.assertEqual(1.1e9, agent._freq_gpu_efficient)

    def test_run_begin_efficient_midpoint_fallback(self):
        self._signal_names = {'GPU_CORE_ACTIVITY', 'GPU_UTILIZATION'}
        agent = GPUActivityAgent()
        agent.run_begin()
        self.assertEqual((_FREQ_MAX + _FREQ_MIN) / 2, agent._freq_gpu_efficient)

    def test_run_begin_efficient_out_of_range(self):
        self._read_values[_FE_CONSTCONFIG] = _FREQ_MAX + 1.0
        with self.assertRaisesRegex(RuntimeError, 'efficient frequency out of range'):
            GPUActivityAgent().run_begin()

    # ---- update_loop: control algorithm -----------------------------------

    def _adjusted_value(self):
        """Return the frequency value passed to the most recent adjust pair."""
        self.assertTrue(self._adjust.call_args_list)
        return self._adjust.call_args_list[-1].args[1]

    def _begin_levelzero(self, phi, activity, utilization):
        agent = GPUActivityAgent()
        agent.update_args(Namespace(phi=phi))
        agent.run_begin()
        self._sample_values = {
            'GPU_CORE_ACTIVITY:0': activity,
            'GPU_UTILIZATION:0': utilization,
        }
        return agent

    def test_update_loop_phi_neutral(self):
        agent = self._begin_levelzero(0.5, activity=0.5, utilization=1.0)
        agent.update_loop()
        # resolved range is full [1.0e9, 1.5e9]; request = eff + range*0.5
        self.assertAlmostEqual(1.25e9, self._adjusted_value(), delta=1.0)
        self._write_batch.assert_called_once()
        # Both min and max controls are written to the same value.
        last_two = self._adjust.call_args_list[-2:]
        self.assertEqual({'GPU_CORE_FREQUENCY_MIN_CONTROL:0',
                          'GPU_CORE_FREQUENCY_MAX_CONTROL:0'},
                         {c.args[0] for c in last_two})

    def test_update_loop_energy_bias(self):
        # phi > 0.5 scales F_max down toward F_efficient.
        agent = self._begin_levelzero(0.75, activity=0.5, utilization=1.0)
        agent.update_loop()
        # resolved_max = 1.25e9, eff = 1.0e9, range = 0.25e9; req = 1.0e9 + 0.125e9
        self.assertAlmostEqual(1.125e9, self._adjusted_value(), delta=1.0)
        self.assertAlmostEqual(1.25e9, agent._resolved_f_gpu_max, delta=1.0)

    def test_update_loop_performance_bias(self):
        # phi < 0.5 scales F_efficient up toward F_max.
        agent = self._begin_levelzero(0.25, activity=0.5, utilization=1.0)
        agent.update_loop()
        # resolved_eff = 1.25e9, max = 1.5e9, range = 0.25e9; req = 1.25e9 + 0.125e9
        self.assertAlmostEqual(1.375e9, self._adjusted_value(), delta=1.0)
        self.assertAlmostEqual(1.25e9, agent._resolved_f_gpu_efficient, delta=1.0)

    def test_update_loop_activity_nan_defaults_to_max(self):
        agent = self._begin_levelzero(0.5, activity=float('nan'), utilization=1.0)
        agent.update_loop()
        self.assertAlmostEqual(_FREQ_MAX, self._adjusted_value(), delta=1.0)

    def test_update_loop_zero_utilization_uses_activity_only(self):
        agent = self._begin_levelzero(0.5, activity=0.5, utilization=0.0)
        agent.update_loop()
        # utilization <= 0: request = eff + range * activity
        self.assertAlmostEqual(1.25e9, self._adjusted_value(), delta=1.0)

    def test_update_loop_no_utilization_uses_activity(self):
        # DCGM path: no GPU_UTILIZATION signal, utilization defaults to 1.0.
        self._signal_names = {'GPU_CORE_ACTIVITY', 'GPU_CORE_FREQUENCY_STATUS',
                              'TIME', _FE_CONSTCONFIG}
        agent = GPUActivityAgent()
        agent.update_args(Namespace(phi=0.5))
        agent.run_begin()
        self.assertFalse(agent._has_utilization)
        self._sample_values = {'GPU_CORE_ACTIVITY:0': 0.5}
        agent.update_loop()
        # request = eff + range * activity = 1.0e9 + 0.5e9 * 0.5
        self.assertAlmostEqual(1.25e9, self._adjusted_value(), delta=1.0)

    def test_update_loop_clamps_and_counts_clip(self):
        # activity/utilization ratio > 1 drives the request above F_max.
        agent = self._begin_levelzero(0.5, activity=1.0, utilization=0.5)
        agent.update_loop()
        self.assertAlmostEqual(_FREQ_MAX, self._adjusted_value(), delta=1.0)
        self.assertEqual(1, agent._frequency_clipped)

    def test_update_loop_no_write_when_unchanged(self):
        agent = self._begin_levelzero(0.5, activity=0.5, utilization=1.0)
        agent.update_loop()
        agent.update_loop()
        # Second identical loop must not re-write the controls.
        self.assertEqual(1, self._write_batch.call_count)
        self.assertEqual(2, self._adjust.call_count)  # min + max, first loop only
        self.assertEqual(1, agent._frequency_requests)

    def test_update_loop_multi_domain(self):
        self._domain_counts[_DOMAIN_GPU] = 2
        agent = GPUActivityAgent()
        agent.update_args(Namespace(phi=0.5))
        agent.run_begin()
        self._sample_values = {
            'GPU_CORE_ACTIVITY:0': 0.5, 'GPU_UTILIZATION:0': 1.0,
            'GPU_CORE_ACTIVITY:1': 1.0, 'GPU_UTILIZATION:1': 1.0,
        }
        agent.update_loop()
        # One write_batch, two domains each writing min+max => 4 adjusts.
        self.assertEqual(1, self._write_batch.call_count)
        self.assertEqual(4, self._adjust.call_count)
        self.assertEqual(2, agent._frequency_requests)

    # ---- update_loop: DRM idle-residency path -----------------------------

    def test_update_loop_drm_idle_busy_fraction(self):
        self._signal_names = {'DRM::IDLE_RESIDENCY', 'GPU_CORE_FREQUENCY_STATUS',
                              'TIME', _FE_CONSTCONFIG}
        agent = GPUActivityAgent()
        agent.update_args(Namespace(phi=0.5))
        agent.run_begin()

        # First loop: no prior interval, so activity is unknown -> F_max.
        self._sample_values = {'TIME:0': 10.0, 'DRM::IDLE_RESIDENCY:0': 5.0}
        # TIME is pushed at board (token 'TIME:0'); idle at domain 0.
        agent.update_loop()
        self.assertAlmostEqual(_FREQ_MAX, self._adjusted_value(), delta=1.0)

        # Second loop: idle advanced 0.3 over 1.0s => busy = 0.7.
        self._sample_values = {'TIME:0': 11.0, 'DRM::IDLE_RESIDENCY:0': 5.3}
        agent.update_loop()
        # request = eff + range * 0.7 = 1.0e9 + 0.5e9 * 0.7 = 1.35e9
        self.assertAlmostEqual(1.35e9, self._adjusted_value(), delta=10.0)

    def test_update_loop_drm_idle_clamps_busy_fraction(self):
        self._signal_names = {'DRM::IDLE_RESIDENCY', 'GPU_CORE_FREQUENCY_STATUS',
                              'TIME', _FE_CONSTCONFIG}
        agent = GPUActivityAgent()
        agent.update_args(Namespace(phi=0.5))
        agent.run_begin()
        self._sample_values = {'TIME:0': 10.0, 'DRM::IDLE_RESIDENCY:0': 5.0}
        agent.update_loop()
        # Idle decreased (counter glitch) -> busy > 1 -> clamped to 1.0 -> F_max.
        self._sample_values = {'TIME:0': 11.0, 'DRM::IDLE_RESIDENCY:0': 4.0}
        agent.update_loop()
        self.assertAlmostEqual(_FREQ_MAX, self._adjusted_value(), delta=1.0)


if __name__ == '__main__':
    unittest.main()
