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

def secure_make_dirs(path):
    """Securely create a directory

    When the path does not exist a directory is created with
    permissions 0o700 along with all required parent directories.

    The security of this path is verified if the path exists.  An
    existing path is considered to be secure only if all of the
    following conditions are met:

        - The path is a regular directory
        - The path is not a link
        - The path is accessible the the caller
        - The path is a directory owned by the calling process uid
        - The path is a directory owned by the calling process gid
        - The permissions for the directory are 0o700

    If the existing path is determined to be insecure, a warning is
    printed to syslog and the existing file will be renamed to
    `<path>-<UUID>-INVALID` so that it may be audited later, but not
    used.  The warning message will display the reason why the
    directory was not secure.  A new directory with the specified path
    will be created in place of the renamed file.

    This function will simply return when the directory already exists
    and is considered secure.

    Args:
        path (str): The path were the directory is created

    """
    if os.path.exists(path):
        is_valid = True
        renamed_path = f'{path}-{uuid.uuid4()}-INVALID'
        # If it's a link
        if os.path.islink(path):
            is_valid = False
            sys.stderr.write(f'Warning: <geopm-service> {path} is a symbolic link, the link will be renamed to {renamed_path}')
            sys.stderr.write(f'Warning: <geopm-service> the symbolic link points to {os.readlink(path)}')
        # If it's a so-called "regular file"
        elif not os.path.isdir(path):
            is_valid = False
            sys.stderr.write(f'Warning: <geopm-service> {path} is not a directory, it will be renamed to {renamed_path}')
        # If it's a directory
        else:
            st = os.stat(path)
            # If the permissions is not what we wanted
            perm_mode = stat.S_IMODE(st.st_mode)
            if perm_mode != 0o700:
                sys.stderr.write(f'Warning: <geopm-service> {path} has wrong permissions, it will be renamed to {renamed_path}')
                sys.stderr.write(f'Warning: <geopm-service> the wrong permissions were {oct(perm_mode)}')
                is_valid = False
            # If the user owner is not what we wanted
            user_owner = st.st_uid
            if user_owner != os.getuid():
                sys.stderr.write(f'Warning: <geopm-service> {path} has wrong user owner, it will be renamed to {renamed_path}')
                sys.stderr.write(f'Warning: <geopm-service> the wrong user owner was {user_owner}')
                is_valid = False
            # If the group owner is not what we wanted
            group_owner = st.st_gid
            if group_owner != os.getgid():
                sys.stderr.write(f'Warning: <geopm-service> {path} has wrong group owner, it will be renamed to {renamed_path}')
                sys.stderr.write(f'Warning: <geopm-service> the wrong group owner was {group_owner}')
                is_valid = False
        # If one of the three above branches revealed invalid file
        if not is_valid:
            os.rename(path, renamed_path)
            os.mkdir(path, mode=0o700)
    # If the path doesn't exist
    else:
        os.mkdir(path, mode=0o700)

def secure_make_file(path, contents):
    """Securely and atomically create a file containing a string

    The file is created by first writing the output to a temporary
    file with restricted permissions (mode 0o600) within the same
    directory, and then renaming the file after it is closed.  This
    temporary file with have the path `<path>-<uuid>-tmp`.

    The rename operation is atomic, and will overwrite any existing
    file located in the specified path.  The permissions on the output
    file are mode 0o600 with uid and gid matching the process values.

    Args:
        path (str): The path where the file is created

        contents (str): The contents of the created file

    """
    temp_path = f'{path}-{uuid.uuid4()}-tmp'
    old_mask = os.umask(0o077)
    with open(os.open(temp_path, os.O_CREAT | os.O_WRONLY, 0o600), 'w') as file:
        file.write(contents)
    os.rename(temp_path, path)
    os.umask(old_mask)

def secure_read_file(path):
    """Securely read a file into a string

    The security of this path is verified after the file is opened. An
    existing path is considered to be secure only if all of the
    following conditions are met:

        - The path is a regular file
        - The path is not a link
        - The path is accessible the the caller
        - The path is a file owned by the calling process uid
        - The path is a file owned by the calling process gid
        - The permissions for the file are 0o600

    If the existing path is determined to be insecure, a warning is
    printed to syslog and the existing file will be renamed to
    `<path>-<UUID>-INVALID` so that it may be audited later, but not
    used.  The warning message will display the reason why the file
    was not secure and None is returned.

    If the path points to an existing file that is determined to be
    secure then the contents of the file is returned.

    Args:
        path (str): The path where the file is created

    Returns:
        str: The contents of the file if file was opened, None if couldn't open file

    """
    daemon_uid = os.getuid()
    daemon_gid = os.getgid()
    contents = None
    # If the path exists
    if os.path.exists(path):
        renamed_path = f'{path}-{uuid.uuid4()}-INVALID'

        # If it is a link
        if os.path.islink(path):
            sys.stderr.write(f'Warning: <geopm-service> {path} is a symbolic link, it will be renamed to {renamed_path}')
            sys.stderr.write(f'Warning: <geopm-service> the symbolic link points to {os.readlink(path)}')
        # If it is not a directory
        elif not os.path.isdir(path):
            with open(path) as fid:
                sess_stat = os.stat(fid.fileno())
                # If the permissions requirements of the file are not satisfied
                if not (stat.S_ISREG(sess_stat.st_mode) and
                        stat.S_IMODE(sess_stat.st_mode) == 0o600 and
                        sess_stat.st_uid == daemon_uid and
                        sess_stat.st_gid == daemon_gid):
                    # If it is not a regular file
                    if not stat.S_ISREG(sess_stat.st_mode):
                        sys.stderr.write(f'Warning: <geopm-service> {path} is not a regular file, it will be renamed to {renamed_path}')
                    # If it is a regular file with bad permissions
                    else:
                        sys.stderr.write(f'Warning: <geopm-service> {path} was discovered with invalid permissions, it will be renamed to {renamed_path}')
                        # If the permissions are wrong
                        if stat.S_IMODE(sess_stat.st_mode) != 0o600:
                            sys.stderr.write(f'Warning: <geopm-service> the wrong permissions were {oct(sess_stat.st_mode)}')
                        # If the user owner is wrong
                        if sess_stat.st_uid != daemon_uid:
                            sys.stderr.write(f'Warning: <geopm-service> the wrong user owner was {sess_stat.st_uid}')
                        # If the group owner is wrong
                        if sess_stat.st_gid != daemon_gid:
                            sys.stderr.write(f'Warning: <geopm-service> the wrong group owner was {sess_stat.st_gid}')

                # If the file satisfies all requirements
                else:
                    # Read whole file into the string
                    contents = fid.read()
        # If it is a directory
        else:
            sys.stderr.write(f'Warning: <geopm-service> {path} is a directory, it will be renamed to {renamed_path}')

        # If the existing path is determined to be insecure it will be renamed to
        # `<path>-<UUID>-INVALID` so that it may be audited later, but not used.
        if contents is None:
            os.rename(path, renamed_path)

    # If the path does not exist
    else:
        sys.stderr.write(f'Warning: <geopm-service> {path} does not exist')

    return contents


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
        renamed and a warning is printed to the syslog.  The directory
        will then be created in its place.  The warning message will
        display the target of the symbolic link and will appear in the
        syslog.

        If the path points to a directory which is not accessible, or
        if the directory is owned by the wrong user or group the
        directory will be renamed and a warning is printed to the
        syslog. The warning message will display the reason why the
        directory was not accessible, or display the permissions or
        user and group of the directory.

        When an ActiveSessions object is created and the directory
        already exists, if the owner of the directory is the geopmd
        user, and the permissions are set to 0o700 parsing will
        proceed.  All files matching the pattern

            "/var/run/geopm-service/session-*.json"

        will be parsed and verified. These files must conform to the
        session JSON schema, be owned by the geopmd user, and have
        restricted access permissions (mode: 0o600).  Files matching
        the session pattern which do not meet these criterion are not
        parsed, will be renamed, and a warning is printed to the
        syslog.  The warning message will display the user and group
        permissions of the file being renamed.

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
                'mode' : {'type' : 'string', "enum" : ["r", "rw"]},
                'signals' : {'type' : 'array', 'items' : {'type' : 'string'}},
                'controls' : {'type' : 'array', 'items' : {'type' : 'string'}},
                'watch_id' : {'type' : 'number'},
                'batch_server': {'type' : 'number'}
            },
            'additionalProperties' : False,
            'required' : ['client_pid', 'mode', 'signals', 'controls']
        }
        secure_make_dirs(self._VAR_PATH)

        # Load all session files in the directory
        for sess_path in glob.glob(self._get_session_path('*')):
            self._load_session_file(sess_path)

    def is_client_active(self, client_pid):
        """Query if a Linux PID currently has an active session

        If a session file matching the client_pid exists, but the file
        was not parsed or created by geopmd, then a warning message is
        printed to syslog and the file is renamed.  The warning
        message will display the user and group permissions of the
        file being deleted.  In this case False is returned.

        Args:
            client_pid (int): Linux PID to query

        Returns:
            bool: True if the PID has an open session, False otherwise

        """
        result = client_pid in self._sessions
        session_path = self._get_session_path(client_pid)
        if not result and os.path.isfile(session_path):
            sys.stderr.write(f'Session file exists, but client {client_pid} is not tracked: {session_path}')
            # TODO if we got here, the file should be moved to INVALID
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
        """Add a new client session to be tracked

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
        jsonschema.validate(session_data, schema=self._session_schema)
        self._sessions[client_pid] = session_data
        self._update_session_file(client_pid)

    def remove_client(self, client_pid):
        """Delete the record of an active session

        Remove the client session file and delete the state associated
        with the client.  Future requests about this client PID will raise
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
        """Query for the GLib watch ID for tracking the session lifetime

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
        """Query for the session file path for client PID

        Args:
            client_pid (int): Linux PID that opened the session

        Returns:
            str: Current client's session file path

        """
        return f'{self._VAR_PATH}/session-{client_pid}.json'

    def _update_session_file(self, client_pid):
        """Write the session data to disk for client PID

        Args:
            client_pid (int): Linux PID that opened the session

        """
        sess = self._sessions[client_pid]
        jsonschema.validate(sess, schema=self._session_schema)
        session_path = self._get_session_path(client_pid)
        secure_make_file(session_path, json.dumps(sess))

    def _is_pid_valid(self, pid, file_time):
        """Verify validity of PID

        If the creation time of the process associated with PID
        is newer than or equal to file_time, or if the PID does not
        exist, then the PID is not valid.

        Args:
            pid (int): Linux PID to be verified

            file_time (int): stat.st_ctime to compare against

        Raises:
           psutil.AccessDenied: Insufficient privileges to query for
                                process creation time.

        """
        result = True
        try:
            proc_time = psutil.Process(pid).create_time()
            if proc_time >= file_time:
                # PID has been recycled, return false
                result = False
        except (ValueError, psutil.NoSuchProcess):
            # PID is no longer active
            result = False
        return result

    def _load_session_file(self, sess_path):
        """Load the session file into memory

        If the session file is not valid, it will not be loaded
        and warnings will be printed.  The invalid file will be
        renamed and preserved.

        Args:
            sess_path (str): Current client's session file path

        """
        contents = secure_read_file(sess_path)
        if contents is None:
            return # Invalid JSON return early
        try:
            sess = json.loads(contents)
            jsonschema.validate(sess, schema=self._session_schema)
        except:
            renamed_path = f'{sess_path}-{uuid.uuid4()}-INVALID'
            sys.stderr.write(f'Warning: <geopm-service> Invalid JSON file, unable to parse, renamed{sess_path} to {renamed_path} and will ignore')
            os.rename(sess_path, renamed_path)
            return # Invalid JSON return early

        sess_stat = os.stat(sess_path)
        file_time = sess_stat.st_ctime
        client_pid = sess['client_pid']
        batch_pid = sess.get('batch_server')
        if self._is_pid_valid(client_pid, file_time):
            # Valid session; verify batch server
            if batch_pid is not None and \
               self._is_pid_valid(batch_pid, file_time) == False:
                sess.pop('batch_server')
        else:
            # Invalid session, remove batch server
            sess['client_pid'] = self._INVALID_PID
            if batch_pid is not None:
                sess.pop('batch_server')

        self._sessions[client_pid] = dict(sess)
        self._update_session_file(client_pid)
