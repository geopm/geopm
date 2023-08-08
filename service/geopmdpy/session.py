#  Copyright (c) 2015 - 2023, Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#

"""Implementation for the geopmsession command line tool

"""

import sys
import os
import time
import math
from argparse import ArgumentParser
from . import topo
from . import pio
from . import runtime


class Session:
    """Object responsible for creating a GEOPM batch read session

    This object's run() method is the main entry point for
    geopmsession command line tool.  The inputs to run() are derived
    from the command line options provided by the user.

    The Session object depends on the RequestQueue object to parse the
    input request buffer from the user.  The Session object also
    depends on the runtime.TimedLoop object when executing a periodic
    read session.

    """

    def __init__(self):
        """Constructor for Session class

        """
        pass

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
        return '{}\n'.format(','.join(result))

    def run_read(self, requests, duration, period, out_stream):
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

            out_stream (typing.IO): Object with write() method where output
                               will be printed (typically sys.stdout).

        """
        num_period = 0
        if period != 0:
            num_period = math.ceil(duration / period)
        signal_handles = []
        for name, dom, dom_idx in requests:
            signal_handles.append(pio.push_signal(name, dom, dom_idx))

        for sample_idx in runtime.TimedLoop(period, num_period):
            pio.read_batch()
            signals = [pio.sample(handle) for handle in signal_handles]
            line = self.format_signals(signals, requests.get_formats())
            out_stream.write(line)

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

    def run(self, run_time, period,
            request_stream=sys.stdin, out_stream=sys.stdout):
        """"Create a GEOPM session with values parsed from the command line

        The implementation for the geopmsession command line tool.
        The inputs to this method are derived from the parsed command
        line provided by the user.

        Args:
            run_time (float): Time duration of the session in seconds.

            period (float): Time interval for each line of output for.
                            Value must be zero for a write mode
                            session.

            request_stream (typing.IO): Input from user describing the
                                   requests to read.

            out_stream (typing.IO): Stream where output from will be
                               printed.

        """
        requests = ReadRequestQueue(request_stream)
        self.check_read_args(run_time, period)
        self.run_read(requests, run_time, period, out_stream)


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
        for line in self.iterate_stream(request_stream):
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

def main():
    """Command line interface for the geopm service batch read features.

    This command can be used to read signals by opening a session with
    the geopm service.

    """
    err = 0
    parser = ArgumentParser(description=main.__doc__)
    parser.add_argument('-t', '--time', dest='time', type=float, default=0.0,
                        help='Total run time of the session to be opened in seconds')
    parser.add_argument('-p', '--period', dest='period', type=float, default = 0.0,
                        help='When used with a read mode session reads all values out periodically with the specified period in seconds')
    args = parser.parse_args()
    try:
        sess = Session()
        sess.run(args.time, args.period)
    except RuntimeError as ee:
        if 'GEOPM_DEBUG' in os.environ:
            # Do not handle exception if GEOPM_DEBUG is set
            raise ee
        sys.stderr.write('Error: {}\n\n'.format(ee))
        err = -1
    return err

if __name__ == '__main__':
    sys.exit(main())
