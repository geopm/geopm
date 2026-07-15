#!/usr/bin/env python3
#
#  Copyright (c) 2015 - 2026 Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#

"""GPU activity agent effectiveness integration test (AI inference workload).

Drives a real, GPU-bound ResNet-50 inference workload while running the
Python ``geopmdpy.gpu_activity_agent`` at several ``--phi`` policy values,
and asserts that the agent is effective:

  * ``phi = 0`` (performance biased, frequency pinned at F_max) does no harm
    to performance relative to a read-only ``geopmsession`` monitor.
  * ``phi = 0.5`` (full dynamic range) saves GPU energy relative to the
    monitor baseline.
  * ``phi = 0.5`` dynamically changes the GPU frequency (multiple control
    writes and observable variation in ``GPU_CORE_FREQUENCY_STATUS``).
  * ``phi = 1`` (energy biased, frequency pinned at F_efficient) saves energy
    at the expected cost of performance.

This is NOT a unit test: it requires a live GEOPM service, a GPU, and the
inference workload.  It is opt-in via ``GEOPM_RUN_GPU_INTEGRATION=1`` and is
skipped otherwise.  See ``README.md`` in this directory.

Environment overrides (all optional):
  GEOPM_GPU_WORKLOAD_SEC     Workload timed duration, seconds (default 30).
  GEOPM_GPU_BATCH_SIZE       Inference batch size (default 64).
  GEOPM_GPU_PERIOD           Agent/monitor sample period, seconds (default 0.02).
  GEOPM_GPU_FOM_TOL          Allowed fractional FoM drop at phi=0 (default 0.05).
  GEOPM_GPU_ENERGY_MARGIN    energy(phi) must be < margin*baseline (default 1.0).
  GEOPM_GPU_FREQ_STD_MIN_HZ  Min freq std-dev proving dynamic control (default 1e7).
"""

import os
import signal
import subprocess
import tempfile
import unittest

from geopmdpy.integration_test import _util

# Agent run window is the workload duration plus this margin so the agent is
# controlling frequency for the entire measured window; it is terminated early
# (which reverts controls) once the workload-driven monitor returns.
_AGENT_MARGIN_SEC = 30.0
# Give the agent session time to establish control before the workload starts.
_AGENT_WARMUP_SEC = 3.0
# Grace period for the agent to print its summary and revert on SIGINT.
_AGENT_STOP_TIMEOUT_SEC = 30.0

_PHI_PERF = 0.0
_PHI_DYNAMIC = 0.5
_PHI_ENERGY = 1.0
_PHI_VALUES = (_PHI_PERF, _PHI_DYNAMIC, _PHI_ENERGY)


@_util.skip_unless_opted_in()
@_util.skip_unless_gpu()
@_util.skip_unless_levelzero()
@_util.skip_unless_workload()
class TestGPUActivityAgentInference(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        from geopmdpy import pio
        from geopmdpy import topo

        cls._workload_sec = float(os.environ.get('GEOPM_GPU_WORKLOAD_SEC', 30))
        cls._batch_size = int(os.environ.get('GEOPM_GPU_BATCH_SIZE', 64))
        cls._period = float(os.environ.get('GEOPM_GPU_PERIOD', 0.02))
        cls._fom_tol = float(os.environ.get('GEOPM_GPU_FOM_TOL', 0.05))
        cls._energy_margin = float(os.environ.get('GEOPM_GPU_ENERGY_MARGIN', 1.0))
        cls._freq_std_min_hz = float(
            os.environ.get('GEOPM_GPU_FREQ_STD_MIN_HZ', 1e7))

        # Retained for logging / tolerance context.
        cls._f_min = pio.read_signal(
            'GPU_CORE_FREQUENCY_MIN_AVAIL', topo.DOMAIN_BOARD, 0)
        cls._f_max = pio.read_signal(
            'GPU_CORE_FREQUENCY_MAX_AVAIL', topo.DOMAIN_BOARD, 0)

        cls._tmpdir = tempfile.mkdtemp(prefix='geopm-gpu-activity-inference-')
        cls._config_path = os.path.join(cls._tmpdir, 'gpu_monitor.config')
        _util.build_monitor_config(cls._config_path)

        cls._workload_cmd = [
            _util.workload_script(),
            '--duration', str(cls._workload_sec),
            '--batch-size', str(cls._batch_size),
        ]

        # Baseline (monitor only) first, then each controlled phi.
        cls._results = {}
        cls._results[None] = cls._run_config(None)
        for phi in _PHI_VALUES:
            cls._results[phi] = cls._run_config(phi)

    @classmethod
    def _run_config(cls, phi):
        """Run the workload once and measure it.

        A read-only ``geopmsession`` monitor launches the workload and records
        the whole-board energy and per-domain frequency trace.  When ``phi`` is
        not None, the GPU activity agent runs concurrently as the frequency
        writer.  Returns a dict with fom, energy, freq_std_hz, freq_requests.
        """
        tag = 'baseline' if phi is None else f'phi{phi}'
        monitor_trace = os.path.join(cls._tmpdir, f'{tag}_monitor.csv')
        monitor_report = os.path.join(cls._tmpdir, f'{tag}_monitor.yaml')

        agent_proc = None
        agent_stderr_path = None
        if phi is not None:
            agent_trace = os.path.join(cls._tmpdir, f'{tag}_agent.csv')
            agent_report = os.path.join(cls._tmpdir, f'{tag}_agent.yaml')
            agent_stderr_path = os.path.join(cls._tmpdir, f'{tag}_agent.stderr')
            agent_cmd = _util.agent_command(
                phi, cls._workload_sec + _AGENT_MARGIN_SEC, cls._period,
                agent_trace, agent_report)
            agent_err = open(agent_stderr_path, 'w')
            agent_proc = subprocess.Popen(
                agent_cmd, stdout=subprocess.DEVNULL, stderr=agent_err,
                start_new_session=True)
            agent_err.close()
            # Let the agent establish control before the workload begins.
            _sleep(_AGENT_WARMUP_SEC)

        try:
            monitor_cmd = _util.geopmsession_command(
                cls._config_path, monitor_trace, cls._period,
                cls._workload_cmd, report_path=monitor_report)
            monitor = subprocess.run(
                monitor_cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE,
                universal_newlines=True)
        finally:
            freq_requests = 0
            if agent_proc is not None:
                # SIGINT triggers the session's graceful stop: run_end() prints
                # the summary and pio.restore_control() reverts frequency.
                agent_proc.send_signal(signal.SIGINT)
                try:
                    agent_proc.wait(timeout=_AGENT_STOP_TIMEOUT_SEC)
                except subprocess.TimeoutExpired:
                    agent_proc.kill()
                    agent_proc.wait()
                with open(agent_stderr_path) as fid:
                    freq_requests = _util.parse_frequency_requests(fid.read())

        if monitor.returncode != 0:
            raise RuntimeError(
                f'geopmsession monitor failed ({tag}) with code '
                f'{monitor.returncode}:\n{monitor.stderr}')

        fom = _util.parse_fom(monitor.stdout)
        rows = _util.read_trace(monitor_trace)
        return {
            'fom': fom,
            'energy': _util.energy_joules(rows),
            'freq_std_hz': _util.max_freq_std_hz(rows),
            'freq_requests': freq_requests,
        }

    def test_phi0_no_performance_harm(self):
        """phi=0 (frequency pinned at F_max) must not reduce throughput."""
        base = self._results[None]['fom']
        perf = self._results[_PHI_PERF]['fom']
        self.assertGreaterEqual(
            perf, base * (1.0 - self._fom_tol),
            msg=(f'phi=0 FoM {perf:.1f} img/s dropped more than '
                 f'{self._fom_tol:.0%} below baseline {base:.1f} img/s'))

    def test_phi05_energy_benefit_vs_monitor(self):
        """phi=0.5 must consume less GPU energy than the monitor baseline."""
        base = self._results[None]['energy']
        dyn = self._results[_PHI_DYNAMIC]['energy']
        self.assertLess(
            dyn, base * self._energy_margin,
            msg=(f'phi=0.5 energy {dyn:.1f} J not below baseline '
                 f'{base:.1f} J (margin {self._energy_margin})'))

    def test_phi05_dynamic_frequency(self):
        """phi=0.5 must dynamically re-tune the GPU frequency."""
        result = self._results[_PHI_DYNAMIC]
        self.assertGreater(
            result['freq_requests'], 1,
            msg=('phi=0.5 issued <= 1 frequency control write '
                 f'({result["freq_requests"]}); no dynamic control observed'))
        self.assertGreater(
            result['freq_std_hz'], self._freq_std_min_hz,
            msg=(f'phi=0.5 GPU_CORE_FREQUENCY_STATUS std-dev '
                 f'{result["freq_std_hz"]:.3e} Hz below threshold '
                 f'{self._freq_std_min_hz:.3e} Hz; frequency did not move'))

    def test_phi1_energy_saving_extreme(self):
        """phi=1 (frequency pinned at F_efficient) must save energy.

        Performance harm is expected at this extreme and is not asserted.
        """
        base = self._results[None]['energy']
        energy = self._results[_PHI_ENERGY]['energy']
        self.assertLess(
            energy, base * self._energy_margin,
            msg=(f'phi=1 energy {energy:.1f} J not below baseline '
                 f'{base:.1f} J (margin {self._energy_margin})'))


def _sleep(seconds):
    import time
    time.sleep(seconds)


if __name__ == '__main__':
    unittest.main()
