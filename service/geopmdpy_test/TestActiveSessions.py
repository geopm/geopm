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

import unittest
from unittest import mock
import os
import stat
import tempfile
from pathlib import Path

from geopmdpy.varrun import ActiveSessions

class TestActiveSessions(unittest.TestCase):
    def setUp(self):
        """Create temporary directory

        """
        self._test_name = 'TestActiveSessions'
        self._TEMP_DIR = tempfile.TemporaryDirectory(self._test_name)

    def tearDown(self):
        """Clean up temporary directory

        """
        self._TEMP_DIR.cleanup()

    def check_dir_perms(self, path):
        """Assert that the path points to a file with mode 0o700
        Assert that the user and group have the right permissions.

        """
        st = os.stat(path)
        perm_mode = stat.S_IMODE(st.st_mode)
        user_owner = st.st_uid
        group_owner = st.st_gid
        self.assertEqual(0o700, perm_mode)
        self.assertEqual(os.getuid(), user_owner)
        self.assertEqual(os.getgid(), group_owner)

    def check_getters(self, session, client_pid, signals, controls, watch_id):
        self.assertIn(client_pid, session.get_clients())
        self.assertEqual(signals, session.get_signals(client_pid))
        self.assertEqual(controls, session.get_controls(client_pid))
        self.assertEqual(watch_id, session.get_watch_id(client_pid))
        self.assertFalse(session.get_batch_server(client_pid))
        self.assertFalse(session.is_write_client(client_pid))

    def create_json_file(self, directory, filename, permissions=0o600):
        """Create a json file that  matches the ActiveSessions._session_schema

        """
        json_file_contents = """{
            "client_pid" : 450,
            "mode" : "0o700",
            "signals" : ["ENERGY_DRAM", "FREQUENCY_MAX", "MSR::DRAM_ENERGY_STATUS:ENERGY"],
            "controls" : ["CPU_FREQUENCY_CONTROL", "MSR::IA32_PERFEVTSEL0:CMASK"],
            "watch_id" : 550
        }
        """
        os.makedirs(directory, exist_ok=True)
        full_path = os.path.join(directory, filename)
        file = open(os.open(full_path, os.O_CREAT | os.O_WRONLY, permissions), 'w')
        file.write(json_file_contents)
        file.close()

    def test_default_creation(self):
        """Default creation of an ActiveSessions object

        Test creates an ActiveSessions object when the geopm-service
        directory is not present.

        """
        sess_path = f'{self._TEMP_DIR.name}/geopm-service'
        act_sess = ActiveSessions(sess_path)
        self.check_dir_perms(sess_path)

    def test_creation_link_not_dir(self):
        """The path specified is a link not a directory.

        Test creates an ActiveSessions object when the geopm-service
        provided path is not a directory, but a link. It asserts that
        a warning message is printed to standard error and that a
        new directory is created while the link is renamed.

        """
        sess_path = f'{self._TEMP_DIR.name}/geopm-service-link'
        os.symlink('/root', sess_path)
        with mock.patch('os.path.islink', return_value=True), \
             mock.patch('os.path.isdir', return_value=False), \
             mock.patch('uuid.uuid4', return_value='uuid4'), \
             mock.patch('sys.stderr.write', return_value=None) as mock_err:
            act_sess = ActiveSessions(sess_path)
            renamed_path = f'{sess_path}-uuid4-INVALID'
            calls = [
                mock.call(f'Warning: <geopm-service> {sess_path} is a symbolic link, the link will be renamed to {renamed_path}'),
                mock.call(f'Warning: <geopm-service> the symbolic link points to /root')
            ]
            for cc, mc in zip(calls, mock_err.call_args_list):
                self.assertEqual(cc, mc)
        self.assertTrue(os.path.exists(renamed_path))
        self.check_dir_perms(sess_path)

    def test_creation_file_not_dir(self):
        """The path specified is a file not a directory.

        Test creates an ActiveSessions object when the geopm-service
        provided path is not a directory, but a file. It asserts that
        a warning message is printed to standard error and that a
        new directory is created while the file is renamed.

        """
        sess_path = f'{self._TEMP_DIR.name}/geopm-service.txt'
        filename = Path(sess_path)
        filename.touch(exist_ok=True)
        with mock.patch('os.path.islink', return_value=False), \
             mock.patch('os.path.isdir', return_value=False), \
             mock.patch('uuid.uuid4', return_value='uuid4'), \
             mock.patch('sys.stderr.write', return_value=None) as mock_err:
            act_sess = ActiveSessions(sess_path)
            renamed_path = f'{sess_path}-uuid4-INVALID'
            calls = [
                mock.call(f'Warning: <geopm-service> {sess_path} is not a directory, it will be renamed to {renamed_path}')
            ]
            for cc, mc in zip(calls, mock_err.call_args_list):
                self.assertEqual(cc, mc)
        self.assertTrue(os.path.exists(renamed_path))
        self.check_dir_perms(sess_path)

    def test_creation_bad_perms(self):
        """Directory exists with bad permissions

        Test creates an ActiveSessions object when the geopm-service
        directory is present with wrong permissions.  It asserts that
        a warning message is printed to standard error and that the
        permissions are changed.

        """
        sess_path = f'{self._TEMP_DIR.name}/geopm-service'
        os.mkdir(sess_path, mode=0o755)
        bad_user = mock.MagicMock()
        bad_user.st_uid = os.getuid()
        bad_user.st_gid = os.getgid()
        bad_user.st_mode = 0o755
        with mock.patch('os.stat', return_value=bad_user), \
             mock.patch('stat.S_IMODE', return_value=0o755), \
             mock.patch('os.path.islink', return_value=False), \
             mock.patch('os.path.isdir', return_value=True), \
             mock.patch('uuid.uuid4', return_value='uuid4'), \
             mock.patch('sys.stderr.write', return_value=None) as mock_err:
            act_sess = ActiveSessions(sess_path)
            renamed_path = f'{sess_path}-uuid4-INVALID'
            calls = [
                mock.call(f'Warning: <geopm-service> {sess_path} has wrong permissions, it will be renamed to {renamed_path}'),
                mock.call(f'Warning: <geopm-service> the wrong permissions were {oct(bad_user.st_mode)}')
            ]
            for cc, mc in zip(calls, mock_err.call_args_list):
                self.assertEqual(cc, mc)
        self.assertTrue(os.path.exists(renamed_path))
        self.check_dir_perms(sess_path)

    def test_creation_bad_user_owner(self):
        """Directory exists with wrong user owner

        Test creates an ActiveSessions object when the geopm-service
        directory is present with wrong ownership.  It asserts that
        a warning message is printed to standard error and that the
        ownership is changed.

        """
        sess_path = f'{self._TEMP_DIR.name}/geopm-service'
        os.mkdir(sess_path, mode=0o700)
        bad_user = mock.MagicMock()
        bad_user.st_uid = os.getuid() + 1
        bad_user.st_gid = os.getgid()
        bad_user.st_mode = 0o700
        with mock.patch('os.stat', return_value=bad_user), \
             mock.patch('stat.S_IMODE', return_value=0o700), \
             mock.patch('os.path.islink', return_value=False), \
             mock.patch('os.path.isdir', return_value=True), \
             mock.patch('uuid.uuid4', return_value='uuid4'), \
             mock.patch('sys.stderr.write', return_value=None) as mock_err:
            act_sess = ActiveSessions(sess_path)
            renamed_path = f'{sess_path}-uuid4-INVALID'
            calls = [
                mock.call(f'Warning: <geopm-service> {sess_path} has wrong user owner, it will be renamed to {renamed_path}'),
                mock.call(f'Warning: <geopm-service> the wrong user owner was {bad_user.st_uid}')
            ]
            for cc, mc in zip(calls, mock_err.call_args_list):
                self.assertEqual(cc, mc)
        self.assertTrue(os.path.exists(renamed_path))
        self.check_dir_perms(sess_path)

    def test_creation_bad_group_owner(self):
        """Directory exists with wrong group owner

        Test creates an ActiveSessions object when the geopm-service
        directory is present with wrong ownership.  It asserts that
        a warning message is printed to standard error and that the
        ownership is changed.

        """
        sess_path = f'{self._TEMP_DIR.name}/geopm-service'
        os.mkdir(sess_path, mode=0o700)
        bad_user = mock.MagicMock()
        bad_user.st_uid = os.getuid()
        bad_user.st_gid = os.getgid() + 1
        bad_user.st_mode = 0o700
        with mock.patch('os.stat', return_value=bad_user), \
             mock.patch('stat.S_IMODE', return_value=0o700), \
             mock.patch('os.path.islink', return_value=False), \
             mock.patch('os.path.isdir', return_value=True), \
             mock.patch('uuid.uuid4', return_value='uuid4'), \
             mock.patch('sys.stderr.write', return_value=None) as mock_err:
            act_sess = ActiveSessions(sess_path)
            renamed_path = f'{sess_path}-uuid4-INVALID'
            calls = [
                mock.call(f'Warning: <geopm-service> {sess_path} has wrong group owner, it will be renamed to {renamed_path}'),
                mock.call(f'Warning: <geopm-service> the wrong group owner was {bad_user.st_gid}')
            ]
            for cc, mc in zip(calls, mock_err.call_args_list):
                self.assertEqual(cc, mc)
        self.assertTrue(os.path.exists(renamed_path))
        self.check_dir_perms(sess_path)

    def test_creation_sessions(self):
        """Valid session files are present in directory

        Test creates an ActiveSessions object when the geopm-service
        directory has one JSON session files that has valid contents.
        The test calls the get_*() interfaces to verify that the files
        data reflects the contents of the valid session file.

        """
        sess_path = f'{self._TEMP_DIR.name}/geopm-service'
        self.create_json_file(sess_path, "session-1.json", 0o600)
        with mock.patch('sys.stderr.write', return_value=None) as mock_err:
            act_sess = ActiveSessions(sess_path)
            self.check_getters(
                act_sess,
                450,
                ["ENERGY_DRAM", "FREQUENCY_MAX", "MSR::DRAM_ENERGY_STATUS:ENERGY"],
                ["CPU_FREQUENCY_CONTROL", "MSR::IA32_PERFEVTSEL0:CMASK"],
                550
            )

    def test_creation_bad_session_perms(self):
        """Bad permissions on session file

        Test creates an ActiveSessions object when the geopm-service
        directory has one JSON session files,  that has permissions mode 0o644.
        The test asserts that the invalid file is not used, deleted and a
        warning message is printed.  This message should contain the
        user and group IDs of the file with invalid permissions as
        well as the file creation time.

        """
        sess_path = f'{self._TEMP_DIR.name}/geopm-service'
        self.create_json_file(sess_path, "session-2.json", 0o644)

        dir_mock = mock.MagicMock()
        dir_mock.st_uid = os.getuid()
        dir_mock.st_gid = os.getgid()
        dir_mock.st_mode = 0o700

        session_2_mock = mock.MagicMock()
        session_2_mock.st_uid = os.getuid()
        session_2_mock.st_gid = os.getgid()
        session_2_mock.st_mode = 0o644

        # os.stat() is called twice in the test with different parameters types
        # first time in __init__() it is called with a string to determine the directory
        # second time in _load_session_file() it is called with a file descriptor to determine the file
        # based on the data type we use a different mock for the os.stat()
        side_effect = lambda path: type(path) is int and session_2_mock or dir_mock

        with mock.patch('os.stat', side_effect=side_effect), \
             mock.patch('os.path.islink', return_value=False), \
             mock.patch('os.path.isdir', return_value=True), \
             mock.patch('uuid.uuid4', return_value='uuid4'), \
             mock.patch('sys.stderr.write', return_value=None) as mock_err:
            act_sess = ActiveSessions(sess_path)
            calls = [
                mock.call(f'Warning: <geopm-service> session file was discovered with invalid permissions, will be ignored and removed: {os.path.join(sess_path, "session-2.json")}'),
                mock.call(f'Warning: <geopm-service> the wrong permissions were {oct(0o644)}')
            ]
            for cc, mc in zip(calls, mock_err.call_args_list):
                self.assertEqual(cc, mc)

    def test_creation_bad_session_user_owner(self):
        """Bad user owner of session file

        Test creates an ActiveSessions object when the geopm-service
        directory has one JSON session file, that apears to be owned by a
        different user through a mock patch of os.stat().  The test
        asserts that the invalid file is not used, deleted and a
        warning message is printed.  This message should contain the
        user and group IDs of the file with invalid ownership as well
        as the file creation time.

        """
        sess_path = f'{self._TEMP_DIR.name}/geopm-service'
        self.create_json_file(sess_path, "session-3.json", 0o644)
        full_file_path = os.path.join(sess_path, "session-3.json")

        dir_mock = mock.MagicMock()
        dir_mock.st_uid = os.getuid()
        dir_mock.st_gid = os.getgid()
        dir_mock.st_mode = 0o700

        session_3_mock = mock.MagicMock()
        session_3_mock.st_uid = os.getuid() + 1
        session_3_mock.st_gid = os.getgid()
        session_3_mock.st_mode = 0o600

        # os.stat() is called twice in the test with different parameters types
        # first time in __init__() it is called with a string to determine the directory
        # second time in _load_session_file() it is called with a file descriptor to determine the file
        # based on the data type we use a different mock for the os.stat()
        side_effect = lambda path: type(path) is int and session_3_mock or dir_mock

        with mock.patch('os.stat', side_effect=side_effect), \
             mock.patch('os.path.islink', return_value=False), \
             mock.patch('os.path.isdir', return_value=True), \
             mock.patch('uuid.uuid4', return_value='uuid4'), \
             mock.patch('sys.stderr.write', return_value=None) as mock_err:
            act_sess = ActiveSessions(sess_path)
            calls = [
                mock.call(f'Warning: <geopm-service> session file was discovered with invalid permissions, will be ignored and removed: {full_file_path}'),
                mock.call(f'Warning: <geopm-service> the wrong user owner was {session_3_mock.st_uid}')
            ]
            for cc, mc in zip(calls, mock_err.call_args_list):
                self.assertEqual(cc, mc)

    def test_creation_bad_session_group_owner(self):
        """Bad group owner of session file

        Test creates an ActiveSessions object when the geopm-service
        directory has one JSON session file, that apears to be owned by a
        different group through a mock patch of os.stat().  The test
        asserts that the invalid file is not used, deleted and a
        warning message is printed.  This message should contain the
        user and group IDs of the file with invalid ownership as well
        as the file creation time.

        """
        sess_path = f'{self._TEMP_DIR.name}/geopm-service'
        self.create_json_file(sess_path, "session-4.json", 0o644)
        full_file_path = os.path.join(sess_path, "session-4.json")

        dir_mock = mock.MagicMock()
        dir_mock.st_uid = os.getuid()
        dir_mock.st_gid = os.getgid()
        dir_mock.st_mode = 0o700

        session_4_mock = mock.MagicMock()
        session_4_mock.st_uid = os.getuid()
        session_4_mock.st_gid = os.getgid() + 1
        session_4_mock.st_mode = 0o600

        # os.stat() is called twice in the test with different parameters types
        # first time in __init__() it is called with a string to determine the directory
        # second time in _load_session_file() it is called with a file descriptor to determine the file
        # based on the data type we use a different mock for the os.stat()
        side_effect = lambda path: type(path) is int and session_4_mock or dir_mock

        with mock.patch('os.stat', side_effect=side_effect), \
             mock.patch('os.path.islink', return_value=False), \
             mock.patch('os.path.isdir', return_value=True), \
             mock.patch('uuid.uuid4', return_value='uuid4'), \
             mock.patch('sys.stderr.write', return_value=None) as mock_err:
            act_sess = ActiveSessions(sess_path)
            calls = [
                mock.call(f'Warning: <geopm-service> session file was discovered with invalid permissions, will be ignored and removed: {full_file_path}'),
                mock.call(f'Warning: <geopm-service> the wrong group owner was {session_4_mock.st_gid}')
            ]
            for cc, mc in zip(calls, mock_err.call_args_list):
                self.assertEqual(cc, mc)

    def test_update_reload(self):
        """Create add clients and create again

        Create an ActiveSessions object with a non-existent directory path.
        The parent directory of said path must exist.  Add two clients to the
        object.  Create a new ActiveSessions object pointing to the same path
        and check that the new object returns the same values for the get_*()
        methods as the first object.

        """
        client_pid_1 = 1234
        signals_1 = ['INSTRUCTIONS_RETIRED', 'CPU_FREQUENCY']
        controls_1 = ['CORE_FREQUENCY', 'POWER_LIMIT']
        watch_id_1 = 4321

        client_pid_2 = 4321
        signals_2 = ['CORE_FREQUENCY', 'POWER_LIMIT']
        controls_2 = ['INSTRUCTIONS_RETIRED', 'CPU_FREQUENCY']
        watch_id_2 = 1234

        sess_path = f'{self._TEMP_DIR.name}/geopm-service'
        act_sess = ActiveSessions(sess_path)
        self.check_dir_perms(sess_path)

        act_sess.add_client(client_pid_1, signals_1, controls_1, watch_id_1)
        self.check_getters(act_sess, client_pid_1, signals_1, controls_1, watch_id_1)
        act_sess.add_client(client_pid_2, signals_2, controls_2, watch_id_2)
        self.check_getters(act_sess, client_pid_2, signals_2, controls_2, watch_id_2)

        new_act_sess = ActiveSessions(sess_path)
        self.check_getters(new_act_sess, client_pid_1, signals_1, controls_1, watch_id_1)
        self.check_getters(new_act_sess, client_pid_2, signals_2, controls_2, watch_id_2)

    def test_add_remove_client(self):
        """Add client session and remove it

        Test that adding a client is reflected in the get_clients()
        and is_client_active() methods.  Checks that the get_*()
        reflect the values passed when adding the client.  Also checks
        that when the remove_client() method is called the client PID
        is no longer returned by get_clients() and is_client_active()
        returns False.

        """
        client_pid = 1234
        signals = ['INSTRUCTIONS_RETIRED', 'CPU_FREQUENCY']
        controls = ['CORE_FREQUENCY', 'POWER_LIMIT']
        watch_id = 4321

        sess_path = f'{self._TEMP_DIR.name}/geopm-service'
        act_sess = ActiveSessions(sess_path)
        self.check_dir_perms(sess_path)

        act_sess.add_client(client_pid, signals, controls, watch_id)
        self.assertTrue(act_sess.is_client_active(client_pid))
        self.check_getters(act_sess, client_pid, signals, controls, watch_id)

        act_sess.remove_client(client_pid)
        self.assertNotIn(client_pid, act_sess.get_clients())
        self.assertFalse(act_sess.is_client_active(client_pid))

    def test_write_client(self):
        """Assign the write privileges to a client session

        Test that when set_write_client() is called that the result of
        is_write_client() changes to reflect this.  Checks that a
        second ApplicationSessions object loaded after the write
        client was assigned reflects this change when calling
        is_write_client() on the second object.

        """
        sess_path = f'{self._TEMP_DIR.name}/geopm-service'
        act_sess = ActiveSessions(sess_path)
        client_pid = 1234
        signals = ['INSTRUCTIONS_RETIRED', 'CPU_FREQUENCY']
        controls = ['CORE_FREQUENCY', 'POWER_LIMIT']
        watch_id = 4321
        act_sess.add_client(client_pid, signals, controls, watch_id)
        is_writer_actual = act_sess.is_write_client(client_pid)
        self.assertFalse(is_writer_actual)
        act_sess.set_write_client(client_pid)
        is_writer_actual = act_sess.is_write_client(client_pid)
        self.assertTrue(is_writer_actual)
        new_act_sess = ActiveSessions(sess_path)
        is_writer_actual = act_sess.is_write_client(client_pid)
        self.assertTrue(is_writer_actual)

    def test_batch_server(self):
        """Assign the batch server PID to a client session

        Test that when set_batch_server() is called that the result of
        get_batch_server() changes to reflect this.  Checks that a
        second ApplicationSessions object loaded after the batch
        server was assigned reflects this change when calling
        get_batch_server() on the second object.

        """
        sess_path = f'{self._TEMP_DIR.name}/geopm-service'
        act_sess = ActiveSessions(sess_path)
        client_pid = 1234
        signals = ['INSTRUCTIONS_RETIRED', 'CPU_FREQUENCY']
        controls = ['CORE_FREQUENCY', 'POWER_LIMIT']
        watch_id = 4321
        batch_server = 8765
        act_sess.add_client(client_pid, signals, controls, watch_id)
        batch_server_actual = act_sess.get_batch_server(client_pid)
        self.assertEqual(None, batch_server_actual)
        act_sess.set_batch_server(client_pid, batch_server)
        batch_server_actual = act_sess.get_batch_server(client_pid)
        self.assertEqual(batch_server, batch_server_actual)
        new_act_sess = ActiveSessions(sess_path)
        batch_server_actual = new_act_sess.get_batch_server(client_pid)
        self.assertEqual(batch_server, batch_server_actual)

    def test_watch_id(self):
        """Assign the watch_id to a client session

        Test that when set_watch_id() is called that the result of
        get_watch_id() changes to reflect this.

        """
        sess_path = f'{self._TEMP_DIR.name}/geopm-service'
        act_sess = ActiveSessions(sess_path)
        client_pid = 1234
        signals = ['INSTRUCTIONS_RETIRED', 'CPU_FREQUENCY']
        controls = ['CORE_FREQUENCY', 'POWER_LIMIT']
        watch_id = 4321
        act_sess.add_client(client_pid, signals, controls, watch_id)
        watch_id_actual = act_sess.get_watch_id(client_pid)
        self.assertEqual(watch_id, watch_id_actual)
        watch_id += 1
        act_sess.set_watch_id(client_pid, watch_id)
        watch_id_actual = act_sess.get_watch_id(client_pid)
        self.assertEqual(watch_id, watch_id_actual)

if __name__ == '__main__':
    unittest.main()
