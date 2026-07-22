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
import re
import subprocess
import sys
import unittest

# CPU core-frequency control that ``geopmopt --sweep cpu-freq`` writes.
_CPU_FREQUENCY_CONTROL = 'CPU_FREQUENCY_MAX_CONTROL'
# Spread (Hz) between the low and high grid frequencies.  Wide enough that a
# single-threaded CPU-bound loop is measurably faster at the high setting.
_FREQUENCY_SPREAD_HZ = 500000000
# Margin (Hz) by which the grid's high endpoint sits below the sticker
# frequency.  Capping below sticker keeps every trial out of the turbo/boundary
# region, where frequency-vs-throughput noise can make the top grid point run
# no faster (or slower) than a lower one and select the wrong endpoint.
_STICKER_MARGIN_HZ = 200000000
# Probe busy-loop iteration count for the integration run.  Kept short (equal to
# the probe's own default) so each trial dissipates little heat: a long,
# sustained loop lets the package temperature climb across the many trials of a
# run, and a hotter high-frequency cap can then be throttled below a cooler
# low-frequency cap -- inverting the throughput ranking and selecting the wrong
# grid endpoint.  A short probe combined with PROBE_COOLDOWN_S below keeps every
# trial near the same thermal baseline.
PROBE_ITERS = 3000000
# Seconds to idle-sleep before each timed busy loop.  The sleep is outside the
# timed region, so it does not affect the throughput measurement; it simply lets
# the package shed heat accumulated by the previous trial so consecutive trials
# start from a comparable temperature.
PROBE_COOLDOWN_S = 5


def _probe_cpu():
    """Return a single CPU index to pin the probe to.

    Scheduler migration between cores -- each with its own competing load and
    cold caches -- is a large source of timing noise for a single-threaded
    busy loop.  Pinning removes it.  The highest CPU index is chosen because
    CPU 0 typically fields the bulk of kernel/interrupt work.
    """
    try:
        from geopmdpy import topo
        n_cpu = int(topo.num_domain(topo.DOMAIN_CPU))
        return max(0, n_cpu - 1)
    except Exception:
        return 0


def probe_env():
    """Return a process environment that runs a short, pinned probe.

    The probe honors ``GEOPMOPT_PROBE_ITERS``, ``GEOPMOPT_PROBE_COOLDOWN_S`` and
    ``GEOPMOPT_PROBE_CPU``; overriding them here makes both the optimizer's
    per-trial probe runs and the constraint-calibration probe run use the same
    short-measurement, cool-start, single-CPU-pinned configuration that keeps
    the grid endpoints comparable and low-noise.
    """
    env = dict(os.environ)
    env['GEOPMOPT_PROBE_ITERS'] = str(PROBE_ITERS)
    env['GEOPMOPT_PROBE_COOLDOWN_S'] = str(PROBE_COOLDOWN_S)
    env['GEOPMOPT_PROBE_CPU'] = str(_probe_cpu())
    return env


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


def skip_unless_energy_domain(domain):
    """Class/method decorator: skip when the given ``--energy-domain`` cannot be
    measured on the platform.

    The general reserved ``power``/``energy`` metrics are sampled through the
    same domain-to-signal mapping as the legacy ``--efficiency`` path, so a
    readable :func:`geopmdpy.optimizer.get_energy` for ``domain`` is the
    precondition for using them.
    """
    try:
        from geopmdpy.optimizer import get_energy
        get_energy(domain)
    except Exception:
        return unittest.skip(
            f'energy for domain {domain!r} is unreadable; the reserved '
            'power/energy metrics cannot be measured')
    return lambda obj: obj


def skip_unless_signal_readable(signal_name, domain_name='DOMAIN_BOARD'):
    """Class/method decorator: skip when ``signal_name`` is unreadable, which a
    ``signal:`` metric over it requires.
    """
    try:
        from geopmdpy import pio
        from geopmdpy import topo
        if signal_name not in pio.signal_names():
            return unittest.skip(
                f'{signal_name} is not available from the GEOPM service')
        pio.read_signal(signal_name, getattr(topo, domain_name), 0)
    except Exception:
        return unittest.skip(
            f'{signal_name} is unreadable; a signal: metric over it cannot be '
            'measured')
    return lambda obj: obj


def run_optimizer(*args, env=None):
    """Run ``python -m geopmdpy.optimizer`` with ``args`` and return the
    :class:`subprocess.CompletedProcess`.

    Unlike the :class:`_GeopmoptHarness` orchestration, this does not raise on a
    non-zero exit, so it suits the negative tests that assert a parse-time
    rejection or a quoting error.
    """
    cmd = [sys.executable, '-m', 'geopmdpy.optimizer', *args]
    return subprocess.run(
        cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE,
        universal_newlines=True, env=env)


def apps_dir():
    return os.path.join(os.path.dirname(os.path.abspath(__file__)), 'apps')


def probe_command():
    """Path to the trivial CPU-bound, frequency-sensitive workload script."""
    return os.path.join(apps_dir(), 'cpu_frequency_probe.sh')


def cpu_frequency_bounds():
    """Return two valid CPU core-frequency settings ``(low, high)`` in Hz.

    ``high`` is the sticker frequency less ``_STICKER_MARGIN_HZ``, keeping the
    grid's top endpoint safely below the turbo/boundary region where
    frequency-vs-throughput noise makes the probe flaky; ``low`` is
    ``_FREQUENCY_SPREAD_HZ`` below ``high``, clamped to the minimum available
    frequency.  The spread is wide enough that the CPU-bound probe is
    measurably faster at ``high`` than at ``low``, giving the optimizer an
    unambiguous, monotonic signal.
    """
    from geopmdpy import pio
    from geopmdpy import topo
    sticker = pio.read_signal('CPU_FREQUENCY_STICKER', topo.DOMAIN_BOARD, 0)
    high = sticker - _STICKER_MARGIN_HZ
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
           '--sweep', f'cpu-freq@board={freq_low}:{freq_high}:{step}',
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


def geopmopt_general_command(output_file, freq_low, freq_high, *, metrics,
                             maximize=None, minimize=None, constraints=None,
                             energy_domain=None, trials=6, n_initial_points=3,
                             application_timeout=120):
    """Build the ``python -m geopmdpy.optimizer`` argv for the general
    objective interface over the same two-point CPU frequency sweep as
    :func:`geopmopt_command`.

    ``metrics`` is a list of ``NAME=SOURCE`` strings emitted as repeated
    ``--metric`` flags; ``maximize``/``minimize`` name the single objective
    metric; ``constraints`` is a list of ``'NAME OP VALUE'`` strings emitted as
    repeated ``--constraint`` flags; ``energy_domain`` (when set) emits
    ``--energy-domain`` so the reserved ``power``/``energy`` metrics are
    sampled. ``--defer-write`` is omitted so each candidate frequency is applied
    live with save/restore auto-revert.
    """
    step = freq_high - freq_low
    cmd = [sys.executable, '-m', 'geopmdpy.optimizer',
           '--sweep', f'cpu-freq@board={freq_low}:{freq_high}:{step}',
           '--trials', str(trials),
           '--n-initial-points', str(n_initial_points),
           '--application-timeout', str(application_timeout),
           '--output-file', output_file,
           '--verbosity', '2']
    for metric in metrics:
        cmd += ['--metric', metric]
    if maximize is not None:
        cmd += ['--maximize', maximize]
    if minimize is not None:
        cmd += ['--minimize', minimize]
    for constraint in (constraints or []):
        cmd += ['--constraint', constraint]
    if energy_domain is not None:
        cmd += ['--energy-domain', energy_domain]
    cmd += ['--', probe_command()]
    return cmd


_FOM_RE = re.compile(r'GEOPMOPT-FOM:\s*([0-9.]+)')


def probe_fom_at_frequency(freq_hz):
    """Apply a CPU core-frequency cap, run the probe once, and return its FoM.

    Used to calibrate a binding figure-of-merit constraint: because the probe's
    throughput is monotonic in the applied frequency, a single measurement at
    one endpoint anchors the throughput scale. Controls are saved before and
    restored after the measurement, mirroring geopmopt's own auto-revert.
    """
    from geopmdpy import pio
    from geopmdpy import topo
    pio.save_control()
    try:
        pio.write_control(_CPU_FREQUENCY_CONTROL, topo.DOMAIN_BOARD, 0,
                          float(freq_hz))
        proc = subprocess.run(
            [probe_command()], stdout=subprocess.PIPE,
            stderr=subprocess.PIPE, universal_newlines=True, check=True,
            env=probe_env())
    finally:
        pio.restore_control()
    match = _FOM_RE.search(proc.stdout)
    if match is None:
        raise RuntimeError(
            f'probe did not print a figure of merit:\n{proc.stdout}')
    return float(match.group(1))
