#  Copyright (c) 2015 - 2025 Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#

"""Implementation for the geopmaccess command line tool.

"""

import sys
import os
import tempfile
import grp
import subprocess # nosec
from argparse import ArgumentParser
from dasbus.connection import SystemMessageBus
from dasbus.error import DBusError
from geopmdpy import system_files
from . import gffi
from . import error
from . import __version_str__

class DirectAccessProxy:
    """A proxy for access-related GEOPM service interactions that attempts
    in-process operations instead of issuing D-Bus calls.
    """

    def __init__(self):
        self._access_lists = system_files.AccessLists(system_files.get_config_path())

    def PlatformGetGroupAccess(self, group):
        return self._access_lists.get_group_access(group)

    def PlatformSetGroupAccess(self, group, allowed_signals, allowed_controls, **call_info):
        self._access_lists.set_group_access(group, allowed_signals, allowed_controls)

    def PlatformSetGroupAccessSignals(self, group, allowed_signals, **call_info):
        self._access_lists.set_group_access_signals(group, allowed_signals)

    def PlatformSetGroupAccessControls(self, group, allowed_controls, **call_info):
        self._access_lists.set_group_access_controls(group, allowed_controls)

    def PlatformGetUserAccess(self, **call_info):
        raise NotImplementedError('Cannot get user signals or controls in direct access mode')

    def PlatformGetAllAccess(self):
        return self._access_lists.get_all_access()


class Access:
    """Extension to the GEOPM D-Bus proxy to support the geopmaccess
       command line interface.

    """
    def __init__(self, geopm_proxy):
        """Constructor for Access class

            Args:
                geopm_proxy (dasbus.client.proxy.InterfaceProxy or GRPCClient): The
                    proxy for the GEOPM interface (DBus or gRPC).

        """
        try:
            dir(geopm_proxy)
        except DBusError as dbus_ee:
            if 'io.github.geopm was not provided' in str(dbus_ee):
                # Switch to gRPC client if DBus is unavailable
                try:
                    from geopmdpy.grpc_client import GRPCClient  # Import the gRPC client
                    geopm_proxy = GRPCClient()
                except ModuleNotFoundError as grpc_ee:
                    raise RuntimeError(f'Could not attach to GEOPM Access Service, consider "geopmccess --direct" option\nAttach with DBus error: {dbus_ee}\nAttach with gRPC error: {grpc_ee}')
            else:
                raise dbus_ee
        self._geopm_proxy = geopm_proxy

    def set_group_signals(self, group, signals, is_dry_run, is_force):
        """Call GEOPM D-Bus API to set signal access

        Sets the signal access list for a group while leaving the
        control access list unchanged.  The user must be 'root' to
        perform this operation.  The PlatformGetGroupAccess D-Bus API
        of the io.github.geopm interface is used.

        Args:
            group (str): Unix Group ID to set access list for.  The
                         call sets the default signal access list if
                         group provided is ''.

            signals (list(str)): List of all signal names that are
                                 allowed for the group or for the
                                 defaults.
            is_dry_run (bool): True to run error checking without file
                               modification
            is_force (bool): True if error checking is disabled


        Raises:
            RuntimeError: The user is not root, the group provided is
                          invalid, or any of the provided signal names
                          are not supported.

        """
        if not is_force:
            signals_supported, _ = self._geopm_proxy.PlatformGetAllAccess()
            signals_requested = set(signals)
            if not signals_requested.issubset(signals_supported):
                missing = ', '.join(sorted(signals_requested.difference(signals_supported)))
                raise RuntimeError(f'Requested access to signals that are not available: {missing}')
        if not is_dry_run:
            try:
                self._geopm_proxy.PlatformSetGroupAccessSignals(group, signals)
            except DBusError as ee:
                raise RuntimeError('Failed to set group signal access list, try with sudo or as "root" user (requires CAP_SYS_ADMIN)') from ee

    def set_group_controls(self, group, controls, is_dry_run, is_force):
        """Call GEOPM D-Bus API to set control access

        Sets the control access list for a group while leaving the
        signal access list unchanged.  The user must be 'root' to
        perform this operation.  The PlatformGetGroupAccess D-Bus API
        of the io.github.geopm interface is used.

        Args:
            group (str): Unix Group ID to set access list for.  The
                         call sets the default control access list if
                         group provided is ''.

            controls (list(str)): List of all control names that are
                                 allowed for the group or for the
                                 defaults.
            is_dry_run (bool): True to run error checking without file
                               modification
            is_force (bool): True if error checking is disabled

        Raises:
            RuntimeError: The user is not root, the group provided is
                          invalid, or any of the provided control names are
                          not supported.

        """
        if not is_force:
            _, controls_supported = self._geopm_proxy.PlatformGetAllAccess()
            controls_requested = set(controls)
            if not controls_requested.issubset(controls_supported):
                missing = ', '.join(sorted(controls_requested.difference(controls_supported)))
                raise RuntimeError(f'Requested access to controls that are not available: {missing}')
        if not is_dry_run:
            try:
                self._geopm_proxy.PlatformSetGroupAccessControls(group, controls)
            except DBusError as ee:
                raise RuntimeError('Failed to set group control access list, try with sudo or as "root" user (requires CAP_SYS_ADMIN)') from ee

    def get_all_signals(self):
        """Call GEOPM D-Bus API and return all supported signal names

        Returns a human readable list of all signals available on the
        platform.  The returned string has one signal name on each
        line, and is in sorted order.  The PlatformGetAllAccess D-Bus API
        of the io.github.geopm interface is used.

        Returns:
            str: All available signals, one on each line, in sorted order

        """
        all_signals, _ = self._geopm_proxy.PlatformGetAllAccess()
        return '\n'.join(sorted(all_signals))

    def get_all_controls(self):
        """Call GEOPM D-Bus API and return all supported control names

        Returns a human readable list of all controls available on the
        system.  The returned string has one control name on each
        line, and is in sorted order.  The PlatformGetAllAccess D-Bus API
        of the io.github.geopm interface is used.

        Returns:
            str: All available controls, one on each line, in sorted order

        """
        _, all_controls = self._geopm_proxy.PlatformGetAllAccess()
        return '\n'.join(sorted(all_controls))

    def get_user_signals(self):
        """Call GEOPM D-Bus API and return user allowed signal names

        Returns a human readable list of user allowed signal names on
        the platform.  The returned string has one signal name on each
        line, and is in sorted order.  The PlatformGetAllAccess D-Bus API
        of the io.github.geopm interface is used.

        Returns:
            str: User allowed signals, one on each line, in sorted order

        """
        user_signals, _ = self._geopm_proxy.PlatformGetUserAccess()
        return '\n'.join(sorted(user_signals))

    def get_user_controls(self):
        """Call GEOPM D-Bus API and return user allowed control names

        Returns a human readable list of user allowed controls on the
        platform.  The returned string has one control name on each
        line, and is in sorted order.  The PlatformGetAllAccess D-Bus API
        of the io.github.geopm interface is used.

        Returns:
            str: User allowed controls, one on each line, in sorted order

        """
        _, user_controls = self._geopm_proxy.PlatformGetUserAccess()
        return '\n'.join(sorted(user_controls))

    def get_group_signals(self, group):
        """Call GEOPM D-Bus API and return the group's signal access list

        Returns a human readable list of the signals that are enabled.
        The returned string has one signal name on each line, and is in sorted order.

        The default signal access list is returned if the group
        provided is the empty string.  If the group provided is not
        empty then the list of signals that are enabled for the
        specified Unix Group ID is returned.

        A user process accessing the GEOPM D-Bus APIs is restricted to
        the combination of the default access list and the access list
        for all groups that the user belongs to.  The results from
        querying a specific Unix group with this method do not reflect
        the default access list that is enabled for all users.

        The Platforms D-Bus API of the io.github.geopm
        interface is called.

        Args:
            group (str): Unix Group ID to get access list for.  Gets
                         the default signal access list if group
                         provided is '': the empty string.

        Returns:
            str: Access list of signals, one on each line, in sorted order.

        """

        all_signals, _ = self._geopm_proxy.PlatformGetGroupAccess(group)
        return '\n'.join(sorted(all_signals))

    def get_group_controls(self, group):
        """Call GEOPM D-Bus API and return the group's control access list

        Returns a human readable list of the controls that are
        enabled.  The returned string has one control name on each
        line, and is in sorted order.

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
            group (str): Unix Group ID to get access list for.  Gets
                         the default control access list if group
                         provided is '': the empty string.

        Returns:
            str: Access list of controls, one on each line, in sorted order.

        """
        _, all_controls = self._geopm_proxy.PlatformGetGroupAccess(group)
        return '\n'.join(sorted(all_controls))

    def _edited_names_helper(self, initial_names):
        default_editor = '/usr/bin/vi'
        if not os.path.exists(default_editor):
            default_editor = '/bin/vi'
        editor = os.environ.get('EDITOR', default_editor)
        with tempfile.NamedTemporaryFile(mode='w+', suffix='geopm-service-access-tmp') as fid:
            path = fid.name
            fid.write(initial_names)
            fid.flush()
            subprocess.run([editor, path], check=True)
            fid.seek(0)
            names = self.read_names(fid)
        return names

    def edited_names(self, is_control, group):
        if is_control:
            initial_names = self.get_group_controls(group)
        else:
            initial_names = self.get_group_signals(group)
        return self._edited_names_helper(initial_names)

    def read_names(self, fid):
        """Returns list names from user

        Parse list of signals or controls from standard input or edit
        existing list.

        """
        return [ll.strip() for ll in fid.readlines() if ll.strip()]

    def get_msr_safe_allowlist(self):
        """Generate an allowlist for msr-safe

        Generates the minimal msr-safe allowlist required for
        GEOPM to access all the expected signals and controls.

        Returns:
            str: msr-safe allowlist string.

        """
        data_max = 2097152 # 2 MiB
        allowlist_cstr = gffi.gffi.new("char[]", data_max)
        err = gffi.dl_geopmd.geopm_msr_allowlist(data_max, allowlist_cstr)
        if err < 0:
            raise RuntimeError('geopm_allowlist() failed: {}'.format(error.message(err)))
        return gffi.gffi.string(allowlist_cstr).decode()

    def run(self, is_write, is_all, is_control, group, is_default, is_delete,
            is_dry_run, is_force, is_edit, is_log, is_msr_safe):
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
            is_default (bool): True if the default user access list
                               should be printed rather than the calling
                               process' access list.
            is_delete (bool): True to remove an access list file.
            is_dry_run (bool): True to run error checking without file
                               modification
            is_force (bool): True if error checking is disabled
            is_log (bool): True if log user requested a log of requests
                           since service restart
            is_msr_safe (bool): True if user requested msr-safe allowlist

        """
        output = None
        # Determine if user provided -g option
        if group is None:
            is_group = False
            if is_log:
                group = system_files.GEOPM_SERVICE_LOG_REQUEST
            else:
                # Empty string is for default access list
                group = ''
        else:
            is_group = True

        if ((is_write or is_edit or is_delete or is_log) and
            (not is_dry_run and not is_force and not system_files.has_cap_sys_admin(os.getpid()))):
            raise RuntimeError('Modifying configuration files requires CAP_SYS_ADMIN, '
                               'try running with "sudo" or as "root"')
        if (is_all or is_log or is_msr_safe) and (is_write or is_edit or is_delete):
            raise RuntimeError('Option -a/--all or -l/--log or -s/--msr-safe are not valid when '
                               'writing a configuration')
        if is_dry_run and (is_edit or is_delete):
            raise RuntimeError('Option -n/--dry-run not valid with -e/--edit or -D/--delete')
        if is_force and is_edit:
            raise RuntimeError('Option -F/--force is not valid with -e/--edit')
        if is_group and (is_default or is_all or is_log or is_msr_safe):
            raise RuntimeError('Option -g/--group is not valid with -u/--default or -a/--all or -l/--log '
                               ' or -s/--msr-safe')
        if not (is_edit or is_write or is_delete) and (is_dry_run or is_force):
            raise RuntimeError('-n/--dry-run or -F/--force not valid when reading')


        names = None
        if is_edit:
            names = self.edited_names(is_control, group)
        elif is_write:
            names = self.read_names(sys.stdin)
        elif is_delete:
            names = []
        if names is not None:
            # Set access list
            if is_control:
                self.set_group_controls(group, names, is_dry_run, is_force)
            else:
                self.set_group_signals(group, names, is_dry_run, is_force)
        else:
            # Get access list
            if is_all:
                if is_control:
                    output = self.get_all_controls()
                else:
                    output = self.get_all_signals()
            elif is_default or is_group or is_log:
                if is_control:
                    output = self.get_group_controls(group)
                else:
                    output = self.get_group_signals(group)
            elif is_msr_safe:
                output = self.get_msr_safe_allowlist()
            else:
                if is_control:
                    output = self.get_user_controls()
                else:
                    output = self.get_user_signals()
        return output

def _group_id(group):
    """Helper function to convert Unix user group name into an ID.

    When input can be converted into an integer, it will return string
    representation of input, otherwise, the grp module is queried to convert the
    input group name into a group ID.  None is returned when group is None.

    Args:
        group (str): Unix user group name or ID or None

    Returns:
        str: The Unix user group ID in string form

    Raises:
       ValueError: When Unix group name cannot be found

    """
    if group is None:
        return None
    result = str(group)
    try:
        int(group)
    except ValueError:
        try:
            gid = grp.getgrnam(group).gr_gid
        except KeyError:
            raise ValueError(f'Specified a non-integer group, but could not determine Unix Group ID from name: {group}')
        result = str(gid)
    return result

def main():
    """Access management for the GEOPM Service.

    Command line tool for reading and writing the access management
    lists for the GEOPM Service signals and controls.

    """

    err = 0
    parser = ArgumentParser(description=main.__doc__)
    parser.add_argument('-v', '--version', dest='version', action='store_true',
                        help='Print version and exit')
    parser.add_argument('-c', '--controls', dest='controls', action='store_true', default=False,
                        help='Command applies to controls not signals')
    parser_group_ugals = parser.add_mutually_exclusive_group(required=False)
    parser_group_ugals.add_argument('-u', '--default', dest='default', action='store_true', default=False,
                                    help='Print the default user access list')
    parser_group_ugals.add_argument('-g', '--group', dest='group', type=str, default=None,
                                    help='Read or write the access list for a specific Unix GROUP')
    parser_group_ugals.add_argument('-a', '--all', dest='all', action='store_true', default=False,
                                    help='Print all signals or controls supported by the service system')
    parser_group_ugals.add_argument('-l', '--log', action='store_true', default=False,
                                    help='Print list of used signals or controls used since last restart of the service')
    parser_group_ugals.add_argument('-s', '--msr-safe', dest='msr_safe', action='store_true', default=False,
                                    help='Generate an allowlist for msr-safe')
    parser_group_weD = parser.add_mutually_exclusive_group(required=False)
    parser_group_weD.add_argument('-w', '--write', dest='write', action='store_true', default=False,
                                  help='Use standard input to write an access list. Implies -u unless -g is provided.')
    parser_group_weD.add_argument('-e', '--edit', dest='edit', action='store_true', default=False,
                                  help='Edit an access list using EDITOR environment variable, default vi')
    parser_group_weD.add_argument('-D', '--delete', dest='delete', action='store_true', default=False,
                                  help='Remove an access list for default user or a particular Unix Group')
    parser_group_nF = parser.add_mutually_exclusive_group(required=False)
    parser_group_nF.add_argument('-n', '--dry-run', dest='dry_run', action='store_true', default=False,
                                 help='Do error checking on all user input, but do not modify configuration files')
    parser_group_nF.add_argument('-F', '--force', dest='force', action='store_true', default=False,
                                 help='Write access list without validating GEOPM Service support for names')
    parser.add_argument('-x', '--direct', action='store_true', help='Write directly to files, do not use DBus')

    args = parser.parse_args()
    if args.direct and not (args.group or args.default):
        # This option is intended for early admin access list management before
        # the GEOPM service is running. The option does not have a meaningful
        # use case when run for the root user.
        parser.error('Must specify either --group or --default with --direct.')

    try:
        if args.version:
            print(__version_str__)
            return 0
        geopm_proxy = (DirectAccessProxy() if args.direct
                       else SystemMessageBus().get_proxy('io.github.geopm', '/io/github/geopm'))
        acc = Access(geopm_proxy)
        output = acc.run(args.write, args.all, args.controls, _group_id(args.group),
                         args.default, args.delete, args.dry_run, args.force,
                         args.edit, args.log, args.msr_safe)
        if output:
            print(output)
    except Exception as ee:
        if 'GEOPM_DEBUG' in os.environ:
            # Do not handle exception if GEOPM_DEBUG is set
            raise ee
        sys.stderr.write('Error: {}\n\n'.format(ee))
        err = -1
    return err

if __name__ == '__main__':
    sys.exit(main())
