#  Copyright (c) 2015 - 2022, Intel Corporation
#
#  Redistribution and use in source and binary forms, with or without
#  modification, are permitted provided that the following conditions
#  are met:
#
#      * Redistributions of source code must retain the above copyright
#        notice, this list of conditions and the following disclaimer.
#
#      * Redistributions in binary form must reproduce the above copyright
#        notice, this list of conditions and the following disclaimer in
#        the documentation and/or other materials provided with the
#        distribution.
#
#      * Neither the name of Intel Corporation nor the names of its
#        contributors may be used to endorse or promote products derived
#        from this software without specific prior written permission.
#
#  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
#  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
#  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
#  A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
#  OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
#  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
#  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
#  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
#  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
#  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY LOG OF THE USE
#  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#

"""Implementation for the geopmsession command line tool

"""

import sys
import os
import time
import math
from argparse import ArgumentParser
from dasbus.connection import SystemMessageBus
from dasbus.error import DBusError
from . import topo
from . import pio
from . import runtime


class Session:
    """Object responsible for creating a GEOPM service session

    This object's run() method is the main entry point for
    geopmsession command line tool.  The inputs to run() are derived
    from the command line options provided by the user.

    The Session object depends on the RequestQueue object to parse the
    input request buffer from the user.  The Session object also
    depends on the runtime.TimedLoop object when executing a periodic
    read session.

    """

    def __init__(self, geopm_proxy):
        """Constructor for Session class

        Args:
            geopm_proxy (dasbus.client.proxy.InterfaceProxy): The
                dasbus proxy for the GEOPM D-Bus interface.

        Raises:
            RuntimeError: The geopm systemd service is not running

        """
        try:
            geopm_proxy.PlatformOpenSession
        except DBusError as ee:
            if 'io.github.geopm was not provided' in str(ee):
                err_msg = """The geopm systemd service is not enabled.
    Install geopm service and run 'systemctl start geopm'"""
                raise RuntimeError(err_msg) from ee
            else:
                raise ee
        self._geopm_proxy = geopm_proxy

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

        Use the GEOPM D-Bus interface to periodically read the
        requested signals. A line of text will be printed to the
        output stream for each period of time.  The line will contain
        a comma separated list of the read values, one for each
        request.

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

            out_stream (file): Object with write() method where output
                               will be printed (typically sys.stdout).

        """
        num_period = 0
        if period != 0:
            num_period = math.ceil(duration / period)
        signal_handles = []
        for name, dom, dom_idx in requests:
            if not name.startswith('SERVICE::'):
                name = f'SERVICE::{name}'
            signal_handles.append(pio.push_signal(name, dom, dom_idx))

        for sample_idx in runtime.TimedLoop(period, num_period):
            pio.read_batch()
            signals = [pio.sample(handle) for handle in signal_handles]
            line = self.format_signals(signals, requests.get_formats())
            out_stream.write(line)

    def run_write(self, requests, duration):
        """Run a write mode session

        Use the GEOPM D-Bus interface to write the requested set of
        controls.  The control settings will be held for the specified
        duration of time and the call to this method blocks until the
        duration of time has elapsed.

        Args:
            requests (WriteRequestQueue): Request object parsed from
                                          user input.

            duration (float): Length of time to hold the requested
                              control settings in units of seconds.

        """
        self._geopm_proxy.PlatformOpenSession()
        key = 'SERVICE::'
        for name, dom, dom_idx, setting in requests:
            if name.startswith(key):
                name = name.replace(key, '', 1)
            self._geopm_proxy.PlatformWriteControl(name, dom, dom_idx, setting)
        time.sleep(duration)
        self._geopm_proxy.PlatformCloseSession()

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
        if period < 0.0 or run_time < 0.0:
            raise RuntimeError('Specified a negative run time or period')

    def check_write_args(self, run_time, period):
        """Check that the run time and period are valid for a write session

        Args:
            run_time (float): Time duration of the session in seconds.

            period (float): The user specified period between samples
                            in units of seconds.

        Raises:
            RuntimeError: The period is non-zero or total time is negative.

        """
        if run_time <= 0.0:
            raise RuntimeError('When opening a write mode session, a time greater than zero must be specified')
        if period != 0.0:
            raise RuntimeError('Cannot specify period with write mode session')

    def run(self, is_write, run_time, period,
            request_stream=sys.stdin, out_stream=sys.stdout):
        """"Create a GEOPM session with values parsed from the command line

        The implementation for the geopmsession command line tool.
        The inputs to this method are derived from the parsed command
        line provided by the user.

        Args:
            is_write (bool): True for write mode session requests, and
                             False for read mode session requests.

            run_time (float): Time duration of the session in seconds.

            period (float): Time interval for each line of output for
                            a read session.  Value must be zero for a
                            write mode session.

            request_stream (file): Input from user describing the
                                   requests to read or write values.

            out_stream (file): Stream where output from a read mode
                               session will be printed.  This
                               parameter is not used for a write mode
                               session.

        """
        if is_write:
            requests = WriteRequestQueue(request_stream)
            self.check_write_args(run_time, period)
            self.run_write(requests, run_time)
        else:
            requests = ReadRequestQueue(request_stream, self._geopm_proxy)
            self.check_read_args(run_time, period)
            self.run_read(requests, run_time, period, out_stream)


class RequestQueue:
    """Object derived from user input that provides request information

    The geopmsession command line tool parses requests for reading or
    writing from standard input.  The RequestQueue object holds the
    logic for parsing the input stream upon construction.  The
    resulting object may be iterated upon to retrieve the requested
    signals/controls that the user would like to read/write.  The
    Request object also provides the enum used to format signal values
    into strings (for read mode sessions).

    """
    def __init__(self):
        raise NotImplementedError('RequestQueue class is an abstract base class for ReadRequestQueue and WriteRequestQueue')

    def __iter__(self):
        """Iterate over list of requests

        """
        yield from self._requests

    def get_names(self):
        """Get the signal or control names from each request

        Strip off "SERVICE::" prefix from any names in request queue,
        so that the name can be passed to the proxy.

        Returns:
            list(string): The name of the signal or control associated
                           with each user request.

        """
        key = 'SERVICE::'
        return [rr[0] if not rr[0].startswith(key) else rr[0].replace(key, '', 1)
                for rr in self._requests]

    def iterate_stream(self, request_stream):
        """Iterate over a stream of requests

        This is a generator function that will filter out comment
        lines and trailing white space from the input stream.  It can
        be used to iterate over a request stream from the user for
        either a read or write mode session.

        Args:
            request_stream (file): Stream containing requests from
                                   user.

        Returns:
            generator: Iterate over filtered lines of the
                       request_stream

        """
        for line in request_stream:
            line = line.strip()
            if line == '':
                break
            if not line.startswith('#'):
                yield line


class ReadRequestQueue(RequestQueue):
    def __init__(self, request_stream, geopm_proxy):
        """Constructor for ReadRequestQueue object

        Args:
            request_stream (file): Input from user describing the
                                   requests to read signals.

            geopm_proxy (dasbus.client.proxy.InterfaceProxy): The
                dasbus proxy for the GEOPM D-Bus interface.

        Raises:
            RuntimeError: The geopm systemd service is not running

        """
        self._requests = self.parse_requests(request_stream)
        self._formats = self.query_formats(self.get_names(), geopm_proxy)

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
        "core", "cpu", "board_memory", "package_memory", "board_nic",
        "package_nic", "board_accelerator", "package_accelerator".
        The domain index is a positive integer indexing the specific
        domain.

        Args:
            request_stream (file): Input stream to parse for read
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

    def query_formats(self, signal_names, geopm_proxy):
        """Call the GEOPM D-Bus API to get the format type for each signal name

        Returns a list of geopm::string_format_e integers that determine
        how each signal value is formatted as a string.

        Args:
            signal_names (list(str)): List of signal names to query.

            geopm_proxy (dasbus.client.proxy.InterfaceProxy): The
                dasbus proxy for the GEOPM D-Bus interface.

        Returns:
            list(int): List of geopm::string_format_e integers, one
                       for each signal name in input list.

        """

        try:
            result = [info[4] for info in
                      geopm_proxy.PlatformGetSignalInfo(signal_names)]
        except DBusError as ee:
            if 'io.github.geopm was not provided' in str(ee):
                err_msg = """The geopm systemd service is not enabled.
    Install geopm service and run 'systemctl start geopm'"""
                raise RuntimeError(err_msg) from ee
            else:
                raise ee
        return result

    def get_formats(self):
        """Get formatting enum values for the parsed read requests

        Returns:
            list(int): The geopm::string_format_e enum value for each
                       read request.

        """
        return self._formats


class WriteRequestQueue(RequestQueue):
    def __init__(self, request_stream):
        """Constructor for RequestQueue object

        Args:
            request_stream (file): Input from user describing the
                                   requests to write controls.

        Raises:
            RuntimeError: The geopm systemd service is not running

        """
        self._requests = self.parse_requests(request_stream)

    def parse_requests(self, request_stream):
        """Parse input stream and return list of write requests

        Parse a user supplied stream into a list of tuples
        representing write requests.  The tuples are of the form
        (control_name, domain_type, domain_idx, setting) and are parsed
        one from each line of the stream.  Each of the values in the
        stream is separated by white space.

        Each control_name should match one of the control names
        provided by the service.  The domain_type is specified as the
        name string, i.e one of the following strings: "board",
        "package", "core", "cpu", "board_memory", "package_memory",
        "board_nic", "package_nic", "board_accelerator",
        "package_accelerator".  The domain index is a positive integer
        indexing the specific domain.  The setting is the requested
        control value.

        Args:
            request_stream (file): Input stream to parse for write
                                   requests

        Returns:
            list((str, int, int, float)): List of request tuples. Each
                request comprises a signal name, domain type, domain
                index, and setting value.

        Raises:
            RuntimeError: Line from stream does not split into four
                          words and is also not a comment or empty
                          line.

        """
        requests = []
        for line in self.iterate_stream(request_stream):
            words = line.split()
            if len(words) != 4:
                raise RuntimeError('Write request must be four words: "{}"'.format(line))
            try:
                control_name = words[0]
                domain_type = topo.domain_type(words[1])
                domain_idx = int(words[2])
                setting = float(words[3])
            except (RuntimeError, ValueError):
                raise RuntimeError('Unable to convert values into a write request: "{}"'.format(line))
            requests.append((control_name, domain_type, domain_idx, setting))
        if len(requests) == 0:
            raise RuntimeError('Empty request stream.')
        return requests


def main():
    """Command line interface for the geopm service read/write features.

    This command can be used to read signals and write controls by
    opening a session with the geopm service.

    """
    err = 0
    parser = ArgumentParser(description=main.__doc__)
    parser.add_argument('-w', '--write', dest='write', action='store_true', default=False,
                        help='Open a write mode session to adjust control values.')
    parser.add_argument('-t', '--time', dest='time', type=float, default=0.0,
                        help='Total run time of the session to be opened in seconds')
    parser.add_argument('-p', '--period', dest='period', type=float, default = 0.0,
                        help='When used with a read mode session reads all values out periodically with the specified period in seconds')
    args = parser.parse_args()
    try:
        sess = Session(SystemMessageBus().get_proxy('io.github.geopm',
                                                    '/io/github/geopm'))
        sess.run(args.write, args.time, args.period)
    except RuntimeError as ee:
        if 'GEOPM_DEBUG' in os.environ:
            # Do not handle exception if GEOPM_DEBUG is set
            raise ee
        sys.stderr.write('Error: {}\n\n'.format(ee))
        err = -1
    return err

if __name__ == '__main__':
    sys.exit(main())
