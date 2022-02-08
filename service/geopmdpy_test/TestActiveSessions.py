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
import re
import os
import stat
import json
import tempfile
from pathlib import Path

from geopmdpy.varrun import ActiveSessions

class TestActiveSessions(unittest.TestCase):
    json_good_example = {
        "client_pid" : 750,
        "mode" : "r",
        "signals" : ["ENERGY_DRAM", "FREQUENCY_MAX", "MSR::DRAM_ENERGY_STATUS:ENERGY"],
        "controls" : ["CPU_FREQUENCY_CONTROL", "MSR::IA32_PERFEVTSEL0:CMASK"],
        "watch_id" : 754
    }
    json_good_example_2 = {
        "client_pid" : 450,
        "mode" : "r",
        "signals" : ["ENERGY_DRAM", "FREQUENCY_MAX", "MSR::DRAM_ENERGY_STATUS:ENERGY"],
        "controls" : ["CPU_FREQUENCY_CONTROL", "MSR::IA32_PERFEVTSEL0:CMASK"],
        "watch_id" : 550
    }
    json_empty_signals_controls = {
        "client_pid" : 450,
        "mode" : "r",
        "signals" : [],
        "controls" : [],
        "watch_id" : 550
    }
    json_no_mode = {
        "client_pid" : 450,
        "signals" : ["ENERGY_DRAM", "FREQUENCY_MAX", "MSR::DRAM_ENERGY_STATUS:ENERGY"],
        "controls" : ["CPU_FREQUENCY_CONTROL", "MSR::IA32_PERFEVTSEL0:CMASK"],
        "watch_id" : 550
    }
    json_bad_mode = {
        "client_pid" : 450,
        "mode" : "q",
        "signals" : ["ENERGY_DRAM", "FREQUENCY_MAX", "MSR::DRAM_ENERGY_STATUS:ENERGY"],
        "controls" : ["CPU_FREQUENCY_CONTROL", "MSR::IA32_PERFEVTSEL0:CMASK"],
        "watch_id" : 550
    }
    json_wrong_data_types = {
        "client_pid" : "450",
        "mode" : "r",
        "signals" : ["ENERGY_DRAM", "FREQUENCY_MAX", "MSR::DRAM_ENERGY_STATUS:ENERGY"],
        "controls" : ["CPU_FREQUENCY_CONTROL", "MSR::IA32_PERFEVTSEL0:CMASK"],
        "watch_id" : "550"
    }
    json_additional_properties = {
        "client_pid" : 450,
        "mode" : "r",
        "signals" : ["ENERGY_DRAM", "FREQUENCY_MAX", "MSR::DRAM_ENERGY_STATUS:ENERGY"],
        "controls" : ["CPU_FREQUENCY_CONTROL", "MSR::IA32_PERFEVTSEL0:CMASK"],
        "actuators" : ["ENERGY_DRAM", "FREQUENCY_MIN", "MSR::IA32_PERFEVTSEL0:CMASK"],
        "watch_id" : 550,
        "batch_id" : 450,
    }
    json_list_of_2 = [ json_good_example, json_good_example_2 ]
    json_empty_dictionary = {}
    string_empty_file = ""
    string_typos_json = """{
        "client_pid" : 450,
        "mode" : "r"
        "signals",  ["ENERGY_DRAM", "FREQUENCY_MAX", "MSR::DRAM_ENERGY_STATUS:ENERGY"],
        "controls" : ["CPU_FREQUENCY_CONTROL", "MSR::IA32_PERFEVTSEL0:CMASK"],
        "watch_id" 550,
    }
    """
    string_c_code = """
    #include <stdio.h>
    #include <stdlib.h>
    int main(void) {
        int i = 0;
        for (i = 0; "Hello World"[i] != '\0'; i++)
            putchar(i["Hello World"]);
        return i;
    }
    """

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
        self.assertIsNone(session.get_batch_server(client_pid))
        self.assertFalse(session.is_write_client(client_pid))

    def create_json_file(self, directory, filename, contents, permissions=0o600):
        """Create a json file

        """
        os.makedirs(directory, mode=0o700, exist_ok=True)
        full_path = os.path.join(directory, filename)
        with open(os.open(full_path, os.O_CREAT | os.O_WRONLY, permissions), 'w') as file:
            # write a string to the file
            if type(contents) is str:
                file.write(contents)
            # write a json file as a dictionary
            else:
                json.dump(contents, file)

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
            mock_err.assert_has_calls(calls)
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
            msg = f'Warning: <geopm-service> {sess_path} is not a directory, it will be renamed to {renamed_path}'
            mock_err.assert_called_once_with(msg)
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
            mock_err.assert_has_calls(calls)
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
            mock_err.assert_has_calls(calls)
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
            mock_err.assert_has_calls(calls)
        self.assertTrue(os.path.exists(renamed_path))
        self.check_dir_perms(sess_path)

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
        self.create_json_file(sess_path, "session-2.json", self.json_good_example, 0o644)
        full_file_path = os.path.join(sess_path, "session-2.json")
        renamed_path = f'{full_file_path}-uuid4-INVALID'

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
        os_stat_effect = lambda path: type(path) is int and session_2_mock or dir_mock
        # mocks os.path.isdir : if the last element of the path has a dot extension
        # then it's a file, else it's a directory.
        os_path_isdir_effect = lambda path: not re.match(".+[.].+", path.split('/')[-1])

        with mock.patch('os.stat', side_effect=os_stat_effect), \
             mock.patch('os.path.islink', return_value=False), \
             mock.patch('os.path.isdir', side_effect=os_path_isdir_effect), \
             mock.patch('stat.S_ISREG', return_value=True), \
             mock.patch('stat.S_ISLNK', return_value=False), \
             mock.patch('uuid.uuid4', return_value='uuid4'), \
             mock.patch('sys.stderr.write', return_value=None) as mock_err:
            act_sess = ActiveSessions(sess_path)
            calls = [
                mock.call(f'Warning: <geopm-service> {full_file_path} was discovered with invalid permissions, it will be renamed to {renamed_path}'),
                mock.call(f'Warning: <geopm-service> the wrong permissions were {oct(0o644)}')
            ]
            mock_err.assert_has_calls(calls)

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
        self.create_json_file(sess_path, "session-3.json", self.json_good_example, 0o644)
        full_file_path = os.path.join(sess_path, "session-3.json")
        renamed_path = f'{full_file_path}-uuid4-INVALID'

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
        os_stat_effect = lambda path: type(path) is int and session_3_mock or dir_mock
        # mocks os.path.isdir : if the last element of the path has a dot extension
        # then it's a file, else it's a directory.
        os_path_isdir_effect = lambda path: not re.match(".+[.].+", path.split('/')[-1])

        with mock.patch('os.stat', side_effect=os_stat_effect), \
             mock.patch('os.path.islink', return_value=False), \
             mock.patch('os.path.isdir', side_effect=os_path_isdir_effect), \
             mock.patch('stat.S_ISREG', return_value=True), \
             mock.patch('stat.S_ISLNK', return_value=False), \
             mock.patch('uuid.uuid4', return_value='uuid4'), \
             mock.patch('sys.stderr.write', return_value=None) as mock_err:
            act_sess = ActiveSessions(sess_path)
            calls = [
                mock.call(f'Warning: <geopm-service> {full_file_path} was discovered with invalid permissions, it will be renamed to {renamed_path}'),
                mock.call(f'Warning: <geopm-service> the wrong user owner was {session_3_mock.st_uid}')
            ]
            mock_err.assert_has_calls(calls)

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
        self.create_json_file(sess_path, "session-4.json", self.json_good_example, 0o644)
        full_file_path = os.path.join(sess_path, "session-4.json")
        renamed_path = f'{full_file_path}-uuid4-INVALID'

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
        os_stat_effect = lambda path: type(path) is int and session_4_mock or dir_mock
        # mocks os.path.isdir : if the last element of the path has a dot extension
        # then it's a file, else it's a directory.
        os_path_isdir_effect = lambda path: not re.match(".+[.].+", path.split('/')[-1])

        with mock.patch('os.stat', side_effect=os_stat_effect), \
             mock.patch('os.path.islink', return_value=False), \
             mock.patch('os.path.isdir', side_effect=os_path_isdir_effect), \
             mock.patch('stat.S_ISREG', return_value=True), \
             mock.patch('stat.S_ISLNK', return_value=False), \
             mock.patch('uuid.uuid4', return_value='uuid4'), \
             mock.patch('sys.stderr.write', return_value=None) as mock_err:
            act_sess = ActiveSessions(sess_path)
            calls = [
                mock.call(f'Warning: <geopm-service> {full_file_path} was discovered with invalid permissions, it will be renamed to {renamed_path}'),
                mock.call(f'Warning: <geopm-service> the wrong group owner was {session_4_mock.st_gid}')
            ]
            mock_err.assert_has_calls(calls)

    def check_json_file(self, name, contents, is_valid):
        sess_path = f'{self._TEMP_DIR.name}/geopm-service'
        self.create_json_file(sess_path, f"session-{name}.json", contents, 0o600)
        full_file_path = os.path.join(sess_path, f"session-{name}.json")
        renamed_path = f'{full_file_path}-uuid4-INVALID'
        with mock.patch('uuid.uuid4', return_value='uuid4'), \
             mock.patch('sys.stderr.write', return_value=None) as mock_err:
            act_sess = ActiveSessions(sess_path)
            msg = f'Warning: <geopm-service> Invalid JSON file, unable to parse, renamed{full_file_path} to {renamed_path} and will ignore'
            if is_valid:
                mock_err.assert_not_called()
                self.check_getters(
                    act_sess,
                    contents["client_pid"],
                    contents["signals"],
                    contents["controls"],
                    contents["watch_id"]
                )
            else:
                mock_err.assert_called_once_with(msg)

        if os.path.exists(full_file_path):
            os.unlink(full_file_path)
        if os.path.exists(renamed_path):
            os.unlink(renamed_path)

    def test_creation_json(self):
        """Create various different valid and invalid JSON files

        """
        # Valid JSON file no batch server field
        self.check_json_file("good_example", self.json_good_example, True)
        # Valid JSON file with signals and controls fields are empty lists
        self.check_json_file("empty_signals_controls", self.json_empty_signals_controls, True)
        # Invalid JSON file which does not have the mode field
        self.check_json_file("no_mode", self.json_no_mode, False)
        # Invalid JSON file which has a value for the mode field not "r" or "rw"
        self.check_json_file("bad_mode", self.json_bad_mode, False)
        # Invalid JSON file which has all the right fields, but the data types of the respective values are wrong.
        self.check_json_file("wrong_data_types", self.json_wrong_data_types, False)
        # Invalid JSON file which has additional extraneous fields
        self.check_json_file("additional_properties", self.json_additional_properties, False)
        # Invalid JSON file having two sessions instead of one, which is not allowed.
        self.check_json_file("list_of_2", self.json_list_of_2, False)
        # Invalid JSON file having no sessions at all!
        self.check_json_file("empty_dictionary", self.json_empty_dictionary, False)
        # Invalid string file which is an empty file
        self.check_json_file("string_empty_file", self.string_empty_file, False)
        # Invalid string file which is a JSON with typos
        self.check_json_file("string_typos_json", self.string_typos_json, False)
        # Invalid string file which is a C source code
        self.check_json_file("string_c_code", self.string_c_code, False)

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
        with mock.patch('geopmdpy.varrun.ActiveSessions._is_pid_valid', return_value=True):
            new_act_sess = ActiveSessions(sess_path)
        batch_server_actual = new_act_sess.get_batch_server(client_pid)
        self.assertEqual(batch_server, batch_server_actual)

    def test_batch_server_service_restart(self):
        """Verify batch pid is returned if it was previously active

        If a batch server was running when the service was started
        it should be returned properly when requested.

        """
        batch_pid = 42
        client_pid = self.json_good_example['client_pid']

        self.json_good_example['batch_server'] = batch_pid
        sess_path = f'{self._TEMP_DIR.name}/geopm-service'
        self.create_json_file(sess_path, 'session-1.json', self.json_good_example, 0o600)

        dir_mock = mock.create_autospec(os.stat_result, spec_set=True)
        dir_mock.st_uid = os.getuid()
        dir_mock.st_gid = os.getgid()
        dir_mock.st_mode = 0o700

        session_mock = mock.create_autospec(os.stat_result, spec_set=True)
        session_mock.st_uid = os.getuid()
        session_mock.st_gid = os.getgid()
        session_mock.st_ctime = 123
        session_mock.st_mode = 0o600 | stat.S_IFREG

        with mock.patch('os.stat', side_effect=[dir_mock, session_mock, session_mock]), \
             mock.patch('os.path.islink', return_value=False), \
             mock.patch('os.path.isdir', side_effect=[True, False]), \
             mock.patch('os.path.exists', return_value=True), \
             mock.patch('geopmdpy.varrun.ActiveSessions._is_pid_valid', return_value=True) as mock_pid_valid:
            act_sess = ActiveSessions(sess_path)
            calls = [mock.call(client_pid, session_mock.st_ctime),
                     mock.call(batch_pid, session_mock.st_ctime)]
            mock_pid_valid.assert_has_calls(calls)
        self.assertEqual(batch_pid, act_sess.get_batch_server(client_pid))

    def test_batch_server_bad_service_restart(self):
        """Verify batch pid is not returned

        If a batch server was running when the service was initially
        started, but has completed while the service was not running,
        it should not be returned when the service is restarted.

        """
        batch_pid = 42
        client_pid = self.json_good_example['client_pid']

        self.json_good_example['batch_server'] = batch_pid
        sess_path = f'{self._TEMP_DIR.name}/geopm-service'
        self.create_json_file(sess_path, 'session-1.json', self.json_good_example, 0o600)

        dir_mock = mock.create_autospec(os.stat_result, spec_set=True)
        dir_mock.st_uid = os.getuid()
        dir_mock.st_gid = os.getgid()
        dir_mock.st_mode = 0o700

        session_mock = mock.create_autospec(os.stat_result, spec_set=True)
        session_mock.st_uid = os.getuid()
        session_mock.st_gid = os.getgid()
        session_mock.st_ctime = 123
        session_mock.st_mode = 0o600 | stat.S_IFREG

        # There are 2 calls into is_pid_valid:  Ths first verifies the client PID
        # against the session PID.  The second verifies the batch PID against the session PID.
        # side_effect is used so that the first call returns True (the client PID is valid) and
        # the second call returns False (the batch PID is *not* valid).
        with mock.patch('os.stat', side_effect=[dir_mock, session_mock, session_mock]), \
             mock.patch('os.path.islink', return_value=False), \
             mock.patch('os.path.isdir', side_effect=[True, False]), \
             mock.patch('os.path.exists', return_value=True), \
             mock.patch('geopmdpy.varrun.ActiveSessions._is_pid_valid', side_effect=[True, False]) as mock_pid_valid:
            act_sess = ActiveSessions(sess_path)
            calls = [mock.call(client_pid, session_mock.st_ctime),
                     mock.call(batch_pid, session_mock.st_ctime)]
            mock_pid_valid.assert_has_calls(calls)
        self.assertIsNone(act_sess.get_batch_server(client_pid))

    def test_is_pid_valid(self):
        sess_path = f'{self._TEMP_DIR.name}/geopm-service'
        act_sess = ActiveSessions(sess_path)
        fake_pid = 321

        with mock.patch('psutil.Process', autospec=True, spec_set=True) as mock_process:
            # A psutil.Process instance is created in the real code, so the mock
            # must be done as follows:
            instance = mock_process.return_value
            instance.create_time.return_value = 123

            # Fake PID created before file_time, PID is valid.
            self.assertTrue(act_sess._is_pid_valid(fake_pid, 222))
            calls = [mock.call(fake_pid), mock.call().create_time()]
            mock_process.assert_has_calls(calls)

            # Fake PID created at file_time, PID is invalid.
            self.assertFalse(act_sess._is_pid_valid(fake_pid, 123))
            mock_process.assert_has_calls(calls * 2)

            # Fake PID created after file_time, PID is invalid.
            instance.create_time.return_value = 333
            self.assertFalse(act_sess._is_pid_valid(fake_pid, 222))
            mock_process.assert_has_calls(calls * 3)

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
