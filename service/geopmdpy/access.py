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

"""Implementation for the geopmaccess command line tool.

"""

import sys
from dasbus.connection import SystemMessageBus
from dasbus.error import DBusError
from argparse import ArgumentParser


def set_group_signals(geopm_proxy, group, signals):
    """Call GEOPM D-Bus API to set signal access

    Sets the signal access list for a group while leaving the control
    access list unchanged.  The user must be 'root' to perform this
    operation.  The PlatformGetGroupAccess D-Bus API of the
    io.github.geopm interface is used.

    Args:
        geopm_proxy (dasbus.client.proxy.InterfaceProxy): The dasbus
            proxy for the GEOPM D-Bus interface.

        group (str): Unix group name to set access list for.  The call
                     sets the default signal access list if group
                     provided is ''.

        signals (list(str)): List of all signal names that are allowed
                             for the group or for the defaults.

    Raises:
        RuntimeError: The user is not root, the group provided is
                      invalid, or any of the provided signal names are
                      not supported.

    """
    try:
        _, current_controls = geopm_proxy.PlatformGetGroupAccess(group)
    except DBusError as ee:
        raise RuntimeError('Failed to read group signal access list for specified group: {}'.format(group)) from ee
    try:
        geopm_proxy.PlatformSetGroupAccess(group, signals, current_controls)
    except DBusError as ee:
        raise RuntimeError('Failed to set group signal access list, request must be made by root user') from ee

def set_group_controls(geopm_proxy, group, controls):
    """Call GEOPM D-Bus API to set control access

    Sets the control access list for a group while leaving the signal
    access list unchanged.  The user must be 'root' to perform this
    operation.  The PlatformGetGroupAccess D-Bus API of the
    io.github.geopm interface is used.

    Args:
        geopm_proxy (dasbus.client.proxy.InterfaceProxy): The dasbus
            proxy for the GEOPM D-Bus interface.

        group (str): Unix group name to set access list for.  The call
                     sets the default control access list if group
                     provided is ''.

        controls (list(str)): List of all control names that are allowed
                             for the group or for the defaults.

    Raises:
        RuntimeError: The user is not root, the group provided is
                      invalid, or any of the provided control names are
                      not supported.

    """
    try:
        current_signals, _ = geopm_proxy.PlatformGetGroupAccess(group)
    except DBusError as ee:
        raise RuntimeError('Failed to read group control access list for specified group: {}'.format(group)) from ee
    try:
        geopm_proxy.PlatformSetGroupAccess(group, current_signals, controls)
    except DBusError as ee:
        raise RuntimeError('Failed to set group control access list, request must be made by root user') from ee

def get_all_signals(geopm_proxy):
    """Call GEOPM D-Bus API and return all supported signal names

    Returns a human readable list of all signals available on the
    system.  The returned string has one signal name on each line.  The
    PlatformGetAllAccess D-Bus API of the io.github.geopm interface is
    used.

    Args:
        geopm_proxy (dasbus.client.proxy.InterfaceProxy): The dasbus
            proxy for the GEOPM D-Bus interface.

    Returns:
        str: All available signals, one on each line

    """
    all_signals, _ = geopm_proxy.PlatformGetAllAccess()
    return '\n'.join(all_signals)

def get_all_controls(geopm_proxy):
    """Call GEOPM D-Bus API and return all supported control names

    Returns a human readable list of all controls available on the
    system.  The returned string has one control name on each line.  The
    PlatformGetAllAccess D-Bus API of the io.github.geopm interface is
    used.

    Args:
        geopm_proxy (dasbus.client.proxy.InterfaceProxy): The dasbus
            proxy for the GEOPM D-Bus interface.

    Returns:
        str: All available controls, one on each line

    """
    _, all_controls = geopm_proxy.PlatformGetAllAccess()
    return '\n'.join(all_controls)

def get_group_signals(geopm_proxy, group):
    """Call GEOPM D-Bus API and return the group's signal access list

    Returns a human readable list of the signals that are enabled when
    a user belongs to the provided Unix group.  The default signal
    access list is returned if the group provided is the empty string.
    If the group provided is not empty then the list of signals that
    are enabled for the group is returned.  A user is restricted to
    the combination of the default access list and the access list for
    all groups that they belong to.  The results from querying a
    specific Unix group do not reflect the default access list.  The
    returned string has one signal name on each line.  The
    PlatformGetGroupAccess D-Bus API of the io.github.geopm interface
    is used.

    Args:
        geopm_proxy (dasbus.client.proxy.InterfaceProxy): The dasbus
            proxy for the GEOPM D-Bus interface.

        group (str): Unix group name to set access list for.  Sets the
                     default control access list if group provided is
                     ''. the empty string.

    Returns:
        str: Access list of signals, one on each line

    """

    all_signals, _ = geopm_proxy.PlatformGetGroupAccess(group)
    return '\n'.join(all_signals)

def get_group_controls(geopm_proxy, group):
    _, all_controls = geopm_proxy.PlatformGetGroupAccess(group)
    return '\n'.join(all_controls)

def run(geopm_proxy, is_write, is_all, is_control, group):
    output = None
    if is_write:
        in_names = [ll.strip() for ll in sys.stdin.readlines() if ll.strip()]
        if is_all:
            raise RuntimeError('Option -a/--all is not allowed if -w/--write is provided')
        else:
            if is_control:
                set_group_controls(geopm_proxy, group, in_names)
            else:
                set_group_signals(geopm_proxy, group, in_names)
    else:
        if is_all:
            if is_control:
                output = get_all_controls(geopm_proxy)
            else:
                output = get_all_signals(geopm_proxy)
        else:
            if is_control:
                output = get_group_controls(geopm_proxy, group)
            else:
                output = get_group_signals(geopm_proxy, group)
    return output

def main():
    description = """Access managment for the geopm service.  Command line tool for
    reading and writing the access management lists for the geopm
    service signals and controls.

    """
    err = 0
    parser = ArgumentParser(description=description)
    parser.add_argument('-c', '--controls', dest='controls', action='store_true', default=False,
                        help='Get or set access for controls, not signals')
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
    try:
        output = run(geopm_proxy, args.write, args.all, args.controls, args.group)
        if output:
            print(output)
    except RuntimeError as ex:
        if 'GEOPM_DEBUG' in os.eniron:
            raise ex
        sys.stderr.write('Error: {}\n\n'.format(ex))
        err = -1
    return err

if __name__ == '__main__':
    err = main()
    exit(err)
