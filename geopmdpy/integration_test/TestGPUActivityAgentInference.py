#!/usr/bin/env python3
#
#  Copyright (c) 2015 - 2026 Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#

"""GPU activity agent effectiveness integration test (AI inference workloads).

Drives real GPU inference workloads while running the Python
``geopmdpy.gpu_activity_agent`` at several ``--phi`` policy values, and
asserts that the agent is effective.  The GPU activity agent sets frequency
proportional to compute activity::

    f_request = f_efficient + (f_max - f_efficient) * (activity / utilization)

so it can only save energy or move the frequency dynamically when the
workload's compute activity actually drops below saturation (i.e. when the
GPU *stalls*).  Three workload profiles are therefore exercised:

``SteadyState`` (control)
    A back-to-back, compute-saturated SYCL inference proxy.  Activity stays pinned
    near 1.0, so the agent keeps the frequency at F_max.  This case validates
    only that the agent does **no harm** to a saturated workload (at phi=0
    and phi=0.5) and that the phi=1 static clamp still saves energy.  It does
    NOT assert dynamic control or phi=0.5 energy savings, because a saturated
    workload gives the agent nothing to exploit.

``Serving`` (regime 1: idle gaps)
    An over-provisioned online-serving profile that idles between
    requests to a target duty cycle.  Compute activity oscillates, so the
    agent drops the frequency during the gaps: dynamic control and energy
    savings at phi=0.5 are asserted.

``Decode`` (regime 2: frequency-insensitive but busy)
    A batch-1, memory-bandwidth-bound autoregressive decode proxy (the LLM
    decode archetype).  The GPU looks busy but the compute engine is only
    partially active, so the agent lowers the frequency with negligible
    performance loss -- the scenario where it is most differentiated from
    the GPU's own hardware DVFS.  Dynamic control and energy savings at
    phi=0.5 are asserted.

This is NOT a unit test: it requires a live GEOPM service, a GPU, and the
inference workload.  It lives in the independent ``integration_test`` directory
and is run explicitly.  See ``README.md`` in this directory.

Environment overrides (all optional):
  GEOPM_GPU_WORKLOAD_SEC     Workload timed duration, seconds (default 30).
    GEOPM_GPU_BATCH_SIZE       Image-profile FoM scaling factor (default 64).
  GEOPM_GPU_DUTY_CYCLE       Serving-mode GPU-active fraction (default 0.5).
  GEOPM_GPU_PERIOD           Agent/monitor sample period, seconds (default 0.02).
  GEOPM_GPU_FOM_TOL          Allowed fractional FoM drop at phi=0 (default 0.05).
  GEOPM_GPU_ENERGY_MARGIN    energy(phi) must be < margin*baseline (default 1.0).
  GEOPM_GPU_FREQ_STD_MIN_HZ  Min freq std-dev proving dynamic control (default 1e7).
"""

import os
import signal
import subprocess
import tempfile
import time
import unittest

from . import _util

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


class _ScenarioHarness(unittest.TestCase):
    """Shared orchestration for a single workload profile.

    Subclasses set ``DRIVER``, ``PHIS`` and implement ``workload_args`` and are
    decorated with the opt-in / hardware / workload skip guards.  This base
    defines no ``test_*`` methods, so unittest collects nothing from it.
    """

    #: Workload driver file name under ``apps/`` (set by subclasses).
    DRIVER = None
    #: phi values to measure in addition to the monitor-only baseline.
    PHIS = ()

    @classmethod
    def workload_args(cls):
        """Return the driver arguments for this scenario (override)."""
        raise NotImplementedError

    @classmethod
    def setUpClass(cls):
        from geopmdpy import pio
        from geopmdpy import topo

        cls._workload_sec = float(os.environ.get('GEOPM_GPU_WORKLOAD_SEC', 30))
        cls._batch_size = int(os.environ.get('GEOPM_GPU_BATCH_SIZE', 64))
        cls._duty_cycle = float(os.environ.get('GEOPM_GPU_DUTY_CYCLE', 0.5))
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

        cls._tmpdir = tempfile.mkdtemp(
            prefix=f'geopm-gpu-activity-{cls.__name__}-')
        cls._config_path = os.path.join(cls._tmpdir, 'gpu_monitor.config')
        _util.build_monitor_config(cls._config_path)

        cls._workload_cmd = _util.workload_command(
            cls.DRIVER, cls.workload_args())

        # Baseline (monitor only) first, then each controlled phi.
        cls._results = {None: cls._run_config(None)}
        for phi in cls.PHIS:
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
            time.sleep(_AGENT_WARMUP_SEC)

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
                f'geopmsession monitor failed ({cls.__name__}/{tag}) with code '
                f'{monitor.returncode}:\n{monitor.stderr}')

        rows = _util.read_trace(monitor_trace)
        try:
            fom = _util.parse_fom(monitor.stdout)
        except RuntimeError as ex:
            raise RuntimeError(
                f'workload failed to report a FOM ({cls.__name__}/{tag})\n'
                f'command: {" ".join(monitor_cmd)}\n'
                f'geopmsession return code: {monitor.returncode}\n'
                f'stdout:\n{monitor.stdout}\n'
                f'stderr:\n{monitor.stderr}') from ex
        return {
            'fom': fom,
            'energy': _util.energy_joules(rows),
            'freq_std_hz': _util.max_freq_std_hz(rows),
            'freq_requests': freq_requests,
        }

    # -- shared assertions -------------------------------------------------

    def _assert_no_harm(self, phi):
        base = self._results[None]['fom']
        got = self._results[phi]['fom']
        self.assertGreaterEqual(
            got, base * (1.0 - self._fom_tol),
            msg=(f'phi={phi} FoM {got:.3f} dropped more than '
                 f'{self._fom_tol:.0%} below baseline {base:.3f}'))

    def _assert_energy_benefit(self, phi):
        base = self._results[None]['energy']
        got = self._results[phi]['energy']
        self.assertLess(
            got, base * self._energy_margin,
            msg=(f'phi={phi} energy {got:.1f} J not below baseline '
                 f'{base:.1f} J (margin {self._energy_margin})'))

    def _assert_dynamic_frequency(self, phi):
        result = self._results[phi]
        self.assertGreater(
            result['freq_requests'], 1,
            msg=(f'phi={phi} issued <= 1 frequency control write '
                 f'({result["freq_requests"]}); no dynamic control observed'))
        self.assertGreater(
            result['freq_std_hz'], self._freq_std_min_hz,
            msg=(f'phi={phi} GPU_CORE_FREQUENCY_STATUS std-dev '
                 f'{result["freq_std_hz"]:.3e} Hz below threshold '
                 f'{self._freq_std_min_hz:.3e} Hz; frequency did not move'))


@_util.skip_unless_gpu()
@_util.skip_unless_levelzero()
@_util.skip_unless_workload()
class TestGPUActivityAgentSteadyState(_ScenarioHarness):
    """Control case: a compute-saturated workload the agent cannot exploit."""

    DRIVER = _util.LOCAL_DRIVER
    PHIS = (_PHI_PERF, _PHI_DYNAMIC, _PHI_ENERGY)

    @classmethod
    def workload_args(cls):
        return ['--profile', 'steady',
                '--duration', cls._workload_sec,
                '--batch-size', cls._batch_size]

    def test_phi0_no_performance_harm(self):
        """phi=0 (pinned F_max) must not reduce saturated throughput."""
        self._assert_no_harm(_PHI_PERF)

    def test_phi05_no_harm_when_saturated(self):
        """phi=0.5 must not reduce throughput of a saturated workload.

        With activity pinned near 1.0 the agent should hold the frequency at
        F_max; this documents that phi=0.5 is safe even when it cannot help.
        """
        self._assert_no_harm(_PHI_DYNAMIC)

    def test_phi1_energy_saving_extreme(self):
        """phi=1 (pinned F_efficient) must save energy (performance cost not
        asserted)."""
        self._assert_energy_benefit(_PHI_ENERGY)


@_util.skip_unless_gpu()
@_util.skip_unless_levelzero()
@_util.skip_unless_workload()
class TestGPUActivityAgentServing(_ScenarioHarness):
    """Regime 1: an over-provisioned server that idles between requests."""

    DRIVER = _util.LOCAL_DRIVER
    PHIS = (_PHI_PERF, _PHI_DYNAMIC, _PHI_ENERGY)

    @classmethod
    def workload_args(cls):
        return ['--profile', 'serving',
                '--duration', cls._workload_sec,
                '--batch-size', cls._batch_size,
                '--duty-cycle', cls._duty_cycle]

    def test_phi0_no_performance_harm(self):
        """phi=0 (pinned F_max) must not reduce served throughput."""
        self._assert_no_harm(_PHI_PERF)

    def test_phi05_dynamic_frequency(self):
        """phi=0.5 must dynamically re-tune frequency across the idle gaps."""
        self._assert_dynamic_frequency(_PHI_DYNAMIC)

    def test_phi05_energy_benefit_vs_monitor(self):
        """phi=0.5 must save GPU energy versus the monitor baseline."""
        self._assert_energy_benefit(_PHI_DYNAMIC)

    def test_phi1_energy_saving_extreme(self):
        """phi=1 must save energy (performance cost not asserted)."""
        self._assert_energy_benefit(_PHI_ENERGY)


@_util.skip_unless_gpu()
@_util.skip_unless_levelzero()
@_util.skip_unless_workload()
class TestGPUActivityAgentDecode(_ScenarioHarness):
    """Regime 2: a memory-bound, batch-1 decode loop (frequency-insensitive)."""

    DRIVER = _util.LOCAL_DRIVER
    PHIS = (_PHI_PERF, _PHI_DYNAMIC, _PHI_ENERGY)

    @classmethod
    def workload_args(cls):
        return ['--profile', 'decode',
            '--duration', cls._workload_sec]

    def test_phi0_no_performance_harm(self):
        """phi=0 (pinned F_max) must not reduce decode throughput."""
        self._assert_no_harm(_PHI_PERF)

    def test_phi05_dynamic_frequency(self):
        """phi=0.5 must dynamically re-tune frequency for the busy-but-idle
        compute engine."""
        self._assert_dynamic_frequency(_PHI_DYNAMIC)

    def test_phi05_energy_benefit_vs_monitor(self):
        """phi=0.5 must save GPU energy versus the monitor baseline -- the
        agent's strongest, most differentiated case."""
        self._assert_energy_benefit(_PHI_DYNAMIC)

    def test_phi1_energy_saving_extreme(self):
        """phi=1 must save energy (performance cost not asserted)."""
        self._assert_energy_benefit(_PHI_ENERGY)


if __name__ == '__main__':
    unittest.main()
