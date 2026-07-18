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
  GEOPMOPT_PROBE_ITERS   Busy-loop iteration count in the probe (default 3e6).
"""

import os
import shutil
import subprocess
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
    def setUpClass(cls):
        cls._freq_low, cls._freq_high = gu.cpu_frequency_bounds()
        cls._tmpdir = tempfile.mkdtemp(prefix=f'geopmopt-{cls.__name__}-')
        cls._config_path = os.path.join(cls._tmpdir, 'best.config')
        cls._command = gu.geopmopt_command(
            cls._config_path, _METRIC_REGEX, cls._freq_low, cls._freq_high,
            minimize=cls.MINIMIZE, efficiency_domain=cls.EFFICIENCY_DOMAIN)
        proc = subprocess.run(
            cls._command, stdout=subprocess.PIPE, stderr=subprocess.PIPE,
            universal_newlines=True)
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

        Higher CPU core frequency shortens the single-threaded busy loop, so
        the throughput FoM is monotonically increasing in frequency and the
        optimizer must choose the grid's high endpoint.
        """
        self.assertEqual(
            self._best_frequency(), float(self._freq_high),
            msg=('maximizing throughput did not select the highest grid '
                 f'frequency {self._freq_high}'))


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


if __name__ == '__main__':
    unittest.main()
