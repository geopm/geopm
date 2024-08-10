#  Copyright (c) 2015 - 2024 Intel Corporation
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
from . import topo
from . import pio
from . import loop
from . import stats
from . import __version_str__

try:
    from mpi4py import MPI
except ModuleNotFoundError as ex:
    MPI = ex

from contextlib import contextmanager
@contextmanager
def _nullcontext():
    yield None
try:
    from contextlib import nullcontext
except ImportError as ex:
    nullcontext = _nullcontext

class Session:
    """Object responsible for creating a GEOPM batch read session

    This object's run() method is the main entry point for
    geopmsession command line tool.  The inputs to run() are derived
    from the command line options provided by the user.

    The Session object depends on the RequestQueue object to parse the
    input request buffer from the user.  The Session object also
    depends on the loop.TimedLoop object when executing a periodic
    read session.

    """

    def __init__(self, delimiter=','):
        """Constructor for Session class

        """
        self._delimiter=delimiter

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

    def run_read(self, requests, duration, period, pid, out_stream, stats_collector=None):
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
        if period != 0:
            num_period = math.ceil(duration / period)
        signal_handles = []
        for name, dom, dom_idx in requests:
            signal_handles.append(pio.push_signal(name, dom, dom_idx))
        for sample_idx in loop.TimedLoop(period, num_period):
            pio.read_batch()
            if out_stream is not None:
                signals = [pio.sample(handle) for handle in signal_handles]
                line = self.format_signals(signals, requests.get_formats())
                out_stream.write(line)
            if stats_collector is not None:
                stats_collector.update()

            if pid is not None:
                try:
                    os.kill(pid, 0)
                except OSError as e:
                    if e.errno == errno.ESRCH:
                        # No such process. Stop watching.
                        break
                    elif e.errno == errno.EPERM:
                        # There's a still a process, but we aren't allowed to
                        # signal it. Since there's still a process, keep going.
                        pass
                    else:
                        # Any other error. Bubble up the exception.
                        raise

    def check_read_args(self, run_time, period):
        """Check that the run time and period are valid for a read session

        Args:
            run_time (float): Time duration of the session open in
                              seconds.

            period (float): The user specified period between samples
                            in units of seconds.

        Raises:
            RuntimeError: The period is greater than the total time or
                          either is negative.

        """
        if period > run_time:
            raise RuntimeError('Specified a period that is greater than the total run time')
        if period > 86400:
            raise RuntimeError('Specified a period greater than 24 hours')
        if period < 0.0 or run_time < 0.0:
            raise RuntimeError('Specified a negative run time or period')

    def header_names(self, requests):
        return [f'"{name}-{topo.domain_name(domain)}-{domain_idx}"'
                if topo.domain_name(domain) != 'board' else f'"{name}"'
                for name, domain, domain_idx in requests]

    def run(self, run_time, period, pid, print_header,
            request_stream=sys.stdin, out_stream=sys.stdout, report_stream=None,
            mpi_comm=None):
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

            request_stream (typing.IO): Input from user describing the
                                        requests to read.

            out_stream (typing.IO): Stream where output from will be
                                    printed.

        """
        rank = 0
        if mpi_comm is not None:
            rank = mpi_comm.Get_rank()
        if rank == 0:
            requests = ReadRequestQueue(request_stream)
        request_raw = None
        if rank == 0 and mpi_comm is not None:
            request_raw = requests.get_raw()
        if mpi_comm is not None:
            request_raw = mpi_comm.bcast(request_raw, root=0)
        if rank != 0:
            requests = ReadRequestQueue(StringIO(request_raw))
        do_stats = report_stream is not None
        self.check_read_args(run_time, period)
        signal_config = [(rr[0], rr[1], rr[2]) for rr in requests]
        if out_stream is not None and print_header:
            header_names = self.header_names(signal_config)
            print(self._delimiter.join(header_names), file=out_stream)
        with stats.Collector(signal_config) if do_stats else nullcontext() as stats_collector:
            self.run_read(requests, run_time, period, pid, out_stream, stats_collector)
            if do_stats:
                report = stats_collector.report_yaml()
        if do_stats and mpi_comm is not None:
            report_list = mpi_comm.gather(report, root=0)
            if rank == 0:
                report = '\n---\n\n'.join(report_list)
        if do_stats and rank == 0:
            report_stream.write(report)

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
    def __init__(self, request_stream):
        """Constructor for ReadRequestQueue object

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
                domain_type = topo.domain_type(words[1])
                domain_idx = int(words[2])
            except (RuntimeError, ValueError):
                raise RuntimeError('Unable to convert values into a read request: "{}"'.format(line))
            requests.append((signal_name, domain_type, domain_idx))
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
        return self._raw

def get_parser():
    parser = ArgumentParser(description=main.__doc__)
    parser.add_argument('-v', '--version', dest='version', action='store_true',
                        help='Print version and exit.')
    parser.add_argument('-t', '--time', dest='time', type=float, default=0.0,
                        help='Total run time of the session to be opened in seconds.')
    parser.add_argument('-p', '--period', dest='period', type=float, default=0.0,
                        help='When used with a read mode session reads all values out periodically with the specified period in seconds.')
    parser.add_argument('--pid', type=int,
                        help='Stop the session when the given process PID ends.')
    parser.add_argument('--print-header', action='store_true',
                        help='Deprecated now this option is the default, see --omit-header.')
    parser.add_argument('--omit-header', action='store_true',
                        help='Do not print a CSV header before printing any sampled values.')
    parser.add_argument('-d', '--delimiter', dest='delimiter', default=',',
                        help='Delimiter used to separate values in CSV output. Default: %(default)s.')
    parser.add_argument('--report-out', dest='report_out', default=None,
                        help='Output summary statistics into a yaml file. Default: No summary statistics are generated.')
    parser.add_argument('--trace-out', dest='trace_out', default='-',
                        help='Output trace data into a CSV file. Default: trace data is printed to stdout.')
    parser.add_argument('--enable-mpi', dest='enable_mpi', action='store_true',
                        help='Gather reports over MPI and write to a single file. Append hostname to trace output file if specified (trace output to stdout not permitted). Requires mpi4py module.')
    return parser

def main():
    """Command line interface for the geopm service batch read features.

    The input to the command line tool has one request per line.  A
    request for reading is made of up three strings separated by white
    space.  The first string is the signal name, the second string is
    the domain name, and the third string is the domain index.

    """
    err = 0
    rank = 0
    trace_out = None
    report_out = None
    try:
        args = get_parser().parse_args()
        if args.version:
            print(__version_str__)
            return 0
        mpi_comm = None
        if args.enable_mpi:
            if type(MPI) == ModuleNotFoundError:
                raise MPI
            mpi_comm = MPI.COMM_WORLD
            rank =  mpi_comm.Get_rank()

        if args.trace_out != "/dev/null":
            if args.trace_out == "-":
                if args.enable_mpi:
                    raise RuntimeError('Cannot write trace to standard output when specifying the --enable-mpi option')
                trace_out = sys.stdout
            else:
                if args.enable_mpi:
                    args.trace_out += f'-{gethostname()}'
                trace_out = open(args.trace_out, "w")
        if args.report_out == "-":
            if rank == 0:
                report_out = sys.stdout
            else:
                report_out = True
        elif args.report_out is not None:
            if rank == 0:
                report_out = open(args.report_out, "w")
            else:
                report_out = True

        sess = Session(args.delimiter)
        sess.run(run_time=args.time, period=args.period, pid=args.pid, print_header=not args.omit_header,
                 out_stream=trace_out, report_stream=report_out, mpi_comm=mpi_comm)
    except RuntimeError as ee:
        if 'GEOPM_DEBUG' in os.environ:
            # Do not handle exception if GEOPM_DEBUG is set
            raise ee
        sys.stderr.write('Error: {}\n\n'.format(ee))
        err = -1
    finally:
        if report_out is not None and rank == 0 and args.report_out != "-":
            report_out.close()
        if trace_out is not None and args.trace_out != "-":
            trace_out.close()
    return err

if __name__ == '__main__':
    sys.exit(main())
