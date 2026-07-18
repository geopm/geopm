#
#  Copyright (c) 2015 - 2026 Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#

"""Skip guards and orchestration helpers for the geopmopt Phase 0
regression integration test.

Like the GPU activity agent helpers in ``_util.py``, these helpers are
geopmdpy-native: they probe the platform through ``geopmdpy.pio`` /
``geopmdpy.topo`` and drive ``geopmopt`` (``python -m geopmdpy.optimizer``)
as a subprocess, so the test carries no dependency on the HPC-runtime test
infrastructure under ``geopm/integration`` (``geopm_test_launcher``,
``geopmpy.io``, MPI).

Phase 0 of the geopmopt enhancement plan is a behavior-preserving refactor
(objective-helper extraction plus the ``TrialResult`` container).  These
helpers exist so the accompanying test can exercise the real end-to-end CLI
path -- which the mocked ``geopmdpy/test`` unit suite cannot -- and confirm
that the raw-metric and ``--efficiency`` workflows still run and select a
sane configuration.
"""

import os
import sys
import unittest

# CPU core-frequency control that ``geopmopt --cpu-frequency`` writes.
_CPU_FREQUENCY_CONTROL = 'CPU_FREQUENCY_MAX_CONTROL'
# Spread (Hz) between the low and high grid frequencies.  Wide enough that a
# single-threaded CPU-bound loop is measurably faster at the high setting.
_FREQUENCY_SPREAD_HZ = 500000000


def skip_unless_skopt():
    """Class/method decorator: skip when scikit-optimize is unavailable.

    ``geopmopt`` imports ``skopt`` at module load, so without it the tool
    cannot run at all.
    """
    try:
        import skopt  # noqa: F401
    except Exception:
        return unittest.skip(
            'scikit-optimize (skopt) is required to run geopmopt')
    return lambda obj: obj


def skip_unless_cpu_frequency_control():
    """Class/method decorator: skip when no live GEOPM service exposes a
    writable CPU core-frequency control.

    ``geopmopt`` applies the frequency cap live for every trial, so the
    control must be present in the session's control list (which reflects the
    GEOPM Access Service allow-list) and the service must be answering reads.
    """
    try:
        from geopmdpy import pio
        from geopmdpy import topo
        if _CPU_FREQUENCY_CONTROL not in pio.control_names():
            return unittest.skip(
                f'{_CPU_FREQUENCY_CONTROL} unavailable; no CPU frequency '
                'control granted or no live GEOPM service')
        # A readable sticker frequency confirms the service is answering.
        pio.read_signal('CPU_FREQUENCY_STICKER', topo.DOMAIN_BOARD, 0)
    except Exception:
        return unittest.skip(
            'unable to query the CPU frequency control from the GEOPM service')
    return lambda obj: obj


def skip_unless_cpu_energy():
    """Class/method decorator: skip when CPU energy is unreadable, which the
    ``--efficiency cpu`` objective requires.
    """
    try:
        from geopmdpy.optimizer import get_energy
        get_energy('cpu')
    except Exception:
        return unittest.skip(
            'CPU energy is unreadable; --efficiency cpu cannot be measured')
    return lambda obj: obj


def apps_dir():
    return os.path.join(os.path.dirname(os.path.abspath(__file__)), 'apps')


def probe_command():
    """Path to the trivial CPU-bound, frequency-sensitive workload script."""
    return os.path.join(apps_dir(), 'cpu_frequency_probe.sh')


def cpu_frequency_bounds():
    """Return two valid CPU core-frequency settings ``(low, high)`` in Hz.

    ``high`` is the platform sticker frequency; ``low`` is
    ``_FREQUENCY_SPREAD_HZ`` below it, clamped to the minimum available
    frequency.  The spread is wide enough that the CPU-bound probe is
    measurably faster at ``high`` than at ``low``, giving the optimizer an
    unambiguous, monotonic signal.
    """
    from geopmdpy import pio
    from geopmdpy import topo
    high = pio.read_signal('CPU_FREQUENCY_STICKER', topo.DOMAIN_BOARD, 0)
    try:
        floor = pio.read_signal(
            'CPU_FREQUENCY_MIN_AVAIL', topo.DOMAIN_BOARD, 0)
    except Exception:
        floor = high - _FREQUENCY_SPREAD_HZ
    low = max(floor, high - _FREQUENCY_SPREAD_HZ)
    return int(low), int(high)


def geopmopt_command(output_file, metric_regex, freq_low, freq_high,
                     minimize=False, efficiency_domain=None, trials=6,
                     n_initial_points=3, application_timeout=120):
    """Build the ``python -m geopmdpy.optimizer`` argv for a two-point CPU
    frequency sweep that runs the probe workload.

    The step is set to ``freq_high - freq_low`` so the search grid holds
    exactly the two endpoint frequencies.  ``--defer-write`` is intentionally
    omitted so geopmopt applies each candidate frequency live (with
    save/restore auto-revert) instead of delegating the write to the
    workload.
    """
    step = freq_high - freq_low
    cmd = [sys.executable, '-m', 'geopmdpy.optimizer',
           '--cpu-frequency', 'board',
           '--cpu-frequency-min', str(freq_low),
           '--cpu-frequency-max', str(freq_high),
           '--cpu-frequency-step', str(step),
           '--trials', str(trials),
           '--n-initial-points', str(n_initial_points),
           '--application-timeout', str(application_timeout),
           '--metric-regex', metric_regex,
           '--output-file', output_file,
           '--verbosity', '2']
    if minimize:
        cmd.append('--minimize')
    if efficiency_domain is not None:
        cmd += ['--efficiency', efficiency_domain]
    cmd += ['--', probe_command()]
    return cmd


def parse_best_config(text):
    """Parse a geopmopt configuration into ``{control_name: value}``.

    Each configuration line is ``CONTROL_NAME domain domain_idx value``; the
    control name and its numeric value are returned.
    """
    config = {}
    for line in text.splitlines():
        fields = line.split()
        if len(fields) < 4:
            continue
        try:
            config[fields[0]] = float(fields[3])
        except ValueError:
            continue
    return config


def best_cpu_frequency(config_text):
    """Return the CPU core-frequency (Hz) selected in a geopmopt config, or
    ``None`` when the control is absent."""
    return parse_best_config(config_text).get(_CPU_FREQUENCY_CONTROL)
