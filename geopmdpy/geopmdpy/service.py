#
#  Copyright (c) 2015 - 2025 Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#

"""Module containing the implementations of the DBus interfaces
exposed by geopmd."""

import os
import sys
import signal
import pwd
import shutil
import psutil
import uuid
from gi.repository import GLib
import math
import subprocess # nosec

from . import pio
from . import topo
from . import dbus_xml
from . import system_files
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
        self._RUN_PATH = system_files.GEOPM_SERVICE_RUN_PATH
        self._SAVE_DIR = 'SAVE_FILES'
        self._PROFILER_LOCK_NAME = 'PROFILER_LOCK'
        self._WATCH_INTERVAL_SEC = 1
        self._active_sessions = system_files.ActiveSessions(self._RUN_PATH)
        self._access_lists = system_files.AccessLists(system_files.get_config_path())
        self._accessed_signals = set()
        self._accessed_controls = set()
        self._batch_subp = dict()
        self._write_pid = None
        for client_pid in self._active_sessions.get_clients():
            is_active = self.check_client(client_pid)
            if is_active:
                watch_id = self._watch_client(client_pid)
                self._active_sessions.set_watch_id(client_pid, watch_id)
        with system_files.WriteLock(self._RUN_PATH) as lock:
            write_pid = lock.try_lock()
            if write_pid is not None:
                if not self._is_client_active(write_pid):
                    self._close_session_write(lock, write_pid)
                else:
                    self._write_pid = write_pid
        with system_files.WriteLock(self._RUN_PATH, self._PROFILER_LOCK_NAME) as lock:
            profiler_pid = lock.try_lock()
            if profiler_pid is not None and not self._is_client_active(profiler_pid):
                lock.unlock(profiler_pid)

    def get_group_access(self, group, client_pid):
        """Get the signal and control access lists for a group.

        Args:
            group (str): Name of the group.
            client_pid (int): Linux PID of the client thread.

        Returns:
            tuple: Signal and control allowed lists, both in sorted order.

        Raises:
            RuntimeError: If the client lacks necessary permissions.
        """
        if group == system_files.GEOPM_SERVICE_LOG_REQUEST:
            if not system_files.has_cap_sys_admin(client_pid):
                raise RuntimeError('Reading the GEOPM Access Service log requires CAP_SYS_ADMIN, try with "sudo" or run as "root".')
            result = (sorted(self._accessed_signals), sorted(self._accessed_controls))
        else:
            result = self._access_lists.get_group_access(group)
        return result

    def set_group_access(self, group, allowed_signals, allowed_controls, client_pid):
        """Set the signal and control access lists for a group.

        Args:
            group (str): Name of the group.
            allowed_signals (list): List of allowed signal names.
            allowed_controls (list): List of allowed control names.
            client_pid (int): Linux PID of the client thread.

        Raises:
            RuntimeError: If the client lacks necessary permissions.
        """
        if not system_files.has_cap_sys_admin(client_pid):
            raise RuntimeError('Setting access lists requires CAP_SYS_ADMIN, try with "sudo" or run as "root".')
        self._access_lists.set_group_access(group, allowed_signals, allowed_controls)

    def set_group_access_signals(self, group, allowed_signals, client_pid):
        """Set the signal access list for a group.

        Args:
            group (str): Name of the group.
            allowed_signals (list): List of allowed signal names.
            client_pid (int): Linux PID of the client thread.

        Raises:
            RuntimeError: If the client lacks necessary permissions.
        """
        if not system_files.has_cap_sys_admin(client_pid):
            raise RuntimeError('Setting access lists requires CAP_SYS_ADMIN, try with "sudo" or run as "root".')
        self._access_lists.set_group_access_signals(group, allowed_signals)

    def set_group_access_controls(self, group, allowed_controls, client_pid):
        """Set the control access list for a group.

        Args:
            group (str): Name of the group.
            allowed_controls (list): List of allowed control names.
            client_pid (int): Linux PID of the client thread.

        Raises:
            RuntimeError: If the client lacks necessary permissions.
        """
        if not system_files.has_cap_sys_admin(client_pid):
            raise RuntimeError('Setting access lists requires CAP_SYS_ADMIN, try with "sudo" or run as "root".')
        self._access_lists.set_group_access_controls(group, allowed_controls)

    def get_user_access(self, user, client_pid):
        """Get the list of all of the signals and controls that are
        accessible to the specified user.

        Returns the default access lists that apply to all non-root
        users if the empty string is provided.

        All available signals and controls are returned if the caller
        specifies the a pid with CAP_SYSADMIN.  A RuntimeError is
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

            client_pid (int): Linux PID of the client thread opening
                              the session.

        Returns:
            list(str), list(str): Signal and control allowed lists, both in sorted order.

        Raises:
            RuntimeError: The user does not exist.

        """
        return self._access_lists.get_user_access(user, client_pid)

    def get_all_access(self):
        """Get all of the signals and controls that the service supports.

        Returns the list of all signals and controls supported by the
        service.  The lists returned are independent of the access
        controls; therefore, calling get_all_access() is equivalent
        to calling get_user_access() for a user with CAP_SYSADMIN.

        Returns:
            list(str), list(str): All supported signals and controls, in sorted order.

        """
        return self._access_lists.get_all_access()

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

    def lock_control(self, client_pid):
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

    def unlock_control(self, client_pid):
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

    def _is_client_active(self, client_pid):
        result = False
        try:
            result = self._active_sessions.is_client_active(client_pid)
        except (system_files.InvalidClientError, psutil.NoSuchProcess) as ex:
            sys.stderr.write(f'Warning: <geopm-service>: {ex}\n')
            self._close_session_completely(client_pid)
        return result

    def _check_client_active(self, client_pid, func_name):
        """Check if client is active and valid

        Print a warning to log an remove any resources associated with
        the session if the client is not active.

        Args:
            client_pid (int): Linux PID to query

            func_name (str): The operation that was attempted which requires
                             an active session.  Used in error message

        """
        result = self._is_client_active(client_pid)
        if not result:
            sys.stderr.write(f"Warning: <geopm-service>: Operation '{func_name}' not allowed without an open session. Client PID: {client_pid}\n")
            self._close_session_completely(client_pid)
        return result

    def open_session(self, user, client_pid):
        """Open a new session for the client thread.

        The creation of a session is the first step that a client
        thread makes to interact with the GEOPM service.  Each Linux
        thread has a unique PID, and this PID can be associated with
        at most one GEOPM service session.  The session is ended by
        passing the client PID to the close_session() method.  The
        session will also be ended if the client thread terminates for
        any reason.

        The client PID may attempt to open a new session multiple
        times.  In this case a reference count records the number of
        calls to open the session by the client PID.  Each call to
        open a session is expected to be paired with a call to close
        the session.  When the reference count falls to zero, all
        resources associated with the session are released.

        Incrementing the reference count is an abstraction for the
        client.  The client itself cannot have more than a single open
        connection at once, so when requesting a new session, other
        than incrementing the counter, nothing else happens.

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
        if self._is_client_active(client_pid):
            self._active_sessions.increment_reference_count(client_pid)
        else:
            signals, controls = self.get_user_access(user, client_pid)
            watch_id = self._watch_client(client_pid)
            self._active_sessions.add_client(client_pid, signals, controls, watch_id)

    def close_session(self, client_pid, request_pid):
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

        A client PID may attempt to open a session multiple times and
        this is tracked with a reference count as described in the
        documentation for opening a session.  In this case, calls to
        close the session will not actually release the resources
        associated with the session until the reference count falls to
        zero.

        A RuntimeError is raised if the client_pid does not have an
        open session.

        Args:
            client_pid (int): Linux PID of the client thread

        """
        if client_pid != request_pid and not system_files.has_cap_sys_admin(request_pid):
            raise RuntimeError('Closing session of another PID requires CAP_SYS_ADMIN, try with "sudo" or run with "root"')
        if not self._check_client_active(client_pid, 'PlatformCloseSession'):
            return
        reference_count = self._active_sessions.get_reference_count(client_pid)
        if reference_count == 0:
            # The daemon died, restarted, and found a session with
            # zero reference count.
            self._close_session_completely(client_pid)
        elif reference_count == 1:
            self._active_sessions.decrement_reference_count(client_pid)
            self._close_session_completely(client_pid)
        else:  # reference_count > 1:
            self._active_sessions.decrement_reference_count(client_pid)

    def close_session_admin(self, client_pid, request_pid):
        """Close an active session for the client process completely.

        This administrative function is used to forcibly close an
        active session regardless of the client's reference count.
        Similarly, if a client process PID is no longer active, but an
        open session exists, this implementation is used to release
        the session resources.

        When closing a write-mode session, the control values that were
        recorded when the session was promoted to write-mode are restored.
        A RuntimeError is raised if the client_pid does not have an
        open session.

        Args:
            client_pid (int): Linux PID of the client thread

        """
        if client_pid != request_pid and not system_files.has_cap_sys_admin(request_pid):
            raise RuntimeError('Closing session of another PID requires CAP_SYS_ADMIN, try with "sudo" or run with "root"')
        self._close_session_completely(client_pid)

    def close_all_sessions(self):
        """Close all active sessions.

        This method should be called when terminating the service but not when
        restarting.

        """
        all_clients = self._active_sessions.get_clients()
        write_idx = all_clients.index(self._write_pid) if self._write_pid in all_clients else None
        if write_idx is not None:
            # Shutdown the write session first to restore controls
            all_clients.insert(0, all_clients.pop(write_idx))
        for client_pid in all_clients:
            try:
                self.close_session_admin(client_pid, client_pid)
            except Exception as ex:
                sys.stderr.write(f'Warning: <geopm-service>: When closing session for client PID {client_pid}: {ex}\n')
                pass
        if all_clients:
            sys.stderr.write(f'Info: <geopm-service>: Closed all sessions for client PIDs: {all_clients}\n')

    def _close_session_completely(self, client_pid):
        """Close an active session for the client process completely.

        This method is called internally by close_session() and it is
        also called explicitly by check_client() when the process dies.

        A RuntimeError is raised if the client_pid does not have an
        open session.

        Args:
            client_pid (int): Linux PID of the client thread

        """
        if client_pid in self._active_sessions.get_clients():
            GLib.source_remove(self._active_sessions.get_watch_id(client_pid))
            batch_pid = self._active_sessions.get_batch_server(client_pid)
            if batch_pid is not None:
                ex = self._stop_batch_server(batch_pid)
                if ex is not None:
                    sys.stderr.write(f"Warning: <geopm-service> Failed to call pio.stop_batch_server({batch_pid}): {type(ex)} {ex}: sending SIGKILL\n")
                    if psutil.pid_exists(batch_pid):
                        os.kill(batch_pid, signal.SIGKILL)
        try:
            client_pid = int(client_pid)
        except ValueError:
            sys.stderr.write(f"Warning: <geopm-service>: StopBatchServer called with non-integer client_pid: '{client_pid}'\n")
            pass
        else:
            self._active_sessions.remove_client(client_pid)
            with system_files.WriteLock(self._RUN_PATH) as lock:
                if lock.try_lock() == client_pid:
                    self._close_session_write(lock, client_pid)
            with system_files.WriteLock(self._RUN_PATH, self._PROFILER_LOCK_NAME) as lock:
                if lock.try_lock() == client_pid:
                    lock.unlock(client_pid)

    def _close_session_write(self, lock, pid):
        save_dir = os.path.join(self._RUN_PATH, self._SAVE_DIR)
        if os.path.isdir(save_dir):
            is_restored = False
            try:
                self._pio.restore_control_dir(save_dir)
                is_restored = True
            except RuntimeError as ex:
                sys.stderr.write(f'Warning: <geopm-service>: Failed to restore control settings for client {pid}: {ex}\n')
            del_dir = f'{save_dir}-{pid}-{uuid.uuid4()}-del'
            os.rename(save_dir, del_dir)
            if is_restored:
                shutil.rmtree(del_dir)
                self._write_pid = None
            else:
                sys.stderr.write(f'Warning: <geopm-service>: Failed to restore controls for PID {pid}, moved to {del_dir}\n')
        else:
            sys.stderr.write(f'Warning: <geopm-service>: Failed to restore controls for PID {pid}, {save_dir} is not a directory\n')
        lock.unlock(pid)

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
        if not self._check_client_active(client_pid, 'PlatformStartBatch'):
            return (-1, '')
        sig_req = {cc[2] for cc in signal_config}
        cont_req = {cc[2] for cc in control_config}
        supported_signals = self._active_sessions.get_signals(client_pid)
        supported_controls = self._active_sessions.get_controls(client_pid)
        if not sig_req.issubset(supported_signals):
            raise RuntimeError('Requested signals that are not in allowed list: {}' \
                               .format(sorted(sig_req.difference(supported_signals))))
        elif not cont_req.issubset(supported_controls):
            raise RuntimeError('Requested controls that are not in allowed list: {}' \
                               .format(sorted(cont_req.difference(supported_controls))))
        if len(control_config) != 0:
            self._write_mode(client_pid)
        batch_pid = self._active_sessions.get_batch_server(client_pid)
        if batch_pid is not None:
            raise RuntimeError(f'Client {client_pid} has already started a batch server: {batch_pid}')
        subp = subprocess.Popen(["geopmbatch", str(client_pid)],
                                stdin=subprocess.PIPE, stdout=subprocess.PIPE,
                                universal_newlines=True, bufsize=0)
        batch_pid = subp.pid
        self._batch_subp[batch_pid] = subp
        for sr in signal_config:
            subp.stdin.write(f'read {sr[2]} {sr[0]} {sr[1]}\n')
        for cr in control_config:
            subp.stdin.write(f'write {cr[2]} {cr[0]} {cr[1]}\n')
        subp.stdin.write('\n')
        subp.stdin.flush()
        batch_key = subp.stdout.readline().strip()
        self._active_sessions.set_batch_server(client_pid, batch_pid)
        for (_, _, sn) in signal_config:
            self._accessed_signals.add(sn)
        for (_, _, cn) in control_config:
            self._accessed_controls.add(cn)
        return batch_pid, batch_key

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
        if not self._check_client_active(client_pid, 'StopBatch'):
            return
        actual_server_pid = self._active_sessions.get_batch_server(client_pid)
        if server_pid != actual_server_pid:
            sys.stderr.write(f'Warning: <geopm-service>: Client PID: {client_pid} requested to stop batch server PID: {server_pid}, actual batch server PID: {actual_server_pid}\n')
        else:
            ex = self._stop_batch_server(server_pid)
            if ex is not None:
                sys.stderr.write(f'Warning: <geopm-service>: Client PID: {client_pid} requested to stop batch server PID: {server_pid}: {ex}\n')
        self._active_sessions.remove_batch_server(client_pid)

    def _stop_batch_server(self, server_pid):
        result = None
        try:
            self._pio.stop_batch_server(server_pid)
        except RuntimeError as ex:
            result = ex
        try:
            subp = self._batch_subp.pop(server_pid)
        except KeyError as ex:
            result = ex
        else:
            _, stderr_data = subp.communicate()
            if subp.returncode != 0:
                sys.stderr.write(f'Warning: <geopm-service>: Batch server returned non-zero exit code: "{subp.returncode}" stderr: "{stderr_data}".\n')
        return result

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
        result = math.nan
        if not self._check_client_active(client_pid, 'PlatformReadSignal'):
            return result
        signal_avail = self._active_sessions.get_signals(client_pid)
        if not signal_name in signal_avail:
            sys.stderr.write(f"Warning: <geopm-service>: Requested signal that is not in allowed list: '{signal_name}'\n")
        else:
            result = self._pio.read_signal(signal_name, domain, domain_idx)
            self._accessed_signals.add(signal_name)
        return result

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


        if not self._check_client_active(client_pid, 'PlatformWriteControl'):
            return
        control_avail = self._active_sessions.get_controls(client_pid)
        if not control_name in control_avail:
            raise RuntimeError('Requested control that is not in allowed list: {}'.format(control_name))
        self._write_mode(client_pid)
        self._pio.write_control(control_name, domain, domain_idx, setting)
        self._accessed_controls.add(control_name)

    def restore_control(self, client_pid):
        """Restore all controls recorded at the start of a session.

        For the session associated to the given process, restore the state
        of all controls, which is recorded at the beginning of the session.

        Raises:
            RuntimeError: If client_pid does not have an open session or
                          if a different client currently has an open
                          session in write mode
        """
        if not self._check_client_active(client_pid, 'PlatformRestoreControl'):
            return
        self._write_mode(client_pid)
        save_dir = os.path.join(self._RUN_PATH, self._SAVE_DIR)
        self._pio.restore_control_dir(save_dir)

    def start_profile(self, user, client_pid, profile_name):
        """Begin profiling a user PID

        Called by a thread to enable profiling as part of a named
        application.

        Args:
            user (str): Unix user name that owns the client thread
                        opening the session.

            client_pid (int): Linux PID of the client thread opening
                              the session.

            profile_name (str): Name of application that PID supports

        """
        self.open_session(user, client_pid)
        self._active_sessions.start_profile(client_pid, profile_name)

    def stop_profile(self, client_pid, region_names):
        """Stop profiling a user PID

        Called by a thread to end profiling as part of a named
        application.

        Args:
            client_pid (int): Linux PID of the client thread opening
                              the session.

            region_names (list(str)): Names of all regions entered.

        """
        if not self._check_client_active(client_pid, 'PlatformStopProfile'):
            return
        self._active_sessions.stop_profile(client_pid, region_names)
        self.close_session(client_pid, client_pid)

    def get_profile_pids(self, client_pid, profile_name):
        """Get PIDs associated with an application

        Called by a profiling thread to find all PIDs associated with
        a named application.

        Args:
            profile_name (str): Name of application that PID supports

        Returns:
            list(int): All PID associated with profile_name or empty
                       list if profile_name is not registered.

        """
        self._profiler_mode(client_pid)
        result = []
        pids = self._active_sessions.get_profile_pids(profile_name)
        if pids is not None:
            result = [pid for pid in pids if psutil.pid_exists(pid)]
        return result

    def pop_profile_region_names(self, client_pid, profile_name):
        """Get region names associated with an application

        Called by a profiling thread to find all region names
        associated with a named application.

        Args:
            profile_name (str): Name of application that PID supports

        Returns:
            list(str): All region names associated with profile_name
                       or empty list if profile_name is not
                       registered.

        """
        result = []
        with system_files.WriteLock(self._RUN_PATH, self._PROFILER_LOCK_NAME) as lock:
            profiler_pid = lock.try_lock()
            if profiler_pid != client_pid:
                raise RuntimeError(f'The PID {client_pid} requested profile region names, but the profiling PID is {profiler_pid}')
            lock.unlock(client_pid)
        region_names = self._active_sessions.pop_profile_region_names(profile_name)
        if region_names is not None:
            result = list(region_names)
            result.sort()
        return result

    def _profiler_mode(self, client_pid):
        with system_files.WriteLock(self._RUN_PATH, self._PROFILER_LOCK_NAME) as lock:
            lock_pid = lock.try_lock()
            if lock_pid is None:
                lock.try_lock(client_pid)
            elif lock_pid != client_pid:
                raise RuntimeError(f'The PID {client_pid} requested control of application profile, but the geopm service already has an application profile controlling client with PID {lock_pid}.  To start a new controller kill {lock_pid} and retry.')

    def _write_mode(self, client_pid):
        write_pid = client_pid
        do_open_session = False
        # If the session leader is an active process then tie the write lock
        # to the session leader, otherwise the write lock is associated with
        # the requesting process.
        client_sid = os.getsid(client_pid)
        if client_sid != client_pid and psutil.pid_exists(client_sid):
            write_pid = client_sid
            do_open_session = True
        with system_files.WriteLock(self._RUN_PATH) as lock:
            lock_pid = lock.try_lock()
            if lock_pid is None:
                if do_open_session:
                    # If the write lock is associated with the session leader
                    # process, then open a session for the session leader
                    session_uid = os.stat(f'/proc/{write_pid}/status').st_uid
                    session_user = pwd.getpwuid(session_uid).pw_name
                    self.open_session(session_user, write_pid)
                save_dir = os.path.join(self._RUN_PATH, self._SAVE_DIR)
                # Clean up existing SAVE_FILES directory if it exists
                if os.path.exists(save_dir):
                    del_dir = f'{save_dir}-{uuid.uuid4()}-del'
                    sys.stderr.write(f'Warning: <geopm-service> Renaming SAVE_FILES directory which not owned by any active session to {del_dir}\n')
                    os.rename(save_dir, del_dir)
                # Save Control values
                tmp_dir = f'{save_dir}-{uuid.uuid4()}-tmp'
                system_files.secure_make_dirs(tmp_dir)
                self._pio.save_control_dir(tmp_dir)
                os.rename(tmp_dir, save_dir)
                # Set the write lock to the writer PID
                lock.try_lock(write_pid)
                self._write_pid = write_pid
            elif lock_pid != write_pid:
                raise RuntimeError(f'The PID {client_pid} requested write access, but the geopm service already has write mode client with PID or SID of {lock_pid}')

    def _watch_client(self, client_pid):
        return GLib.timeout_add_seconds(self._WATCH_INTERVAL_SEC, self.check_client, client_pid)

    def check_client(self, client_pid):
        """Called by GLib periodically to monitor if a PID is active

        GLib queries check_client() to see if the process is still alive.

        This method gets triggered upon abnormal termination of the
        session, such as when the client process unexpectedly crashes
        or ends without closing all sessions.

        """
        if (client_pid in self._active_sessions.get_clients() and
            not psutil.pid_exists(client_pid)):
            self._close_session_completely(client_pid)
            return False
        return True

    def close_inactive_clients(self):
        """Close all sessions with inactive clients."""
        for pid in self._active_sessions.get_clients():
            self._is_client_active(pid)

    def watch_interval(self):
        """Return the PID polling interval, in seconds."""
        return self._WATCH_INTERVAL_SEC


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
        self._topo_path = os.path.join(system_files.GEOPM_SERVICE_RUN_PATH, 'geopm-topo-cache')

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
        with open(self._topo_path) as fid:
            result = fid.read()
        return result

    def rm_cache(self):
        """Remove an existing cache file if it exists

        """
        try:
            os.unlink(self._topo_path)
        except FileNotFoundError:
            pass

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

    def __init__(self):
        self._topo = TopoService()
        self._platform = PlatformService()
        self._dbus_proxy = SystemMessageBus().get_proxy('org.freedesktop.DBus',
                                                        '/org/freedesktop/DBus')

    def TopoGetCache(self):
        return self._topo.get_cache()

    @accepts_additional_arguments
    def PlatformGetGroupAccess(self, group, **call_info):
        return self._platform.get_group_access(group, self._get_pid(**call_info))

    @accepts_additional_arguments
    def PlatformSetGroupAccess(self, group, allowed_signals, allowed_controls, **call_info):
        self._platform.set_group_access(group, allowed_signals, allowed_controls, self._get_pid(**call_info))

    @accepts_additional_arguments
    def PlatformSetGroupAccessSignals(self, group, allowed_signals, **call_info):
        self._platform.set_group_access_signals(group, allowed_signals, self._get_pid(**call_info))

    @accepts_additional_arguments
    def PlatformSetGroupAccessControls(self, group, allowed_controls, **call_info):
        self._platform.set_group_access_controls(group, allowed_controls, self._get_pid(**call_info))

    @accepts_additional_arguments
    def PlatformGetUserAccess(self, **call_info):
        return self._platform.get_user_access(self._get_user(**call_info), self._get_pid(**call_info))

    def PlatformGetAllAccess(self):
        return self._platform.get_all_access()

    def PlatformGetSignalInfo(self, signal_names):
        return self._platform.get_signal_info(signal_names)

    def PlatformGetControlInfo(self, control_names):
        return self._platform.get_control_info(control_names)

    @accepts_additional_arguments
    def PlatformLockControl(self, **call_info):
        self._platform.lock_control(self._get_pid(**call_info))

    @accepts_additional_arguments
    def PlatformUnlockControl(self, **call_info):
        self._platform.unlock_control(self._get_pid(**call_info))

    @accepts_additional_arguments
    def PlatformOpenSession(self, **call_info):
        self._platform.open_session(self._get_user(**call_info), self._get_pid(**call_info))

    @accepts_additional_arguments
    def PlatformCloseSession(self, **call_info):
        pid = self._get_pid(**call_info)
        self._platform.close_session(pid, pid)

    @accepts_additional_arguments
    def PlatformCloseSessionAdmin(self, client_pid, **call_info):
        caller_pid = self._get_pid(**call_info)
        self._platform.close_session_admin(client_pid, caller_pid)

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

    @accepts_additional_arguments
    def PlatformRestoreControl(self, **call_info):
        caller_pid = self._get_pid(**call_info)
        self._platform.restore_control(caller_pid)

    @accepts_additional_arguments
    def PlatformStartProfile(self, profile_name, **call_info):
        return self._platform.start_profile(self._get_user(**call_info), self._get_pid(**call_info), profile_name)

    @accepts_additional_arguments
    def PlatformStopProfile(self, region_names, **call_info):
        return self._platform.stop_profile(self._get_pid(**call_info), region_names)

    # TODO: This method should check credentials
    @accepts_additional_arguments
    def PlatformGetProfilePids(self, profile_name, **call_info):
        return self._platform.get_profile_pids(self._get_pid(**call_info), profile_name)

    # TODO: This method should check credentials
    @accepts_additional_arguments
    def PlatformPopProfileRegionNames(self, profile_name, **call_info):
        return self._platform.pop_profile_region_names(self._get_pid(**call_info), profile_name)

    def topo_rm_cache(self):
        """Remove an existing topo cache file if it exists

        """
        self._topo.rm_cache()

    def close_all_sessions(self):
        self._platform.close_all_sessions()

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
