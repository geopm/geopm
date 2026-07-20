#!/usr/bin/env python3
#
#  Copyright (c) 2015 - 2026 Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#

"""geopmgrid ``--sweep`` CLI integration test.

Phase F of the grid-sweep CLI plan (#4046) replaces the 28 per-control grid
flags with a single repeatable ``--sweep`` option plus ``--list-controls``.
The mocked unit suite under ``geopmdpy/test`` verifies the parsing and wiring
in isolation; this live-hardware test closes the end-to-end gap by driving the
real ``geopmgrid`` (``python -m geopmdpy.grid``) as a subprocess and asserting:

* ``--list-controls`` exits 0 and lists every control with the fixed-width
  header (a control whose range is unreadable on the current platform is shown
  as ``n/a`` rather than aborting the listing), and
* a ``--sweep CONTROL@DOMAIN=MIN:MAX:STEP`` specification produces a
  deterministic grid whose coordinate endpoints map to the requested control
  values -- the parity regression that the removed ``-min/-max/-step`` flags
  used to cover.

``--list-controls`` degrades gracefully without a live service, so its smoke
test always runs.  The parity test requires a live GEOPM service that grants
the CPU core-frequency control and skips cleanly otherwise.  It prints
configuration only (no ``--write``), so it is read-only and safe.
"""

import subprocess
import sys
import unittest

from . import _geopmopt_util as gu

# Preferred --sweep aliases, one per control, in catalog order.
_CONTROL_ALIASES = ('cpu-freq', 'uncore-freq', 'cpu-power', 'gpu-freq',
                    'gpu-power', 'board-power', 'prefetch')


def _run_grid(*args):
    """Run ``python -m geopmdpy.grid`` with ``args`` and return the result."""
    cmd = [sys.executable, '-m', 'geopmdpy.grid', *args]
    return subprocess.run(
        cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE,
        universal_newlines=True)


class TestGeopmgridListControls(unittest.TestCase):
    """``geopmgrid --list-controls`` smoke test."""

    def test_lists_all_controls(self):
        proc = _run_grid('--list-controls')
        self.assertEqual(proc.returncode, 0, msg=proc.stderr)
        lines = proc.stdout.splitlines()
        # A header line plus one row per control.
        self.assertGreaterEqual(len(lines), 1 + len(_CONTROL_ALIASES))
        for column in ('CONTROL', 'DOMAIN', 'UNITS', 'MIN', 'MAX', 'STEP'):
            self.assertIn(column, lines[0])
        for alias in _CONTROL_ALIASES:
            self.assertIn(
                alias, proc.stdout,
                msg=f'{alias} missing from --list-controls output')


@gu.skip_unless_cpu_frequency_control()
class TestGeopmgridSweep(unittest.TestCase):
    """``--sweep`` override parity regression."""

    def test_sweep_override_grid_endpoints(self):
        """A two-point override sweep maps coordinate 0/1 to the low/high
        endpoint control values."""
        low, high = gu.cpu_frequency_bounds()
        step = high - low  # exactly two grid points: low and high
        spec = f'cpu-freq@board={low}:{high}:{step}'

        # --coordinate-range reports the size of each grid dimension.
        rng = _run_grid('--sweep', spec, '--coordinate-range')
        self.assertEqual(rng.returncode, 0, msg=rng.stderr)
        sizes = rng.stdout.split()
        self.assertTrue(
            sizes and all(int(size) == 2 for size in sizes),
            msg=f'expected a two-point grid, got {rng.stdout!r}')

        # Coordinate 0 -> low endpoint on every dimension.
        lo = _run_grid('--sweep', spec, '--coordinate', *(['0'] * len(sizes)))
        self.assertEqual(lo.returncode, 0, msg=lo.stderr)
        self.assertEqual(
            gu.parse_best_config(lo.stdout).get(gu._CPU_FREQUENCY_CONTROL),
            float(low),
            msg=f'coordinate 0 did not map to {low}:\n{lo.stdout}')

        # Coordinate 1 -> high endpoint on every dimension.
        hi = _run_grid('--sweep', spec, '--coordinate', *(['1'] * len(sizes)))
        self.assertEqual(hi.returncode, 0, msg=hi.stderr)
        self.assertEqual(
            gu.parse_best_config(hi.stdout).get(gu._CPU_FREQUENCY_CONTROL),
            float(high),
            msg=f'coordinate 1 did not map to {high}:\n{hi.stdout}')


if __name__ == '__main__':
    unittest.main()
