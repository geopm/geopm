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

"""Interface for /var/run/geopm-service

Module used to read and write the files in /var/run/geopm-service.
These files maintain the state used by geopmd to track ongoing client
sessions.  These files are loaded at geopmd start-up time and enable
the daemon to cleanly restart.

"""

import os
import sys
import stat
import json
import jsonschema
import glob
import psutil
import uuid


GEOPM_SERVICE_VAR_PATH = '/var/run/geopm-service'

class ActiveSessions(object):
    """Class that manages the session files for the service

    The state about active sessions opened with geopmd by a client
    processes is stored in files in /var/run/geopm-service.  These
    files are loaded when the geopmd process starts.  The files are
    modified each time a client opens a session, closes a session,
    requests write permission, or starts a batch server.  The class is
    responsible for managing file access permissions, and atomic file
    creation.

    """

    def __init__(self, var_path=GEOPM_SERVICE_VAR_PATH):
        """Create an ActiveSessions object that tracks geopmd session files

        The geopmd session files are stored in the directory

            "/var/run/geopm-service"

        by default, but the user may specify a different path.  The
        creation of an ActiveSessions object will make the directory
        if it does not exist.  This directory is created with
        restricted access permissions (mode: 0o700).

        If the path points to a symbolic link, the link will be
        deleted and a warning is printed to the syslog.  The directory
        will then be created in its place.  The warning message will
        display the user and group permissions of the directory being
        deleteted.

        When an ActiveSessions object is created and the directory
        already exists the owner of the directory will be set to the
        geopmd user and the permissions will be set to mode 0o700.
        All files matching the pattern

            "/var/run/geopm-service/session-*.json"

        will be parsed and verified. These files must conform to the
        session JSON schema, be owned by the geopmd user, and have
        restricted access permissions (mode: 0o600).  Files matching
        the session pattern which do not meet these criterion are not
        parsed, will be deleted, and a warning is printed to standard
        syslog.  The warning message will display the user and group
        permissions of the file being deleted.

        Args:
            var_path (str): Optional argument to override the default
                            path to the session files which is
                            "/var/run/geopm-service"

        Returns:
            ActiveSessions: Object containing all valid sessions data
                            read from the var_path directory

        """
        self._VAR_PATH = var_path
        self._INVALID_PID = -1
        self._daemon_uid = os.getuid()
        self._daemon_gid = os.getgid()
        self._sessions = dict()
        self._session_schema = {
            'type' : 'object',
            'properties' : {
                'client_pid' : {'type' : 'number'},
                'mode' : {'type' : 'string'},
                'signals' : {'type' : 'array', 'items' : {'type' : 'string'}},
                'controls' : {'type' : 'array', 'items' : {'type' : 'string'}},
                'watch_id' : {'type' : 'number'},
                'batch_server': {'type' : 'number'}
            },
            'additionalProperties' : False,
            'required' : ['client_pid', 'mode', 'signals', 'controls']
        }
        if os.path.islink(self._VAR_PATH):
            # TODO enhance warning message to provide user and group
            # ownership of the symbolic link that was deleted
            sys.stderr.write(f'Warning: <geopm> {self._VAR_PATH} is a symbolic link, the link will be deleted')
            os.unlink(self._VAR_PATH)
        if not os.path.isdir(self._VAR_PATH):
            os.mkdir(self._VAR_PATH, mode=0o700)
        else:
            # Do we need a chown? print a warning?
            st = os.stat(self._VAR_PATH)
            perm_mode = stat.S_IMODE(st.st_mode)
            if perm_mode != 0o700:
                sys.stderr.write(f'Warning: <geopm> {self._VAR_PATH} has wrong permissions, reseting to 0o700, current value: {oct(perm_mode)}')
                os.chmod(self._VAR_PATH, mode=0o700)
        for sess_path in glob.glob(self._get_session_path('*')):
            self._load_session_file(sess_path)

    def is_client_active(self, client_pid):
        """Query if a Linux PID currently has an active session

        If a session file matching the client_pid exists, but the file
        was not parsed or created by geopmd, then a warning message is
        printed to syslog and the file is deleted.  The warning
        message will display the user and group permissions of the
        file being deleteted.  In this case False is returned.

        Args:
            client_pid (int): Linux PID to query

        Returns:
            bool: True if the PID has an open session, False otherwise

        """
        result = client_pid in self._sessions
        session_path = self._get_session_path(client_pid)
        if not result and os.path.isfile(session_path):
            # TODO print and delete, dont raise
            raise RuntimeError(f'Session file exists, but client {client_pid} is not tracked: {session_file}')
        return result

    def check_client_active(self, client_pid, msg=''):
        """Raise an exception if a PID does not have an active session

        Args:
            client_pid (int): Linux PID to query

            msg (str): The operation that was attempted which requires
                       an active session.  Used in error message

        Raises:
            RuntimeError: Operation not allowed without an open session

        """
        if not self.is_client_active(client_pid):
            raise RuntimeError(f"Operation '{msg}' not allowed without an open session. Client PID: {client_pid}")

    def add_client(self, client_pid, signals, controls, watch_id):
        """Add an new client session to be tracked

        Create a new session file that contains the JSON data provided
        as the call parameters in a format that conforms to the
        session file schema.

        This file will be created atomically such that if this method
        is interrupted before the session file is ready to be used,
        then no file will be present that matches load pattern below.

            /var/run/geopm-service/session-*.json

        The session file is created with the JSON object property
        "mode" set to "r" (read mode) and without the "batch_server"
        property specified.  These properties may be modified by the
        set_write_client() or set_batch_server() methods after the
        client is added.

        This operation creates the session file atomically, but does
        not modify the session properties nor the session file if the
        client PID already has an open session.

        Args:
            client_pid (int): Linux PID that opened the session

            signals (list(str)): List of signal names that are allowed
                                 to be read by the client

            controls (list(str)): List of control names that are allowed
                                  to be written by the client

            watch_id (int): The watch ID handle returned by GLib for
                            tracking that the PID is active

        """
        if self.is_client_active(client_pid):
            return
        session_data = {'client_pid': client_pid,
                        'mode': 'r',
                        'signals': list(signals),
                        'controls': list(controls),
                        'watch_id': watch_id}
        self._sessions[client_pid] = session_data
        self._update_session_file(client_pid)

    def remove_client(self, client_pid):
        """Delete the record of an active session

        Remove the client session file and delete the state associated
        with client.  Future requests about this client PID will raise
        an exception until another call to add_client() is made.  This
        exception will be raised for a second call to remove_client().

        Args:
            client_pid (int): Linux PID that opened the session

        Raises:
            RuntimeError: Client does not have an open session

        """
        self.check_client_active(client_pid, 'remove_client')
        session_file = self._get_session_path(client_pid)
        os.remove(session_file)
        self._sessions.pop(client_pid)

    def get_clients(self):
        """Get list of the client PID values for all active sessions

        Creates a list of the PID values for all active session clients.

        Returns:
            list(int): The Linux PID values for all active clients

        """
        return list(self._sessions.keys())

    def is_write_client(self, client_pid):
        """Query if the client PID currently holds the write lock

        Returns:
            bool: True if the client PID holds the write lock, and
                  false otherwise

        Raises:
            RuntimeError: Client does not have an open session

        """
        self.check_client_active(client_pid, 'get_mode')
        return self._sessions[client_pid]['mode'] == 'rw'

    def get_signals(self, client_pid):
        """Query all signal names that are available

        Creates a list of all signal names that are available to the
        session opened by the client PID.

        Args:
            client_pid (int): Linux PID that opened the session

        Returns:
            list(str): Signal name access list for the session

        Raises:
            RuntimeError: Client does not have an open session

        """
        self.check_client_active(client_pid, 'get_signals')
        return list(self._sessions[client_pid]['signals'])

    def get_controls(self, client_pid):
        """Query all control names that are available

        Creates a list of all control names that are available to the
        session opened by the client PID.

        Args:
            client_pid (int): Linux PID that opened the session

        Returns:
            list(str): Control name access list for the session

        Raises:
            RuntimeError: Client does not have an open session

        """
        self.check_client_active(client_pid, 'get_controls')
        return list(self._sessions[client_pid]['controls'])

    def get_watch_id(self, client_pid):
        """Query the GLib watch ID for tracking the session lifetime

        The watch ID is not valid in the case where the geopmd process
        restarts.  When a new geopmd process starts it will read files
        that contain watch ID values, however, the set_watch_id()
        method must be called with an updated watch ID for each active
        client session when geopmd starts.

        Args:
            client_pid (int): Linux PID that opened the session

        Returns:
            int: GLib watch ID to track the session lifetime

        Raises:
            RuntimeError: Client does not have an open session

        """
        self.check_client_active(client_pid, 'get_watch_id')
        return self._sessions[client_pid]['watch_id']

    def set_watch_id(self, client_pid, watch_id):
        """Set the GLib watch ID for tracking the session lifetime

        Store the GLib watch ID after registering the callback with
        GLib.timeout_add().

        Args:
            client_pid (int): Linux PID that opened the session

            watch_id (int): GLib watch ID returned by the
                            GLib.timeout_add() method

        Raises:
            RuntimeError: Client does not have an open session

        """
        self.check_client_active(client_pid, 'get_watch_id')
        self._sessions[client_pid]['watch_id'] = watch_id
        self._update_session_file(client_pid)

    def get_batch_server(self, client_pid):
        """Query the batch server for the client session

        In the case where the client has an open batch server, this
        method returns the Linux PID of the batch server.  If there is
        no active batch server then None is returned.

        Args:
            client_pid (int): Linux PID that opened the session

        Returns:
            int: The Linux PID of the batch server or None

        Raises:
            RuntimeError: Client does not have an open session

        """
        self.check_client_active(client_pid, 'get_batch_server')
        return self._sessions[client_pid].get('batch_server')

    def set_write_client(self, client_pid):
        """Mark a client session as the write mode client

        This method identifies that the client session is able to
        write through the service.  The file that stores this
        information about the session will be updated atomically,
        however it is the user's responsibility to maintain the
        requirement for one write mode session at any one time.

        Args:
            client_pid (int): Linux PID that opened the session

        Raises:
            RuntimeError: Client does not have an open session

        """
        self.check_client_active(client_pid, 'set_write_client')
        self._sessions[client_pid]['mode'] = 'rw'
        self._update_session_file(client_pid)

    def set_batch_server(self, client_pid, batch_pid):
        """Set the Linux PID of the batch server supporting a client session

        This method is used to store the PID after a batch server is created.

        Args:
            client_pid (int): Linux PID that opened the session

            batch_pid (int): Linux PID of the batch server created for
                             the client session

        Raises:
            RuntimeError: Client does not have an open session

        """
        self.check_client_active(client_pid, 'set_batch_server')
        if 'batch_server' in self._sessions[client_pid]:
            current_server = self._sessions[client_pid]['batch_server']
            raise RuntimeError(f'Client {client_pid} has already started a batch server: {current_server}')
        self._sessions[client_pid]['batch_server'] = batch_pid
        self._update_session_file(client_pid)

    def remove_batch_server(self, client_pid):
        """Remove the record for the batch server supporting a client session

        This method is used to remove the record of a batch server
        after it is closed.

        Args:
            client_pid (int): Linux PID that opened the session

        Raises:
            RuntimeError: Client does not have an open session

        """
        self.check_client_active(client_pid, 'remove_batch_server')
        self._sessions[client_pid].pop('batch_server')
        self._update_session_file(client_pid)

    def _get_session_path(self, client_pid):
        return f'{self._VAR_PATH}/session-{client_pid}.json'

    def _update_session_file(self, client_pid):
        sess = self._sessions[client_pid]
        jsonschema.validate(sess, schema=self._session_schema)
        session_path = self._get_session_path(client_pid)
        tmp_id = uuid.uuid4()
        session_path_tmp = f'{session_path}-tmp-{tmp_id}'
        os.umask(0o077)
        with open(session_path_tmp, 'w') as fid:
            json.dump(sess, fid)
        os.rename(session_path_tmp, session_path)

    def _load_session_file(self, sess_path):
        with open(sess_path) as fid:
            sess_stat = os.stat(fid.fileno())
            # Check required permissions
            if not (stat.S_ISREG(sess_stat.st_mode) and
                    stat.S_IMODE(sess_stat.st_mode) == 0o600 and
                    sess_stat.st_uid == self._daemon_uid and
                    sess_stat.st_gid == self._daemon_gid):
                # TODO: more verbose logging of user and group id
                err_msg = f'Warning: <geopm-service> session file was discovered with invalid permissions, will be ignored and removed: {sess_path}\n'
                sys.stderr.write(err_msg)
                os.unlink(sess_path)
                return  # Bad permissions return early
            sess = json.load(fid)
        file_time = sess_stat.st_ctime
        jsonschema.validate(sess, schema=self._session_schema)
        client_pid = sess['client_pid']
        if client_pid != self._INVALID_PID:
            try:
                proc_time = psutil.Process(client_pid).create_time()
                # TODO: Write test to check this comparison logic
                if proc_time >= file_time:
                    # PID has been recycled, so set it to invalid
                    sess['client_pid'] = self._INVALID_PID
            except psutil.NoSuchProcess:
                # PID is no longer active, so set it to invalid
                sess['client_pid'] = self._INVALID_PID
        self._sessions[client_pid] = dict(sess)
