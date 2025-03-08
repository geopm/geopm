#!/usr/bin/env python3
#
#  Copyright (c) 2015 - 2025 Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#

import unittest
from unittest import mock
import re
import os
import stat
import json
import tempfile
from pathlib import Path

with mock.patch('cffi.FFI.dlopen', return_value=mock.MagicMock()):
    from geopmdpy.system_files import ActiveSessions
    from geopmdpy.system_files import ActiveSessions
    from geopmdpy.system_files import GEOPM_SERVICE_RUN_PATH_PERM
    from geopmdpy.system_files import InvalidClientError

class TestActiveSessions(unittest.TestCase):
    json_good_example = {
        'client_pid' : 750,
        'client_uid' : 123,
        'client_gid' : 321,
        'create_time' : 42.0,
        'reference_count' : 1,
        'signals' : ['CPU_FREQUENCY_MAX_AVAIL', 'DRAM_ENERGY', 'MSR::DRAM_ENERGY_STATUS:ENERGY'],
        'controls' : ['CPU_FREQUENCY_MAX_CONTROL', 'MSR::IA32_PERFEVTSEL0:CMASK'],
        'watch_id' : 754
    }
    json_good_example_2 = {
        'client_pid' : 450,
        'client_uid' : 321,
        'client_gid' : 123,
        'create_time' : 52.0,
        'reference_count' : 1,
        'signals' : ['CPU_FREQUENCY_MAX_AVAIL', 'DRAM_ENERGY', 'MSR::DRAM_ENERGY_STATUS:ENERGY'],
        'controls' : ['CPU_FREQUENCY_MAX_CONTROL', 'MSR::IA32_PERFEVTSEL0:CMASK'],
        'watch_id' : 550
    }
    json_empty_signals_controls = {
        'client_pid' : 450,
        'client_uid' : 123,
        'client_gid' : 321,
        'create_time' : 42.0,
        'reference_count' : 1,
        'signals' : [],
        'controls' : [],
        'watch_id' : 550
    }
    json_negative_reference_count = {
        'client_pid' : 750,
        'client_uid' : 123,
        'client_gid' : 321,
        'create_time' : 42.0,
        'reference_count' : -2,
        'signals' : ['DRAM_ENERGY', 'CPU_FREQUENCY_MAX_AVAIL', 'MSR::DRAM_ENERGY_STATUS:ENERGY'],
        'controls' : ['CPU_FREQUENCY_MAX_CONTROL', 'MSR::IA32_PERFEVTSEL0:CMASK'],
        'watch_id' : 754
    }
    json_float_reference_count = {
        'client_pid' : 750,
        'client_uid' : 123,
        'client_gid' : 321,
        'create_time' : 42.0,
        'reference_count' : 2.5,
        'signals' : ['DRAM_ENERGY', 'CPU_FREQUENCY_MAX_AVAIL', 'MSR::DRAM_ENERGY_STATUS:ENERGY'],
        'controls' : ['CPU_FREQUENCY_MAX_CONTROL', 'MSR::IA32_PERFEVTSEL0:CMASK'],
        'watch_id' : 754
    }
    json_wrong_data_types = {
        'client_pid' : '450',
        'client_uid' : 123,
        'client_gid' : 321,
        'create_time' : 42.0,
        'reference_count' : 1,
        'signals' : ['DRAM_ENERGY', 'CPU_FREQUENCY_MAX_AVAIL', 'MSR::DRAM_ENERGY_STATUS:ENERGY'],
        'controls' : ['CPU_FREQUENCY_MAX_CONTROL', 'MSR::IA32_PERFEVTSEL0:CMASK'],
        'watch_id' : '550'
    }
    json_additional_properties = {
        'client_pid' : 450,
        'client_uid' : 123,
        'client_gid' : 321,
        'create_time' : 42.0,
        'reference_count' : 1,
        'signals' : ['DRAM_ENERGY', 'CPU_FREQUENCY_MAX_AVAIL', 'MSR::DRAM_ENERGY_STATUS:ENERGY'],
        'controls' : ['CPU_FREQUENCY_MAX_CONTROL', 'MSR::IA32_PERFEVTSEL0:CMASK'],
        'actuators' : ['DRAM_ENERGY', 'FREQUENCY_MIN', 'MSR::IA32_PERFEVTSEL0:CMASK'],
        'watch_id' : 550,
        'batch_id' : 450,
    }
    json_list_of_2 = [ json_good_example, json_good_example_2 ]
    json_empty_dictionary = {}
    string_empty_file = ''
    string_typos_json = '''{
        'client_pid' : 450,
        'client_uid' : 123,
        'client_gid' : 321,
        'signals',  ['DRAM_ENERGY', 'CPU_FREQUENCY_MAX_AVAIL', 'MSR::DRAM_ENERGY_STATUS:ENERGY'],
        'controls' : ['CPU_FREQUENCY_MAX_CONTROL', 'MSR::IA32_PERFEVTSEL0:CMASK'],
        'watch_id' 550,
    }
    '''
    string_c_code = '''
    #include <stdio.h>
    #include <stdlib.h>
    int main(void) {
        int i = 0;
        for (i = 0; 'Hello World'[i] != '\0'; i++)
            putchar(i['Hello World']);
        return i;
    }
    '''

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

    def check_getters(self, session, client_pid, reference_count, signals, controls, watch_id):
        self.assertIn(client_pid, session.get_clients())
        self.assertEqual(reference_count, session.get_reference_count(client_pid))
        self.assertEqual(signals, session.get_signals(client_pid))
        self.assertEqual(controls, session.get_controls(client_pid))
        self.assertEqual(watch_id, session.get_watch_id(client_pid))
        self.assertIsNone(session.get_batch_server(client_pid))

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

    def check_json_file(self, name, contents, is_valid):
        sess_path = f'{self._TEMP_DIR.name}/geopm'
        full_file_path = os.path.join(sess_path, f'session-{name}.json')
        renamed_path = f'{full_file_path}-uuid4-INVALID'

        string_contents = ''
        if type(contents) is str:
            string_contents = contents
        else:
            string_contents = json.dumps(contents)

        session_mock = mock.create_autospec(os.stat_result, spec_set=True)
        session_mock.st_ctime = 123

        try:
            client_uid = contents['client_uid']
            client_gid = contents['client_gid']
            create_time = contents['create_time']
        except (TypeError, KeyError):
            client_uid = None
            client_gid = None
            create_time = None

        with mock.patch('geopmdpy.system_files.secure_make_dirs') as mock_smd, \
             mock.patch('geopmdpy.system_files.secure_read_file', return_value=string_contents) as mock_srf, \
             mock.patch('geopmdpy.system_files.secure_make_file') as mock_smf, \
             mock.patch('os.rename', return_value=None) as mock_os_rename, \
             mock.patch('os.stat', return_value=session_mock) as mock_os_stat, \
             mock.patch('geopmdpy.system_files.ActiveSessions._is_pid_valid', return_value=True) as mock_pid_valid, \
             mock.patch('geopmdpy.system_files.ActiveSessions._get_session_path', return_value=full_file_path) as mock_get_session_path, \
             mock.patch('geopmdpy.system_files.ActiveSessions._pid_info', return_value=(client_uid, client_gid, create_time)) as mock_pid_info, \
             mock.patch('glob.glob', side_effect=[[], [full_file_path]]), \
             mock.patch('uuid.uuid4', return_value='uuid4'), \
             mock.patch('sys.stderr.write', return_value=None) as mock_err:
            act_sess = ActiveSessions(sess_path)
            mock_smd.assert_called_once_with(sess_path, perm_mode=GEOPM_SERVICE_RUN_PATH_PERM)
            mock_srf.assert_called_once_with(full_file_path)

            if is_valid:
                mock_get_session_path.assert_has_calls([mock.call('*'), mock.call(contents['client_pid'])])
                mock_os_stat.assert_called_once_with(full_file_path)
                mock_pid_valid.assert_called_once_with(contents['client_pid'], session_mock.st_ctime)
                mock_smf.assert_called_once_with(full_file_path, string_contents)
                mock_err.assert_not_called()
                mock_os_rename.assert_not_called()
                self.check_getters(act_sess, contents['client_pid'], contents['reference_count'], contents['signals'], contents['controls'], contents['watch_id'])
            else:
                mock_get_session_path.assert_called_once_with('*')

                mock_os_stat.assert_not_called()
                mock_pid_valid.assert_not_called()
                mock_smf.assert_not_called()
                mock_err.assert_called_once_with(f'Warning: <geopm-service> Invalid JSON file, unable to parse, renamed{full_file_path} to {renamed_path} and will ignore\n')
                mock_os_rename.assert_called_once_with(full_file_path, renamed_path)

    def test_creation_json(self):
        """Create various different valid and invalid JSON files

        """
        # Valid JSON file no batch server field
        self.check_json_file('good_example', self.json_good_example, True)
        # Valid JSON file with signals and controls fields are empty lists
        self.check_json_file('empty_signals_controls', self.json_empty_signals_controls, True)
        # Invalid JSON file which has a negative number for the reference_count
        self.check_json_file('negative_reference_count', self.json_negative_reference_count, False)
        # Invalid JSON file which has a floating point number for the reference_count
        self.check_json_file('float_reference_count', self.json_float_reference_count, False)
        # Invalid JSON file which has all the right fields, but the data types of the respective values are wrong.
        self.check_json_file('wrong_data_types', self.json_wrong_data_types, False)
        # Invalid JSON file which has additional extraneous fields
        self.check_json_file('additional_properties', self.json_additional_properties, False)
        # Invalid JSON file having two sessions instead of one, which is not allowed.
        self.check_json_file('list_of_2', self.json_list_of_2, False)
        # Invalid JSON file having no sessions at all!
        self.check_json_file('empty_dictionary', self.json_empty_dictionary, False)
        # Invalid string file which is an empty file
        self.check_json_file('string_empty_file', self.string_empty_file, False)
        # Invalid string file which is a JSON with typos
        self.check_json_file('string_typos_json', self.string_typos_json, False)
        # Invalid string file which is a C source code
        self.check_json_file('string_c_code', self.string_c_code, False)

    def test_load_clients(self):
        """ Create from existing clients

        Create mocks such that 2 clients are active but the service is down.
        Verify that when ActiveSessions is created, the clients are loaded
        properly.

        """
        client_pid_1 = self.json_good_example['client_pid']
        client_uid_1 = self.json_good_example['client_uid']
        client_gid_1 = self.json_good_example['client_gid']
        create_time_1 = self.json_good_example['create_time']
        refcount_1 = self.json_good_example['reference_count']
        signals_1 = self.json_good_example['signals']
        controls_1 = self.json_good_example['controls']
        watch_id_1 = self.json_good_example['watch_id']

        client_pid_2 = self.json_good_example_2['client_pid']
        client_uid_2 = self.json_good_example_2['client_uid']
        client_gid_2 = self.json_good_example_2['client_gid']
        create_time_2 = self.json_good_example_2['create_time']
        refcount_2 = self.json_good_example['reference_count']
        signals_2 = self.json_good_example_2['signals']
        controls_2 = self.json_good_example_2['controls']
        watch_id_2 = self.json_good_example_2['watch_id']

        sess_path = f'{self._TEMP_DIR.name}/geopm'
        full_file_path_1 = os.path.join(sess_path, f'session-{client_pid_1}.json')
        full_file_path_2 = os.path.join(sess_path, f'session-{client_pid_2}.json')

        with mock.patch('geopmdpy.system_files.secure_make_dirs', autospec=True, specset=True) as mock_smd, \
             mock.patch('geopmdpy.system_files.secure_make_file', autospec=True, specset=True) as mock_smf, \
             mock.patch('geopmdpy.system_files.secure_read_file', autospec=True, specset=True,
                        side_effect=[json.dumps(self.json_good_example),
                                     json.dumps(self.json_good_example_2)]) as mock_srf, \
             mock.patch('os.stat') as mock_stat, \
             mock.patch('glob.glob', side_effect=[[],[full_file_path_1, full_file_path_2]]), \
             mock.patch('geopmdpy.system_files.ActiveSessions._is_pid_valid', return_value=True) as mock_pid_valid:
            act_sess = ActiveSessions(sess_path)
            mock_smd.assert_called_once_with(sess_path, GEOPM_SERVICE_RUN_PATH_PERM)
            calls = [mock.call(full_file_path_1, json.dumps(self.json_good_example)),
                     mock.call(full_file_path_2, json.dumps(self.json_good_example_2))]
            mock_smf.assert_has_calls(calls)

            calls = [mock.call(full_file_path_1), mock.call(full_file_path_2)]
            mock_srf.assert_has_calls(calls)
            mock_stat.assert_has_calls(calls)

            with mock.patch('geopmdpy.system_files.ActiveSessions._pid_info', return_value=(client_uid_1, client_gid_1, create_time_1)) as mock_pid_info:
                self.check_getters(act_sess, client_pid_1, refcount_1, signals_1, controls_1, watch_id_1)
            with mock.patch('geopmdpy.system_files.ActiveSessions._pid_info', return_value=(client_uid_2, client_gid_2, create_time_2)) as mock_pid_info:
                self.check_getters(act_sess, client_pid_2, refcount_2, signals_2, controls_2, watch_id_2)

    def test_add_remove_client(self):
        """Add client session and remove it

        Test that adding a client is reflected in the get_clients()
        and is_client_active() methods.  Checks that the get_*()
        reflect the values passed when adding the client.  Also checks
        that when the remove_client() method is called the client PID
        is no longer returned by get_clients() and is_client_active()
        returns False.

        """
        client_pid = self.json_good_example['client_pid']
        client_uid = self.json_good_example['client_uid']
        client_gid = self.json_good_example['client_gid']
        create_time = self.json_good_example['create_time']
        refcount = self.json_good_example['reference_count']
        signals = self.json_good_example['signals']
        controls = self.json_good_example['controls']
        watch_id = self.json_good_example['watch_id']

        sess_path = f'{self._TEMP_DIR.name}/geopm'
        full_file_path = os.path.join(sess_path, f'session-{client_pid}.json')

        with mock.patch('geopmdpy.system_files.secure_make_dirs', autospec=True, specset=True) as mock_smd, \
             mock.patch('geopmdpy.system_files.secure_make_file', autospec=True, specset=True) as mock_smf, \
             mock.patch('os.remove', autospec=True, specset=True) as mock_remove, \
             mock.patch('geopmdpy.system_files.ActiveSessions._pid_info', return_value=(client_uid, client_gid, create_time)) as mock_pid_info:
            act_sess = ActiveSessions(sess_path)
            mock_smd.assert_called_once_with(sess_path, GEOPM_SERVICE_RUN_PATH_PERM)

            act_sess.add_client(client_pid, signals, controls, watch_id)
            mock_pid_info.assert_called_once_with(client_pid)
            self.assertTrue(act_sess.is_client_active(client_pid))
            self.check_getters(act_sess, client_pid, refcount, signals, controls, watch_id)
            mock_smf.assert_called_once_with(full_file_path, json.dumps(self.json_good_example))

            act_sess.remove_client(client_pid)
            self.assertNotIn(client_pid, act_sess.get_clients())
            self.assertFalse(act_sess.is_client_active(client_pid))
            mock_remove.assert_called_once_with(full_file_path)

    def test_add_remove_client_with_profiling(self):
        """Add client session with profiling and remove it

        """
        client_pid = self.json_good_example['client_pid']
        client_uid = self.json_good_example['client_uid']
        client_gid = self.json_good_example['client_gid']
        create_time = self.json_good_example['create_time']
        refcount = self.json_good_example['reference_count']
        signals = self.json_good_example['signals']
        controls = self.json_good_example['controls']
        watch_id = self.json_good_example['watch_id']
        profile_name = 'profile_test'

        sess_path = f'{self._TEMP_DIR.name}/geopm'
        full_file_path = os.path.join(sess_path, f'session-{client_pid}.json')

        with mock.patch('geopmdpy.system_files.secure_make_dirs', autospec=True, specset=True) as mock_smd, \
             mock.patch('geopmdpy.system_files.secure_make_file', autospec=True, specset=True) as mock_smf, \
             mock.patch('os.remove', autospec=True, specset=True) as mock_remove, \
             mock.patch('psutil.Process', autospec=True, spec_set=True) as mock_process, \
             mock.patch('geopmdpy.shmem.create_prof', autospec=True, specset=True) as mock_shmem_create, \
             mock.patch('geopmdpy.shmem.path_prof', autospec=True, specset=True, return_value='file_path') as mock_shmem_path, \
             mock.patch('os.unlink') as mock_unlink:

            act_sess = ActiveSessions(sess_path)
            mock_smd.assert_called_once_with(sess_path, GEOPM_SERVICE_RUN_PATH_PERM)

            instance = mock_process.return_value
            instance.uids.return_value.effective = client_uid
            instance.gids.return_value.effective = client_gid
            instance.create_time.return_value = create_time

            act_sess.add_client(client_pid, signals, controls, watch_id)
            self.assertTrue(act_sess.is_client_active(client_pid))
            self.check_getters(act_sess, client_pid, refcount, signals, controls, watch_id)
            mock_smf.assert_called_once_with(full_file_path, json.dumps(self.json_good_example))
            act_sess.start_profile(client_pid, profile_name)
            calls = [mock.call(client_pid), mock.call().uids(), mock.call().gids(), mock.call().create_time()]
            mock_process.assert_has_calls(calls)

            calls = [mock.call('status', 64 * os.cpu_count(), client_pid, client_uid, client_gid),
                     mock.call('record-log', 57384, client_pid, client_uid, client_gid)]
            mock_shmem_create.assert_has_calls(calls)
            self.assertEqual({client_pid}, act_sess.get_profile_pids(profile_name))
            updated_json_contents = dict(self.json_good_example)
            updated_json_contents.update({'profile_name' : profile_name})
            calls = [mock.call(full_file_path, json.dumps(self.json_good_example)),
                     mock.call(full_file_path, json.dumps(updated_json_contents))]
            mock_smf.assert_has_calls(calls)
            err_msg = f'Client pid {client_pid} has requested profiling twice'
            with self.assertRaisesRegex(RuntimeError, err_msg):
                act_sess.start_profile(client_pid, 'overwrite_name')

            act_sess.stop_profile(client_pid, ['test_region'])
            calls = [mock.call('record-log', client_pid, client_uid, client_gid),
                     mock.call('status', client_pid, client_uid, client_gid)]
            mock_shmem_path.assert_has_calls(calls)
            calls = [mock.call('file_path'), mock.call('file_path')]
            mock_unlink.assert_has_calls(calls)
            self.assertIsNone(act_sess.get_profile_pids(profile_name))
            self.assertEqual({'test_region'}, act_sess.pop_profile_region_names(profile_name))

            act_sess.remove_client(client_pid)
            self.assertNotIn(client_pid, act_sess.get_clients())
            self.assertFalse(act_sess.is_client_active(client_pid))
            mock_remove.assert_called_once_with(full_file_path)

    def test_batch_server(self):
        """Assign the batch server PID to a client session

        Test that when set_batch_server() is called that the result of
        get_batch_server() changes to reflect this.

        """
        json_good_example = dict(self.json_good_example)
        client_pid = json_good_example['client_pid']
        client_uid = json_good_example['client_uid']
        client_gid = json_good_example['client_gid']
        create_time = json_good_example['create_time']
        signals = json_good_example['signals']
        controls = json_good_example['controls']
        watch_id = json_good_example['watch_id']
        batch_pid = 8765

        sess_path = f'{self._TEMP_DIR.name}/geopm'
        full_file_path = os.path.join(sess_path, f'session-{client_pid}.json')

        with mock.patch('geopmdpy.system_files.secure_make_dirs', autospec=True, specset=True) as mock_smd, \
             mock.patch('geopmdpy.system_files.secure_make_file', autospec=True, specset=True) as mock_smf, \
             mock.patch('geopmdpy.system_files.ActiveSessions._pid_info', return_value=(client_uid, client_gid, create_time)) as mock_pid_info:
            act_sess = ActiveSessions(sess_path)
            mock_smd.assert_called_once_with(sess_path, GEOPM_SERVICE_RUN_PATH_PERM)

            # Add client BEFORE adding batch PID
            act_sess.add_client(client_pid, signals, controls, watch_id)
            first_smf_call = mock.call(full_file_path, json.dumps(json_good_example))
            mock_smf.assert_has_calls([first_smf_call])
            mock_pid_info.assert_called_once_with(client_pid)
            batch_server_actual = act_sess.get_batch_server(client_pid)
            self.assertEqual(None, batch_server_actual)

            # Add batch PID
            act_sess.set_batch_server(client_pid, batch_pid)
            json_good_example['batch_server'] = batch_pid
            calls = [first_smf_call,
                     mock.call(full_file_path, json.dumps(json_good_example))]
            mock_smf.assert_has_calls(calls)
            batch_pid_actual = act_sess.get_batch_server(client_pid)
            self.assertEqual(batch_pid, batch_pid_actual)

    def test_batch_server_service_restart(self):
        """Verify batch pid is returned if it was previously active

        If a batch server was running when the service was started
        it should be returned properly when requested.

        """
        # Copy the object to ensure modifications don't leak between tests
        json_good_example = dict(self.json_good_example)
        client_pid = json_good_example['client_pid']
        client_uid = json_good_example['client_uid']
        client_gid = json_good_example['client_gid']
        create_time = json_good_example['create_time']

        batch_pid = 42
        json_good_example['batch_server'] = batch_pid
        sess_path = f'{self._TEMP_DIR.name}/geopm'
        full_file_path = os.path.join(sess_path, f"session-{client_pid}.json")

        session_mock = mock.create_autospec(os.stat_result, spec_set=True)
        session_mock.st_ctime = 123

        with mock.patch('geopmdpy.system_files.secure_make_dirs', autospec=True, specset=True) as mock_smd, \
             mock.patch('geopmdpy.system_files.secure_make_file', autospec=True, specset=True) as mock_smf, \
             mock.patch('geopmdpy.system_files.secure_read_file', autospec=True, specset=True, return_value=json.dumps(json_good_example)) as mock_srf, \
             mock.patch('os.stat', return_value=session_mock) as mock_stat, \
             mock.patch('glob.glob', side_effect=[[], [full_file_path]]), \
             mock.patch('geopmdpy.system_files.ActiveSessions._is_pid_valid', return_value=True) as mock_pid_valid:
            act_sess = ActiveSessions(sess_path)
            mock_smd.assert_called_once_with(sess_path, GEOPM_SERVICE_RUN_PATH_PERM)
            mock_srf.assert_called_once_with(full_file_path)
            mock_stat.assert_called_once_with(full_file_path)

            calls = [mock.call(client_pid, session_mock.st_ctime),
                     mock.call(batch_pid, session_mock.st_ctime)]
            mock_pid_valid.assert_has_calls(calls)

            mock_smf.assert_called_once_with(full_file_path, json.dumps(json_good_example))

            self.assertEqual(batch_pid, act_sess.get_batch_server(client_pid))

    def test_batch_server_bad_service_restart(self):
        """Verify batch pid is not returned

        If a batch server was running when the service was initially
        started, but has completed while the service was not running,
        it should not be returned when the service is restarted.

        """
        # Copy the object to ensure modifications don't leak between tests
        json_good_example = dict(self.json_good_example)
        client_pid = json_good_example['client_pid']
        client_uid = json_good_example['client_uid']
        client_gid = json_good_example['client_gid']
        create_time = json_good_example['create_time']

        batch_pid = 42
        json_good_example['batch_server'] = batch_pid
        sess_path = f'{self._TEMP_DIR.name}/geopm'
        full_file_path = os.path.join(sess_path, f"session-{client_pid}.json")

        session_mock = mock.create_autospec(os.stat_result, spec_set=True)
        session_mock.st_ctime = 123

        # There are 2 calls into is_pid_valid:  This first verifies the client PID
        # against the session PID.  The second verifies the batch PID against the session PID.
        # side_effect is used so that the first call returns True (the client PID is valid) and
        # the second call returns False (the batch PID is *not* valid).
        with mock.patch('geopmdpy.system_files.secure_make_dirs', autospec=True, specset=True) as mock_smd, \
             mock.patch('geopmdpy.system_files.secure_make_file', autospec=True, specset=True) as mock_smf, \
             mock.patch('geopmdpy.system_files.secure_read_file', autospec=True, specset=True, return_value=json.dumps(json_good_example)) as mock_srf, \
             mock.patch('os.stat', return_value=session_mock) as mock_stat, \
             mock.patch('glob.glob', side_effect=[[], [full_file_path]]), \
             mock.patch('geopmdpy.system_files.ActiveSessions._is_pid_valid', side_effect=[True, False]) as mock_pid_valid:
            act_sess = ActiveSessions(sess_path)
            mock_smd.assert_called_once_with(sess_path, GEOPM_SERVICE_RUN_PATH_PERM)
            mock_srf.assert_called_once_with(full_file_path)
            mock_stat.assert_called_once_with(full_file_path)

            # In _load_session_file, the Batch PID will be verified to see if it is still valid
            json_good_example.pop('batch_server')
            mock_smf.assert_called_once_with(full_file_path, json.dumps(json_good_example))

            calls = [mock.call(client_pid, session_mock.st_ctime),
                     mock.call(batch_pid, session_mock.st_ctime)]

            mock_pid_valid.assert_has_calls(calls)
            self.assertIsNone(act_sess.get_batch_server(client_pid))

    def test_remove_batch_server(self):
        """Assign the batch server PID to a client session

        Test that when set_batch_server() is called that the result of
        get_batch_server() changes to reflect this.

        """
        json_good_example = dict(self.json_good_example)
        client_pid = json_good_example['client_pid']
        client_uid = json_good_example['client_uid']
        client_gid = json_good_example['client_gid']
        create_time = json_good_example['create_time']
        signals = json_good_example['signals']
        controls = json_good_example['controls']
        watch_id = json_good_example['watch_id']
        batch_pid = 8765

        sess_path = f'{self._TEMP_DIR.name}/geopm'
        full_file_path = os.path.join(sess_path, f'session-{client_pid}.json')

        signal_shmem_key = 'geopm-service-batch-buffer-' + str(batch_pid) + '-signal'
        control_shmem_key = 'geopm-service-batch-buffer-' + str(batch_pid) + '-control'
        read_fifo_key = 'batch-status-' + str(batch_pid) + '-in'
        write_fifo_key = 'batch-status-' + str(batch_pid) + '-out'
        signal_shmem_path = os.path.join(sess_path, signal_shmem_key)
        control_shmem_path = os.path.join(sess_path, control_shmem_key)
        read_fifo_path = os.path.join(sess_path, read_fifo_key)
        write_fifo_path = os.path.join(sess_path, write_fifo_key)

        with mock.patch('geopmdpy.system_files.secure_make_dirs', autospec=True, specset=True) as mock_smd, \
             mock.patch('geopmdpy.system_files.secure_make_file', autospec=True, specset=True) as mock_smf, \
             mock.patch('os.unlink') as os_unlink, \
             mock.patch('os.path.exists', return_value=True) as os_path_exists, \
             mock.patch('sys.stderr.write', return_value=None) as mock_err, \
             mock.patch('geopmdpy.system_files.ActiveSessions._pid_info', return_value=(client_uid, client_gid, create_time)) as mock_pid_info:
            act_sess = ActiveSessions(sess_path)
            mock_smd.assert_called_once_with(sess_path, GEOPM_SERVICE_RUN_PATH_PERM)

            # Add client BEFORE adding batch PID
            act_sess.add_client(client_pid, signals, controls, watch_id)
            first_smf_call = mock.call(full_file_path, json.dumps(json_good_example))
            mock_smf.assert_has_calls([first_smf_call])
            batch_pid_actual = act_sess.get_batch_server(client_pid)
            self.assertEqual(None, batch_pid_actual)

            # Set the batch server, add batch PID
            act_sess.set_batch_server(client_pid, batch_pid)
            json_good_example['batch_server'] = batch_pid
            calls = [first_smf_call,
                     mock.call(full_file_path, json.dumps(json_good_example))]
            mock_smf.assert_has_calls(calls)
            batch_pid_actual = act_sess.get_batch_server(client_pid)
            self.assertEqual(batch_pid, batch_pid_actual)

            # Remove the batch server
            act_sess.remove_batch_server(client_pid)
            mock_smf.assert_has_calls(calls)
            calls = [mock.call(signal_shmem_path),
                     mock.call(control_shmem_path),
                     mock.call(read_fifo_path),
                     mock.call(write_fifo_path)]
            os_path_exists.assert_has_calls(calls)
            os_unlink.assert_has_calls(calls)
            calls = [mock.call(f'Warning: <geopm-service>: {signal_shmem_path} file was left over, deleting it now.\n'),
                     mock.call(f'Warning: <geopm-service>: {control_shmem_path} file was left over, deleting it now.\n'),
                     mock.call(f'Warning: <geopm-service>: {read_fifo_path} file was left over, deleting it now.\n'),
                     mock.call(f'Warning: <geopm-service>: {write_fifo_path} file was left over, deleting it now.\n')]
            mock_err.assert_has_calls(calls)
            batch_pid_actual = act_sess.get_batch_server(client_pid)
            self.assertEqual(None, batch_pid_actual)

    def test_is_pid_valid(self):
        sess_path = f'{self._TEMP_DIR.name}/geopm'
        with mock.patch('geopmdpy.system_files.secure_make_dirs', autospec=True, specset=True) as mock_smd:
            act_sess = ActiveSessions(sess_path)
            mock_smd.assert_called_once_with(sess_path, GEOPM_SERVICE_RUN_PATH_PERM)
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
        json_good_example = dict(self.json_good_example)
        client_pid = json_good_example['client_pid']
        client_uid = json_good_example['client_uid']
        client_gid = json_good_example['client_gid']
        create_time = json_good_example['create_time']
        signals = json_good_example['signals']
        controls = json_good_example['controls']
        watch_id = json_good_example['watch_id']

        sess_path = f'{self._TEMP_DIR.name}/geopm'
        full_file_path = os.path.join(sess_path, f'session-{client_pid}.json')

        with mock.patch('geopmdpy.system_files.secure_make_dirs', autospec=True, specset=True) as mock_smd, \
             mock.patch('geopmdpy.system_files.secure_make_file', autospec=True, specset=True) as mock_smf, \
             mock.patch('geopmdpy.system_files.ActiveSessions._pid_info', return_value=(client_uid, client_gid, create_time)) as mock_pid_info:
            act_sess = ActiveSessions(sess_path)
            mock_smd.assert_called_once_with(sess_path, GEOPM_SERVICE_RUN_PATH_PERM)

            act_sess.add_client(client_pid, signals, controls, watch_id)
            calls = [mock.call(full_file_path, json.dumps(json_good_example))]
            mock_smf.assert_has_calls(calls)
            watch_id_actual = act_sess.get_watch_id(client_pid)
            self.assertEqual(watch_id, watch_id_actual)

            watch_id += 1
            json_good_example['watch_id'] = watch_id

            act_sess.set_watch_id(client_pid, watch_id)
            calls += [mock.call(full_file_path, json.dumps(json_good_example))]
            mock_smf.assert_has_calls(calls)
            watch_id_actual = act_sess.get_watch_id(client_pid)
            self.assertEqual(watch_id, watch_id_actual)

    def test_reference_count(self):
        """Test the *_reference_count()  methods

        """
        # Copy the object to ensure modifications don't leak between tests
        session_json = dict(self.json_good_example)
        client_pid = session_json['client_pid']
        client_uid = session_json['client_uid']
        client_gid = session_json['client_gid']
        create_time = session_json['create_time']
        signals = session_json['signals']
        controls = session_json['controls']
        watch_id = session_json['watch_id']

        sess_path = f'{self._TEMP_DIR.name}/geopm'
        full_file_path = os.path.join(sess_path, f'session-{client_pid}.json')

        with mock.patch('geopmdpy.system_files.secure_make_dirs', autospec=True, specset=True) as mock_smd, \
             mock.patch('geopmdpy.system_files.secure_make_file', autospec=True, specset=True) as mock_smf, \
             mock.patch('geopmdpy.system_files.ActiveSessions._pid_info', return_value=(client_uid, client_gid, create_time)) as mock_pid_info:
            act_sess = ActiveSessions(sess_path)
            mock_smd.assert_called_once_with(sess_path, GEOPM_SERVICE_RUN_PATH_PERM)

            act_sess.add_client(client_pid, signals, controls, watch_id)
            calls = [mock.call(full_file_path, json.dumps(session_json))]
            mock_smf.assert_has_calls(calls)
            mock_pid_info.assert_called_once_with(client_pid)

            reference_count_actual = act_sess.get_reference_count(client_pid)
            self.assertEqual(1, reference_count_actual)

            act_sess.set_reference_count(client_pid, 8)
            reference_count_actual = act_sess.get_reference_count(client_pid)
            self.assertEqual(8, reference_count_actual)

            act_sess.increment_reference_count(client_pid)
            reference_count_actual = act_sess.get_reference_count(client_pid)
            self.assertEqual(9, reference_count_actual)

            act_sess.decrement_reference_count(client_pid)
            reference_count_actual = act_sess.get_reference_count(client_pid)
            self.assertEqual(8, reference_count_actual)

    def test_is_client_active(self):
        """Test the is_client_active()  method

        """
        # Copy the object to ensure modifications don't leak between tests
        session_json = dict(self.json_good_example)
        client_pid = session_json['client_pid']
        client_uid = session_json['client_uid']
        client_gid = session_json['client_gid']
        create_time = session_json['create_time']
        signals = session_json['signals']
        controls = session_json['controls']
        watch_id = session_json['watch_id']

        sess_path = f'{self._TEMP_DIR.name}/geopm'
        full_file_path = os.path.join(sess_path, f'session-{client_pid}.json')

        with mock.patch('geopmdpy.system_files.secure_make_dirs', autospec=True, specset=True) as mock_smd, \
             mock.patch('geopmdpy.system_files.secure_make_file', autospec=True, specset=True) as mock_smf, \
             mock.patch('geopmdpy.system_files.ActiveSessions._pid_info', side_effect=[(client_uid, client_gid, create_time), (client_uid, client_gid, create_time), (client_uid + 1, client_gid, create_time + 1000)]) as mock_pid_info, \
             mock.patch('sys.stderr.write') as mock_stderr:
            act_sess = ActiveSessions(sess_path)
            mock_smd.assert_called_once_with(sess_path, GEOPM_SERVICE_RUN_PATH_PERM)

            act_sess.add_client(client_pid, signals, controls, watch_id)
            calls = [mock.call(full_file_path, json.dumps(session_json))]
            mock_smf.assert_has_calls(calls)
            self.assertTrue(act_sess.is_client_active(client_pid))
            exception_msg = f'Session PID {client_pid} identifying property has changed during the session: uid_orig={client_uid} uid_new={client_uid + 1} PID creation time has changed'
            with self.assertRaisesRegex(InvalidClientError, exception_msg):
                act_sess.is_client_active(client_pid)
            calls = [mock.call(client_pid), mock.call(client_pid), mock.call(client_pid)]
            mock_pid_info.assert_has_calls(calls)


if __name__ == '__main__':
    unittest.main()
