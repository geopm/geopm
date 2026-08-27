#!/usr/bin/python3
#
#  Copyright (c) 2015 - 2026 Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#

"""GPU activity agent for the GEOPM Python session interface.

This module provides a ``GPUActivityAgent`` for use with the GEOPM
Python session interface.  It is a pure-Python reimplementation of the
C++ ``GPUActivityAgent`` (plugin name ``gpu_activity``) that drives GPU
core frequency based on GPU compute activity and utilization, without
requiring the GEOPM HPC Runtime (Controller, MPI/OpenMP interposition,
or DBus profile registration).

The agent selects a per-GPU-domain core frequency by interpolating
between an efficient frequency and the maximum available frequency.
The interpolation is driven by the ``GPU_CORE_ACTIVITY`` signal scaled
by ``GPU_UTILIZATION``.  When the Level Zero ``GPU_CORE_ACTIVITY`` signal
is unavailable (e.g. on the DRM ``xe`` driver), the agent instead derives
a GPU busy fraction from the monotonic ``DRM::IDLE_RESIDENCY`` counter.
A single ``--phi`` knob (0.0 - 1.0) biases the selection toward
performance (phi < 0.5) or energy savings (phi > 0.5).

Example usage:
    python -m geopmdpy.gpu_activity_agent -t 30 -p 0.02
    python -m geopmdpy.gpu_activity_agent -t 30 -p 0.02 --phi 0.8
"""

import math
import sys

from . import pio
from . import topo
from .session import main
from .session import Agent

_POLICY_PHI_DEFAULT = 0.5

# The C++ gpu_activity agent (plugin 'gpu_activity') samples GPU activity
# every 20 ms.  The geopmdpy session defaults to a 100 ms sampling period;
# override the default to match the C++ agent (an explicit '-p/--period'
# on the command line still takes precedence).
_PERIOD_DEFAULT = 0.02

# Frequencies are interpolated between an efficient frequency and the
# maximum available frequency.  Source signals for the efficient
# frequency, in order of preference.
_FE_CONSTCONFIG = 'CONST_CONFIG::GPU_FREQUENCY_EFFICIENT_HIGH_INTENSITY'
_FE_SIG_NAME = 'LEVELZERO::GPU_CORE_FREQUENCY_EFFICIENT'

# Level Zero stall signal.  When available it reduces the compute activity
# used for the frequency decision (stall cycles are not frequency sensitive).
_STALL_SIG_NAME = 'LEVELZERO::METRIC:XVE_STALL'

# GPU compute activity at or above this fraction is treated as an active
# region for the ROI on-time/on-energy tracking reported in the summary.
_GPU_ACTIVITY_CUTOFF = 0.05

# Number of consecutive idle samples (utilization at/near zero) before the
# agent drops a GPU chip to the minimum frequency when energy biased.
_GPU_IDLE_TIMER_RESET = 10


class GPUActivityAgent(Agent):
    """Agent that sets GPU core frequency based on GPU activity.

    The agent reimplements the control logic of the C++
    ``GPUActivityAgent`` plugin for use with the GEOPM Python session.
    The session drives the timed loop and calls ``pio.read_batch()``
    before each ``update_loop()``; this agent samples the cached
    signals, computes a frequency request per agent domain, and writes
    the GPU core frequency min/max controls when the request changes.

    Command-line options:
      --phi   GPU frequency bias in the range [0.0, 1.0].  Values below
              0.5 bias toward performance, values above 0.5 bias toward
              energy savings.  Default 0.5.

    Example:
        python -m geopmdpy.gpu_activity_agent -t 30 -p 0.02 --phi 0.5
    """

    def __init__(self):
        """Initialize the GPUActivityAgent."""
        self._phi = _POLICY_PHI_DEFAULT
        self._hi_res = False

        self._agent_domain = None
        self._agent_domain_count = 0

        # batch indices for pushed signals/controls, per agent domain
        self._activity_idx = []
        self._utilization_idx = []
        self._freq_min_idx = []
        self._freq_max_idx = []
        # last value written to each control, used for change detection
        self._freq_min_last = []
        self._freq_max_last = []

        # Level Zero XVE_STALL activity: when the signal is available it
        # reduces the compute activity used for the frequency decision (the
        # raw activity is still used for ROI tracking).  Level-Zero only.
        self._has_stall = False
        self._stall_idx = []

        # Per-agent-domain idle timer.  When energy biased (phi >= 0.5) and the
        # utilization stays at/near zero for _GPU_IDLE_TIMER_RESET consecutive
        # samples, the chip is dropped to the minimum frequency.  Level-Zero only.
        self._gpu_idle_timer = []
        self._gpu_idle_samples = []

        # ROI proxy tracking (Level-Zero only).  Energy and time are sampled at
        # the GPU domain and used to report per-GPU active-region and on
        # time/energy; they do not affect the control algorithm.  A GPU's
        # tracked activity is taken from the first agent-domain unit of that GPU.
        self._num_gpu = 0
        self._agent_units_per_gpu = 1
        self._energy_idx = []
        self._prev_time = math.nan
        self._prev_gpu_energy = []
        self._gpu_region_active = []
        self._gpu_active_region_start = []
        self._gpu_active_region_stop = []
        self._gpu_active_energy_start = []
        self._gpu_active_energy_stop = []
        self._gpu_on_time = []
        self._gpu_on_energy = []

        # Activity source: 'levelzero' samples GPU_CORE_ACTIVITY directly
        # (and GPU_UTILIZATION when available); 'drm_idle' derives a GPU busy
        # fraction from the monotonic DRM::IDLE_RESIDENCY counter (used when
        # the Level Zero activity signal is unavailable, e.g. on the xe
        # driver).  GPU_UTILIZATION is not exposed by every activity source
        # (e.g. the DCGM IOGroup provides GPU_CORE_ACTIVITY but not
        # GPU_UTILIZATION), so it is treated as optional.
        self._activity_source = None
        self._has_utilization = False
        self._idle_idx = []
        self._time_idx = None
        self._idle_last = []
        self._time_last = math.nan

        self._freq_gpu_min = 0.0
        self._freq_gpu_max = 0.0
        self._freq_gpu_efficient = 0.0

        # Resolved values after phi is applied, retained for the summary.
        self._resolved_f_gpu_max = 0.0
        self._resolved_f_gpu_efficient = 0.0
        self._f_range = 0.0

        self._frequency_requests = 0
        self._frequency_clipped = 0

    def help(self):
        """Help documentation.

        """
        return ('The gpu_activity agent sets GPU core frequency based on GPU '
                'compute activity and utilization. Use --phi to bias frequency '
                'selection toward performance (phi < 0.5) or energy savings '
                '(phi > 0.5).')

    def update_parser(self, parser):
        """Add the --phi argument to the parser.

        Args:
            parser (argparse.ArgumentParser): The parser to update.

        Returns:
            argparse.ArgumentParser: The updated parser.
        """
        parser.add_argument('--phi', dest='phi', type=float,
                            default=_POLICY_PHI_DEFAULT,
                            help='GPU frequency bias in the range [0.0, 1.0]. '
                                 'Lower values favor performance, higher values '
                                 'favor energy savings. Default %(default)s.')
        parser.add_argument('--hi-res', action='store_true',
                            help='Measure signals at finest granularity (all domains/indices)')
        # Override the session's 100 ms default sampling period with the
        # 20 ms period used by the C++ gpu_activity agent.  An explicit
        # '-p/--period' on the command line still takes precedence.
        parser.set_defaults(period=_PERIOD_DEFAULT)
        # The session's default '-' would read signal requests from standard
        # input, but this agent supplies its own default signal set via
        # signal_config_override(); correct the option's documentation.
        for action in parser._actions:
            if action.dest == 'config_path':
                action.help = (
                    'Input file containing GEOPM signal requests. The default '
                    '"-" uses the gpu_activity agent\'s built-in signal set '
                    '(TIME, GPU core frequency, activity, utilization, and/or '
                    'DRM idle residency, depending on availability). Specify a '
                    'file path to trace a custom set of signal requests instead.')
                break
        return parser

    def update_args(self, args):
        """Validate and store the --phi argument.

        Args:
            args (argparse.Namespace): Parsed command-line arguments.

        Returns:
            argparse.Namespace: The (possibly updated) arguments.

        Raises:
            RuntimeError: The phi value is outside the range [0.0, 1.0].
        """
        if math.isnan(args.phi) or args.phi < 0.0 or args.phi > 1.0:
            raise RuntimeError(
                f'--phi value out of range: {args.phi}. '
                'Acceptable values are in the range [0.0, 1.0].')
        self._phi = args.phi
        self._hi_res = getattr(args, 'hi_res', False)
        return args

    def signal_config_override(self):
        """Provide a default trace configuration.

        Provided so the agent can be run with ``--signal-config -`` and
        an empty stdin.  ``geopmdpy.session.main()`` always uses this
        agent-provided configuration when ``--signal-config`` is ``-``
        (standard input is ignored), so a custom request list must be
        supplied via a file path (``--signal-config FILE``) instead.

        Returns:
            str: Signal configuration string for the session.
        """
        names = pio.signal_names()
        if self._hi_res:
            suffix = ' * *'
        else:
            suffix = ' board 0'
        lines = ['TIME board 0']
        if 'GPU_CORE_FREQUENCY_STATUS' in names:
            lines.append('GPU_CORE_FREQUENCY_STATUS' + suffix)
        if 'GPU_CORE_ACTIVITY' in names:
            lines.append('GPU_CORE_ACTIVITY' + suffix)
        if _STALL_SIG_NAME in names:
            lines.append(_STALL_SIG_NAME + suffix)
        if 'GPU_UTILIZATION' in names:
            lines.append('GPU_UTILIZATION' + suffix)
        if 'GPU_ENERGY' in names:
            lines.append('GPU_ENERGY' + suffix)
        if 'DRM::IDLE_RESIDENCY' in names:
            lines.append('DRM::IDLE_RESIDENCY' + suffix)
        return '\n'.join(lines) + '\n'

    def run_begin(self):
        """Resolve the agent domain and push signals and controls.

        Determines the coarsest domain shared by the required GPU
        signals and controls, pushes the per-domain signals and
        controls, reads the static frequency characterization, and
        saves the current control settings for restoration on exit.

        Raises:
            RuntimeError: No GPUs are present, the required signals and
                controls are not available at the GPU or GPU_CHIP domain,
                or the efficient frequency is out of range.
        """
        num_gpu = topo.num_domain(topo.DOMAIN_GPU)
        if num_gpu == 0:
            raise RuntimeError('GPUActivityAgent requires at least one GPU')

        # Prefer the Level Zero compute-activity signal; fall back to a
        # busy fraction derived from the DRM idle-residency counter.
        all_signals = pio.signal_names()
        if 'GPU_CORE_ACTIVITY' in all_signals:
            self._activity_source = 'levelzero'
        elif 'DRM::IDLE_RESIDENCY' in all_signals:
            self._activity_source = 'drm_idle'
        else:
            raise RuntimeError(
                'GPUActivityAgent: no GPU activity signal available; expected '
                'GPU_CORE_ACTIVITY (Level Zero) or DRM::IDLE_RESIDENCY')

        # GPU_UTILIZATION scales the compute activity but is not provided by
        # every activity source (e.g. the DCGM IOGroup exposes
        # GPU_CORE_ACTIVITY without GPU_UTILIZATION).  When it is absent the
        # activity is used directly (utilization treated as 1.0).
        self._has_utilization = (self._activity_source == 'levelzero' and
                                 'GPU_UTILIZATION' in all_signals)

        # The Level Zero XVE_STALL signal, when present, reduces the compute
        # activity used for the frequency decision.  Level-Zero only.
        self._has_stall = (self._activity_source == 'levelzero' and
                           _STALL_SIG_NAME in all_signals)

        # Use the coarsest granularity supported by any of the controls
        # or signals used by the control algorithm.
        domains = [
            pio.control_domain_type('GPU_CORE_FREQUENCY_MIN_CONTROL'),
            pio.control_domain_type('GPU_CORE_FREQUENCY_MAX_CONTROL'),
        ]
        # GPU_CORE_FREQUENCY_STATUS is optional and only used for the
        # default trace configuration; include it in the domain
        # resolution only when the platform provides it.
        if 'GPU_CORE_FREQUENCY_STATUS' in all_signals:
            domains.append(pio.signal_domain_type('GPU_CORE_FREQUENCY_STATUS'))
        if self._activity_source == 'levelzero':
            domains.append(pio.signal_domain_type('GPU_CORE_ACTIVITY'))
            if self._has_stall:
                domains.append(pio.signal_domain_type(_STALL_SIG_NAME))
            if self._has_utilization:
                domains.append(pio.signal_domain_type('GPU_UTILIZATION'))
        else:
            domains.append(pio.signal_domain_type('DRM::IDLE_RESIDENCY'))
        # In GEOPM the coarsest domain has the smallest domain-type value.
        self._agent_domain = min(domains)

        if self._agent_domain not in (topo.DOMAIN_GPU, topo.DOMAIN_GPU_CHIP):
            raise RuntimeError(
                'GPUActivityAgent: required signals and controls do not exist '
                'at the GPU or GPU_CHIP domain')

        self._agent_domain_count = topo.num_domain(self._agent_domain)

        self._activity_idx = []
        self._utilization_idx = []
        self._stall_idx = []
        self._idle_idx = []
        self._freq_min_idx = []
        self._freq_max_idx = []
        self._gpu_idle_timer = []
        self._gpu_idle_samples = []
        for domain_idx in range(self._agent_domain_count):
            if self._activity_source == 'levelzero':
                self._activity_idx.append(
                    pio.push_signal('GPU_CORE_ACTIVITY', self._agent_domain, domain_idx))
                if self._has_stall:
                    self._stall_idx.append(
                        pio.push_signal(_STALL_SIG_NAME, self._agent_domain, domain_idx))
                if self._has_utilization:
                    self._utilization_idx.append(
                        pio.push_signal('GPU_UTILIZATION', self._agent_domain, domain_idx))
            else:
                self._idle_idx.append(
                    pio.push_signal('DRM::IDLE_RESIDENCY', self._agent_domain, domain_idx))
            self._freq_min_idx.append(
                pio.push_control('GPU_CORE_FREQUENCY_MIN_CONTROL', self._agent_domain, domain_idx))
            self._freq_max_idx.append(
                pio.push_control('GPU_CORE_FREQUENCY_MAX_CONTROL', self._agent_domain, domain_idx))
            self._gpu_idle_timer.append(_GPU_IDLE_TIMER_RESET)
            self._gpu_idle_samples.append(0)
        if self._activity_source == 'drm_idle':
            # TIME is shared across domains; used to convert the idle-residency
            # counter into a busy fraction over each sample interval.
            self._time_idx = pio.push_signal('TIME', topo.DOMAIN_BOARD, 0)
            self._idle_last = [math.nan] * self._agent_domain_count
            self._time_last = math.nan
        if self._activity_source == 'levelzero':
            # ROI proxy tracking.  Energy and time are sampled at the GPU
            # domain for reporting only; a GPU's tracked activity is taken
            # from the first agent-domain unit of that GPU.
            self._num_gpu = num_gpu
            self._agent_units_per_gpu = self._agent_domain_count // num_gpu
            self._time_idx = pio.push_signal('TIME', topo.DOMAIN_BOARD, 0)
            self._energy_idx = []
            energy_domain = pio.signal_domain_type('GPU_ENERGY')
            for gpu_idx in range(num_gpu):
                self._energy_idx.append(
                    pio.push_signal('GPU_ENERGY', energy_domain, gpu_idx))
            self._prev_time = math.nan
            self._prev_gpu_energy = [math.nan] * num_gpu
            self._gpu_region_active = [False] * num_gpu
            self._gpu_active_region_start = [0.0] * num_gpu
            self._gpu_active_region_stop = [0.0] * num_gpu
            self._gpu_active_energy_start = [0.0] * num_gpu
            self._gpu_active_energy_stop = [0.0] * num_gpu
            self._gpu_on_time = [0.0] * num_gpu
            self._gpu_on_energy = [0.0] * num_gpu
        self._freq_min_last = [math.nan] * self._agent_domain_count
        self._freq_max_last = [math.nan] * self._agent_domain_count

        self._freq_gpu_min = pio.read_signal('GPU_CORE_FREQUENCY_MIN_AVAIL', topo.DOMAIN_BOARD, 0)
        self._freq_gpu_max = pio.read_signal('GPU_CORE_FREQUENCY_MAX_AVAIL', topo.DOMAIN_BOARD, 0)

        if _FE_CONSTCONFIG in all_signals:
            self._freq_gpu_efficient = pio.read_signal(_FE_CONSTCONFIG, topo.DOMAIN_BOARD, 0)
        elif _FE_SIG_NAME in all_signals:
            self._freq_gpu_efficient = pio.read_signal(_FE_SIG_NAME, topo.DOMAIN_BOARD, 0)
        else:
            self._freq_gpu_efficient = (self._freq_gpu_max + self._freq_gpu_min) / 2

        if (self._freq_gpu_efficient > self._freq_gpu_max or
                self._freq_gpu_efficient < self._freq_gpu_min):
            raise RuntimeError(
                'GPUActivityAgent: GPU efficient frequency out of range: '
                f'{self._freq_gpu_efficient}')

        self._frequency_requests = 0
        self._frequency_clipped = 0

        pio.save_control()

    def update_loop(self):
        """Sample GPU activity and adjust GPU frequency.

        Called by the session after ``pio.read_batch()``.  Samples the
        cached activity and utilization signals, computes a frequency
        request per agent domain, and writes the GPU core frequency
        controls when the request differs from the last written value.
        """
        f_gpu_range = self._freq_gpu_max - self._freq_gpu_efficient
        phi = self._phi

        # Default phi = 0.5 spans the full efficient-to-max range.
        self._resolved_f_gpu_max = self._freq_gpu_max
        self._resolved_f_gpu_efficient = self._freq_gpu_efficient

        if phi > 0.5:
            # Energy biased: scale F_max down toward F_efficient.
            self._resolved_f_gpu_max = max(
                self._freq_gpu_efficient,
                self._freq_gpu_max - f_gpu_range * (phi - 0.5) / 0.5)
        elif phi < 0.5:
            # Performance biased: scale F_efficient up toward F_max.
            self._resolved_f_gpu_efficient = min(
                self._freq_gpu_max,
                self._freq_gpu_efficient + f_gpu_range * (0.5 - phi) / 0.5)

        self._f_range = self._resolved_f_gpu_max - self._resolved_f_gpu_efficient

        # For the DRM idle-residency path, compute the elapsed time once;
        # the busy fraction is derived per domain from the idle counter.
        time_delta = math.nan
        time_now = math.nan
        if self._activity_source == 'drm_idle':
            time_now = pio.sample(self._time_idx)
            if not math.isnan(self._time_last):
                time_delta = time_now - self._time_last

        # ROI proxy tracking (Level-Zero only): sample time and per-GPU energy
        # and capture each GPU's raw (pre-stall) activity from its first unit.
        gpu_scoped_activity = []
        gpu_energy = []
        if self._activity_source == 'levelzero':
            time_now = pio.sample(self._time_idx)
            gpu_energy = [pio.sample(idx) for idx in self._energy_idx]
            gpu_scoped_activity = [math.nan] * self._num_gpu

        do_write_batch = False
        for domain_idx in range(self._agent_domain_count):
            if self._activity_source == 'levelzero':
                activity = pio.sample(self._activity_idx[domain_idx])
                if self._has_stall:
                    stall = pio.sample(self._stall_idx[domain_idx])
                else:
                    stall = math.nan
                if self._has_utilization:
                    utilization = pio.sample(self._utilization_idx[domain_idx])
                else:
                    # No GPU_UTILIZATION signal (e.g. DCGM): use the compute
                    # activity directly by treating utilization as 1.0.
                    utilization = 1.0
            else:
                # Derive a busy fraction from the monotonic idle-residency
                # counter: busy = 1 - delta_idle / delta_time, clamped to
                # [0, 1].  GPU_UTILIZATION is unavailable, so treat the busy
                # fraction as fully utilized (utilization = 1.0).
                idle_now = pio.sample(self._idle_idx[domain_idx])
                idle_prev = self._idle_last[domain_idx]
                if (not math.isnan(time_delta) and time_delta > 0
                        and not math.isnan(idle_prev)):
                    busy = 1.0 - (idle_now - idle_prev) / time_delta
                    activity = min(max(busy, 0.0), 1.0)
                else:
                    # First sample (no interval yet): fall back to F_max.
                    activity = math.nan
                self._idle_last[domain_idx] = idle_now
                stall = math.nan
                utilization = 1.0

            # Default to F_max.
            f_request = self._resolved_f_gpu_max

            if not math.isnan(activity):
                activity = min(activity, 1.0)
                activity = max(activity, 0.0)

                # Stall only lowers the frequency decision, so preserve the raw
                # bounded activity for ROI/on-time tracking.
                tracked_activity = activity
                if not math.isnan(stall):
                    stall = min(max(stall, 0.0), 1.0)
                    activity = activity * (1.0 - stall)

                # Scale the compute activity by GPU utilization to handle
                # short, frequency-sensitive phases.  Inactive regions
                # fall back to the efficient frequency.
                if not math.isnan(utilization) and utilization > 0:
                    utilization = min(utilization, 1.0)
                    f_request = self._resolved_f_gpu_efficient + \
                        self._f_range * (activity / utilization)
                else:
                    f_request = self._resolved_f_gpu_efficient + \
                        self._f_range * activity

                # Use the first agent-domain unit per GPU as a rough estimate
                # of total GPU activity for ROI tracking below.
                if (self._activity_source == 'levelzero' and
                        domain_idx % self._agent_units_per_gpu == 0):
                    gpu_scoped_activity[domain_idx // self._agent_units_per_gpu] = \
                        tracked_activity

            # Frequency clamping.
            if (f_request > self._resolved_f_gpu_max or
                    f_request < self._resolved_f_gpu_efficient):
                self._frequency_clipped += 1
            f_request = min(f_request, self._resolved_f_gpu_max)
            f_request = max(f_request, self._resolved_f_gpu_efficient)

            # Energy biased: drop to the minimum frequency after a run of idle
            # (near-zero utilization) samples.  Level-Zero only (drm_idle has
            # no utilization signal, so utilization is 1.0 and never trips).
            if phi >= 0.5:
                if (not math.isnan(utilization) and
                        (utilization == 0 or
                         (utilization < 0.02 and self._has_stall))):
                    if self._gpu_idle_timer[domain_idx] > 0:
                        self._gpu_idle_timer[domain_idx] -= 1
                else:
                    self._gpu_idle_timer[domain_idx] = _GPU_IDLE_TIMER_RESET
                if self._gpu_idle_timer[domain_idx] <= 0:
                    f_request = self._freq_gpu_min
                    self._gpu_idle_samples[domain_idx] += 1

            # Write the min/max frequency controls only on change.
            if (f_request != self._freq_min_last[domain_idx] or
                    f_request != self._freq_max_last[domain_idx]):
                pio.adjust(self._freq_min_idx[domain_idx], f_request)
                pio.adjust(self._freq_max_idx[domain_idx], f_request)
                self._freq_min_last[domain_idx] = f_request
                self._freq_max_last[domain_idx] = f_request
                self._frequency_requests += 1
                do_write_batch = True

        if self._activity_source == 'levelzero':
            self._track_regions(gpu_scoped_activity, time_now, gpu_energy)
            self._prev_time = time_now
            self._prev_gpu_energy = gpu_energy

        if self._activity_source == 'drm_idle':
            self._time_last = time_now

        if do_write_batch:
            pio.write_batch()

    def _track_regions(self, gpu_scoped_activity, time_now, gpu_energy):
        """Update per-GPU active-region and on time/energy tracking.

        This provides ROI proxy metrics for the summary only; it does not
        affect the control algorithm and may be removed when GPU region
        support is added to GEOPM.

        Args:
            gpu_scoped_activity (list): Per-GPU raw (pre-stall) compute
                activity, NaN when no valid sample this cycle.
            time_now (float): Current TIME sample.
            gpu_energy (list): Per-GPU GPU_ENERGY sample.
        """
        for gpu_idx in range(self._num_gpu):
            activity = gpu_scoped_activity[gpu_idx]
            if math.isnan(activity):
                # No valid first-unit activity sample for this GPU this cycle.
                continue
            if activity >= _GPU_ACTIVITY_CUTOFF:
                # Open a new region on entry into activity so the report
                # reflects the current region rather than spanning earlier
                # regions and the idle gaps between.
                if not self._gpu_region_active[gpu_idx]:
                    self._gpu_region_active[gpu_idx] = True
                    self._gpu_active_region_start[gpu_idx] = time_now
                    self._gpu_active_energy_start[gpu_idx] = gpu_energy[gpu_idx]

                energy_diff = gpu_energy[gpu_idx] - self._prev_gpu_energy[gpu_idx]
                if not math.isnan(energy_diff):
                    self._gpu_on_energy[gpu_idx] += energy_diff
                time_diff = time_now - self._prev_time
                if not math.isnan(time_diff):
                    self._gpu_on_time[gpu_idx] += time_diff
            else:
                # Close the region on the first inactive sample, at the
                # previous sample, which was the last active one.
                if self._gpu_region_active[gpu_idx]:
                    self._gpu_region_active[gpu_idx] = False
                    self._gpu_active_region_stop[gpu_idx] = self._prev_time
                    self._gpu_active_energy_stop[gpu_idx] = \
                        self._prev_gpu_energy[gpu_idx]

    def run_end(self):
        """Print a summary of the agent's activity to stderr."""
        if self._agent_domain is None:
            return
        sys.stderr.write(
            'gpu_activity agent summary:\n'
            f'  Agent Domain: {topo.domain_name(self._agent_domain)}\n'
            f'  Use Level Zero Stall Tracking: {self._has_stall}\n'
            f'  GPU Frequency Requests: {self._frequency_requests}\n'
            f'  GPU Clipped Frequency Requests: {self._frequency_clipped}\n'
            f'  Resolved Max Frequency: {self._resolved_f_gpu_max}\n'
            f'  Resolved Efficient Frequency: {self._resolved_f_gpu_efficient}\n'
            f'  Resolved Frequency Range: {self._f_range}\n')

        for gpu_idx in range(self._num_gpu):
            energy_start = self._gpu_active_energy_start[gpu_idx]
            energy_stop = self._gpu_active_energy_stop[gpu_idx]
            region_start = self._gpu_active_region_start[gpu_idx]
            region_stop = self._gpu_active_region_stop[gpu_idx]
            # A region still open ends at the latest sample, so the reported
            # active-region time/energy covers the in-progress region.
            if self._gpu_region_active[gpu_idx]:
                region_stop = self._prev_time
                energy_stop = self._prev_gpu_energy[gpu_idx]
            sys.stderr.write(
                f'  GPU {gpu_idx} Active Region Energy: {energy_stop - energy_start}\n'
                f'  GPU {gpu_idx} Active Region Time: {region_stop - region_start}\n'
                f'  GPU {gpu_idx} On Energy: {self._gpu_on_energy[gpu_idx]}\n'
                f'  GPU {gpu_idx} On Time: {self._gpu_on_time[gpu_idx]}\n')

        for domain_idx in range(len(self._gpu_idle_samples)):
            sys.stderr.write(
                f'  GPU Chip {domain_idx} Idle Agent Actions: '
                f'{self._gpu_idle_samples[domain_idx]}\n')


if __name__ == '__main__':
    sys.exit(main(GPUActivityAgent()))
