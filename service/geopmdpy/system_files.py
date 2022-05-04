#  Copyright (c) 2015 - 2022, Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#

"""Manage system files used by the geopm service

Provides secure interfaces for manipulating the files in

    /run/geopm-service

that enable the service to be restarted and

   /etc/geopm-service

where the access lists are located.  These interfaces provide
guarantees about the security of these system files, and an
abstraction for updating the contents.

"""

import os
import sys
import stat
import json
import jsonschema
import glob
import psutil
import uuid
import grp
import pwd
import fcntl

from . import pio


GEOPM_SERVICE_RUN_PATH = '/run/geopm-service'
GEOPM_SERVICE_CONFIG_PATH = '/etc/geopm-service'

GEOPM_SERVICE_RUN_PATH_PERM = 0o711
"""Default permissions for the GEOPM service run path

"""

GEOPM_SERVICE_CONFIG_PATH_PERM = 0o700
"""Default permissions for the GEOPM service config path

"""

def secure_make_dirs(path, perm_mode=0o700):
    """Securely create a directory

    When the path does not exist a directory is created with permissions
    0o700 by default along with all required parent directories.

    The security of this path is verified if the path exists.  An
    existing path is considered to be secure only if all of the
    following conditions are met:

        - The path is a regular directory
        - The path is not a link
        - The path is accessible the the caller
        - The path is a directory owned by the calling process uid
        - The path is a directory owned by the calling process gid
        - The permissions for the directory are 0o700 by default

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
            sys.stderr.write(f'Warning: <geopm-service> {path} is a symbolic link, the link will be renamed to {renamed_path}\n')
            sys.stderr.write(f'Warning: <geopm-service> the symbolic link points to {os.readlink(path)}\n')
        # If it's not a directory
        elif not os.path.isdir(path):
            is_valid = False
            sys.stderr.write(f'Warning: <geopm-service> {path} is not a directory, it will be renamed to {renamed_path}\n')
        # If it's a directory
        else:
            st = os.stat(path)
            # If the permissions are not what we wanted
            set_perm_mode = stat.S_IMODE(st.st_mode)
            if set_perm_mode != perm_mode:
                sys.stderr.write(f'Warning: <geopm-service> {path} has wrong permissions, it will be renamed to {renamed_path}\n')
                sys.stderr.write(f'Warning: <geopm-service> the wrong permissions were {oct(set_perm_mode)}\n')
                is_valid = False
            # If the user owner is not what we wanted
            user_owner = st.st_uid
            if user_owner != os.getuid():
                sys.stderr.write(f'Warning: <geopm-service> {path} has wrong user owner, it will be renamed to {renamed_path}\n')
                sys.stderr.write(f'Warning: <geopm-service> the wrong user owner was {user_owner}\n')
                is_valid = False
            # If the group owner is not what we wanted
            group_owner = st.st_gid
            if group_owner != os.getgid():
                sys.stderr.write(f'Warning: <geopm-service> {path} has wrong group owner, it will be renamed to {renamed_path}\n')
                sys.stderr.write(f'Warning: <geopm-service> the wrong group owner was {group_owner}\n')
                is_valid = False
        # If one of the three above branches revealed an invalid file
        if not is_valid:
            os.rename(path, renamed_path)
            os.mkdir(path, mode=perm_mode)
    # If the path doesn't exist
    else:
        os.mkdir(path, mode=perm_mode)


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
    try:
        with open(os.open(temp_path, os.O_CREAT | os.O_WRONLY, 0o600), 'w') as file:
            file.write(contents)
        os.rename(temp_path, path)
    finally:
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
    secure then the contents of the file are returned.

    Args:
        path (str): The path where the file is created

    Returns:
        str: The contents of the file if file was opened, None if couldn't open file

    """
    contents = None
    if is_secure_path(path):
        with open(path) as fid:
            if is_secure_file(path, fid):
                contents = fid.read()
    return contents


def is_secure_path(path):
    """Query if path may be openend safely

    Check if path exists, and refers to a regular file that is not a link.  A
    warning message is printed if the path is a link, a directory or is not a
    regular file and the file is renamed to a path of the form
    `<PATH>-<UUID>-INVALID`.

    Args:
        path (str): The file path

    Returns:
        bool: True if the path is a regular file and not a link

    """
    result = False
    if os.path.exists(path):
        renamed_path = f'{path}-{uuid.uuid4()}-INVALID'
        if os.path.islink(path):
            sys.stderr.write(f'Warning: <geopm-service> {path} is a symbolic link, it will be renamed to {renamed_path}\n')
            sys.stderr.write(f'Warning: <geopm-service> the symbolic link points to {os.readlink(path)}\n')
        elif os.path.isdir(path):
            sys.stderr.write(f'Warning: <geopm-service> {path} is a directory, it will be renamed to {renamed_path}\n')
        elif not stat.S_ISREG(os.stat(path).st_mode):
            sys.stderr.write(f'Warning: <geopm-service> {path} is not a regular file, it will be renamed to {renamed_path}\n')
        else:
            result = True
        if not result:
            os.rename(path, renamed_path)
    else:
        sys.stderr.write(f'Warning: <geopm-service> {path} does not exist\n')
    return result


def is_secure_file(path, fid):
    """Query if file descriptor may be safely read

    After opening a file this function is called to determine if the file
    descriptor is a regular file owned by the calling process user and group
    with restricted permissions (i.e. mode 0o600).  If these conditions are
    not met then a warning message is printed and the file is renamed to a
    path of the form `<PATH>-<UUID>-INVALID`.

    Args:
        path (str): The file path that was passed to open()

        fid (file): File descriptor returned by open()

    Returns:
        bool: True if regular file with restricted permissions

    """
    result = False
    daemon_uid = os.getuid()
    daemon_gid = os.getgid()
    renamed_path = f'{path}-{uuid.uuid4()}-INVALID'
    warn_msg = f'Warning: <geopm-service> {path} was discovered with invalid permissions, it will be renamed to {renamed_path}\n'
    sess_stat = os.stat(fid.fileno())
    if not stat.S_ISREG(sess_stat.st_mode):
        sys.stderr.write(warn_msg)
        sys.stderr.write(f'Warning: <geopm-service> {path} is not a regular file\n')
    elif stat.S_IMODE(sess_stat.st_mode) != 0o600:
        sys.stderr.write(warn_msg)
        sys.stderr.write(f'Warning: <geopm-service> the wrong permissions were {oct(sess_stat.st_mode)}\n')
    elif sess_stat.st_uid != daemon_uid:
        sys.stderr.write(warn_msg)
        sys.stderr.write(f'Warning: <geopm-service> the wrong user owner was {sess_stat.st_uid}\n')
    elif sess_stat.st_gid != daemon_gid:
        sys.stderr.write(warn_msg)
        sys.stderr.write(f'Warning: <geopm-service> the wrong group owner was {sess_stat.st_gid}\n')
    else:
        result = True
    if not result:
        os.rename(path, renamed_path)
    return result


class ActiveSessions(object):
    """Class that manages the session files for the service

    The state about active sessions opened with geopmd by a client
    processes is stored in files in /run/geopm-service.  These
    files are loaded when the geopmd process starts.  The files are
    modified each time a client opens a session, closes a session,
    requests write permission, or starts a batch server.  The class is
    responsible for managing file access permissions, and atomic file
    creation.

    """

    def __init__(self, run_path=GEOPM_SERVICE_RUN_PATH):
        """Create an ActiveSessions object that tracks geopmd session files

        The geopmd session files are stored in the directory

            "/run/geopm-service"

        by default, but the user may specify a different path.  The
        creation of an ActiveSessions object will make the directory
        if it does not exist.  This directory is created with
        restricted access permissions (mode: GEOPM_SERVICE_RUN_PATH_PERM).

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
        user, and the permissions are set to GEOPM_SERVICE_RUN_PATH_PERM
        parsing will proceed.  All files matching the pattern

            "/run/geopm-service/session-*.json"

        will be parsed and verified. These files must conform to the
        session JSON schema, be owned by the geopmd user, and have
        restricted access permissions (mode: 0o600).  Files matching
        the session pattern which do not meet these criterion are not
        parsed, will be renamed, and a warning is printed to the
        syslog.  The warning message will display the user and group
        permissions of the file being renamed.

        Args:
            run_path (str): Optional argument to override the default
                            path to the session files which is
                            "/run/geopm-service"

        Returns:
            ActiveSessions: Object containing all valid sessions data
                            read from the run_path directory

        """
        self._RUN_PATH = run_path
        self._daemon_uid = os.getuid()
        self._daemon_gid = os.getgid()
        self._sessions = dict()
        self._session_schema = {
            'type' : 'object',
            'properties' : {
                'client_pid' : {'type' : 'number'},
                'reference_count' : {'type' : 'integer', 'minimum' : 0},
                'signals' : {'type' : 'array', 'items' : {'type' : 'string'}},
                'controls' : {'type' : 'array', 'items' : {'type' : 'string'}},
                'watch_id' : {'type' : 'number'},
                'batch_server': {'type' : 'number'}
            },
            'additionalProperties' : False,
            'required' : ['client_pid', 'signals', 'controls']
        }
        secure_make_dirs(self._RUN_PATH,
                         perm_mode=GEOPM_SERVICE_RUN_PATH_PERM)

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
            renamed_path = f'{session_path}-{uuid.uuid4()}-INVALID'
            sys.stderr.write(f'Session file exists, but client {client_pid} is not tracked: {session_path} will be moved to {renamed_path}\n')
            os.rename(session_path, renamed_path)
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

            ``/run/geopm-service/session-*.json``

        The session file is created without the JSON object property
        "batch_server" specified.  This properties may be modified by the
        set_batch_server() method after the client is added.

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
                        'reference_count': 1,
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

    def get_reference_count(self, client_pid):
        """Query the reference count for the client session.

        The reference count is basically just an abstraction for the client.
        The client itself cannot have more than a single open session at once,
        since there is a one to one mapping between client pid and open sessions,
        and lifetime of our session is coupled to the lifetime of the connection.

        The reference count is used when in the client process, we have independent components,
        which all may request that same connection as a kind of shared resource.
        We want to provide that same connection as a singleton, and keep track of
        how many independent components have access to that same connection as a resource,
        in order to know when to deallocate that resource when all these components
        have disconnected from the session.

        Args:
            client_pid (int): Linux PID that opened the session

        Returns:
            int: The reference count

        Raises:
            RuntimeError: Client does not have an open session

        """
        self.check_client_active(client_pid, 'get_reference_count')
        return self._sessions[client_pid]['reference_count']

    def set_reference_count(self, client_pid, reference_count):
        """Set the reference count for the session

        This method is used whenever we want to update the reference_count to
        reflect the number of independent components that have connected to the session.

        Args:
            client_pid (int): Linux PID that opened the session

            reference_count (int): The new reference count value

        Raises:
            RuntimeError: Client does not have an open session

        """
        self.check_client_active(client_pid, 'set_reference_count')
        self._sessions[client_pid]['reference_count'] = reference_count
        self._update_session_file(client_pid)

    def increment_reference_count(self, client_pid):
        """Increment the reference_count value by one

        This method is called when a new independent component calls open_session()
        a subsequent time after the connection has already been opened.

        Args:
            client_pid (int): Linux PID that opened the session

        Raises:
            RuntimeError: Client does not have an open session

        """
        self.check_client_active(client_pid, 'increment_reference_count')
        self._sessions[client_pid]['reference_count'] += 1
        self._update_session_file(client_pid)

    def decrement_reference_count(self, client_pid):
        """Decrement the reference_count value by one

        This method is called when an independent component of the client
        calls close_session() and the connection is still open.
        For example when there are multiple independent components using that
        open connection, and one of them wants to close the session, we just
        decrement the reference count, we do not close the connection, as that
        would impact the other independent components which are using the connection.

        Args:
            client_pid (int): Linux PID that opened the session

        Raises:
            RuntimeError: Client does not have an open session

        """
        self.check_client_active(client_pid, 'decrement_reference_count')
        self._sessions[client_pid]['reference_count'] -= 1
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
        # TODO: Unlink any shared memory files left around

    def _get_session_path(self, client_pid):
        """Query for the session file path for client PID

        Args:
            client_pid (int): Linux PID that opened the session

        Returns:
            str: Current client's session file path

        """
        return f'{self._RUN_PATH}/session-{client_pid}.json'

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
        renamed and retained.

        Args:
            sess_path (str): Current client's session file path

        """
        renamed_path = f'{sess_path}-{uuid.uuid4()}-INVALID'
        contents = secure_read_file(sess_path)
        if contents is None:
            return # Invalid JSON return early
        try:
            sess = json.loads(contents)
            jsonschema.validate(sess, schema=self._session_schema)
        except:
            sys.stderr.write(f'Warning: <geopm-service> Invalid JSON file, unable to parse, renamed{sess_path} to {renamed_path} and will ignore\n')
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
            self._sessions[client_pid] = dict(sess)
            self._update_session_file(client_pid)
        else:
           os.rename(sess_path, renamed_path)


class AccessLists(object):
    """Class that manages the access list files

    """
    def __init__(self, config_path=GEOPM_SERVICE_CONFIG_PATH):
        self._CONFIG_PATH = config_path
        secure_make_dirs(self._CONFIG_PATH)
        self._DEFAULT_ACCESS = '0.DEFAULT_ACCESS'
        self._pio = pio

    def _validate_group(self, group):
        if group is None or group == '':
            group = self._DEFAULT_ACCESS
        else:
            group = str(group)
            if group[0].isdigit():
                raise RuntimeError('Linux group name cannot begin with a digit: group = "{}"'.format(group))
            try:
                grp.getgrnam(group)
            except KeyError:
                raise RuntimeError('Linux group is not defined: group = "{}"'.format(group))
        return group

    def _read_allowed(self, path):
        contents = secure_read_file(path)
        result = []
        if contents is not None:
            result = [line.strip() for line in contents.splitlines()
                      if line.strip() and not line.strip().startswith('#')]
        return result

    def _write_allowed(self, path, allowed):
        allowed.append('')
        contents = '\n'.join(allowed)
        secure_make_file(path, contents)

    def _filter_valid_signals(self, signals):
        signals = set(signals)
        all_signals = self._pio.signal_names()
        return list(signals.intersection(all_signals))

    def _filter_valid_controls(self, controls):
        controls = set(controls)
        all_controls = self._pio.control_names()
        return list(controls.intersection(all_controls))

    def _get_user_groups(self, user):
        user_gid = pwd.getpwnam(user).pw_gid
        all_gid = os.getgrouplist(user, user_gid)
        return [grp.getgrgid(gid).gr_name for gid in all_gid]

    def get_group_access(self, group):
        """Get the signal and control access lists

        Read the list of allowed signals and controls for the
        specified group.  If the group is None or the empty string
        then the default lists of allowed signals and controls are
        returned.

        The values are securely read from files located in
        /etc/geopm-service using the secure_read_file() interface.

        If no secure file exist for the specified group, then two
        empty lists are returned.

        Args:
            group (str): Name of group

        Returns:
            list(str)), list(str): Signal and control allowed lists

        Raises:
            RuntimeError: The group name is not valid on the system.

        """
        group = self._validate_group(group)
        group_dir = os.path.join(self._CONFIG_PATH, group)
        if os.path.isdir(group_dir):
            secure_make_dirs(group_dir)
            path = os.path.join(group_dir, 'allowed_signals')
            signals = self._read_allowed(path)
            signals = self._filter_valid_signals(signals)
            path = os.path.join(group_dir, 'allowed_controls')
            controls = self._read_allowed(path)
            controls = self._filter_valid_controls(controls)
        else:
            signals = []
            controls = []
        return signals, controls

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

    def set_group_access(self, group, allowed_signals, allowed_controls):
        """Set signals and controls in the allowed lists

        Write the list of allowed signals and controls for the
        specified group.  If the group is None or the empty string
        then the default lists of allowed signals and controls are
        updated.

        The values are securely written atomically to files located in
        /etc/geopm-service using the secure_make_dirs() and
        secure_make_file() interfaces.

        Args:
            group (str): Name of group

            allowed_signals (list(str)): Signal names that are allowed

            allowed_controls (list(str)): Control names that are allowed

        Raises:
            RuntimeError: The group name is not valid on the system.

        """
        group = self._validate_group(group)
        self._validate_signals(allowed_signals)
        self._validate_controls(allowed_controls)
        group_dir = os.path.join(self._CONFIG_PATH, group)
        secure_make_dirs(group_dir,
                         perm_mode=GEOPM_SERVICE_CONFIG_PATH_PERM)
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

        Raises:
            RuntimeError: The user does not exist.

        """
        if user == 'root':
            return self.get_all_access()
        user_groups = []
        if user != '':
            try:
                user_groups = self._get_user_groups(user)
            except KeyError as e:
                raise RuntimeError("Specified user '{}' does not exist.".format(user))
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


class WriteLock(object):
    """Class for interacting with control lock file

    This class provides the interface to query and set the PID that owns the
    GEOPM Service control write lock.  The state of this lock is stored in the
    file path:

        /run/geopm-service/CONTROL_LOCK

    This file is empty when the lock is free, and contains the PID of the
    controlling process when the lock is held.  The class manages the advisory
    lock to serialize any attempts read or write the file.  Additionally the
    class checks that the lock file is a regular file with restricted
    permissions.  Maniuplations of the control lock file should be done
    exclusively with the WriteLock object to insure that the advisory lock is
    effective.

    """
    def __init__(self, run_path=GEOPM_SERVICE_RUN_PATH):
        """Set up initial state

        The WriteLock must be used within a context manager.  Use a
        non-default run_path only for testing purposes.  This constructor will
        securely create the run_path if it does not exist.

        Args:
            run_path: Directory to create control lock within

        """
        self._RUN_PATH = run_path
        self._LOCK_PATH = os.path.join(self._RUN_PATH, "CONTROL_LOCK")
        self._fid = None
        secure_make_dirs(self._RUN_PATH,
                         perm_mode=GEOPM_SERVICE_RUN_PATH_PERM)

    def __enter__(self):
        """Enter context management for interacting with write lock

        Securely open the lock file for editing and hold exclusive advisory
        lock.  The file pointer is rewound after the lock is held.  Two
        attempts will be made to securely open the file and any insecure files
        will be renamed.

        """
        self._rpc_lock()
        old_mask = os.umask(0o077)
        try:
            trial_count = 0
            while self._fid is None and trial_count < 2:
                if os.path.exists(self._LOCK_PATH):
                    # Rename existing file if it is not secure
                    is_secure_path(self._LOCK_PATH)
                self._fid = open(self._LOCK_PATH, 'a+')
                if not is_secure_file(self._LOCK_PATH, self._fid):
                    # File is insecure and was renamed, try again
                    self._fid.close()
                    self._fid = None
                trial_count += 1
        finally:
            os.umask(old_mask)
        if self._fid == None:
            self._rpc_unlock()
            raise RuntimeError('Unable to open write lock securely after two tries')
        # Advisory lock protects against simultanious multi-process
        # modifications to the file, although we expect only one geopmd
        # process using this class.
        fcntl.lockf(self._fid, fcntl.LOCK_EX)
        self._fid.seek(0)
        return self

    def __exit__(self, type, value, traceback):
        """Exit context management for interacting with write lock

        Release the advisory lock and close the file.

        """
        if self._fid is not None:
            fcntl.lockf(self._fid, fcntl.LOCK_UN)
            self._fid.close()
            self._rpc_unlock()


    _is_file_active = False
    @classmethod
    def _rpc_lock(cls):
        """Lock to protect against nested use of the write lock within one process

        """
        if cls._is_file_active:
            raise RuntimeError('Attempt to modify control lock file while file lock is held by the same process')
        cls._is_file_active = True

    @classmethod
    def _rpc_unlock(cls):
        """Unlock single process mutex

        """
        if not cls._is_file_active:
            raise RuntimeError('Unlock failed, file is not active')
        cls._is_file_active = False

    def try_lock(self, pid=None):
        """Get the PID that holds the lock or set lock

        Returns the PID that currently holds the lock.  If the user specifies
        a PID and the lock is not held by another process, then the lock will
        be assigned, and the input pid will be returned.  The user must check
        the return value to determine if a request to assign the lock was
        successful, an error is not raised if the lock is held by another PID.

        None is returned if the write lock is not held by any active session
        and the user does not specify a pid.

        Args:
            pid (int): The PID to assign the lock

        Returns:
            int: The PID of the session that holds the write lock upon return
                 or None if the write lock is not held

        """
        contents = self._read()
        if contents == '':
            if pid is not None:
                self._write(pid)
            file_pid = pid
        else:
            file_pid = int(contents)
        return file_pid

    def unlock(self, pid):
        """Release the write lock

        Release the write lock from ownership by a specified PID.  If the lock
        is not currently assigned to the specified PID, then a RuntimeError is
        raised.

        Args:
            pid (int): The PID that the lock is assigned to

        Raises:
            RuntimeError: The specified PID does not currently hold the lock

        """
        contents = self._read()
        if contents == '':
            raise RuntimeError('Lock is not held by any PID')
        else:
            file_pid = int(contents)
            if file_pid == pid:
                self._fid.truncate(0)
            else:
                raise RuntimeError(f'Lock is held by another PID: {file_pid}')

    def _read(self):
        """Read the contents of the lock file

        Rewinds the file pointer after the read.

        Returns:
           str: contents of the file

        Raises:
           RuntimeError: WriteLock object not used within a context manager

        """

        if self._fid is None:
            raise RuntimeError('The WriteLock object must be used within a context manager')
        contents = self._fid.readline(64)
        self._fid.seek(0)
        return contents

    def _write(self, pid):
        """Write a PID to a lock file

        Rewinds the file pointer after the write.

        Args:
           pid (int):  PID to write into the lock file

        """
        pid = int(pid)
        self._fid.truncate(0)
        self._fid.write(str(pid))
        self._fid.flush()
        self._fid.seek(0)
