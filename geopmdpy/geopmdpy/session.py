#  Copyright (c) 2015 - 2025 Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#

"""Implementation for the geopmsession command line tool

"""

import sys
import os
import errno
import math
from socket import gethostname
from io import StringIO
from argparse import ArgumentParser
from signal import signal
from signal import SIGTERM, SIGINT
from time import sleep
from . import topo
from . import pio
from . import loop
from . import stats
from . import __version_str__

from contextlib import contextmanager
@contextmanager
def _nullcontext():
    yield None
try:
    from contextlib import nullcontext
except ImportError as ex:
    nullcontext = _nullcontext

g_session_handler = None
_STARTUP_SLEEP = 0.005

def _term_handler(signum, frame):
    sys.stderr.write(f'Received signal {signum}, flushing buffers and exiting.\n')
    if g_session_handler is not None:
        g_session_handler.stop()
    sys.exit(128 + signum)

def _check_valid_output(path):
    return path is not None and path != '/dev/null'

class _SessionIO:
    def __init__(self, request_stream, trace_path, report_path):
        self._request_stream = request_stream
        self._trace_path = trace_path
        self._report_path = report_path
        self._report_fid = None
        if self._report_path == '-':
            self._report_fid = sys.stdout
        elif self._report_path is not None:
            self._report_fid = open(self._report_path, 'w')

    def get_request_queue(self):
        return ReadRequestQueue(self._request_stream)

    def open_trace_stream(self):
        if not self.do_trace():
            return None
        elif self._trace_path == '-':
            return sys.stdout
        else:
            return open(self._trace_path, "w")

    def close_trace_stream(self, fid):
        if fid is not None and self._trace_path != '-':
            fid.close()

    def close_report_stream(self):
        if self._report_fid is not None and self._report_path != '-':
            self._report_fid.close()

    def do_report(self):
        return _check_valid_output(self._report_path)

    def do_trace(self):
        return _check_valid_output(self._trace_path)

    def write_report(self, report):
        if self.do_report():
            self._report_fid.write(report)

    def is_rank_zero(self):
        return True

class _MPISessionIO:
    def __init__(self, request_stream, trace_path, report_path, report_format):
        try:
            from mpi4py import MPI
        except Exception as ex:
            raise RuntimeError('Using --enable-mpi requires the mpi4py module see: https://mpi4py.readthedocs.io/en/stable/install.html>') from ex
        if trace_path == "-":
            raise RuntimeError('Cannot write trace to standard output when specifying the --enable-mpi option')
        self._request_stream = request_stream
        self._trace_path = trace_path
        self._report_path = report_path
        self._comm = MPI.COMM_WORLD
        self._rank = self._comm.Get_rank()
        self._join_str = ''
        if report_format == 'yaml':
            self._join_str = '\n---\n\n'
        self._report_fid = None
        if self._rank == 0:
            if self._report_path == '-':
                self._report_fid = sys.stdout
            elif self._report_path is not None:
                self._report_fid = open(self._report_path, 'w')

    def get_request_queue(self):
        request_raw = None
        if self._rank == 0:
            result = ReadRequestQueue(self._request_stream)
            request_raw = result.get_raw()
        request_raw = self._comm.bcast(request_raw, root=0)
        if self._rank != 0:
            result = ReadRequestQueue(StringIO(request_raw))
        return result

    def open_trace_stream(self):
        if not self.do_trace():
            return None
        else:
            return open(f'{self._trace_path}-{gethostname()}', "w")

    def close_trace_stream(self, fid):
        if fid is not None:
            fid.close()

    def close_report_stream(self):
        if self._report_fid is not None and self._report_path != '-':
            self._report_fid.close()

    def do_report(self):
        return _check_valid_output(self._report_path)

    def do_trace(self):
        return _check_valid_output(self._trace_path)

    def write_report(self, report, do_gather=True):
        if not self.do_report():
            return
        if do_gather:
            report_list = self._comm.gather(report, root=0)
            if self._rank == 0:
                report = self._join_str.join(report_list)
        if self._report_fid is not None:
            self._report_fid.write(report)
            self._report_fid.flush()
        elif self._rank == 0 and self._report_path == '-':
            sys.stdout.write(report)
            sys.stdout.flush()

    def is_rank_zero(self):
        return self._rank == 0

class Agent:
    """Base class for user-defined agents in GEOPM sessions.

    Agents encapsulate custom logic for controlling and monitoring
    GEOPM sessions. To use an agent, subclass this class and override
    any of the methods to customize argument parsing, signal configuration,
    trace output, and periodic update behavior.

    Methods that can be overridden:
      - update_parser: Add custom command-line arguments.
      - update_args: Process parsed arguments.
      - signal_config_override: Provide a custom signal configuration string.
      - run_begin: Called at the start of a session run.
      - run_end: Called at the end of a session run.
      - update_loop: Called at each sampling period.
      - header_names: Add custom trace column headers.
      - trace_out: Provide custom trace output values.

    Example usage:
        class MyAgent(Agent):
            def update_parser(self, parser):
                parser.add_argument('--foo', ...)
                return parser
            def update_args(self, args):
                self._foo = args.foo
                return args
            def update_loop(self):
                # Custom logic here
                pass

        main(agent=MyAgent())

    """
    def __init__(self):
        """Initialize the agent. Override to set up agent state."""
        pass

    def update_parser(self, parser):
        """Update the argument parser with agent-specific options.

        Args:
            parser (argparse.ArgumentParser): The parser to update.

        Returns:
            argparse.ArgumentParser: The updated parser.

        """
        return parser

    def update_args(self, args):
        """Process parsed arguments and update agent state.

        Args:
            args (argparse.Namespace): Parsed command-line arguments.

        Returns:
            argparse.Namespace: The (possibly updated) arguments.

        """
        return args

    def signal_config_override(self):
        """Override the default signal configuration.

        Returns:
            str or None: A string containing the signal configuration,
                         or None to use the Session default (stdin).

        """
        return None

    def run_begin(self):
        """Called by Session at the start of each run.

        Override to perform setup before the session starts. Calls to
        pio.push_signal() and pio.push_control() may be made here.

        """
        pass

    def run_end(self):
        """Called by Session at the end of each run.

        Override to perform cleanup after the session ends.  Releases any
        resources allocated in the run_begin() method.

        """
        pass

    def update_loop(self):
        """Called periodically by Session during the run.

        Override to implement periodic agent logic. This method is called by the
        Session implementation just after a call to pio.read_batch() in the
        timed loop, so the Agent implementation should not call
        pio.read_batch().  Calls to pio.sample(), pio.adjust() and
        pio.write_batch() may be part of the update_loop() implementation.

        """
        pass

    def header_names(self):
        """Return additional trace column headers provided by the agent.

        Returns:
            list of str: List of header names for custom trace columns.

        """
        return []

    def trace_out(self):
        """Return additional trace values provided by the agent.

        The agent is responsible for the string format of the values returned,
        numeric values are not allowed.

        Returns:
            list of str: List of string values for custom trace columns.

        """
        return []

def agent_factory(agent):
    """Ensure the agent parameter is an Agent instance.

    Args:
        agent (Agent or None): The agent to use.

    Returns:
        Agent: An instance of Agent or a subclass.

    Raises:
        RuntimeError: If agent is not None and not an Agent.

    """
    if agent is None:
        return Agent()
    if not isinstance(agent, Agent):
        raise RuntimeError('The agent parameter must be derived from the Agent class')
    return agent

class Session:
    """Object responsible for creating and running a GEOPM batch read session.

    The Session object manages the main loop for reading signals,
    formatting output, and integrating with user-defined agents.

    Args:
        delimiter (str): Delimiter for CSV output.
        agent (Agent or None): Optional agent object to customize session behavior.

    """
    def __init__(self, delimiter=',', agent=None):
        """Initialize the Session.

        Args:
            delimiter (str): Delimiter for CSV output.
            agent (Agent or None): Optional agent object.
        """
        self._delimiter = delimiter
        self._agent = agent_factory(agent)
        self._num_agent_trace = 0

    def format_signals(self, signals, signal_format):
        """Format a list of signal values for printing

        Args:
            signals (list(float)): Values to be printed

            signal_format (list(int)): The geopm::format_string_e enum
                                       value describing the formatting
                                       for each signal provided.

        Returns:
            str: Ready-to-print line of formatted values

        """
        if len(signals) != len(signal_format):
            raise RuntimeError(
                'Number of signal values does not match the number of requests')
        result = [pio.format_signal(ss, ff) for (ss, ff) in
                  zip(signals, signal_format)]
        return '{}\n'.format(self._delimiter.join(result))

    def run_read(self, requests, duration, period, pid, out_stream,
                 stats_collector=None, report_samples=None, report_stream=None):
        """Run a read mode session

        Periodically read the requested signals. A line of text will
        be printed to the output stream for each period of time.  The
        line will contain a comma separated list of the read values,
        one for each request.

        Only one read of the requests is made if the period is zero.
        If the period is non-zero then the requested signals are read
        periodically.  The first read is immediate, and then a delay
        of period seconds elapses until the next sample is made.
        These periodic reads are executed and printed until the
        duration of time specified has been met or exceeded.

        Args:
            requests (ReadRequestQueue): Request object parsed from
                                         user input.

            duration (float): The user specified minimum length of time
                              for the samples to span in units of
                              seconds.

            period (float): The user specified period between samples
                            in units of seconds.

            pid (int or None): If not None, stop monitoring when the
                                      given process finishes.

            out_stream (typing.IO): Object with write() method where output
                               will be printed (typically sys.stdout).

        """
        num_period = 0
        if pid is not None:
            num_period = None
        elif period != 0:
            num_period = math.ceil(duration / period)
        signal_handles = []
        for name, dom, dom_idx in requests:
            try:
                signal_handles.append(pio.push_signal(name, dom, dom_idx))
            except Exception as e:
                domain_name = topo.domain_name(dom)
                raise RuntimeError(f'Unable to push "{name} {domain_name} {dom_idx}". Reason: {e}')

        pio.read_batch()
        if any([math.isnan(pio.sample(handle)) for handle in signal_handles]):
            # Purge first sample for derivative based signals
            sleep(_STARTUP_SLEEP)
            pio.read_batch()
        for sample_idx in loop.TimedLoop(period, num_period):
            if sample_idx != 0:
                pio.read_batch()
            self._agent.update_loop()
            if out_stream is not None:
                signals = [pio.sample(handle) for handle in signal_handles]
                line = self.format_signals(signals, requests.get_formats())
                if self._num_agent_trace != 0:
                    agent_trace = self._agent.trace_out()
                    if len(agent_trace) != self._num_agent_trace:
                        raise RuntimeError('Agent provided trace data of length inconsistent with the header length')
                    agent_line = self._delimiter.join(agent_trace)
                    line = f'{line[:-1]}{self._delimiter}{agent_line}\n'
                out_stream.write(line)
            if stats_collector is not None:
                stats_collector.update()
            if pid is not None and not self.is_pid_active(pid):
                break;
            if report_samples is not None and report_stream is not None and sample_idx != 0 and sample_idx % report_samples == 0:
                report_stream.write()
                stats_collector.reset()

    def is_pid_active(self, pid):
        """Check if pid is active

        Returns:
           bool: True if pid is active, False otherwise

        """
        try:
            os.kill(pid, 0)
            result = True
        except OSError as e:
            if e.errno == errno.ESRCH:
                # No such process. Stop watching.
                result = False
            elif e.errno == errno.EPERM:
                # There's a still a process, but we aren't allowed to
                # signal it. Since there's still a process, keep going.
                result = True
            else:
                # Any other error. Bubble up the exception.
                raise
        return result

    def check_read_args(self, run_time, period, report_samples, pid):
        """Check that the run time and period are valid for a read session

        Args:
            run_time (float): Time duration of the session open in
                              seconds.

            period (float): The user specified period between samples
                            in units of seconds.

            report_samples (int): Number of samples gathered between periodic
                                  reports.

            pid (int): PID to track
        Raises:
            RuntimeError: The period is greater than the total time, or any
                          option is negative.

        """
        if period > 86400:
            raise RuntimeError('Specified a period greater than 24 hours')
        if period < 0.0 or run_time < 0.0:
            raise RuntimeError('Specified a negative run time or period')
        if report_samples is not None and report_samples < 0:
            raise RuntimeError('Specified report samples is negative')

    def check_requests(self, requests):
        """Check whether the signal requests are valid.

        Args:
            requests (ReadRequestQueue): Request object parsed from
                                         user input.

        Raises:
            ValueError: An invalid request is present in the request queue.

        """
        for name, dom, dom_idx in requests:
            domain_name = topo.domain_name(dom)
            domain_size = topo.num_domain(dom)
            if dom_idx < 0 or dom_idx >= domain_size:
                raise ValueError(f'Request index in "{name} {domain_name} {dom_idx}" is out of range. Acceptable {domain_name} indices are non-negative integers up to and including {domain_size - 1}')
            signal_native_domain = pio.signal_domain_type(name)
            try:
                inner_domains_in_outer_domain = len(topo.domain_nested(signal_native_domain, dom, 0))
            except RuntimeError:
                inner_domains_in_outer_domain = 0
            if inner_domains_in_outer_domain == 0:
                raise ValueError(f'Requested domain in "{name} {domain_name} {dom_idx}" is invalid. The {name} signal is available in the {topo.domain_name(signal_native_domain)} domain or a parent domain.')


    def header_names(self, requests):
        """Format trace CSV header strings for the set of requests.

        Includes both the default signal headers and any additional
        headers provided by the agent.

        Args:
            requests (list(tuple(str, int, int))):
                List of request tuples. Each request comprises a signal name,
                domain type, and domain index.

        Returns:
            list of str: List of header names for the trace output.

        """
        result = [f'"{name}-{topo.domain_name(domain)}-{domain_idx}"'
                  if topo.domain_name(domain) != 'board' else f'"{name}"'
                  for name, domain, domain_idx in requests]
        agent_header = self._agent.header_names()
        self._num_agent_trace = len(agent_header)
        result.extend(agent_header)
        return result

    def run(self, run_time, period, pid, print_header,
            request_stream=sys.stdin, out_stream=sys.stdout,
            report_path=None, session_io=None, delimiter=',', report_format='yaml',
            report_samples=None):
        """"Create a GEOPM session with values parsed from the command line

        The implementation for the geopmsession command line tool.
        The inputs to this method are derived from the parsed command
        line provided by the user.

        Args:
            run_time (float): Time duration of the session in seconds.

            period (float): Time interval for each line of output for.
                            Value must be zero for a write mode
                            session.

            pid (int or None): If not None, stop monitoring when the
                                      given process finishes.

            print_header (bool): Whether to print a row of headers before printing
                                 CSV data.

            request_stream (typing.IO): Input from user describing the requests
                                        to read, invalid if using session_io
                                        argument.

            out_stream (typing.IO): Stream where output from will be printed,
                                    not valid if using session_io argument.

            report_path (str): Path to output report file, invalid if using
                                  session_io argument.

            session_io (object):
                Object with _SessionIO interface to get request stream and
                create report and trace output.

            delimiter (str): CSV delimiter for traces and reports.

            report_format (str): Format for output report, either 'yaml' or
                                    'csv'.

            report_samples (int): If not None, enable periodic reporting after
                                  this many samples.

        """
        global g_session_handler
        if session_io is not None:
            if request_stream is not None:
                raise ValueError('Using a request_stream is incompatible with using session_io')
            if report_path is not None:
                raise ValueError('Using a report_path is incompatible with using session_io')
            requests = session_io.get_request_queue()
            do_stats = session_io.do_report()
        else:
            requests = ReadRequestQueue(request_stream)
            do_stats = report_path is not None
        self.check_read_args(run_time, period, report_samples, pid)
        self.check_requests(requests)
        signal_config = list(requests)
        if out_stream is not None and print_header:
            header_names = self.header_names(signal_config)
            print(self._delimiter.join(header_names), file=out_stream)
        with stats.Collector(signal_config) if do_stats else nullcontext() as stats_collector:
            report_stream = None
            if do_stats:
                report_stream = _ReportStream(stats_collector, session_io, print_header, delimiter, report_format)
            try:
                g_session_handler = _SessionHandler(out_stream, report_stream, self._agent)
                self._agent.run_begin()
                self.run_read(requests, run_time, period, pid, out_stream, stats_collector, report_samples, report_stream)
            finally:
                g_session_handler.stop()
                g_session_handler = None

class _ReportStream:
    def __init__(self, stats_collector, session_io, print_header, delimiter, report_format):
        if report_format not in ('yaml', 'csv'):
            raise ValueError(f'Invalid report format: {report_format}')
        self.stats_collector = stats_collector
        self.session_io = session_io
        self.print_header = print_header and session_io.is_rank_zero()
        self.delimiter = delimiter
        self.report_format = report_format
        self.is_first_write = True

    def write(self):
        if self.stats_collector.update_count() == 0:
            return
        if self.report_format == 'yaml':
            report = self.stats_collector.report_yaml()
            if not self.is_first_write and self.session_io.is_rank_zero():
                report = f'\n---\n\n{report}'
        elif self.report_format == 'csv':
            report = self.stats_collector.report_csv(self.delimiter, self.print_header)
        self.session_io.write_report(report)
        self.is_first_write = False
        self.print_header = False

class _SessionHandler:
    def __init__(self, out_stream, report_stream, agent):
        self.out_stream = out_stream
        self.report_stream = report_stream
        self.agent = agent

    def stop(self):
        if self.out_stream is not None:
            self.out_stream.flush()
        if self.report_stream is not None:
            self.report_stream.write()
        self.agent.run_end()

class RequestQueue:
    """Object derived from user input that provides request information

    The geopmsession command line tool parses requests for reading
    from standard input.  The RequestQueue object holds the logic for
    parsing the input stream upon construction.  The resulting object
    may be iterated upon to retrieve the requested signals that the
    user would like to read.  The Request object also provides the
    enum used to format signal values into strings.

    """
    def __init__(self):
        raise NotImplementedError('RequestQueue class is an abstract base class for ReadRequestQueue')

    def __iter__(self):
        """Iterate over list of requests

        """
        yield from self._requests

    def get_names(self):
        """Get the signal or control names from each request

        Returns:
            list(str): The name of the signal or control associated
                           with each user request.

        """
        return [rr[0] for rr in self._requests]

    def iterate_stream(self, request_stream):
        """Iterate over a stream of requests

        This is a generator function that will filter out comment
        lines and trailing white space from the input stream.  It can
        be used to iterate over a request stream from the user for
        either a read or write mode session.

        Args:
            request_stream (typing.IO): Stream containing requests from
                                   user.

        Returns:
            typing.Generator[str]: Iterate over filtered lines of the
                                   request_stream

        """
        for line in request_stream:
            line = line.strip()
            if line == '':
                break
            if not line.startswith('#'):
                yield line


class ReadRequestQueue(RequestQueue):
    """Class to encapsulate session signal configuration requests.

    """
    def __init__(self, request_stream):
        """Initialize the request queue

        The constructor parses the input stream and stores the
        requests for reading signals.  The input stream is expected
        to contain a list of requests, one per line.  Each request
        is a string of three words separated by white space.  The
        first word is the signal name, the second word is the domain
        type, and the third word is the domain index.  The domain
        type is specified as the name string, i.e one of the
        following strings: "board", "package", "core", "cpu",
        "memory", "package_integrated_memory", "nic",
        "package_integrated_nic", "gpu", "package_integrated_gpu".

        Args:
            request_stream (typing.IO): Input from user describing the
                                   requests to read signals.

        Raises:
            RuntimeError: The geopm systemd service is not running

        """
        self._requests = self.parse_requests(request_stream)
        self._formats = self.query_formats(self.get_names())

    def parse_requests(self, request_stream):
        """Parse input stream and return list of read requests

        Parse a user supplied stream into a list of tuples
        representing read requests.  The tuples are of the form
        (signal_name, domain_type, domain_idx) and are parsed one from
        each line of the stream.  Each of the values in the stream is
        separated by white space.

        Each signal_name should match one of the signal names provided
        by the service.  The domain_type is specified as the name
        string, i.e one of the following strings: "board", "package",
        "core", "cpu", "memory", "package_integrated_memory", "nic",
        "package_integrated_nic", "gpu", "package_integrated_gpu".
        The domain index is a positive integer indexing the specific
        domain.

        Args:
            request_stream (typing.IO): Input stream to parse for read
                                   requests

        Returns:
            list((str, int, int)): List of request tuples. Each
                                   request comprises a signal name,
                                   domain type, and domain index.

        Raises:
            RuntimeError: Line from stream does not split into three
                          words and is also not a comment or empty
                          line.

        """
        requests = []
        raw = []
        for line in self.iterate_stream(request_stream):
            raw.append(line)
            words = line.split()
            if len(words) != 3:
                raise RuntimeError('Read request must be three words: "{}"'.format(line))
            try:
                signal_name = words[0]
                if words[1] == '*':
                    domain_type = pio.signal_domain_type(signal_name)
                else:
                    domain_type = topo.domain_type(words[1])
                if words[2] == '*':
                    requests.extend([(signal_name, domain_type, domain_idx) for domain_idx in range(topo.num_domain(domain_type))])
                else :
                    domain_idx = int(words[2])
                    requests.append((signal_name, domain_type, domain_idx))
            except (RuntimeError, ValueError):
                raise RuntimeError('Unable to convert values into a read request: "{}"'.format(line))

        if len(requests) == 0:
            raise RuntimeError('Empty request stream.')
        self._raw = '\n'.join(raw)
        return requests

    def query_formats(self, signal_names):
        """Get  the format type for each signal name

        Returns a list of geopm::string_format_e integers that determine
        how each signal value is formatted as a string.

        Args:
            signal_names (list(str)): List of signal names to query.

        Returns:
            list(int): List of geopm::string_format_e integers, one
            for each signal name in input list.

        """

        return [pio.signal_info(sn)[1] for sn in signal_names]

    def get_formats(self):
        """Get formatting enum values for the parsed read requests

        Returns:
            list(int): The geopm::string_format_e enum value for each
                       read request.

        """
        return self._formats

    def get_raw(self):
        """Get raw string representation

        Returns:
           str: The request as a string
        """
        return self._raw

def get_parser():
    parser = ArgumentParser(description=main.__doc__)
    parser.add_argument('-v', '--version', dest='version', action='store_true',
                        help='Print version and exit.')
    parser.add_argument('-t', '--time', dest='time', type=float, default=0.0,
                        help='Total run time of the session to be opened in seconds.')
    parser.add_argument('-p', '--period', dest='period', type=float, default=0.1,
                        help='When used with a read mode session reads all values out periodically with the specified period in seconds. Default %(default)s.')
    parser.add_argument('--pid', type=int,
                        help='Stop the session when the given process PID ends.')
    header_group = parser.add_mutually_exclusive_group()
    header_group.add_argument('--print-header', action='store_true',
                              help='Deprecated now this option is the default, see --no-header.')
    header_group.add_argument('-n', '--no-header', action='store_true',
                              help='Do not print a CSV header before printing any sampled values.')
    parser.add_argument('-d', '--delimiter', dest='delimiter', default=',',
                        help='Delimiter used to separate values in CSV output. Default: %(default)s.')
    parser.add_argument('-r', '--report-out', dest='report_out', default=None,
                        help='Output summary statistics into a yaml file. Default: No summary statistics are generated.')
    parser.add_argument('-o', '--trace-out', dest='trace_out', default='-',
                        help='Output trace data into a CSV file. Default: trace data is printed to stdout.')
    parser.add_argument('--enable-mpi', dest='enable_mpi', action='store_true',
                        help='Gather reports over MPI and write to a single file. Append MPI rank to trace output file if specified (trace output to stdout not permitted). Requires mpi4py module.')
    parser.add_argument('-f', '--report-format', dest='report_format', default='yaml',
                        help='Format for report: "csv" or "yaml".  Default: %(default)s.')
    parser.add_argument('-s', '--report-samples', dest='report_samples', type=int, default=None,
                        help='Create reports each time the specified number of periods have elapsed')
    parser.add_argument('-i', '--signal-config', dest='config_path', default='-',
                        help='Input file containing GEOPM signal requests, specify "-" to use standard input which is also the default.')

    return parser

def main(agent=None):
    """Command line interface for the geopm service batch read features.

    This function can be used as a script entry point or called directly
    from Python. To use a custom agent, pass an instance of a subclass
    of Agent as the `agent` argument.

    Args:
        agent (Agent or None): Optional agent object to customize session behavior.

    Returns:
        int: 0 on success, nonzero on error.

    """
    err = 0
    trace_out = None
    session_io = None
    _config_stream = None
    signal(SIGTERM, _term_handler)
    signal(SIGINT, _term_handler)
    agent = agent_factory(agent)
    try:
        parser = get_parser()
        parser = agent.update_parser(parser)
        args = parser.parse_args()
        args = agent.update_args(args)
        if args.version:
            print(__version_str__)
            return 0
        if args.report_format not in ('yaml', 'csv'):
            raise RuntimeError(f'Invalid report format: {args.report_format}')
        if args.report_samples is not None and args.trace_out == args.report_out:
            raise RuntimeError('When using the --report-samples option the trace and report output must differ, use --report-out or --trace-out to specify a unique value')
        if args.config_path == '-':
            override = agent.signal_config_override()
            if override is None:
                config_stream = sys.stdin
            else:
                config_stream = StringIO(override)
        else:
            _config_stream = open(args.config_path)
            config_stream = _config_stream
        if args.enable_mpi:
            session_io = _MPISessionIO(request_stream=config_stream, trace_path=args.trace_out, report_path=args.report_out, report_format=args.report_format)
        else:
            session_io = _SessionIO(request_stream=config_stream, trace_path=args.trace_out, report_path=args.report_out)
        trace_out = session_io.open_trace_stream()
        sess = Session(args.delimiter, agent)
        sess.run(run_time=args.time, period=args.period, pid=args.pid, print_header=not args.no_header,
                 request_stream=None, out_stream=trace_out, report_path=None, session_io=session_io,
                 report_format=args.report_format, delimiter=args.delimiter, report_samples=args.report_samples)
    except Exception as ee:
        if 'GEOPM_DEBUG' in os.environ:
            # Do not handle exception if GEOPM_DEBUG is set
            raise ee
        sys.stderr.write('Error: {}\n\n'.format(ee))
        err = -1
    finally:
        if session_io is not None:
            session_io.close_trace_stream(trace_out)
            session_io.close_report_stream()
        if _config_stream is not None:
            _config_stream.close()
    return err

if __name__ == '__main__':
    sys.exit(main())
