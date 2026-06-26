#!/usr/bin/python3
#
#  Copyright (c) 2015 - 2026 Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#

"""CPU activity agent for the GEOPM Python session interface.

This module provides a ``CPUActivityAgent`` for use with the GEOPM
Python session interface.  It is a pure-Python reimplementation of the
C++ ``CPUActivityAgent`` (plugin name ``cpu_activity``) that drives CPU
core and uncore frequency based on CPU compute scalability and memory
bandwidth utilization, without requiring the GEOPM HPC Runtime
(Controller, MPI/OpenMP interposition, or DBus profile registration).

The agent steers two frequency domains:

* **Core** frequency is interpolated between an efficient frequency and
  the maximum available frequency using ``MSR::CPU_SCALABILITY_RATIO``.
* **Uncore** frequency is interpolated using a memory-bandwidth
  utilization derived from ``MSR::QM_CTR_SCALED_RATE`` normalized by a
  characterized maximum bandwidth for the current uncore frequency.

Unlike the C++ agent, all static characterization input (the efficient
frequencies and the uncore-frequency-to-maximum-bandwidth table) is
supplied through command-line arguments rather than the
``ConstConfigIOGroup``:

* ``--cpu-freq-efficient`` core efficient frequency in Hz.  Defaults to
  the platform ``CPU_FREQUENCY_MIN_AVAIL``.
* ``--cpu-uncore-freq-efficient`` uncore efficient frequency in Hz.
  Defaults to the current ``CPU_UNCORE_FREQUENCY_MIN_CONTROL``.
* ``--cpu-uncore-bandwidth`` a repeatable ``FREQ:BW`` pair giving an
  uncore frequency (Hz) and the maximum memory bandwidth (bytes/s)
  achievable at that frequency.  When no pairs are supplied the agent
  runs in **core-only mode** and leaves the uncore frequency untouched.

A single ``--phi`` knob (0.0 - 1.0) biases the frequency selection
toward performance (phi < 0.5) or energy savings (phi > 0.5).

Example usage:
    python -m geopmdpy.cpu_activity_agent -t 30 -p 0.02
    python -m geopmdpy.cpu_activity_agent -t 30 -p 0.02 --phi 0.8 \\
        --cpu-freq-efficient 1.8e9 --cpu-uncore-freq-efficient 1.6e9 \\
        --cpu-uncore-bandwidth 1.6e9:1.4e11 --cpu-uncore-bandwidth 2.4e9:2.0e11
"""

import bisect
import math
import sys

from . import pio
from . import topo
from .session import main
from .session import Agent

_POLICY_PHI_DEFAULT = 0.5

# QM event ID 0x2 selects Total Memory Bandwidth Monitoring, matching the
# configuration used to generate the bandwidth characterization data.
_QM_EVENT_ID_TOTAL_MEM_BW = 2


class CPUActivityAgent(Agent):
    """Agent that sets CPU core and uncore frequency based on activity.

    The agent reimplements the control logic of the C++
    ``CPUActivityAgent`` plugin for use with the GEOPM Python session.
    The session drives the timed loop and calls ``pio.read_batch()``
    before each ``update_loop()``; this agent samples the cached
    signals, computes core and uncore frequency requests, and writes the
    frequency controls when a request changes.

    Command-line options:
      --phi                       CPU frequency bias in [0.0, 1.0].
      --cpu-freq-efficient        Core efficient frequency in Hz.
      --cpu-uncore-freq-efficient Uncore efficient frequency in Hz.
      --cpu-uncore-bandwidth      Repeatable FREQ:BW characterization pair.

    Example:
        python -m geopmdpy.cpu_activity_agent -t 30 -p 0.02 --phi 0.5
    """

    def __init__(self):
        """Initialize the CPUActivityAgent."""
        self._phi = _POLICY_PHI_DEFAULT
        self._hi_res = False

        # CLI-supplied characterization; None means "resolve from platform".
        self._cpu_freq_efficient_arg = None
        self._cpu_uncore_freq_efficient_arg = None
        # uncore frequency (Hz) -> maximum memory bandwidth (bytes/s)
        self._uncore_bandwidth_map = {}
        # sorted keys of the bandwidth map for the lookup in update_loop
        self._uncore_bandwidth_freqs = []

        # Whether uncore steering is enabled (bandwidth pairs supplied).
        self._uncore_enabled = False

        self._core_domain = None
        self._core_domain_count = 0
        self._uncore_domain = None
        self._uncore_domain_count = 0

        # batch indices for pushed signals/controls, per domain
        self._scalability_idx = []
        self._core_freq_min_idx = []
        self._core_freq_max_idx = []
        self._qm_rate_idx = []
        self._uncore_status_idx = []
        self._uncore_freq_min_idx = []
        self._uncore_freq_max_idx = []
        # last value written to each control, used for change detection
        self._core_freq_min_last = []
        self._core_freq_max_last = []
        self._uncore_freq_min_last = []
        self._uncore_freq_max_last = []

        self._freq_core_min = 0.0
        self._freq_core_max = 0.0
        self._freq_core_efficient = 0.0
        self._freq_uncore_min = 0.0
        self._freq_uncore_max = 0.0
        self._freq_uncore_efficient = 0.0

        # Resolved values after phi is applied, retained for the summary.
        self._resolved_f_core_max = 0.0
        self._resolved_f_core_efficient = 0.0
        self._resolved_f_uncore_max = 0.0
        self._resolved_f_uncore_efficient = 0.0

        self._core_frequency_requests = 0
        self._core_frequency_clipped = 0
        self._uncore_frequency_requests = 0
        self._uncore_frequency_clipped = 0

    def help(self):
        """Help documentation.

        """
        return ('The cpu_activity agent sets CPU core and uncore frequency '
                'based on CPU compute scalability and memory bandwidth '
                'utilization. Use --phi to bias frequency selection toward '
                'performance (phi < 0.5) or energy savings (phi > 0.5). '
                'Supply --cpu-uncore-bandwidth FREQ:BW pairs to enable uncore '
                'frequency steering; without them the agent steers only the '
                'core frequency.')

    def update_parser(self, parser):
        """Add the agent-specific arguments to the parser.

        Args:
            parser (argparse.ArgumentParser): The parser to update.

        Returns:
            argparse.ArgumentParser: The updated parser.
        """
        parser.add_argument('--phi', dest='phi', type=float,
                            default=_POLICY_PHI_DEFAULT,
                            help='CPU frequency bias in the range [0.0, 1.0]. '
                                 'Lower values favor performance, higher values '
                                 'favor energy savings. Default %(default)s.')
        parser.add_argument('--cpu-freq-efficient', dest='cpu_freq_efficient',
                            type=float, default=None,
                            help='Core efficient frequency in Hz. Default: the '
                                 'platform CPU_FREQUENCY_MIN_AVAIL.')
        parser.add_argument('--cpu-uncore-freq-efficient',
                            dest='cpu_uncore_freq_efficient',
                            type=float, default=None,
                            help='Uncore efficient frequency in Hz. Default: '
                                 'the current CPU_UNCORE_FREQUENCY_MIN_CONTROL.')
        parser.add_argument('--cpu-uncore-bandwidth', dest='cpu_uncore_bandwidth',
                            action='append', default=None, metavar='FREQ:BW',
                            help='Uncore characterization pair FREQ:BW giving an '
                                 'uncore frequency in Hz and the maximum memory '
                                 'bandwidth in bytes/s achievable at that '
                                 'frequency. May be given multiple times. When '
                                 'omitted the agent runs in core-only mode.')
        parser.add_argument('--hi-res', action='store_true',
                            help='Measure signals at finest granularity (all domains/indices)')
        return parser

    def update_args(self, args):
        """Validate and store the agent-specific arguments.

        Args:
            args (argparse.Namespace): Parsed command-line arguments.

        Returns:
            argparse.Namespace: The (possibly updated) arguments.

        Raises:
            RuntimeError: An argument value is invalid.
        """
        if math.isnan(args.phi) or args.phi < 0.0 or args.phi > 1.0:
            raise RuntimeError(
                f'--phi value out of range: {args.phi}. '
                'Acceptable values are in the range [0.0, 1.0].')
        self._phi = args.phi

        self._cpu_freq_efficient_arg = self._validate_positive_freq(
            args.cpu_freq_efficient, '--cpu-freq-efficient')
        self._cpu_uncore_freq_efficient_arg = self._validate_positive_freq(
            args.cpu_uncore_freq_efficient, '--cpu-uncore-freq-efficient')

        self._uncore_bandwidth_map = self._parse_bandwidth_pairs(
            args.cpu_uncore_bandwidth)
        self._uncore_bandwidth_freqs = sorted(self._uncore_bandwidth_map)
        self._uncore_enabled = bool(self._uncore_bandwidth_map)
        self._hi_res = getattr(args, 'hi_res', False)
        return args

    @staticmethod
    def _validate_positive_freq(value, name):
        """Validate an optional positive frequency argument."""
        if value is None:
            return None
        if math.isnan(value) or value <= 0.0:
            raise RuntimeError(
                f'{name} value must be a positive frequency in Hz: {value}.')
        return value

    @staticmethod
    def _parse_bandwidth_pairs(pairs):
        """Parse the --cpu-uncore-bandwidth FREQ:BW pairs into a map.

        Args:
            pairs (list of str or None): Raw FREQ:BW argument values.

        Returns:
            dict: Mapping of uncore frequency (Hz) to maximum memory
                bandwidth (bytes/s).

        Raises:
            RuntimeError: A pair is malformed or non-positive.
        """
        result = {}
        for item in pairs or []:
            tokens = item.split(':')
            if len(tokens) != 2:
                raise RuntimeError(
                    f'--cpu-uncore-bandwidth value must be FREQ:BW: {item}.')
            try:
                freq = float(tokens[0])
                bandwidth = float(tokens[1])
            except ValueError:
                raise RuntimeError(
                    f'--cpu-uncore-bandwidth value must be FREQ:BW: {item}.')
            if (math.isnan(freq) or math.isnan(bandwidth) or
                    freq <= 0.0 or bandwidth <= 0.0):
                raise RuntimeError(
                    '--cpu-uncore-bandwidth FREQ and BW must be positive: '
                    f'{item}.')
            result[freq] = bandwidth
        return result

    def signal_config_override(self):
        """Provide a default trace configuration.

        Provided so the agent can be run with ``--signal-config -`` and
        an empty stdin.  Users may pipe their own request list to
        override this default.

        Returns:
            str: Signal configuration string for the session.
        """
        names = pio.signal_names()
        suffix = ' * *' if self._hi_res else ' board 0'
        lines = ['TIME board 0']
        if 'MSR::CPU_SCALABILITY_RATIO' in names:
            lines.append('MSR::CPU_SCALABILITY_RATIO' + suffix)
        if 'MSR::QM_CTR_SCALED_RATE' in names:
            lines.append('MSR::QM_CTR_SCALED_RATE' + suffix)
        if 'CPU_FREQUENCY_STATUS' in names:
            lines.append('CPU_FREQUENCY_STATUS' + suffix)
        if 'CPU_UNCORE_FREQUENCY_STATUS' in names:
            lines.append('CPU_UNCORE_FREQUENCY_STATUS' + suffix)
        return '\n'.join(lines) + '\n'

    def run_begin(self):
        """Resolve the agent domains and push signals and controls.

        Determines the core control domain, optionally the uncore
        (package) domain, pushes the per-domain signals and controls,
        reads the static frequency characterization, and saves the
        current control settings for restoration on exit.

        Raises:
            RuntimeError: No CPUs are present or a resolved efficient
                frequency is out of range.
        """
        num_core = topo.num_domain(topo.DOMAIN_CORE)
        if num_core == 0:
            raise RuntimeError('CPUActivityAgent requires at least one CPU core')

        # Use the coarsest granularity supported by the core controls and the
        # scalability signal.  In GEOPM the coarsest domain has the smallest
        # domain-type value.
        self._core_domain = min(
            pio.control_domain_type('CPU_FREQUENCY_MIN_CONTROL'),
            pio.control_domain_type('CPU_FREQUENCY_MAX_CONTROL'),
            pio.signal_domain_type('MSR::CPU_SCALABILITY_RATIO'))
        self._core_domain_count = topo.num_domain(self._core_domain)

        # Snapshot controls before any modification so restore_control()
        # reverts the platform to its pre-agent state on exit.
        pio.save_control()

        self._scalability_idx = []
        self._core_freq_min_idx = []
        self._core_freq_max_idx = []
        for domain_idx in range(self._core_domain_count):
            self._scalability_idx.append(
                pio.push_signal('MSR::CPU_SCALABILITY_RATIO',
                                self._core_domain, domain_idx))
            self._core_freq_min_idx.append(
                pio.push_control('CPU_FREQUENCY_MIN_CONTROL',
                                 self._core_domain, domain_idx))
            self._core_freq_max_idx.append(
                pio.push_control('CPU_FREQUENCY_MAX_CONTROL',
                                 self._core_domain, domain_idx))
        self._core_freq_min_last = [math.nan] * self._core_domain_count
        self._core_freq_max_last = [math.nan] * self._core_domain_count

        self._freq_core_min = pio.read_signal('CPU_FREQUENCY_MIN_AVAIL',
                                              topo.DOMAIN_BOARD, 0)
        self._freq_core_max = pio.read_signal('CPU_FREQUENCY_MAX_AVAIL',
                                              topo.DOMAIN_BOARD, 0)

        if self._cpu_freq_efficient_arg is not None:
            self._freq_core_efficient = self._cpu_freq_efficient_arg
        else:
            self._freq_core_efficient = self._freq_core_min
        if (self._freq_core_efficient > self._freq_core_max or
                self._freq_core_efficient < self._freq_core_min):
            raise RuntimeError(
                'CPUActivityAgent: core efficient frequency out of range: '
                f'{self._freq_core_efficient}')

        if self._uncore_enabled:
            self._run_begin_uncore()

        self._core_frequency_requests = 0
        self._core_frequency_clipped = 0
        self._uncore_frequency_requests = 0
        self._uncore_frequency_clipped = 0

    def _run_begin_uncore(self):
        """Set up the uncore (package) domain signals and controls."""
        # Configure QM counters to measure total memory bandwidth.  Assign all
        # cores to resource monitoring association ID 0 and select the total
        # memory bandwidth event so MSR::QM_CTR_SCALED_RATE reports it.
        pio.write_control('MSR::PQR_ASSOC:RMID', topo.DOMAIN_BOARD, 0, 0)
        pio.write_control('MSR::QM_EVTSEL:RMID', topo.DOMAIN_BOARD, 0, 0)
        pio.write_control('MSR::QM_EVTSEL:EVENT_ID', topo.DOMAIN_BOARD, 0,
                          _QM_EVENT_ID_TOTAL_MEM_BW)

        self._uncore_domain = topo.DOMAIN_PACKAGE
        self._uncore_domain_count = topo.num_domain(self._uncore_domain)

        self._qm_rate_idx = []
        self._uncore_status_idx = []
        self._uncore_freq_min_idx = []
        self._uncore_freq_max_idx = []
        for domain_idx in range(self._uncore_domain_count):
            self._qm_rate_idx.append(
                pio.push_signal('MSR::QM_CTR_SCALED_RATE',
                                self._uncore_domain, domain_idx))
            self._uncore_status_idx.append(
                pio.push_signal('CPU_UNCORE_FREQUENCY_STATUS',
                                self._uncore_domain, domain_idx))
            self._uncore_freq_min_idx.append(
                pio.push_control('CPU_UNCORE_FREQUENCY_MIN_CONTROL',
                                 self._uncore_domain, domain_idx))
            self._uncore_freq_max_idx.append(
                pio.push_control('CPU_UNCORE_FREQUENCY_MAX_CONTROL',
                                 self._uncore_domain, domain_idx))
        self._uncore_freq_min_last = [math.nan] * self._uncore_domain_count
        self._uncore_freq_max_last = [math.nan] * self._uncore_domain_count

        # These reflect the currently-set uncore bounds, matching the C++ agent.
        self._freq_uncore_min = pio.read_signal('CPU_UNCORE_FREQUENCY_MIN_CONTROL',
                                               topo.DOMAIN_BOARD, 0)
        self._freq_uncore_max = pio.read_signal('CPU_UNCORE_FREQUENCY_MAX_CONTROL',
                                               topo.DOMAIN_BOARD, 0)

        if self._cpu_uncore_freq_efficient_arg is not None:
            self._freq_uncore_efficient = self._cpu_uncore_freq_efficient_arg
        else:
            self._freq_uncore_efficient = self._freq_uncore_min
        if (self._freq_uncore_efficient > self._freq_uncore_max or
                self._freq_uncore_efficient < self._freq_uncore_min):
            raise RuntimeError(
                'CPUActivityAgent: uncore efficient frequency out of range: '
                f'{self._freq_uncore_efficient}')

    @staticmethod
    def _resolve_phi_range(freq_min_anchor, freq_efficient, freq_max, phi):
        """Apply phi to an efficient/max frequency range.

        Returns the resolved (efficient, max) pair.  phi == 0.5 spans the
        full efficient-to-max range; phi > 0.5 scales the max down toward
        the efficient frequency (energy biased); phi < 0.5 scales the
        efficient frequency up toward the max (performance biased).
        """
        f_range = freq_max - freq_efficient
        resolved_max = freq_max
        resolved_efficient = freq_efficient
        if phi > 0.5:
            resolved_max = max(freq_efficient,
                               freq_max - f_range * (phi - 0.5) / 0.5)
        elif phi < 0.5:
            resolved_efficient = min(freq_max,
                                     freq_efficient + f_range * (0.5 - phi) / 0.5)
        return resolved_efficient, resolved_max

    def _max_bandwidth_for(self, uncore_freq):
        """Look up the characterized max bandwidth for an uncore frequency.

        Returns the bandwidth associated with the largest characterization
        frequency that is less than or equal to ``uncore_freq``.  If
        ``uncore_freq`` is below the smallest key (or is NaN), the bandwidth
        of the smallest key is used (mirrors the C++ ``upper_bound() - 1``
        clamped at ``begin()``).
        """
        freqs = self._uncore_bandwidth_freqs
        if not freqs:
            return math.nan
        if math.isnan(uncore_freq):
            return self._uncore_bandwidth_map[freqs[0]]
        pos = bisect.bisect_right(freqs, uncore_freq)
        if pos == 0:
            pos = 1
        return self._uncore_bandwidth_map[freqs[pos - 1]]

    def update_loop(self):
        """Sample CPU activity and adjust core and uncore frequency.

        Called by the session after ``pio.read_batch()``.  Samples the
        cached scalability and bandwidth signals, computes per-domain
        frequency requests, and writes the frequency controls when a
        request differs from the last written value.
        """
        phi = self._phi

        self._resolved_f_core_efficient, self._resolved_f_core_max = \
            self._resolve_phi_range(self._freq_core_min,
                                    self._freq_core_efficient,
                                    self._freq_core_max, phi)
        f_core_range = self._resolved_f_core_max - self._resolved_f_core_efficient

        do_write_batch = False

        # Per core-domain frequency request from CPU scalability.
        for domain_idx in range(self._core_domain_count):
            scalability = pio.sample(self._scalability_idx[domain_idx])
            if math.isnan(scalability):
                scalability = 1.0

            f_request = self._resolved_f_core_efficient + f_core_range * scalability

            if (f_request > self._resolved_f_core_max or
                    f_request < self._resolved_f_core_efficient):
                self._core_frequency_clipped += 1
            f_request = min(f_request, self._resolved_f_core_max)
            f_request = max(f_request, self._resolved_f_core_efficient)

            if (f_request != self._core_freq_min_last[domain_idx] or
                    f_request != self._core_freq_max_last[domain_idx]):
                pio.adjust(self._core_freq_min_idx[domain_idx], f_request)
                pio.adjust(self._core_freq_max_idx[domain_idx], f_request)
                self._core_freq_min_last[domain_idx] = f_request
                self._core_freq_max_last[domain_idx] = f_request
                self._core_frequency_requests += 1
                do_write_batch = True

        if self._uncore_enabled:
            do_write_batch |= self._update_loop_uncore(phi)

        if do_write_batch:
            pio.write_batch()

    def _update_loop_uncore(self, phi):
        """Compute and write the per-package uncore frequency requests.

        Returns:
            bool: True if any uncore control was adjusted.
        """
        self._resolved_f_uncore_efficient, self._resolved_f_uncore_max = \
            self._resolve_phi_range(self._freq_uncore_min,
                                    self._freq_uncore_efficient,
                                    self._freq_uncore_max, phi)
        f_uncore_range = (self._resolved_f_uncore_max -
                          self._resolved_f_uncore_efficient)

        do_write_batch = False
        for domain_idx in range(self._uncore_domain_count):
            uncore_status = pio.sample(self._uncore_status_idx[domain_idx])
            qm_rate = pio.sample(self._qm_rate_idx[domain_idx])
            max_bandwidth = self._max_bandwidth_for(uncore_status)

            scalability = 1.0
            if (not math.isnan(qm_rate) and not math.isnan(max_bandwidth) and
                    max_bandwidth != 0):
                scalability = qm_rate / max_bandwidth

            f_request = (self._resolved_f_uncore_efficient +
                         f_uncore_range * scalability)

            if (f_request > self._resolved_f_uncore_max or
                    f_request < self._resolved_f_uncore_efficient):
                self._uncore_frequency_clipped += 1
            f_request = min(f_request, self._resolved_f_uncore_max)
            f_request = max(f_request, self._resolved_f_uncore_efficient)

            if math.isnan(f_request):
                f_request = self._freq_uncore_max

            if (f_request != self._uncore_freq_min_last[domain_idx] or
                    f_request != self._uncore_freq_max_last[domain_idx]):
                pio.adjust(self._uncore_freq_min_idx[domain_idx], f_request)
                pio.adjust(self._uncore_freq_max_idx[domain_idx], f_request)
                self._uncore_freq_min_last[domain_idx] = f_request
                self._uncore_freq_max_last[domain_idx] = f_request
                self._uncore_frequency_requests += 1
                do_write_batch = True
        return do_write_batch

    def run_end(self):
        """Print a summary of the agent's activity to stderr."""
        if self._core_domain is None:
            return
        lines = [
            'cpu_activity agent summary:',
            f'  Core Domain: {topo.domain_name(self._core_domain)}',
            f'  Core Frequency Requests: {self._core_frequency_requests}',
            f'  Core Clipped Frequency Requests: {self._core_frequency_clipped}',
            f'  Resolved Max Core Frequency: {self._resolved_f_core_max}',
            f'  Resolved Efficient Core Frequency: {self._resolved_f_core_efficient}',
        ]
        if self._uncore_enabled:
            lines.extend([
                f'  Uncore Domain: {topo.domain_name(self._uncore_domain)}',
                f'  Uncore Frequency Requests: {self._uncore_frequency_requests}',
                '  Uncore Clipped Frequency Requests: '
                f'{self._uncore_frequency_clipped}',
                f'  Resolved Max Uncore Frequency: {self._resolved_f_uncore_max}',
                '  Resolved Efficient Uncore Frequency: '
                f'{self._resolved_f_uncore_efficient}',
            ])
        else:
            lines.append('  Uncore: disabled (core-only mode)')
        sys.stderr.write('\n'.join(lines) + '\n')


if __name__ == '__main__':
    sys.exit(main(CPUActivityAgent()))
