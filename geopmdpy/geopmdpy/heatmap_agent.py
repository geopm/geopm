#!/usr/bin/python3
#
#  Copyright (c) 2015 - 2026 Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#

"""Component-saturation heatmap agent for the GEOPM Python session interface.

This module provides a ``HeatmapAgent`` for use with the GEOPM Python
session interface.  It samples a set of per-component signals (CPU, DRAM,
uncore, and GPU power/activity/frequency) in-process through
``geopmdpy.pio`` and, when the session ends, renders a normalized heatmap
(one row per component instance -- every package and every GPU -- and
columns = time) that shows which component is saturated over the course of a
run.  A component pinned near its own maximum while the others sit idle reads
out as the bottleneck.

When both ``CPU_INSTRUCTIONS_RETIRED`` and ``CPU_CYCLES_THREAD`` are
available the agent derives one ``IPC`` row per package (socket) from the
per-sample counter deltas read at the package domain, and also emits each as
a trace column.  High IPC alongside modest DRAM/uncore
activity indicates a compute-bound region; low IPC with saturated DRAM or
uncore indicates a memory-bound region.  An external figure-of-merit (FOM)
time series may be overlaid with ``--fom`` to correlate FOM against the
telemetry.

The agent supplies its own default signal request set via
``signal_config_override``; users may pipe their own request list to
override it.  Rendering requires ``matplotlib`` (and ``pandas`` for the FOM
overlay); install them with ``pip install geopmdpy[heatmap]``.

Note: GEOPM has no NIC telemetry and no notion of data *flowing* between
components, so this shows per-component saturation over time rather than a
data-flow diagram.

Example usage:
    python3 -m geopmdpy.heatmap_agent -t 30
    python3 -m geopmdpy.heatmap_agent -t 60 --heatmap-out hm.png --fom fom.csv
    python3 -m geopmdpy.heatmap_agent -t 30 -o trace.csv --no-plot
    python3 -m geopmdpy.heatmap_agent --replot-csv trace.csv --heatmap-out hm.png
"""

import re
import sys

from . import pio
from . import topo
from .session import main
from .session import Agent

# The component heatmap benefits from finer sampling than the session's
# 100 ms default; an explicit '-p/--period' on the command line still takes
# precedence.
_PERIOD_DEFAULT = 0.05

# Candidate signals to sample, in the order they should appear (top to
# bottom).  Each entry is (signal_name, domain, human_label).  Every
# available domain index of the given domain is sampled, producing one row
# per index (e.g. one per package and one per GPU); any signal not granted
# on the platform is silently dropped.
_CANDIDATE_SIGNALS = (
    ('CPU_POWER', 'package', 'CPU power'),
    ('MSR::CPU_SCALABILITY_RATIO', 'package', 'CPU scalability'),
    ('CPU_FREQUENCY_STATUS', 'package', 'CPU core freq'),
    ('CPU_UNCORE_FREQUENCY_STATUS', 'package', 'CPU uncore freq'),
    ('DRAM_POWER', 'package', 'DRAM power'),
    ('GPU_POWER', 'gpu', 'GPU power'),
    ('GPU_CORE_ACTIVITY', 'gpu', 'GPU core activity'),
    ('GPU_UNCORE_ACTIVITY', 'gpu', 'GPU mem activity'),
    ('GPU_UTILIZATION', 'gpu', 'GPU utilization'),
)

# Short per-domain tag used to disambiguate row labels when a signal is
# sampled at more than one domain index (e.g. "CPU power pkg0").
_DOMAIN_TAG = {'package': 'pkg', 'gpu': 'gpu'}

# Signals that are physically bounded to the unit interval [0, 1].  Level Zero
# occasionally reports activity/utilization values outside this range; such
# readings are treated as missing (NaN) so they are ignored rather than
# clipped -- a single erroneous spike would otherwise dominate the per-row
# percentile normalization and wash the rest of the row to black.
_UNIT_INTERVAL_SIGNALS = frozenset((
    'GPU_CORE_ACTIVITY', 'GPU_UNCORE_ACTIVITY', 'GPU_UTILIZATION'))

# Cumulative counters used to derive the IPC rows.  Both must be granted for
# the derived IPC rows to appear.  They are read at the package domain so the
# derived value is per-socket, matching the other CPU signals; one IPC row is
# produced per package.
_IPC_INSTR = 'CPU_INSTRUCTIONS_RETIRED'
_IPC_CYCLES = 'CPU_CYCLES_THREAD'
_IPC_DOMAIN = 'package'
_IPC_LABEL = 'CPU IPC'

# Component groups rendered as separate, labeled heatmap panels (subplots),
# stacked top to bottom and sharing the time axis.  Each entry is
# (group_title, ordered base row labels, accent color).  The base labels must
# match the human labels in _CANDIDATE_SIGNALS (and _IPC_LABEL); every row
# whose base label falls in a group is drawn in that group's panel, in the
# order listed here.  This is also what fixes the row order (e.g. GPU
# utilization above GPU core activity within the GPU compute panel).
_ROW_GROUPS = (
    ('CPU compute',
     ('CPU power', 'CPU IPC', 'CPU scalability', 'CPU core freq'),
     'tab:blue'),
    ('CPU memory',
     ('CPU uncore freq', 'DRAM power'),
     'tab:green'),
    ('GPU compute',
     ('GPU power', 'GPU utilization', 'GPU core activity'),
     'tab:red'),
    ('GPU memory',
     ('GPU mem activity',),
     'tab:orange'),
)

# Physically achievable IPC upper bound.  A near-zero cycle delta (counter
# read jitter) can otherwise yield a huge IPC spike that dominates the
# per-row min-max normalization and washes the whole IPC row to black.
_IPC_MAX = 8.0

# Controls that turn on the fixed performance counters backing the IPC
# signals.  On some platforms FIXED_CTR0 (INST_RETIRED_ANY) is left disabled
# while FIXED_CTR1 (CPU_CLK_UNHALTED_THREAD) runs, so CPU_INSTRUCTIONS_RETIRED
# reads a constant and the derived IPC is a flat zero.  These are written
# inside the session at run_begin and reverted automatically on exit because
# the session snapshots (save_control) and restores every control it touched.
_IPC_ENABLE_CONTROLS = (
    'MSR::FIXED_CTR_CTRL:EN0_OS',
    'MSR::FIXED_CTR_CTRL:EN0_USR',
    'MSR::FIXED_CTR_CTRL:EN1_OS',
    'MSR::FIXED_CTR_CTRL:EN1_USR',
    'MSR::PERF_GLOBAL_CTRL:EN_FIXED_CTR0',
    'MSR::PERF_GLOBAL_CTRL:EN_FIXED_CTR1',
)


class HeatmapAgent(Agent):
    """Agent that renders a component-saturation heatmap for a GEOPM session.

    The session drives the timed loop and calls ``pio.read_batch()`` before
    each ``update_loop()``; this agent samples the cached signals, derives a
    per-sample IPC value, and renders the heatmap in ``run_end()``.  It is a
    monitoring agent: the only controls it writes are the fixed-counter enable
    bits needed for the derived IPC rows, and the session reverts those
    automatically when it ends.

    Command-line options:
      --heatmap-out   Output PNG path for the heatmap.
      --fom           Optional FOM CSV to overlay (columns: time_seconds,value).
      --no-plot       Collect the trace but skip rendering the heatmap.
      --replot-csv    Render an existing geopmsession trace and exit (no
                      session is run).

    Example:
        python3 -m geopmdpy.heatmap_agent -t 30 --heatmap-out heatmap.png
    """

    def __init__(self):
        """Initialize the HeatmapAgent."""
        self._out_path = 'component_heatmap.png'
        self._fom_path = None
        self._no_plot = False
        self._replot_csv = None

        # Signals selected for plotting, resolved from availability.
        self._signals = []

        # Batch indices for pushed signals.
        self._time_idx = None
        self._signal_idx = []
        self._instr_idx = []
        self._cycles_idx = []
        self._ipc_packages = []
        self._ipc_resolved = False
        self._counters_enabled = False

        # Accumulated samples.
        self._times = []
        self._rows = []
        self._ipc = []
        self._prev_counts = None
        self._cur_ipc = []

    def help(self):
        """Help documentation.

        """
        return ('The heatmap agent samples per-component power, activity, and '
                'frequency signals and renders a normalized component-saturation '
                'heatmap (rows=components, cols=time) when the session ends. It '
                'adds a derived IPC row and an optional --fom overlay to help '
                'distinguish compute-bound from memory-bound regions.')

    def update_parser(self, parser):
        """Add the heatmap options and adjust session defaults.

        Args:
            parser (argparse.ArgumentParser): The parser to update.

        Returns:
            argparse.ArgumentParser: The updated parser.
        """
        parser.add_argument('--heatmap-out', dest='heatmap_out',
                            default='component_heatmap.png',
                            help='Output PNG path for the heatmap. '
                                 'Default %(default)s.')
        parser.add_argument('--fom', dest='fom', default=None,
                            help='Optional figure-of-merit CSV to overlay '
                                 '(columns: time_seconds,value).')
        parser.add_argument('--no-plot', dest='no_plot', action='store_true',
                            help='Collect the trace but skip rendering the heatmap.')
        parser.add_argument('--replot-csv', dest='replot_csv', default=None,
                            help='Render a heatmap from an existing geopmsession '
                                 'trace CSV and exit without running a session. '
                                 'The PNG is written to --heatmap-out.')
        # Override the session's 100 ms default sampling period with a finer
        # default.  An explicit '-p/--period' on the command line still takes
        # precedence.
        parser.set_defaults(period=_PERIOD_DEFAULT)
        # The session's default '-' would read signal requests from standard
        # input, but this agent supplies its own default signal set via
        # signal_config_override(); correct the option's documentation.
        for action in parser._actions:
            if action.dest == 'config_path':
                action.help = (
                    'Input file containing GEOPM signal requests. The default '
                    '"-" uses the heatmap agent\'s built-in signal set of '
                    'available CPU, DRAM, uncore, and GPU metrics. Specify a '
                    'file path to trace a custom set of signal requests instead.')
                break
        return parser

    def update_args(self, args):
        """Store the heatmap options in the agent.

        Args:
            args (argparse.Namespace): Parsed command-line arguments.

        Returns:
            argparse.Namespace: The (possibly updated) arguments.
        """
        self._out_path = args.heatmap_out
        self._fom_path = args.fom
        self._no_plot = args.no_plot
        self._replot_csv = getattr(args, 'replot_csv', None)
        if self._replot_csv is not None:
            # Render the existing trace and exit before any session is opened;
            # no PlatformIO access or signal pushing is required.
            times, labels, rows, ipc_rows = _load_trace(self._replot_csv)
            _render_heatmap(times, labels, rows, ipc_rows, self._fom_path,
                            self._out_path)
            sys.exit(0)
        return args

    def signal_config_override(self):
        """Provide a default signal configuration for the heatmap.

        Selects the candidate component signals that are granted on this
        platform and requests every domain index of each with the "*"
        wildcard (e.g. all packages, all GPUs), prefixed by TIME for the
        heatmap's x-axis.

        Returns:
            str: Signal configuration string for the session.
        """
        names = set(pio.signal_names())
        granted = [(name, domain)
                   for name, domain, _ in _CANDIDATE_SIGNALS
                   if name in names]
        if not granted:
            raise RuntimeError(
                'HeatmapAgent: none of the candidate signals are available '
                'on this platform')
        lines = ['TIME board 0']
        lines += [f'{name} {domain} *' for name, domain in granted]
        return '\n'.join(lines) + '\n'

    def _resolve_ipc_packages(self):
        """Determine which packages can supply a derived IPC row.

        Populates ``self._ipc_packages`` from signal availability and the
        package count.  This is idempotent and independent of ``run_begin``
        because the session queries ``header_names`` (which needs the IPC
        column set) *before* it calls ``run_begin``; resolving here keeps the
        trace header and the pushed signal handles consistent.
        """
        if self._ipc_resolved:
            return
        names = set(pio.signal_names())
        if _IPC_INSTR in names and _IPC_CYCLES in names:
            self._ipc_packages = list(range(topo.num_domain(_IPC_DOMAIN)))
        else:
            self._ipc_packages = []
            sys.stderr.write(
                'note: skipping derived IPC rows (counters not both available)\n')
        self._ipc_resolved = True

    def _enable_ipc_counters(self):
        """Enable the fixed performance counters backing the IPC signals.

        Writes the FIXED_CTR_CTRL / PERF_GLOBAL_CTRL enable bits so
        ``CPU_INSTRUCTIONS_RETIRED`` and ``CPU_CYCLES_THREAD`` advance during
        the run.  Writes go through the session, which snapshots controls with
        ``pio.save_control()`` and restores them on exit, so the counter
        enable state is reverted automatically when the session ends.  Missing
        controls (e.g. on a platform without MSR access) are skipped with a
        note rather than aborting the run.
        """
        if self._counters_enabled or not self._ipc_packages:
            return
        available = set(pio.control_names())
        controls = [c for c in _IPC_ENABLE_CONTROLS if c in available]
        if not controls:
            sys.stderr.write(
                'note: fixed-counter enable controls unavailable; '
                'derived IPC may read zero\n')
            return
        # Snapshot every control so the session's restore_control() reverts
        # the counter enables when the run ends.
        pio.save_control()
        for name in controls:
            domain = pio.control_domain_type(name)
            for index in range(topo.num_domain(domain)):
                try:
                    pio.write_control(name, domain, index, 1.0)
                except RuntimeError as ex:
                    sys.stderr.write(
                        f'note: could not enable {name}: {ex}\n')
        self._counters_enabled = True

    def run_begin(self):
        """Push the signal handles for the sampled signals.

        Pushes TIME and the resolved component signals.  When the IPC counters
        are granted it also pushes the per-package counter handles and enables
        the backing fixed counters; those control writes are reverted
        automatically when the session ends.
        """
        if not self._signals:
            self._signals = self._discover_signals()
        self._time_idx = pio.push_signal('TIME', 'board', 0)
        self._signal_idx = [pio.push_signal(name, domain, index)
                            for name, domain, index, _ in self._signals]
        self._resolve_ipc_packages()
        self._enable_ipc_counters()
        self._instr_idx = []
        self._cycles_idx = []
        for pkg in self._ipc_packages:
            self._instr_idx.append(
                pio.push_signal(_IPC_INSTR, _IPC_DOMAIN, pkg))
            self._cycles_idx.append(
                pio.push_signal(_IPC_CYCLES, _IPC_DOMAIN, pkg))
        self._prev_counts = None
        self._cur_ipc = [float('nan')] * len(self._ipc_packages)

    def update_loop(self):
        """Sample the pushed signals and derive per-package IPC for this period.

        Called by the session after ``pio.read_batch()``.  Accumulates the
        per-component samples and, when the IPC counters are present,
        computes one IPC value per package from the counter deltas over the
        sample interval.
        """
        self._times.append(pio.sample(self._time_idx))
        self._rows.append([
            _sanitize_unit(pio.sample(idx), name in _UNIT_INTERVAL_SIGNALS)
            for (name, _domain, _index, _label), idx
            in zip(self._signals, self._signal_idx)])
        if self._instr_idx:
            counts = [(pio.sample(i), pio.sample(c))
                      for i, c in zip(self._instr_idx, self._cycles_idx)]
            if self._prev_counts is not None:
                self._cur_ipc = [
                    _delta_ipc(cur[0] - prev[0], cur[1] - prev[1])
                    for cur, prev in zip(counts, self._prev_counts)]
            self._prev_counts = counts
            self._ipc.append(list(self._cur_ipc))

    def header_names(self):
        """Return the derived trace column headers (one IPC per package).

        Returns:
            list of str: ['IPC-package-0', ...] when the counters are present,
                else [].
        """
        self._resolve_ipc_packages()
        return [f'IPC-{_IPC_DOMAIN}-{pkg}' for pkg in self._ipc_packages]

    def trace_out(self):
        """Return the derived per-package IPC values for the current period.

        Returns:
            list of str: One formatted IPC value per package, or an empty list.
        """
        if not self._instr_idx:
            return []
        return [f'{v:.4f}' for v in self._cur_ipc]

    def run_end(self):
        """Render the component-saturation heatmap to the output PNG.

        Skipped when ``--no-plot`` is set or no samples were collected.
        """
        if self._no_plot:
            return
        if not self._times:
            sys.stderr.write('note: no samples collected; nothing to plot\n')
            return
        labels = [label for _, _, _, label in self._signals]
        _render_heatmap(self._times, labels, self._rows, self._ipc_rows(),
                        self._fom_path, self._out_path)

    def _ipc_rows(self):
        """Return the derived per-package IPC rows for rendering.

        Returns:
            list of tuple: ``(label, series)`` pairs, one per package, or an
                empty list when no IPC counters were sampled.
        """
        if not self._ipc_packages or not self._ipc:
            return []
        series = list(zip(*self._ipc))
        multi = len(self._ipc_packages) > 1
        rows = []
        for pos, pkg in enumerate(self._ipc_packages):
            label = (f'{_IPC_LABEL} {_domain_tag(_IPC_DOMAIN)}{pkg}'
                     if multi else _IPC_LABEL)
            rows.append((label, list(series[pos])))
        return rows

    @staticmethod
    def _discover_signals():
        """Return the granted candidate signals expanded over all domains.

        Each granted candidate is expanded into one entry per domain index
        of its domain type (e.g. one per package, one per GPU).  Returns a
        list of ``(signal_name, domain, domain_index, label)`` tuples; the
        label is suffixed with the domain index when more than one index is
        sampled.
        """
        names = set(pio.signal_names())
        signals = []
        for name, domain, label in _CANDIDATE_SIGNALS:
            if name not in names:
                continue
            count = topo.num_domain(domain)
            for index in range(count):
                row_label = (f'{label} {_domain_tag(domain)}{index}'
                             if count > 1 else label)
                signals.append((name, domain, index, row_label))
        dropped = [name for name, _, _ in _CANDIDATE_SIGNALS if name not in names]
        if dropped:
            sys.stderr.write(
                f'note: skipping unavailable signals: {", ".join(dropped)}\n')
        if not signals:
            raise RuntimeError(
                'HeatmapAgent: none of the candidate signals are available '
                'on this platform')
        return signals


def _domain_tag(domain):
    """Return the short row-label tag for a domain type (e.g. package->pkg)."""
    return _DOMAIN_TAG.get(domain, domain)


def _signal_columns(columns, name):
    """Return the trace columns belonging to a signal, ordered by index.

    geopmsession writes board-domain signals without a suffix and other
    domains as ``NAME-domain-index``.  Returns a list of
    ``(domain, index, column)`` tuples ordered by domain index.

    Args:
        columns (list of str): Available trace column names.
        name (str): Signal name.

    Returns:
        list of tuple: ``(domain, index, column)`` for each matching column.
    """
    pattern = re.compile(re.escape(name) + r'-([A-Za-z_]+)-(\d+)$')
    matched = []
    for column in columns:
        if column == name:
            matched.append(('board', 0, column))
            continue
        found = pattern.match(column)
        if found:
            matched.append((found.group(1), int(found.group(2)), column))
    matched.sort(key=lambda entry: entry[1])
    return matched


def _match_column(columns, name, domain, index):
    """Return the trace column for a request, or None if absent.

    geopmsession writes board-domain signals without a domain suffix and
    other domains as ``NAME-domain-index``.

    Args:
        columns (list of str): Available trace column names.
        name (str): Signal name.
        domain (str): Domain type name.
        index (int): Domain index.

    Returns:
        str or None: The matching column name, or None.
    """
    for candidate in (f'{name}-{domain}-{index}', name):
        if candidate in columns:
            return candidate
    return None


def _sanitize_unit(value, bounded):
    """Ignore out-of-range readings for unit-interval signals.

    Args:
        value (float): Raw sampled value.
        bounded (bool): True when the signal is physically bounded to [0, 1].

    Returns:
        float: ``value`` unchanged, or NaN when ``bounded`` and the value
            falls outside [0, 1] (an erroneous Level Zero reading).  NaN
            samples are dropped by the per-row percentile normalization
            instead of being clipped to a bound.
    """
    if bounded and not 0.0 <= value <= 1.0:
        return float('nan')
    return value


def _delta_ipc(d_instr, d_cycles):
    """Return a bounded IPC from a counter delta, or NaN if not derivable.

    Guards against a non-positive cycle delta and clips to ``_IPC_MAX`` so a
    near-zero denominator cannot produce a spike that dominates the per-row
    normalization.

    Args:
        d_instr (float): Retired-instruction delta over the interval.
        d_cycles (float): Unhalted-cycle delta over the interval.

    Returns:
        float: IPC in ``[0, _IPC_MAX]``, or NaN.
    """
    if d_cycles <= 0:
        return float('nan')
    ipc = d_instr / d_cycles
    if ipc < 0.0:
        return float('nan')
    return min(ipc, _IPC_MAX)


def _ipc_series(instr, cycles):
    """Return a bounded per-sample IPC series from cumulative counters.

    Args:
        instr (numpy.ndarray): Cumulative retired instructions.
        cycles (numpy.ndarray): Cumulative unhalted cycles.

    Returns:
        numpy.ndarray: Per-sample IPC, clipped to ``[0, _IPC_MAX]``, with the
            first (delta-less) sample back-filled from the second.
    """
    import numpy as np
    d_instr = np.diff(instr, prepend=np.nan)
    d_cycles = np.diff(cycles, prepend=np.nan)
    with np.errstate(divide='ignore', invalid='ignore'):
        ipc = np.where(d_cycles > 0, d_instr / d_cycles, np.nan)
    ipc = np.clip(ipc, 0.0, _IPC_MAX)
    if len(ipc) > 1:
        ipc[0] = ipc[1]
    return ipc


def _ipc_from_trace(df, columns):
    """Derive or read per-package IPC rows from a loaded trace.

    Prefers deriving one IPC row per package from the deltas of the
    ``CPU_INSTRUCTIONS_RETIRED`` and ``CPU_CYCLES_THREAD`` counters when both
    are present per package; otherwise uses existing ``IPC`` columns (as
    written by this agent's trace_out).

    Args:
        df (pandas.DataFrame): The loaded trace.
        columns (list of str): Available trace column names.

    Returns:
        list of tuple: ``(label, series)`` pairs, one per package, or [].
    """
    instr_cols = _signal_columns(columns, _IPC_INSTR)
    cycles_cols = _signal_columns(columns, _IPC_CYCLES)
    if instr_cols and cycles_cols:
        cyc_by_key = {(domain, index): column
                      for domain, index, column in cycles_cols}
        multi = len(instr_cols) > 1
        rows = []
        for domain, index, instr_col in instr_cols:
            cycles_col = cyc_by_key.get((domain, index))
            if cycles_col is None:
                continue
            instr = df[instr_col].astype(float).to_numpy()
            cycles = df[cycles_col].astype(float).to_numpy()
            label = (f'{_IPC_LABEL} {_domain_tag(domain)}{index}'
                     if multi else _IPC_LABEL)
            rows.append((label, _ipc_series(instr, cycles).tolist()))
        if rows:
            return rows
    ipc_cols = _signal_columns(columns, 'IPC')
    if ipc_cols:
        multi = len(ipc_cols) > 1
        return [((f'{_IPC_LABEL} {_domain_tag(domain)}{index}'
                  if multi else _IPC_LABEL),
                 df[column].astype(float).tolist())
                for domain, index, column in ipc_cols]
    return []


def _load_trace(csv_path):
    """Load a geopmsession trace CSV into heatmap render inputs.

    Selects the candidate component columns that are present, resolves the
    TIME column, and derives (or reads) an IPC series.

    Args:
        csv_path (str): Path to an existing geopmsession trace CSV.

    Returns:
        tuple: ``(times, labels, rows, ipc_rows)`` suitable for
            ``_render_heatmap``.

    Raises:
        RuntimeError: pandas is unavailable, or the trace lacks a TIME column
            or any of the candidate component signals.
    """
    try:
        import pandas as pd
    except ImportError as ex:
        raise RuntimeError(
            'HeatmapAgent --replot-csv requires pandas; install with '
            '"pip install geopmdpy[heatmap]".') from ex

    df = pd.read_csv(csv_path)
    df.columns = [str(c).strip().strip('"') for c in df.columns]
    columns = list(df.columns)

    time_col = _match_column(columns, 'TIME', 'board', 0)
    if time_col is None:
        time_candidates = [c for c in columns if 'TIME' in c.upper()]
        if not time_candidates:
            raise RuntimeError(f'No TIME column found in {csv_path}')
        time_col = time_candidates[0]
    times = df[time_col].astype(float).tolist()

    labels = []
    cols = []
    bounded_cols = set()
    for name, domain, label in _CANDIDATE_SIGNALS:
        matched = _signal_columns(columns, name)
        multi = len(matched) > 1
        for col_domain, index, column in matched:
            labels.append(f'{label} {_domain_tag(col_domain)}{index}'
                          if multi else label)
            cols.append(column)
            if name in _UNIT_INTERVAL_SIGNALS:
                bounded_cols.add(column)
    if not cols:
        raise RuntimeError(
            f'None of the expected component signal columns are in {csv_path}')
    frame = df[cols].astype(float)
    # Ignore (rather than clip) erroneous out-of-range readings for the
    # unit-interval signals so a single Level Zero glitch does not distort the
    # per-row normalization.
    for column in bounded_cols:
        series = frame[column]
        frame[column] = series.where((series >= 0.0) & (series <= 1.0))
    rows = frame.values.tolist()

    ipc_rows = _ipc_from_trace(df, columns)
    return times, labels, rows, ipc_rows


def _base_label(label):
    """Return a row label with any trailing domain-index suffix removed.

    Multi-index rows are labeled like ``'CPU power pkg0'`` or
    ``'GPU power gpu3'``; the group lookup keys on the base label
    (``'CPU power'``), so strip the ``' pkg<N>'`` / ``' gpu<N>'`` suffix.

    Args:
        label (str): A row label, possibly with a domain-index suffix.

    Returns:
        str: The base label.
    """
    suffix = r' (?:%s)\d+$' % '|'.join(
        re.escape(tag) for tag in _DOMAIN_TAG.values())
    return re.sub(suffix, '', label)


def _group_rows(series_by_label):
    """Partition rows into the labeled component panels.

    Args:
        series_by_label (list of tuple): ``(label, row)`` pairs in the order
            they were sampled.

    Returns:
        list of tuple: ``(group_title, accent_color, [(label, row), ...])``
            for each non-empty group, in ``_ROW_GROUPS`` order, with any rows
            not matched by a group collected into a trailing "Other" panel.
    """
    by_base = {}
    for label, row in series_by_label:
        by_base.setdefault(_base_label(label), []).append((label, row))

    panels = []
    consumed = set()
    for title, base_labels, color in _ROW_GROUPS:
        rows_p = []
        for base in base_labels:
            for pair in by_base.get(base, []):
                rows_p.append(pair)
                consumed.add(pair[0])
        if rows_p:
            panels.append((title, color, rows_p))

    leftovers = [(label, row) for label, row in series_by_label
                 if label not in consumed]
    if leftovers:
        panels.append(('Other', '0.4', leftovers))
    return panels


def _render_heatmap(times, labels, rows, ipc_rows, fom_path, out_path):
    """Render the accumulated samples as a normalized heatmap PNG.

    Heavy plotting dependencies are imported here so that importing this
    module (and running the unit tests) does not require matplotlib.

    Each component group is drawn in its own labeled, color-accented panel
    (subplot), stacked top to bottom and sharing the time axis, with an
    optional figure-of-merit panel at the bottom.

    Args:
        times (list of float): Sample timestamps in seconds.
        labels (list of str): Component row labels.
        rows (list of list of float): Per-sample component values.
        ipc_rows (list of tuple): ``(label, series)`` pairs for derived IPC
            rows (one per package); may be empty.
        fom_path (str or None): Optional FOM CSV path to overlay.
        out_path (str): Output PNG path.
    """
    try:
        import numpy as np
        import matplotlib
        matplotlib.use('Agg')  # headless-safe
        import matplotlib.pyplot as plt
    except ImportError as ex:
        raise RuntimeError(
            'HeatmapAgent rendering requires matplotlib (and numpy); install '
            'with "pip install geopmdpy[heatmap]" or pass --no-plot.') from ex

    times = np.asarray(times, dtype=float)
    times = times - times[0]

    # Assemble every row (component signals then derived IPC) as a
    # (label, series) pair; grouping and ordering are resolved by label.
    series_by_label = []
    comp_matrix = np.asarray(rows, dtype=float).T
    for label, row in zip(labels, comp_matrix):
        series_by_label.append((label, row))
    for ipc_label, ipc_series in (ipc_rows or []):
        ipc_row = np.asarray(ipc_series, dtype=float)
        if len(ipc_row) > 1 and np.isnan(ipc_row[0]):
            ipc_row[0] = ipc_row[1]  # first period has no delta; back-fill
        series_by_label.append((ipc_label, ipc_row))

    # Normalize each row to its own 1st-99th percentile range rather than its
    # raw min-max.  A single garbage sample (e.g. a Level Zero counter glitch
    # reporting GPU_UTILIZATION in the thousands, or an MSR ratio spike) would
    # otherwise blow out the min-max scale and collapse every real value to
    # ~0, washing the whole row to black.  Percentile clipping ignores those
    # rare outliers so the row's actual dynamics remain visible.
    matrix = np.vstack([row for _, row in series_by_label])
    lo = np.nanpercentile(matrix, 1, axis=1, keepdims=True)
    hi = np.nanpercentile(matrix, 99, axis=1, keepdims=True)
    norm = np.clip((matrix - lo) / (hi - lo + 1e-9), 0.0, 1.0)
    norm_by_label = {label: norm[idx]
                     for idx, (label, _) in enumerate(series_by_label)}

    # Ignored samples (out-of-range unit-interval readings set to NaN) are
    # drawn in a neutral gray so they read as "no data" rather than as a
    # zero-activity (black) or full-activity (bright) cell.
    cmap = matplotlib.colormaps['magma'].copy()
    cmap.set_bad(color='0.5')

    panels = _group_rows(
        [(label, norm_by_label[label]) for label, _ in series_by_label])
    row_counts = [len(rows_p) for _, _, rows_p in panels]
    total_rows = sum(row_counts) or 1

    have_fom = fom_path is not None
    height_ratios = list(row_counts) + ([2] if have_fom else [])
    n_axes = len(panels) + (1 if have_fom else 0)

    # Keep each heatmap row thin, add a little padding per panel, and cap the
    # overall height so many-socket/many-GPU nodes stay readable.
    fig_h = min(0.3 * total_rows + 0.55 * len(panels)
                + (1.5 if have_fom else 0.5), 20.0)
    fig, axes = plt.subplots(
        n_axes, 1, figsize=(12, fig_h), sharex=True, squeeze=False,
        gridspec_kw={'height_ratios': height_ratios}, layout='constrained')
    axes = axes[:, 0]
    heat_axes = axes[:len(panels)]

    im = None
    x0, x1 = float(times[0]), float(times[-1])
    for ax, (title, color, rows_p) in zip(heat_axes, panels):
        panel = np.vstack([row for _, row in rows_p])
        n = len(rows_p)
        im = ax.imshow(panel, aspect='auto', cmap=cmap,
                       interpolation='nearest', extent=[x0, x1, n, 0],
                       vmin=0.0, vmax=1.0)
        ax.set_yticks(np.arange(n) + 0.5)
        ax.set_yticklabels([label for label, _ in rows_p])
        ax.set_ylabel(title, color=color, fontweight='bold')
        # Accent the panel frame in the group color to highlight the grouping.
        for spine in ax.spines.values():
            spine.set_color(color)
            spine.set_linewidth(2.0)

    fig.colorbar(im, ax=list(heat_axes), fraction=0.025, pad=0.02,
                 label='normalized activity (per-row 1st-99th pctile)')

    if have_fom:
        import pandas as pd
        ax_fom = axes[-1]
        fom = pd.read_csv(fom_path)
        fom.columns = [str(c).strip() for c in fom.columns]
        ft = fom.iloc[:, 0].astype(float)
        fv = fom.iloc[:, 1].astype(float)
        ax_fom.plot(ft - ft.iloc[0], fv, color='tab:blue')
        ax_fom.set_ylabel('FOM')
        ax_fom.grid(True, alpha=0.3)
    axes[-1].set_xlabel('time (s)')

    fig.suptitle('GEOPM component-saturation heatmap')
    fig.savefig(out_path, dpi=120)
    sys.stderr.write(f'wrote {out_path}\n')


if __name__ == '__main__':
    sys.exit(main(HeatmapAgent()))
