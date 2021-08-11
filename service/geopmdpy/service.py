#!/usr/bin/env python3
#
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

"""Module containing the implementations of the DBus interfaces
exposed by geopmd."""

import os
import pwd
import grp
import json
import shutil
import psutil
import gi
from gi.repository import GLib

from . import pio
from . import topo
from . import dbus_xml
from dasbus.connection import SystemMessageBus
try:
    from dasbus.server.interface import accepts_additional_arguments
except ImportError as ee:
    err_msg = '''dasbus version greater than 1.5 required:
    https://github.com/rhinstaller/dasbus/pull/57'''
    raise ImportError(err_msg) from ee

class PlatformService(object):
    """Provides the concrete implementation for all of the GEOPM DBus
    interfaces that use PlatformIO.  This class is used by the
    GEOPMService class that maps DBus interfaces to their
    implementations.

    """
    def __init__(self):
        """PlatformService constructor that initializes all private members.

        """
        self._pio = pio
        self._CONFIG_PATH = '/etc/geopm-service'
        self._VAR_PATH = '/var/run/geopm-service'
        self._ALL_GROUPS = [gg.gr_name for gg in grp.getgrall()]
        self._DEFAULT_ACCESS = '0.DEFAULT_ACCESS'
        self._SAVE_DIR = 'SAVE_FILES'
        self._WATCH_INTERVAL_MSEC = 1000
        self._write_pid = None
        self._sessions = dict()

    def get_group_access(self, group):
        """Get the signals and controls in the allowed lists.

        Provides the default allowed lists or the allowed lists for a
        particular Unix group.  These lists correspond to values
        stored in the GEOPM service configuration files.  The
        configuration files are read with each call to this method.
        If no files exist that match the query, empty lists are
        returned.  Empty lines and lines that begin with the '#'
        character are ignored.

        Args:
            group (str): Unix group name to query. The default allowed
                lists are returned if group is the empty string.

        Returns:
            list(str), list(str): Signal and control allowed lists

        Raises:
            RuntimeError: The group name is not valid on the system.

        """
        group = self._validate_group(group)
        group_dir = os.path.join(self._CONFIG_PATH, group)
        if os.path.isdir(group_dir):
            path = os.path.join(group_dir, 'allowed_signals')
            signals = self._read_allowed(path)
            path = os.path.join(group_dir, 'allowed_controls')
            controls = self._read_allowed(path)
        else:
            signals = []
            controls = []
        return signals, controls

    def set_group_access(self, group, allowed_signals, allowed_controls):
        """Set the signals and controls in the allowed lists.

        Writes the configuration files that control the default
        allowed lists or the allowed lists for a particular Unix
        group.  These lists restrict user access to signals or
        controls provided by the service.  If the user specifies a
        list that contains signals or controls that are not currently
        supported, the request will raise a RuntimeError without
        modifying any configuration files.  A RuntimeError will also
        be raised if the group name is not valid on the system.

        Args:
            group (str): Which Unix group name to query; if this is
                         the empty string, the default allowed
                         lists are written.

            allowed_signals (list(str)): Signal names that are allowed

            allowed_controls (list(str)): Control names that are allowed

        """
        group = self._validate_group(group)
        self._validate_signals(allowed_signals)
        self._validate_controls(allowed_controls)
        group_dir = os.path.join(self._CONFIG_PATH, group)
        os.makedirs(group_dir, exist_ok=True)
        path = os.path.join(group_dir, 'allowed_signals')
        self._write_allowed(path, allowed_signals)
        path = os.path.join(group_dir, 'allowed_controls')
        self._write_allowed(path, allowed_controls)

    def get_user_access(self, user):
        """Get the list of all of the signals and controls that are
        accessible to the specified user.

        Returns the default access lists that apply to all non-root
        users if the empty string is provided.

        All available signals and controls are returned if the caller
        specifies the user name 'root'.  A RuntimeError is
        raised if the user does not exist on the system.

        When a user requests a signal or control through one of the
        other PlatformService methods, they are restricted to the
        union of the default allowed lists and the allowed lists for
        all Unix groups that the user belongs to.  These combined
        lists are what this method returns.

        Args:
            user (str): Which Unix user name to query; if the empty
                        string is provided, the default allowed list
                        is returned.

        Returns:
            list(str), list(str): Signal and control allowed lists

        """
        if user == 'root':
            return self.get_all_access()
        user_groups = self._get_user_groups(user)
        user_groups.append('') # Default access list
        signal_set = set()
        control_set = set()
        for group in user_groups:
            signals, controls = self.get_group_access(group)
            signal_set.update(signals)
            control_set.update(controls)
        signals = sorted(signal_set)
        controls = sorted(control_set)
        return signals, controls

    def get_all_access(self):
        """Get all of the signals and controls that the service supports.

        Returns the list of all signals and controls supported by the
        service.  The lists returned are independent of the access
        controls; therefore, calling get_all_access() is equivalent
        to calling get_user_access('root').

        Returns:
            list(str), list(str): All supported signals and controls

        """
        return self._pio.signal_names(), self._pio.control_names()

    def get_signal_info(self, signal_names):
        """For each specified signal name, return a tuple of information.

        The caller provides a list of signal names.  This method returns
        a list of the same length which consists of a tuple of information
        for each signal.

        The method will raise a RuntimeError if any of the requested
        signal names are not supported by the service.

        Args:
            signal_names (list(str)): List of signal names to query

        Returns:
            list(tuple((str), (str), (int), (int), (int), (int))):
                name (str): The signal name specified by the caller

                description (str): A human-readable description of
                                   what the signal measures.

                domain_type (int): One of the geopmpy.topo.DOMAIN_*
                                   integers corresponding to a domain
                                   type where the signal is natively
                                   supported.

                aggregation (int): One of the geopm::Agg::m_type_e
                                   enum values defined in Agg.hpp that
                                   specifies how to aggregate the
                                   signal over domains.

                format_type (int): One of the geopm::string_format_e
                                   enum values defined in Helper.hpp
                                   that specifies how to convert the
                                   signal value to a human readable
                                   string.

                behavior (int): One of the
                                IOGroup::m_signal_behavior_e enum
                                values that specifies how the signal
                                changes over time.

        """
        result = []
        for name in signal_names:
            description = self._pio.signal_description(name)
            domain_type = self._pio.signal_domain_type(name)
            aggregation, format_type, behavior = self._pio.signal_info(name)
            result.append((name, description, domain_type, aggregation, format_type, behavior))
        return result

    def get_control_info(self, control_names):
        """For each specified control name, return a tuple of information.

        The caller provides a list of control names.  This method returns
        a list of the same length which consists of a tuple of information
        for each control.

        The method will raise a RuntimeError if any of the requested
        control names are not supported by the service.

        Args:
            control_names (list(str)): List of control names to query

        Returns:
            list(tuple((str), (str), (int))):
                name (str): The control name specified by the caller

                description (str): A human-readable description of the
                                   effect of setting the control.

                domain_type (int): One of the geopmpy.topo.DOMAIN_*
                                   integers corresponding to a domain
                                   type that is the finest available
                                   granularity for the control.

        """
        result = []
        for name in control_names:
            description = self._pio.control_description(name)
            domain_type = self._pio.control_domain_type(name)
            result.append((name, description, domain_type))
        return result

    def lock_control(self):
        """Block all write-mode sessions.

        A call to this method will end any currently running
        write-mode session and block the creation of any new
        write-mode sessions.  Use the unlock_control() method to
        enable write-mode sessions to begin again.

        Calls to this method through the DBus interface will be
        blocked for all non-root users due to the rules established by
        the "io.github.geopm.conf" DBus configuration file.

        Until write-mode sessions are re-enabled, any calls to
        write_control() or calls to start_batch() with a non-empty
        control_config parameter will raise a RuntimeError.

        The default system configuration may be updated by the system
        administrator while write-mode is locked.  For instance, the
        system administrator may use the geopmwrite command-line tool
        as user root between calls to lock_control() and
        unlock_control().  These changes will be reflected by the
        save/restore executed by the service for the next client
        write-mode session after the controls are unlocked.

        No action is taken and no error is raised if controls are already
        disabled when this method is called.

        """
        raise NotImplementedError('PlatformService: Implementation incomplete')

    def unlock_control(self):
        """Unblock access to create new write-mode sessions.

        A call to this method will re-enable write-mode sessions to be
        created after they were previously disabled by a call to
        lock_control().

        Although new sessions may be created after a call to
        unlock_control(), write-mode sessions that were ended due to
        previous calls to lock_control() will remain closed.

        No action is taken and no error is raised if controls are already
        enabled when this method is called.

        """
        raise NotImplementedError('PlatformService: Implementation incomplete')

    def open_session(self, user, client_pid):
        """Open a new session session for the client process.

        The creation of a session is the first step that a client
        thread makes to interact with the GEOPM service.  Each Linux
        thread has a unique PID, and this PID can be associated with
        at most one GEOPM service session.  The session is ended by
        passing the client PID to the close_session() method.  The
        session will also be ended if the client thread terminates for
        any reason.

        No action is taken and no error is raised if there is an existing
        session associated with the client PID.

        All sessions are opened in read-mode, and may later be
        promoted to write-mode.  This promotion will occur with the
        first successful call requiring access to controls.  This
        includes any calls to write_control() or calls to
        start_batch() with a non-empty control_config.  These calls
        will fail if there is an active write-mode session opened by
        another thread.

        Prior to being promoted to write-mode, the current value for
        all controls that are supported by the GEOPM service will be
        recorded.  This record enables the service to restore all
        controls when the write-mode session ends.

        Permissions for the session's access to signals and controls
        are determined based on the policy for the user when
        the session is first opened.  Calling the set_group_access()
        method to alter the policy will not affect active sessions.

        Args:
            user (str): Unix user name that owns the client thread
                        opening the session.

            client_pid (int): Linux PID of the client thread opening
                              the session.

        """
        signals, controls = self.get_user_access(user)
        os.makedirs(self._VAR_PATH, exist_ok=True)
        session_file = self._get_session_file(client_pid)
        if os.path.isfile(session_file):
            raise RuntimeError('Session file for connecting process already exists: {}'.format(session_file))
        watch_id = self._watch_client(client_pid)
        session_data = {'client_pid': client_pid,
                        'mode': 'r',
                        'signals': signals,
                        'controls': controls,
                        'watch_id': watch_id}
        self._sessions[client_pid] = session_data
        with open(session_file, 'w') as fid:
            json.dump(session_data, fid)

    def close_session(self, client_pid):
        """Close an active session for the client process.

        After closing a session, the client process is required to
        call open_session() again before using any of the client-facing
        member functions.

        Closing an active session will remove the record of which
        signals and controls the client process has access to; thus,
        this record is updated to reflect changes to the policy when the
        next session is opened by the process.

        When closing a write-mode session, the control values that were
        recorded when the session was promoted to write-mode are restored.

        This method supports both the client and administrative DBus
        interfaces that are used to close a session.  The client DBus
        interface only allows users to close sessions that were created
        with their user ID.

        A RuntimeError is raised if the client_pid does not have an
        open session.

        Args:
            client_pid (int): Linux PID of the client thread

        """
        session = self._get_session(client_pid, 'PlatformCloseSession')
        if client_pid == self._write_pid:
            save_dir = os.path.join(self._VAR_PATH, self._SAVE_DIR)
            self._pio.restore_control()
            shutil.rmtree(save_dir)
            self._write_pid = None
        GLib.source_remove(session['watch_id'])
        self._sessions.pop(client_pid)
        session_file = self._get_session_file(client_pid)
        os.remove(session_file)

    def start_batch(self, client_pid, signal_config, control_config):
        """Start a batch server to support a client session.

        Configure the signals and controls that will be enabled by the
        batch server and start the server process.  The server enables
        fast access for the signals and controls that are configured
        by the caller.  These are configured by specifying a name,
        domain and domain index for each of the signals and controls
        that the server will support.

        After a batch server is successfully created, the client will
        interact with the batch server though PlatformIO interfaces
        that do not go over DBus.  That is, once access is established
        by DBus, a faster protocol can be safely used.

        The batch server does not enable features beyond those of
        the read_signal() or write_control() methods; it simply
        provides a much higher performance interface.

        A RuntimeError is raised if the client does not have
        permission to read or write any of configured signals or
        controls or if the client_pid does not have an open session.
        The server process will not be created if any error occurs.

        Args:
            client_pid (int): Linux PID of the client thread

            signal_config (list(tuple((int), (int), (str))):
                domain_type (int): One of the geopmpy.topo.DOMAIN_*
                                   integers corresponding to a domain
                                   type to read from.

                domain_idx (int): Specifies the particular domain
                                  index to read from.

                signal_name (str): The name of the signal to read.

            control_config (list(tuple((int), (int), (str))):
                domain_type (int): One of the geopmpy.topo.DOMAIN_*
                                   integers corresponding to a domain
                                   type to write to.

                domain_idx (int): Specifies the particular domain
                                  index to write to.

                control_name (str): The name of the control to write.

        Returns:
            tuple(int, str):
                server_pid (int): The Linux PID of the batch server process.

                server_key (str): A unique identifier enabling the
                                  server/client connection across
                                  inter-process shared memory.

        """
        session = self._get_session(client_pid, 'PlatformStartBatch')
        sig_req = {cc[2] for cc in signal_config}
        cont_req = {cc[2] for cc in control_config}
        if not sig_req.issubset(session['signals']):
            raise RuntimeError('Requested signals that are not in allowed list')
        elif not cont_req.issubset(session['controls']):
            raise RuntimeError('Requested controls that are not in allowed list')
        if len(control_config) != 0:
            self._write_mode(client_pid)
        return self._pio.start_batch_server(client_pid, signal_config, control_config)

    def stop_batch(self, client_pid, server_pid):
        """End a batch server previously started by the client.

        Terminate the batch server process and free up all resources
        associated with the batch server.  Any future calls by the
        client to interfaces that require the batch server will result
        in errors.

        The batch server will also be terminated if the client session
        that created the server terminates for any reason.

        A RuntimeError will be raised if the specified server PID was
        not previously created by the client's call to start_batch(),
        or if the batch server has already been closed for any reason.

        Args:
            client_pid (int): Linux PID of the client thread.

            server_pid (int): Linux PID of a batch server process
                              returned by a previous call to
                              start_batch().

        """
        self._get_session(client_pid, 'StopBatch')
        self._pio.stop_batch_server(server_pid)

    def read_signal(self, client_pid, signal_name, domain, domain_idx):
        """Read a signal from a particular domain.

        Select a signal by name and read the current value from the
        domain type and index specified.  The value is returned
        as a floating point number in SI units.

        A RuntimeError is raised if the client_pid does not have an
        open session or if the client does not have permission to read the
        signal from the specified domain.

        A RuntimeError is raised if the requested signal is not
        supported, the domain is invalid or the domain index is out of
        range.

        Args:
            client_pid (int): Linux PID of the client thread.

            signal_name (str): Name of the signal to read.

            domain (int): One of the geopmpy.topo.DOMAIN_* integers
                          corresponding to a domain type to read from.

            domain_idx (int): Specifies the particular domain index to
                              read from.

        Returns:
            (float): The value of the signal in SI units.

        """
        session = self._get_session(client_pid, 'PlatformReadSignal')
        if not signal_name in session['signals']:
            raise RuntimeError('Requested signal that is not in allowed list')
        return self._pio.read_signal(signal_name, domain, domain_idx)

    def write_control(self, client_pid, control_name, domain, domain_idx, setting):
        """Write a control value to a particular domain.

        Select a control by name and write a new setting for the
        domain type and index specified.  The written control setting
        is a floating point number in SI units.

        A RuntimeError is raised if the client_pid does not have an
        open session, or if the client does not have permission to write the
        control to the specified domain.

        A RuntimeError is raised if a different client currently has an
        open write-mode session.

        A RuntimeError is raised if the requested control is not
        supported, the domain is invalid or the domain index is out of
        range.

        Args:
            client_pid (int): Linux PID of the client thread.

            control_name (str): The name of the control to write.

            domain (int): One of the geopmpy.topo.DOMAIN_* integers
                          corresponding to a domain type to written
                          to.

            domain_idx (int): Specifies the particular domain index to
                              write to.

            setting (float): Value of the control to be written.

        """
        session = self._get_session(client_pid, 'PlatformWriteControl')
        if not control_name in session['controls']:
            raise RuntimeError('Requested control that is not in allowed list')
        self._write_mode(client_pid)
        self._pio.write_control(control_name, domain, domain_idx, setting)

    def _read_allowed(self, path):
        try:
            with open(path) as fid:
                result = [line.strip() for line in fid.readlines()
                          if line.strip() and not line.strip().startswith('#')]
        except FileNotFoundError:
            result = []
        return result

    def _write_allowed(self, path, allowed):
        allowed.append('')
        with open(path, 'w') as fid:
            fid.write('\n'.join(allowed))

    def _validate_group(self, group):
        if group is None or group == '':
            group = self._DEFAULT_ACCESS
        else:
            group = str(group)
            if group[0].isdigit():
                raise RuntimeError('Linux group name cannot begin with a digit: group = "{}"'.format(group))
            if group not in self._ALL_GROUPS:
                raise RuntimeError('Linux group is not defined: group = "{}"'.format(group))
        return group

    def _validate_signals(self, signals):
        signals = set(signals)
        all_signals = self._pio.signal_names()
        if not signals.issubset(all_signals):
            unmatched = signals.difference(all_signals)
            err_msg = 'The service does not support any signals that match: "{}"'.format('", "'.join(unmatched))
            raise RuntimeError(err_msg)

    def _validate_controls(self, controls):
        controls = set(controls)
        all_controls = self._pio.control_names()
        if not controls.issubset(all_controls):
            unmatched = controls.difference(all_controls)
            err_msg = 'The service does not support any controls that match: "{}"'.format('", "'.join(unmatched))
            raise RuntimeError(err_msg)

    def _get_user_groups(self, user):
        user_gid = pwd.getpwnam(user).pw_gid
        all_gid = os.getgrouplist(user, user_gid)
        return [grp.getgrgid(gid).gr_name for gid in all_gid]

    def _get_session(self, client_pid, operation):
        try:
            session = self._sessions[client_pid]
        except KeyError:
            raise RuntimeError('Operation {} not allowed without an open session'.format(operation))
        return session

    def _get_session_file(self, client_pid):
        return os.path.join(self._VAR_PATH, 'session-{}.json'.format(client_pid))

    def _write_mode(self, client_pid):
        if self._sessions[client_pid]['mode'] != 'rw':
            if self._write_pid is not None:
                raise RuntimeError('The geopm service already has a connected "rw" mode client')
            session_file = self._get_session_file(client_pid)
            self._sessions[client_pid]['mode'] = 'rw'
            with open(session_file, 'w') as fid:
                json.dump(self._sessions[client_pid], fid)
            self._write_pid = client_pid
            save_dir = os.path.join(self._VAR_PATH, self._SAVE_DIR)
            os.makedirs(save_dir)
            # TODO: Will need to save to disk in order to support
            # daemon restart
            self._pio.save_control()

    def _watch_client(self, client_pid):
        return GLib.timeout_add(self._WATCH_INTERVAL_MSEC, self._check_client, client_pid)

    def _check_client(self, client_pid):
        if client_pid in self._sessions and not psutil.pid_exists(client_pid):
            self.close_session(client_pid)
            return False
        return True

class TopoService(object):
    """Provides the concrete implementation for all of the GEOPM DBus
    interfaces that use PlatformTopo.  This class is used by the
    GEOPMService class that maps DBus interfaces to their
    implementations.

    """
    def __init__(self, topo=topo):
        """TopoService constructor that initializes all private members.

        """
        self._topo = topo

    def get_cache(self):
        """Return the contents of the PlatformTopo cache file.

        Create the PlatformTopo cache file if it does not exist and
        then return the contents of the file as a string.  This
        provides all the information required to associate all domains
        with CPUs.

        Returns:
            (str): Contents of the topology cache file that defines
                   the system topology.

        """
        self._topo.create_cache()
        with open('/tmp/geopm-topo-cache') as fid:
            result = fid.read()
        return result


class GEOPMService(object):
    """The dasbus service object that is published.

    Object used by dasbus to map GEOPM service DBus APIs to their
    implementation.  A GEOPMService object is published by geopmd
    using the publish_object() method of dasbus.SystemMessageBus
    class which enables the GEOPM service.

    The GEOPMService methods are named after the DBus API they provide
    within the io.github.geopm DBus namespace.  The parameter type
    conversion is configured with the __dbus_xml__ class member.

    The implementation for each method is a pass-through call to a
    member object method.  These member objects do not have an
    explicit dasbus dependency which facilitates unit testing.

    """
    __dbus_xml__ = dbus_xml.geopm_dbus_xml(TopoService, PlatformService)

    def __init__(self, topo=TopoService(),
                 platform=PlatformService()):
        self._topo = topo
        self._platform = platform
        self._write_pid = None
        self._dbus_proxy = SystemMessageBus().get_proxy('org.freedesktop.DBus',
                                                        '/org/freedesktop/DBus')

    def TopoGetCache(self):
        return self._topo.get_cache()

    def PlatformGetGroupAccess(self, group):
        return self._platform.get_group_access(group)

    def PlatformSetGroupAccess(self, group, allowed_signals, allowed_controls):
        self._platform.set_group_access(group, allowed_signals, allowed_controls)

    @accepts_additional_arguments
    def PlatformGetUserAccess(self, **call_info):
        return self._platform.get_user_access(self._get_user(**call_info))

    def PlatformGetAllAccess(self):
        return self._platform.get_all_access()

    def PlatformGetSignalInfo(self, signal_names):
        return self._platform.get_signal_info(signal_names)

    def PlatformGetControlInfo(self, control_names):
        return self._platform.get_control_info(control_names)

    def PlatformLockControl(self):
        self._platform.lock_control()

    def PlatformUnlockControl(self):
        self._platform.unlock_control()

    @accepts_additional_arguments
    def PlatformOpenSession(self, **call_info):
        self._platform.open_session(self._get_user(**call_info), self._get_pid(**call_info))

    @accepts_additional_arguments
    def PlatformCloseSession(self, **call_info):
        self._platform.close_session(self._get_pid(**call_info))

    def PlatformCloseSessionAdmin(self, client_pid):
        self._platform.close_session(client_pid)

    @accepts_additional_arguments
    def PlatformStartBatch(self, signal_config, control_config, **call_info):
        return self._platform.start_batch(self._get_pid(**call_info), signal_config, control_config)

    @accepts_additional_arguments
    def PlatformStopBatch(self, server_pid, **call_info):
        self._platform.stop_batch(self._get_pid(**call_info), server_pid)

    @accepts_additional_arguments
    def PlatformReadSignal(self, signal_name, domain, domain_idx, **call_info):
        return self._platform.read_signal(self._get_pid(**call_info), signal_name, domain, domain_idx)

    @accepts_additional_arguments
    def PlatformWriteControl(self, control_name, domain, domain_idx, setting, **call_info):
        self._platform.write_control(self._get_pid(**call_info), control_name, domain, domain_idx, setting)

    def _get_user(self, call_info):
        """Use DBus proxy object to derive the user name that owns the client
           process.

        Args:
           call_info (dict): The dictionary provided by the dasbus
                             @accepts_additional_arguments decorator.

        Returns:
            (str): The Unix user name of the client process owner.

        """
        unique_name = call_info['sender']
        uid = self._dbus_proxy.GetConnectionUnixUser(unique_name)
        return pwd.getpwuid(uid).pw_name

    def _get_pid(self, call_info):
        """Use DBus proxy object to derive the Linux PID of the client
           thread.

        Args:
           call_info (dict): The dictionary provided by the dasbus
                             @accepts_additional_arguments decorator.

        Returns:
            (int): The Linux PID of the client thread.

        """
        unique_name = call_info['sender']
        return self._dbus_proxy.GetConnectionUnixProcessID(unique_name)
