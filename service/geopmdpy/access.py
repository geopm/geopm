#  Copyright (c) 2015 - 2022, Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#

"""Implementation for the geopmaccess command line tool.

"""

import sys
import os
from argparse import ArgumentParser
from dasbus.connection import SystemMessageBus
from dasbus.error import DBusError


class Access:
    """Extension to the GEOPM D-Bus proxy to support the geopmaccess
       command line interface.

    """
    def __init__(self, geopm_proxy):
        """Constructor for Access class

            Args:
                geopm_proxy (dasbus.client.proxy.InterfaceProxy): The
                    dasbus proxy for the GEOPM D-Bus interface.

        """
        try:
            geopm_proxy.PlatformGetGroupAccess
        except DBusError as ee:
            if 'io.github.geopm was not provided' in str(ee):
                err_msg = """The geopm systemd service is not enabled.
    Install geopm service and run 'systemctl start geopm'"""
                raise RuntimeError(err_msg) from ee
            else:
                raise ee
        self._geopm_proxy = geopm_proxy

    def set_group_signals(self, group, signals, is_dry_run, skip_check):
        """Call GEOPM D-Bus API to set signal access

        Sets the signal access list for a group while leaving the
        control access list unchanged.  The user must be 'root' to
        perform this operation.  The PlatformGetGroupAccess D-Bus API
        of the io.github.geopm interface is used.

        Args:
            group (str): Unix group name to set access list for.  The
                         call sets the default signal access list if
                         group provided is ''.

            signals (list(str)): List of all signal names that are
                                 allowed for the group or for the
                                 defaults.
            is_dry_run (bool): True to run error checking without file
                               modification
            skip_check (bool): True if error checking is disabled


        Raises:
            RuntimeError: The user is not root, the group provided is
                          invalid, or any of the provided signal names
                          are not supported.

        """
        try:
            _, current_controls = self._geopm_proxy.PlatformGetGroupAccess(group)
        except DBusError as ee:
            raise RuntimeError('Failed to read group signal access list for specified group: {}'.format(group)) from ee
        if not skip_check:
            signals_supported, _ = self._geopm_proxy.PlatformGetAllAccess()
            signals_requested = set(signals)
            if not signals_requested.issubset(signals_supported):
                missing = ', '.join(sorted(signals_requested.difference(signals_supported)))
                raise RuntimeError(f'Requested access to signals that are not available: {missing}')
        if not is_dry_run:
            try:
                self._geopm_proxy.PlatformSetGroupAccess(group, signals, current_controls)
            except DBusError as ee:
                raise RuntimeError('Failed to set group signal access list, try with sudo or as "root" user (requires CAP_SYS_ADMIN)') from ee

    def set_group_controls(self, group, controls, is_dry_run, skip_check):
        """Call GEOPM D-Bus API to set control access

        Sets the control access list for a group while leaving the
        signal access list unchanged.  The user must be 'root' to
        perform this operation.  The PlatformGetGroupAccess D-Bus API
        of the io.github.geopm interface is used.

        Args:
            group (str): Unix group name to set access list for.  The
                         call sets the default control access list if
                         group provided is ''.

            controls (list(str)): List of all control names that are
                                 allowed for the group or for the
                                 defaults.
            is_dry_run (bool): True to run error checking without file
                               modification
            skip_check (bool): True if error checking is disabled

        Raises:
            RuntimeError: The user is not root, the group provided is
                          invalid, or any of the provided control names are
                          not supported.

        """
        try:
            current_signals, _ = self._geopm_proxy.PlatformGetGroupAccess(group)
        except DBusError as ee:
            raise RuntimeError('Failed to read group control access list for specified group: {}'.format(group)) from ee
        if not skip_check:
            _, controls_supported = self._geopm_proxy.PlatformGetAllAccess()
            controls_requested = set(controls)
            if not controls_requested.issubset(controls_supported):
                missing = ', '.join(sorted(controls_requested.difference(controls_supported)))
                raise RuntimeError(f'Requested access to controls that are not available: {missing}')
        if not dry_run:
            try:
                self._geopm_proxy.PlatformSetGroupAccess(group, current_signals, controls)
            except DBusError as ee:
                raise RuntimeError('Failed to set group control access list, try with sudo or as "root" user (requires CAP_SYS_ADMIN)') from ee

    def get_all_signals(self):
        """Call GEOPM D-Bus API and return all supported signal names

        Returns a human readable list of all signals available on the
        platform.  The returned string has one signal name on each
        line.  The PlatformGetAllAccess D-Bus API of the
        io.github.geopm interface is used.

        Returns:
            str: All available signals, one on each line

        """
        all_signals, _ = self._geopm_proxy.PlatformGetAllAccess()
        return '\n'.join(all_signals)

    def get_all_controls(self):
        """Call GEOPM D-Bus API and return all supported control names

        Returns a human readable list of all controls available on the
        system.  The returned string has one control name on each
        line.  The PlatformGetAllAccess D-Bus API of the
        io.github.geopm interface is used.

        Returns:
            str: All available controls, one on each line

        """
        _, all_controls = self._geopm_proxy.PlatformGetAllAccess()
        return '\n'.join(all_controls)

    def get_group_signals(self, group):
        """Call GEOPM D-Bus API and return the group's signal access list

        Returns a human readable list of the signals that are enabled.
        The returned string has one signal name on each line.

        The default signal access list is returned if the group
        provided is the empty string.  If the group provided is not
        empty then the list of signals that are enabled for the
        specified Unix group is returned.

        A user process accessing the GEOPM D-Bus APIs is restricted to
        the combination of the default access list and the access list
        for all groups that the user belongs to.  The results from
        querying a specific Unix group with this method do not reflect
        the default access list that is enabled for all users.

        The Platforms D-Bus API of the io.github.geopm
        interface is called.

        Args:
            group (str): Unix group name to get access list for.  Gets
                         the default signal access list if group
                         provided is '': the empty string.

        Returns:
            str: Access list of signals, one on each line

        """

        all_signals, _ = self._geopm_proxy.PlatformGetGroupAccess(group)
        return '\n'.join(all_signals)

    def get_group_controls(self, group):
        """Call GEOPM D-Bus API and return the group's control access list

        Returns a human readable list of the controls that are
        enabled.  The returned string has one control name on each
        line.

        The default control access list is returned if the group
        provided is the empty string.  If the group provided is not
        empty then the list of controls that are enabled for the
        specified Unix group is returned.

        A user process accessing the GEOPM D-Bus APIs is restricted to
        the combination of the default access list and the access list
        for all groups that the user belongs to.  The results from
        querying a specific Unix group with this method do not reflect
        the default access list that is enabled for all users.

        The PlatformGetGroupAccess D-Bus API of the io.github.geopm
        interface is called.

        Args:
            group (str): Unix group name to get access list for.  Gets
                         the default control access list if group
                         provided is '': the empty string.

        Returns:
            str: Access list of controls, one on each line

        """
        _, all_controls = self._geopm_proxy.PlatformGetGroupAccess(group)
        return '\n'.join(all_controls)

    def read_stdin(self):
        """Parse list of signals or controls from standard input

        """
        return [ll.strip() for ll in sys.stdin.readlines() if ll.strip()]

    def run(self, is_write, is_all, is_control, group, is_user, is_delete,
            is_dry_run, skip_check):
        """Execute geopmaccess command line interface

        The inputs to this method are parsed from the command line
        interface of geopmaccess.  All of the features of the
        geopmaccess tool are implemented with this method.

        Args:

            is_write (bool): True if user requested to write to the
                             access lists, False if it is a read
                             operation.

            is_all (bool): True if the user requested that all
                           available signals or controls be printed as
                           opposed to one of the access lists.

            is_control (bool): True if the user requested to read or
                               write the control access lists.

            group (str): If is_all is not specified, the group
                         determines which access list will be read or
                         written.  If the group is the empty string
                         then the default access list is used,
                         otherwise the parameter specifies the Unix
                         group.
            is_user (bool): True if the default user access list
                            should be printed rather than the calling
                            process' access list.
            is_delete (bool): True to remove an access list file.
            is_dry_run (bool): True to run error checking without file
                               modification
            skip_check (bool): True if error checking is disabled

        """
        output = None
        if is_write:
            if is_all or is_delete:
                raise RuntimeError('Option -a/--all and -D/--delete are not valid when -w/--write is provided')
            else:
                if is_control:
                    self.set_group_controls(group, self.read_stdin(),
                                            is_dry_run, skip_check)
                else:
                    self.set_group_signals(group, self.read_stdin(),
                                           is_dry_run, skip_check)
        else:
            if is_dry_run or skip_check:
                raise RuntimeError('-n/--dry-run, and -E/--skip-check not valid unless -w/--write is provided')
            if is_all:
                if is_control:
                    output = self.get_all_controls()
                else:
                    output = self.get_all_signals()
            else:
                if is_user:
                    group = ''
                if is_control:
                    output = self.get_group_controls(group)
                else:
                    output = self.get_group_signals(group)
        return output

def main():
    """Access management for the geopm service.  Command line tool for
    reading and writing the access management lists for the geopm
    service signals and controls.

    """

    err = 0
    parser = ArgumentParser(description=main.__doc__)
    parser.add_argument('-c', '--controls', dest='controls', action='store_true', default=False,
                        help='Get or set access for controls, not signals')
    parser_group_uga = parser.add_mutually_exclusive_group(required=False)
    parser_group_uga.add_argument('-u', '--user', dest='user', action='store_true', default=False,
                                  help='Print default user access list')
    parser_group_uga.add_argument('-g', '--group', dest='group', type=str, default='',
                                 help='Read or write access for a Unix group (default is for all users)')
    parser_group_uga.add_argument('-a', '--all', dest='all', action='store_true', default=False,
                                  help='Print all available signals or controls on the system (invalid with -w)')
    parser_group_wD = parser.add_mutually_exclusive_group(required=False)
    parser_group_wD.add_argument('-w', '--write', dest='write', action='store_true', default=False,
                                 help='Write restricted access list for default user or a particular Unix group')
    parser_group_wD.add_argument('-D', '--delete', dest='delete', action='store_true', default=False,
                                 help='Remove an access list for default user or a particular Unix Group')
    parser_group_nE = parser.add_mutually_exclusive_group(required=False)
    parser_group_nE.add_argument('-n', '--dry-run', dest='dry_run', action='store_true', default=False,
                                 help='Do error checking on all user input, but do not modify configuration files')
    parser_group_nE.add_argument('-E', '--skip-check', dest='skip_check', action='store_true', default=False,
                                 help='Write access list to disk without error checking')
    args = parser.parse_args()
    try:
        acc = Access(SystemMessageBus().get_proxy('io.github.geopm',
                                                  '/io/github/geopm'))
        output = acc.run(args.write, args.all, args.controls, args.group,
                         args.user, args.delete, args.dry_run, args.skip_check)
        if output:
            print(output)
    except RuntimeError as ee:
        if 'GEOPM_DEBUG' in os.environ:
            # Do not handle exception if GEOPM_DEBUG is set
            raise ee
        sys.stderr.write('Error: {}\n\n'.format(ee))
        err = -1
    return err

if __name__ == '__main__':
    sys.exit(main())
