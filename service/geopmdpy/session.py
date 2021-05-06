#!/usr/bin/env python3

#  Copyright (c) 2015 - 2021, Intel Corporation
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

import sys
import time
from dasbus.connection import SystemMessageBus
from argparse import ArgumentParser
from . import topo

def read_session(geopm_proxy, duration, period, requests):
    geopm_proxy.PlatformOpenSession()
    if period == 0:
        num_period = 1
    else:
        num_period = int(duration / period)  + 1
    for period_idx in range(num_period):
        result = []
        for rr in requests:
            result.append(str(geopm_proxy.PlatformReadSignal(*rr)))
        result_line = '{}\n'.format(','.join(result))
        sys.stdout.write(result_line)
        time.sleep(period)
    geopm_proxy.PlatformCloseSession()

def write_session(geopm_proxy, duration, requests):
    geopm_proxy.PlatformOpenSession()
    for rr in requests:
        geopm_proxy.PlatformWriteControl(*rr)
    time.sleep(duration)
    geopm_proxy.PlatformCloseSession()

def main():
    description="""Command line interface for the geopm service read/write features.
    This command can be used to read signals and write controls by
    opening a session with the geopm service.

    """
    parser = ArgumentParser(description=description)
    parser.add_argument('-w', '--write', dest='write', action='store_true', default=False,
                        help='Open a write mode session to adjust control values.')
    parser.add_argument('-t', '--time', dest='time', type=float, default=0.0,
                        help='Total run time of the session to be openend in seconds')
    parser.add_argument('-p', '--period', dest='period', type=float, default = 0.0,
                        help='When used with a read mode session reads all values out periodically with the specified period in seconds')
    # Validate arguments
    args = parser.parse_args()
    if not args.write:
        if args.period > args.time:
            raise RuntimeError('Specified a period that is greater than the total time')
        if args.period < 0.0 or args.time < 0.0:
            raise RuntimeError('Specified a negative time or period')

    else:
        if args.time <= 0.0:
            raise RuntimeError('When opening a write mode session, a time greater than zero must be specified')
        if args.period != 0.0:
            raise RuntimeError('Cannot specify period with write mode session')

    # Parse standard input
    requests = []
    for line in sys.stdin:
        line = line.strip()
        if line == '':
            break
        if not line.startswith('#'):
            words = line.split()
            if args.write:
                if len(words) != 4:
                    raise RuntimeError('Invalid command for writing: "{}"'.format(line))
                control_name = words[0]
                domain_type = topo.domain_type(words[1])
                domain_idx = int(words[2])
                setting = float(words[3])
                requests.append((control_name, domain_type, domain_idx, setting))
            else:
                if len(words) != 3:
                    raise RuntimeError('Invalid command for reading: "{}"'.format(line))
                signal_name = words[0]
                domain_type = topo.domain_type(words[1])
                domain_idx = int(words[2])
                requests.append((signal_name, domain_type, domain_idx))

    bus = SystemMessageBus()
    geopm_proxy = bus.get_proxy('io.github.geopm','/io/github/geopm')
    if args.write:
        write_session(geopm_proxy, args.time, requests)
    else:
        read_session(geopm_proxy, args.time, args.period, requests)

if __name__ == '__main__':
    main()
