#
#  Copyright (c) 2015 - 2026 Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#

"""Skip guards and orchestration helpers for the GPU activity agent
effectiveness integration test.

These helpers are intentionally geopmdpy-native: they probe the platform
through ``geopmdpy.pio``/``geopmdpy.topo`` and orchestrate subprocesses
with the Python standard library, so the integration test carries no
dependency on the HPC-runtime test infrastructure under
``geopm/integration/test`` (``geopm_test_launcher``, ``geopmpy.io``, MPI).
"""

import csv
import math
import os
import re
import shutil
import sys

import unittest

# Workload driver names (run through run_workload.sh).
LOCAL_DRIVER = 'gpu_activity_benchmark'
RESNET_DRIVER = 'ipex_resnet50_infer.py'
DECODE_DRIVER = 'torch_decode_infer.py'


def skip_unless_gpu():
    """Class/method decorator: skip when no GPU/GEOPM service is available.
    """
    try:
        from geopmdpy import pio
        from geopmdpy import topo
        pio.read_signal('GPU_ENERGY', topo.DOMAIN_GPU, 0)
    except Exception:
        return unittest.skip(
            'GPU_ENERGY is unreadable; no GPU or no live GEOPM service')
    return lambda obj: obj


def skip_unless_levelzero():
    """Class/method decorator: skip when the GPU activity signal the agent
    samples is unavailable (its primary Intel/LevelZero path).

    The agent reads the high-level ``GPU_CORE_ACTIVITY`` signal (see
    ``gpu_activity_agent.py``); this is the LevelZero-backed alias, not a
    ``LEVELZERO::``-prefixed name, so probe the alias the agent actually uses.
    """
    try:
        from geopmdpy import pio
        if 'GPU_CORE_ACTIVITY' not in pio.signal_names():
            return unittest.skip(
                'GPU_CORE_ACTIVITY unavailable; the GPU activity agent has no '
                'LevelZero activity signal on this platform')
    except Exception:
        return unittest.skip('unable to query signal names from GEOPM service')
    return lambda obj: obj


def skip_unless_workload():
    """Class/method decorator: skip when the inference workload cannot run.

    The default workload is a locally built SYCL/oneAPI benchmark.  It requires
    either an existing benchmark binary or a SYCL compiler on ``PATH`` (or in
    ``GEOPM_GPU_BENCH_CXX``).  The optional Python/container drivers still use
    the old ``GEOPM_GPU_WORKLOAD_NATIVE`` / container-engine checks.
    """
    script = workload_wrapper()
    if not os.path.exists(script):
        return unittest.skip(f'workload wrapper not found: {script}')
    build_script = os.path.join(apps_dir(), 'build_gpu_activity_benchmark.sh')
    build_dir = os.environ.get(
        'GEOPM_GPU_BENCH_BUILD_DIR', os.path.join(apps_dir(), 'build'))
    benchmark = os.path.join(build_dir, LOCAL_DRIVER)
    compiler = os.environ.get('GEOPM_GPU_BENCH_CXX')
    compiler_ok = bool(compiler and shutil.which(compiler)) or any(
        shutil.which(candidate) for candidate in ('icpx', 'dpcpp'))
    if os.environ.get('GEOPM_GPU_WORKLOAD_BACKEND', 'local') == 'local':
        if not os.path.exists(build_script):
            return unittest.skip(
                f'local benchmark build script not found: {build_script}')
        if not os.path.exists(benchmark) and not compiler_ok:
            return unittest.skip(
                'local SYCL benchmark requires an existing binary or a SYCL '
                'compiler (icpx, dpcpp, or GEOPM_GPU_BENCH_CXX)')
        return lambda obj: obj
    if os.environ.get('GEOPM_GPU_WORKLOAD_NATIVE') == '1':
        import importlib.util
        for mod in ('torch', 'intel_extension_for_pytorch'):
            if importlib.util.find_spec(mod) is None:
                return unittest.skip(
                    f'native workload requires the "{mod}" module')
    else:
        engine = os.environ.get('GEOPM_GPU_CONTAINER_ENGINE', 'docker')
        if shutil.which(engine) is None:
            return unittest.skip(
                f'container engine "{engine}" not found on PATH')
    return lambda obj: obj


def apps_dir():
    return os.path.join(os.path.dirname(os.path.abspath(__file__)), 'apps')


def workload_wrapper():
    return os.path.join(apps_dir(), 'run_workload.sh')


def workload_command(driver, extra_args):
    """Build the command that runs ``driver`` (a file name under ``apps/``)
    through the container/native wrapper, forwarding ``extra_args``."""
    return [workload_wrapper(), driver] + [str(a) for a in extra_args]


def build_monitor_config(path):
    """Write a ``geopmsession`` signal config that traces energy, frequency,
    and activity across every discovered GPU domain.

    The signals are emitted at their native domain so the whole board is
    captured (energy is later summed across the per-GPU columns).

    Returns:
        list[str]: the request lines written, for logging/debugging.
    """
    from geopmdpy import pio
    from geopmdpy import topo

    names = pio.signal_names()
    lines = ['TIME board 0']
    for sig in ('GPU_ENERGY', 'GPU_CORE_FREQUENCY_STATUS', 'GPU_CORE_ACTIVITY'):
        if sig not in names:
            continue
        dom = pio.signal_domain_type(sig)
        dom_name = topo.domain_name(dom)
        for idx in range(topo.num_domain(dom)):
            lines.append(f'{sig} {dom_name} {idx}')
    with open(path, 'w') as fid:
        fid.write('\n'.join(lines) + '\n')
    return lines


def read_trace(path):
    """Read a geopmsession CSV trace into a list of dict rows (float values).

    Column names follow the session convention: ``TIME`` for board-domain
    signals and ``SIGNAL-domain-idx`` (e.g. ``GPU_ENERGY-gpu-0``) otherwise.
    """
    rows = []
    with open(path, newline='') as fid:
        reader = csv.DictReader(fid)
        for raw in reader:
            row = {}
            for key, val in raw.items():
                if key is None:
                    continue
                try:
                    row[key.strip()] = float(val)
                except (TypeError, ValueError):
                    row[key.strip()] = math.nan
            rows.append(row)
    return rows


def _columns_with_prefix(rows, prefix):
    if not rows:
        return []
    return [c for c in rows[0] if c.startswith(prefix)]


def energy_joules(rows):
    """Sum of per-domain ``GPU_ENERGY`` deltas (last minus first) over the run.

    ``GPU_ENERGY`` is a monotonically increasing joules counter, so the
    delta over the trace window is the energy consumed during the run.
    """
    cols = _columns_with_prefix(rows, 'GPU_ENERGY')
    if not cols or len(rows) < 2:
        return math.nan
    total = 0.0
    for col in cols:
        first = rows[0][col]
        last = rows[-1][col]
        if math.isnan(first) or math.isnan(last):
            continue
        total += last - first
    return total


def max_freq_std_hz(rows):
    """Maximum standard deviation (Hz) of any ``GPU_CORE_FREQUENCY_STATUS``
    column, i.e. the largest observed frequency excursion across GPU domains.
    """
    cols = _columns_with_prefix(rows, 'GPU_CORE_FREQUENCY_STATUS')
    best = 0.0
    for col in cols:
        vals = [r[col] for r in rows if not math.isnan(r[col])]
        if len(vals) > 1:
            mean = sum(vals) / len(vals)
            var = sum((v - mean) ** 2 for v in vals) / len(vals)
            best = max(best, math.sqrt(var))
    return best


def parse_fom(text):
    """Extract the ``FOM (<unit>): <float>`` figure of merit from workload
    output (e.g. ``FOM (images/sec):`` or ``FOM (tokens/sec):``)."""
    match = re.search(r'FOM \([^)]*\):\s*([0-9.]+)', text)
    if match is None:
        raise RuntimeError(
            'Could not parse "FOM (<unit>): <n>" from workload output:\n'
            + text)
    return float(match.group(1))


def parse_frequency_requests(text):
    """Extract the agent's ``GPU Frequency Requests: <N>`` summary counter."""
    match = re.search(r'GPU Frequency Requests:\s*(\d+)', text)
    return int(match.group(1)) if match else 0


def agent_command(phi, run_time, period, trace_path, report_path):
    """Command to run the Python GPU activity agent (frequency writer)."""
    return [sys.executable, '-m', 'geopmdpy.gpu_activity_agent',
            '-t', str(run_time),
            '-p', str(period),
            '--phi', str(phi),
            '-o', trace_path,
            '-r', report_path]


def geopmsession_command(config_path, trace_path, period, workload_cmd,
                         report_path=None):
    """Command to run a read-only geopmsession monitor that launches the
    workload and exits when it finishes."""
    cmd = ['geopmsession', '-i', config_path, '-p', str(period),
           '-o', trace_path]
    if report_path is not None:
        cmd += ['-r', report_path]
    cmd += ['--'] + list(workload_cmd)
    return cmd
