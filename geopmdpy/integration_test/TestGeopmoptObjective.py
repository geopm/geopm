#!/usr/bin/env python3
#
#  Copyright (c) 2015 - 2026 Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#

"""geopmopt Phase 0 end-to-end regression integration test.

Phase 0 of the geopmopt enhancement plan (#4043 / #4044 / #3927) is a
behavior-preserving refactor: the ``BayesianOptimizer.optimize`` objective
closure is split into helper methods and ``ApplicationEvaluator.evaluate``
now returns a ``TrialResult`` container instead of a bare float.  The mocked
unit suite under ``geopmdpy/test`` verifies the refactor's internals but
never launches the real CLI.  This live-hardware test closes that gap: it
runs the real ``geopmopt`` (``python -m geopmdpy.optimizer``) end to end and
asserts that the two objectives that existed before Phase 0 still work --

* a raw-metric maximize run selects the highest frequency in the grid for a
  CPU-bound workload (the throughput objective), and
* an ``--efficiency cpu`` run completes and selects a frequency from the grid
  (the efficiency objective, which exercises the energy/time sampling path).

It requires a live GEOPM service that grants the CPU core-frequency control,
scikit-optimize, and a shell to run the probe workload.  Without them the
test skips cleanly.  It lives in the independent ``integration_test``
directory and is run explicitly.

Environment overrides (optional):
  GEOPMOPT_PROBE_ITERS       Busy-loop iteration count in the probe (default 3e6).
  GEOPMOPT_PROBE_COOLDOWN_S  Idle seconds before each timed loop (default 0).
"""

import os
import shutil
import subprocess
import sys
import tempfile
import unittest

from . import _geopmopt_util as gu

# Figure-of-merit pattern printed by apps/cpu_frequency_probe.sh.
_METRIC_REGEX = r'GEOPMOPT-FOM: ([0-9.]+)'


class _GeopmoptHarness(unittest.TestCase):
    """Shared orchestration: run geopmopt once in ``setUpClass`` and expose
    the resulting best configuration to the test methods.

    Subclasses set ``EFFICIENCY_DOMAIN`` / ``MINIMIZE`` and are decorated with
    the skip guards.  This base defines no ``test_*`` methods, so unittest
    collects nothing from it.
    """

    #: ``--efficiency`` domain, or None for the raw-metric objective.
    EFFICIENCY_DOMAIN = None
    #: Whether to pass ``--minimize`` (default is maximize).
    MINIMIZE = False

    @classmethod
    def _make_command(cls):
        """Return the geopmopt argv to run in ``setUpClass``.

        The base builds the legacy raw-metric / ``--efficiency`` command.
        General-interface subclasses override this hook to emit
        ``--metric``/``--maximize``/``--minimize``/``--constraint`` flags via
        :func:`_geopmopt_util.geopmopt_general_command`.  It is called once,
        after ``cls._freq_low``/``cls._freq_high``/``cls._config_path`` are set.
        """
        return gu.geopmopt_command(
            cls._config_path, _METRIC_REGEX, cls._freq_low, cls._freq_high,
            minimize=cls.MINIMIZE, efficiency_domain=cls.EFFICIENCY_DOMAIN)

    @classmethod
    def setUpClass(cls):
        cls._freq_low, cls._freq_high = gu.cpu_frequency_bounds()
        cls._tmpdir = tempfile.mkdtemp(prefix=f'geopmopt-{cls.__name__}-')
        cls._config_path = os.path.join(cls._tmpdir, 'best.config')
        cls._command = cls._make_command()
        proc = subprocess.run(
            cls._command, stdout=subprocess.PIPE, stderr=subprocess.PIPE,
            universal_newlines=True, env=gu.probe_env())
        if proc.returncode != 0:
            raise RuntimeError(
                f'geopmopt failed ({cls.__name__}) with return code '
                f'{proc.returncode}\n'
                f'command: {" ".join(cls._command)}\n'
                f'stdout:\n{proc.stdout}\n'
                f'stderr:\n{proc.stderr}')
        with open(cls._config_path) as fid:
            cls._config_text = fid.read()

    @classmethod
    def tearDownClass(cls):
        shutil.rmtree(cls._tmpdir, ignore_errors=True)

    def _best_frequency(self):
        freq = gu.best_cpu_frequency(self._config_text)
        self.assertIsNotNone(
            freq,
            msg=('geopmopt best configuration did not contain '
                 f'CPU_FREQUENCY_MAX_CONTROL:\n{self._config_text}'))
        return freq


@gu.skip_unless_skopt()
@gu.skip_unless_cpu_frequency_control()
class TestGeopmoptRawMetric(_GeopmoptHarness):
    """Raw-metric maximize objective (unchanged by Phase 0)."""

    EFFICIENCY_DOMAIN = None
    MINIMIZE = False

    def test_selects_max_frequency(self):
        """Maximizing CPU-bound throughput selects the highest grid frequency.

        The grid's high endpoint is capped 200 MHz below the sticker frequency
        (see ``cpu_frequency_bounds``), keeping every trial out of the
        turbo/boundary region.  The probe runs a short busy loop
        (``gu.PROBE_ITERS``) with a per-trial cooldown (``gu.PROBE_COOLDOWN_S``)
        so every trial starts from a comparable package temperature and the high
        grid frequency is not throttled below the low one by accumulated heat.
        Higher CPU core frequency then reliably yields higher throughput, so the
        optimizer must choose the maximum allowed frequency in the grid.
        """
        self.assertEqual(
            self._best_frequency(), float(self._freq_high),
            msg=('maximizing throughput did not select the maximum allowed '
                 f'grid frequency {self._freq_high}'))


@gu.skip_unless_skopt()
@gu.skip_unless_cpu_frequency_control()
@gu.skip_unless_cpu_energy()
class TestGeopmoptEfficiency(_GeopmoptHarness):
    """Efficiency objective (``--efficiency cpu``) exercised end to end.

    The efficiency winner is hardware dependent, so this is a smoke/regression
    check: the refactored energy/time sampling path must complete and yield a
    configuration drawn from the search grid.
    """

    EFFICIENCY_DOMAIN = 'cpu'
    MINIMIZE = False

    def test_completes_and_selects_grid_frequency(self):
        """The efficiency run completes and selects a grid-endpoint frequency."""
        freq = self._best_frequency()
        self.assertIn(
            freq, (float(self._freq_low), float(self._freq_high)),
            msg=(f'selected frequency {freq} is not one of the grid endpoints '
                 f'{self._freq_low}/{self._freq_high}'))


@gu.skip_unless_skopt()
@gu.skip_unless_cpu_frequency_control()
class TestGeopmoptGeneralRawMetric(_GeopmoptHarness):
    """Raw-metric maximize through the general ``--metric``/``--maximize``
    interface.

    This is the general-interface analogue of :class:`TestGeopmoptRawMetric`:
    ``--metric fom=regex:'GEOPMOPT-FOM: ...' --maximize fom`` must select the
    same maximum grid frequency the legacy ``--metric-regex`` path does.
    """

    @classmethod
    def _make_command(cls):
        return gu.geopmopt_general_command(
            cls._config_path, cls._freq_low, cls._freq_high,
            metrics=[f'fom=regex:{_METRIC_REGEX}'], maximize='fom')

    def test_selects_max_frequency(self):
        """``--metric``/``--maximize`` matches the legacy raw-metric result."""
        self.assertEqual(
            self._best_frequency(), float(self._freq_high),
            msg=('the general interface did not select the maximum allowed '
                 f'grid frequency {self._freq_high}'))


@gu.skip_unless_skopt()
@gu.skip_unless_cpu_frequency_control()
class TestGeopmoptConstraint(_GeopmoptHarness):
    """A binding ``--constraint`` prunes the infeasible endpoint.

    :class:`TestGeopmoptGeneralRawMetric` shows the unconstrained objective
    selects ``freq_high``.  Here an identical objective plus a figure-of-merit
    ceiling placed between the two endpoints' throughput makes the high
    endpoint infeasible, so the optimizer must fall back to the feasible
    ``freq_low``.  Throughput is monotonic in the applied frequency, so one
    calibration run at ``freq_high`` anchors the scale and the ceiling is
    scaled to the midpoint frequency (``fom`` is ~linear in frequency).
    """

    @classmethod
    def _make_command(cls):
        cls._fom_high = gu.probe_fom_at_frequency(cls._freq_high)
        freq_mid = (cls._freq_low + cls._freq_high) / 2.0
        cls._fom_ceiling = cls._fom_high * freq_mid / cls._freq_high
        return gu.geopmopt_general_command(
            cls._config_path, cls._freq_low, cls._freq_high,
            metrics=[f'fom=regex:{_METRIC_REGEX}'],
            maximize='fom',
            constraints=[f'fom <= {cls._fom_ceiling:.3f}'])

    def test_binding_constraint_selects_feasible_endpoint(self):
        """With ``fom <= midpoint-throughput`` the high-frequency endpoint is
        infeasible, so the feasible low-frequency endpoint is selected."""
        self.assertEqual(
            self._best_frequency(), float(self._freq_low),
            msg=('the binding figure-of-merit constraint '
                 f'(fom <= {self._fom_ceiling:.3f}) did not prune the '
                 'infeasible high-frequency endpoint'))


@gu.skip_unless_skopt()
class TestGeopmoptListMetrics(unittest.TestCase):
    """``--list-metrics`` enumerates reserved metrics and referenced signals.

    This needs no live control (rows for unreadable signals render ``n/a``), so
    it only guards on skopt.  It asserts the reserved metric names always
    appear and, where the signals resolve, that the behavior-to-aggregation
    mapping is reported (``CPU_ENERGY`` is monotone -> ``delta``).
    """

    def test_lists_reserved_and_referenced_signals(self):
        cmd = [sys.executable, '-m', 'geopmdpy.optimizer', '--list-metrics',
               '--metric', 'power=signal:CPU_POWER@board:mean',
               '--metric', 'energy=signal:CPU_ENERGY@board']
        proc = subprocess.run(
            cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE,
            universal_newlines=True)
        self.assertEqual(proc.returncode, 0, msg=proc.stderr)
        out = proc.stdout
        for name in ('time', 'energy', 'power', 'fom'):
            self.assertIn(
                name, out,
                msg=f'reserved metric {name!r} missing from --list-metrics '
                    f'output:\n{out}')
        self.assertIn('CPU_POWER', out)
        self.assertIn('CPU_ENERGY', out)
        # Where the signal resolves, CPU_ENERGY is monotone -> delta; where it
        # does not, the row renders n/a.  Accept either so the assertion is
        # node independent while still checking the mapping when present.
        energy_row = next(
            (line for line in out.splitlines()
             if line.startswith('CPU_ENERGY')), '')
        self.assertTrue(
            'delta' in energy_row or 'n/a' in energy_row,
            msg=f'unexpected CPU_ENERGY row: {energy_row!r}')


@gu.skip_unless_skopt()
@gu.skip_unless_cpu_frequency_control()
@gu.skip_unless_signal_readable('CPU_POWER')
class TestGeopmoptSignalMetric(_GeopmoptHarness):
    """A general ``signal:`` metric is sampled and reduced end to end (P0-1).

    Before Phase 1 a ``signal:`` metric on the general interface aborted with
    ``Unsupported domain None`` because the ``(signal, domain)`` sampling union
    was never wired to the session sampler.  Here a ``CPU_POWER`` signal metric
    is combined with the throughput figure of merit into an efficiency
    expression (``fom / pw``): the run must sample the signal, reduce it,
    evaluate the derived objective, and select a grid-endpoint frequency.  The
    efficiency winner is hardware dependent, so this is a completion/selection
    smoke check.
    """

    @classmethod
    def _make_command(cls):
        return gu.geopmopt_general_command(
            cls._config_path, cls._freq_low, cls._freq_high,
            metrics=[f'fom=regex:{_METRIC_REGEX}',
                     'pw=signal:CPU_POWER@board:mean',
                     'eff=expr:fom / pw'],
            maximize='eff')

    def test_completes_and_selects_grid_frequency(self):
        """The signal-metric efficiency objective completes and selects a grid
        endpoint, proving the signal was sampled and reduced."""
        freq = self._best_frequency()
        self.assertIn(
            freq, (float(self._freq_low), float(self._freq_high)),
            msg=(f'the signal-metric efficiency objective selected {freq}, '
                 f'not a grid endpoint {self._freq_low}/{self._freq_high}'))


@gu.skip_unless_skopt()
@gu.skip_unless_cpu_frequency_control()
@gu.skip_unless_energy_domain('board')
class TestGeopmoptReservedPowerExpr(_GeopmoptHarness):
    """``expr:'fom / power'`` resolves once ``--energy-domain`` samples power
    (P0-2).

    The reserved ``power`` metric is populated only when ``--energy-domain``
    (or legacy ``--efficiency``) selects a sampling domain.  With
    ``--energy-domain board`` the derived tokens-per-watt objective must
    evaluate per trial and the run must select a grid-endpoint frequency.
    """

    @classmethod
    def _make_command(cls):
        return gu.geopmopt_general_command(
            cls._config_path, cls._freq_low, cls._freq_high,
            metrics=[f'fom=regex:{_METRIC_REGEX}',
                     'tpw=expr:fom / power'],
            maximize='tpw', energy_domain='board')

    def test_completes_and_selects_grid_frequency(self):
        """The reserved-power efficiency objective completes and selects a grid
        endpoint, proving ``--energy-domain`` populated ``power``."""
        freq = self._best_frequency()
        self.assertIn(
            freq, (float(self._freq_low), float(self._freq_high)),
            msg=(f'the reserved-power efficiency objective selected {freq}, '
                 f'not a grid endpoint {self._freq_low}/{self._freq_high}'))


@gu.skip_unless_skopt()
@gu.skip_unless_cpu_frequency_control()
class TestGeopmoptTwoRegex(_GeopmoptHarness):
    """Two ``regex:`` metrics round-trip with a ``regex:``-based constraint
    (P0-3).

    Before the fix only a single regex figure of merit was honored and
    user-named regex metrics were absent from the evaluation context.  Here two
    distinct markers are scraped (``fom`` and ``aux``): ``fom`` is maximized
    while a non-binding ``aux`` constraint is enforced.  Both values must
    populate, the constraint must be satisfied, and the throughput objective
    must still select the high-frequency endpoint.
    """

    @classmethod
    def _make_command(cls):
        return gu.geopmopt_general_command(
            cls._config_path, cls._freq_low, cls._freq_high,
            metrics=[f'fom=regex:{_METRIC_REGEX}',
                     r'aux=regex:GEOPMOPT-AUX: ([0-9.]+)'],
            maximize='fom', constraints=['aux <= 2.0'])

    def test_selects_max_frequency_with_both_metrics(self):
        """Both regex metrics populate, the aux constraint holds, and the
        throughput objective selects the maximum grid frequency."""
        self.assertEqual(
            self._best_frequency(), float(self._freq_high),
            msg=('the two-regex objective did not select the maximum grid '
                 f'frequency {self._freq_high}; both metrics must populate and '
                 'the aux constraint (aux <= 2.0) must be satisfied'))


@gu.skip_unless_skopt()
@gu.skip_unless_cpu_frequency_control()
class TestGeopmoptParseRejection(unittest.TestCase):
    """An unmeasured metric reference is rejected before any trial runs (P0-4).

    ``expr:'fom / power'`` without ``--energy-domain`` names ``power``, which is
    not sampled on the general interface.  geopmopt must fail during objective
    construction -- before launching a single trial -- with a message that
    names the offending metric, rather than failing late during evaluation.
    """

    def test_unmeasured_power_reference_fails_fast(self):
        proc = gu.run_optimizer(
            '--sweep', 'cpu-freq@board',
            '--metric', 'fom=regex:GEOPMOPT-FOM: ([0-9.]+)',
            '--metric', 'tpw=expr:fom / power',
            '--maximize', 'tpw',
            '--trials', '1', '--n-initial-points', '1',
            '--', gu.probe_command())
        combined = proc.stdout + proc.stderr
        self.assertNotEqual(
            proc.returncode, 0,
            msg=f'geopmopt accepted an unmeasured power reference:\n{combined}')
        self.assertIn(
            'power', combined,
            msg=f'the error did not name the offending metric:\n{combined}')
        # A parse/setup-time rejection: no trial ran and no best config emitted.
        self.assertNotIn(
            'Best configuration', combined,
            msg=f'expected a fail-fast rejection, not a completed run:\n{combined}')


@gu.skip_unless_skopt()
@gu.skip_unless_cpu_frequency_control()
class TestGeopmoptConstraintQuoting(unittest.TestCase):
    """``--constraint`` requires the single quoted ``'NAME OP VALUE'`` form
    (P1-1).

    Splitting the constraint across three argv tokens (``--constraint p99 <=
    2.0``) makes ``--constraint`` capture only ``p99``; the run must error with
    a message that names the expected ``'NAME OP VALUE'`` form so the quoting
    requirement is discoverable.
    """

    def test_three_token_form_errors_with_quoting_hint(self):
        proc = gu.run_optimizer(
            '--sweep', 'cpu-freq@board',
            '--metric', 'fom=regex:GEOPMOPT-FOM: ([0-9.]+)',
            '--maximize', 'fom',
            '--constraint', 'p99', '<=', '2.0',
            '--trials', '1', '--n-initial-points', '1',
            '--', gu.probe_command())
        combined = proc.stdout + proc.stderr
        self.assertNotEqual(
            proc.returncode, 0,
            msg=f'the three-token --constraint form was accepted:\n{combined}')
        self.assertIn(
            'NAME OP VALUE', combined,
            msg=('the error did not point at the quoted constraint form:\n'
                 f'{combined}'))


if __name__ == '__main__':
    unittest.main()
