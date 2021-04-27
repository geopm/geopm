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
from dasbus.connection import SystemMessageBus
from argparse import ArgumentParser
from . import topo

def read_session(geopm_proxy, time, period, requests):
    if time != 0.0:
        raise NotImplementedError('Current only support for one-shot reads')
    geopm_proxy.PlatformOpenSession()
    result = []
    for rr in requests:
        result.append(str(geopm_proxy.PlatformReadSignal(rr[0], rr[1], rr[2])))
    geopm_proxy.PlatformCloseSession()
    result_line = '{}\n'.format(','.join(result))
    sys.stdout.write(result_line)

def write_session(geopm_proxy, time, requests):
    raise NotImplementedError('Current only support for one-shot reads')

def main():
    description="""Command line interface for the geopm service read/write features.
    This command can be used to read signals and write controls by
    opening a session with the geopm service.

    """
    parser = ArgumentParser(description=description)
    parser.add_argument('-m', '--mode', dest='mode', type=str, default='r',
                        help='Session mode options are: "read" or "write", may also be specified in short form as "r" or "w"')
    parser.add_argument('-t', '--time', dest='time', type=float, default=0.0,
                        help='Total run time of the session to be openend in seconds')
    parser.add_argument('-p', '--period', dest='period', type=float, default = 0.0,
                        help='When used with a read mode session reads all values out periodically with the specified period in seconds')
    # Validate arguments
    args = parser.parse_args()
    if args.mode in ('read' or 'r'):
        mode = 'r'
        if args.period > args.time:
            raise RuntimeError('Specified a period that is greater than the total time')
        if args.period < 0.0 or args.time < 0.0:
            raise RuntimeError('Specified a negative time or period')

    elif args.mode in ('write' or 'w'):
        mode = 'w'
        if args.time <= 0.0:
            raise RuntimeError('When opening a write mode session, a time greater than zero must be specified')
    else:
        raise RuntimeError('Invalid mode specified: "{}"'.format(args.mode))
    # Parse standard input
    requests = []
    for line in sys.stdin:
        line = line.strip()
        if line == '':
            break
        if not line.startswith('#'):
            words = line.split()
            if mode == 'r':
                if len(words) != 3:
                    raise RuntimeError('Invalid command for reading: "{}"'.format(line))
                signal_name = words[0]
                domain_type = topo.domain_type(words[1])
                domain_idx = int(words[2])
                requests.append((signal_name, domain_type, domain_idx))
            else:
                if len(words) != 4:
                    raise RuntimeError('Invalid command for writing: "{}"'.format(line))
                control_name = words[0]
                domain_type = topo.domain_type(words[1])
                domain_idx = int(words[2])
                setting = float(words[3])
                requests.append((control_name, domain_type, domain_idx, setting))

    bus = SystemMessageBus()
    geopm_proxy = bus.get_proxy('io.github.geopm','/io/github/geopm')
    if mode == 'r':
        read_session(geopm_proxy, args.time, args.period, requests)
    else:
        write_session(geopm_proxy, args.time, requests)

if __name__ == '__main__':
    main()
