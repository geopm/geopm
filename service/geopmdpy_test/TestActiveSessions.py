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

        """
        st = os.stat(path)
        self.assertEqual(0o700, stat.S_IMODE(st.st_mode))

    def test_default_creation(self):
        """Default creation of an ActiveSessions object

        Test creates an ActiveSessions object when the geopm-service
        directory is not present.

        """
        sess_path = f'{self._TEMP_DIR.name}/geopm-service'
        act_sess = ActiveSessions(sess_path)
        self.check_dir_perms(sess_path)

    def test_creation_bad_perms(self):
        """Directory exists with bad permissions

        Test creates an ActiveSessions object when the geopm-service
        directory is present with wrong permissions.  It asserts that
        a warning message is printed to standard error and that the
        permissions are changed.

        """
        sess_path = f'{self._TEMP_DIR.name}/geopm-service'
        os.umask(0o000)
        os.mkdir(sess_path, mode=0o755)
        with mock.patch('sys.stderr.write', return_value=None) as mock_err:
            act_sess = ActiveSessions(sess_path)
            mock_err.assert_called_once_with(f'Warning: <geopm> {sess_path} has wrong permissions, reseting to 0o700, current value: 0o755')
        self.check_dir_perms(sess_path)

    def test_creation_bad_owner(self):
        """Directory exists with wrong owner

        Test creates an ActiveSessions object when the geopm-service
        directory is present with wrong ownership.  It asserts that
        a warning message is printed to standard error and that the
        ownership is changed.

        """
        sess_path = f'{self._TEMP_DIR.name}/geopm-service'
        os.umask(0o077)
        os.mkdir(sess_path, mode=0o700)
        bad_user = mock.MagicMock()
        bad_user.st_uid = os.getuid() + 1
        bad_user.st_gid = os.getgid()
        bad_user.st_mode = 0o600
        good_user = mock.MagicMock()
        good_user.st_uid = os.getuid()
        good_user.st_gid = os.getgid()
        good_user.st_mode = 0o600
        side_effect = lambda path: 'bad_user' in path and bad_user or good_user
        with mock.patch('os.stat', side_effect=side_effect), \
             mock.patch('stat.S_IMODE', return_value=0o600), \
             mock.patch('stat.S_ISREG', return_value=True), \
             mock.patch('sys.stderr.write', return_value=None) as mock_err:
            act_sess = ActiveSessions(sess_path)
        self.check_dir_perms(sess_path)


    def test_creation_sessions(self):
        """Valid session files are present in directory

        Test creates an ActiveSessions object when the geopm-service
        directory has two JSON session files that both have valid
        contents.  The test calls the get_*() interfaces to verify
        that the files data reflects the contents of the session
        files.

        """
        pass

    def test_creation_bad_session_perms(self):
        """Bad permissions on session file

        Test creates an ActiveSessions object when the geopm-service
        directory has two JSON session files, one that meets all
        requirements and another that has permissions mode 0o644.  The
        test asserts that the valid file is parsed properly.  The test
        also asserts that the invalid file is not used, deleted and a
        warning message is printed.  This message should contain the
        user and group IDs of the file with invalid permissions as
        well as the file creation time.

        """

        pass

    def test_creation_bad_session_owner(self):
        """Bad owner of session file

        Test creates an ActiveSessions object when the geopm-service
        directory has two JSON session files, one that meets all
        requirements and another that apears to be owned by a
        different user through a mock patch of os.stat().  The test
        asserts that the valid file is parsed properly.  The test also
        asserts that the invalid file is not used, deleted and a
        warning message is printed.  This message should contain the
        user and group IDs of the file with invalid ownership as well
        as the file creation time.

        """
        sess_path = f'{self._TEMP_DIR.name}/geopm-service'
        # TODO: Add files including one with 'bad_user' in name
        bad_user = mock.MagicMock()
        bad_user.st_uid = os.getuid() + 1
        bad_user.st_gid = os.getgid()
        bad_user.st_mode = 0o600
        good_user = mock.MagicMock()
        good_user.st_uid = os.getuid()
        good_user.st_gid = os.getgid()
        good_user.st_mode = 0o600
        side_effect = lambda path: 'bad_user' in path and bad_user or good_user
        with mock.patch('os.stat', side_effect=side_effect), \
             mock.patch('stat.S_IMODE', return_value=0o600), \
             mock.patch('stat.S_ISREG', return_value=True):
            act_sess = ActiveSessions(sess_path)
        # TODO: Add assertions

    def test_update_reload(self):
        """Create add clients and create again

        Create an ActiveSessions object with no directory present.
        Add two clients to the object.  Create a new ActiveSessions
        object pointing to the same path and check that the new object
        returns the same values for the get_*() methods as the first
        object.

        """
        pass

    def test_add_remove_client(self):
        """Add client session and remove it

        Test that adding a client is reflected in the get_clients()
        and is_client_active() methods.  Checks that the get_*()
        reflect the values passed when adding the client.  Also checks
        that when the remove_client() method is called the client PID
        is no longer returned by get_clients() and is_client_active()
        returns False.

        """
        pass

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
