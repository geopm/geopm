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
from dasbus.error import DBusError
from argparse import ArgumentParser


def set_group_signals(geopm_proxy, group, signals):
    _, current_controls = geopm_proxy.PlatformGetGroupAccess(group)
    try:
        geopm_proxy.PlatformSetGroupAccess(group, signals, current_controls)
    except DBusError as ee:
        raise RuntimeError('Failed to set group signal access list, request must be made by root user') from ee

def set_group_controls(geopm_proxy, group, controls):
    current_signals, _ = geopm_proxy.PlatformGetGroupAccess(group)
    try:
        geopm_proxy.PlatformSetGroupAccess(group, current_signals, controls)
    except DBusError as ee:
        raise RuntimeError('Failed to set group control access list, request must be made by root user') from ee

def get_all_signals(geopm_proxy):
    all_signals, _ = geopm_proxy.PlatformGetAllAccess()
    return '\n'.join(all_signals)

def get_all_controls(geopm_proxy):
    _, all_controls = geopm_proxy.PlatformGetAllAccess()
    return '\n'.join(all_controls)

def get_group_signals(geopm_proxy, group):
    all_signals, _ = geopm_proxy.PlatformGetGroupAccess(group)
    return '\n'.join(all_signals)

def get_group_controls(geopm_proxy, group):
    _, all_controls = geopm_proxy.PlatformGetGroupAccess(group)
    return '\n'.join(all_controls)


def main():
    description = """Access managment for the geopm service.  Command line tool for
    reading and writing the access management lists for the geopm
    service signals and controls.

    """
    parser = ArgumentParser(description=description)
    parser_group_sc = parser.add_mutually_exclusive_group(required=True)
    parser_group_sc.add_argument('-s', '--signals', dest='signals', action='store_true', default=False,
                                 help='List signal names')
    parser_group_sc.add_argument('-c', '--controls', dest='controls', action='store_true', default=False,
                                 help='List control names')
    parser_group_ga = parser.add_mutually_exclusive_group(required=False)
    parser_group_ga.add_argument('-g', '--group', dest='group', type=str, default='',
                                help='Read or write access for a Unix group (default is for all users)')
    parser_group_ga.add_argument('-a', '--all', dest='all', action='store_true', default=False,
                                 help='Print all available signals or controls on the system (invalid with -w)')
    parser.add_argument('-w', '--write', dest='write', action='store_true', default=False,
                        help='Write restricted access list for default user or a particular Unix group from standard input')
    args = parser.parse_args()

    bus = SystemMessageBus()
    geopm_proxy = bus.get_proxy('io.github.geopm','/io/github/geopm')
    if args.write:
        in_names = [ll.strip() for ll in sys.stdin.readlines() if ll.strip()]
        if args.all:
            raise RuntimeError('Option -a/--all is not allowed if -w/--write is provided')
        else:
            if args.signals:
                set_group_signals(geopm_proxy, args.group, in_names)
            elif args.controls:
                set_group_controls(geopm_proxy, args.group, in_names)
    else:
        if args.all:
            if args.signals:
                print(get_all_signals(geopm_proxy))
            elif args.controls:
                print(get_all_controls(geopm_proxy))
        else:
            if args.signals:
                print(get_group_signals(geopm_proxy, args.group))
            elif args.controls:
                print(get_group_controls(geopm_proxy, args.group))

if __name__ == '__main__':
    main()
