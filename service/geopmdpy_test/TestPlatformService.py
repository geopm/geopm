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

import os
import pwd
import grp
import json
import unittest
import re
from unittest import mock
import tempfile

# Patch dlopen to allow the tests to run when there is no build
with mock.patch('cffi.FFI.dlopen', return_value=mock.MagicMock()):
    from geopmdpy.service import PlatformService
    from geopmdpy.service import TopoService

class TestPlatformService(unittest.TestCase):
    def setUp(self):
        self._test_name = 'TestPlatformService'
        self._CONFIG_PATH = tempfile.TemporaryDirectory('{}_config'.format(self._test_name))
        self._VAR_PATH = tempfile.TemporaryDirectory('{}_var'.format(self._test_name))
        self._platform_service = PlatformService()
        self._platform_service._CONFIG_PATH = self._CONFIG_PATH.name
        self._platform_service._VAR_PATH = self._VAR_PATH.name
        self._platform_service._ALL_GROUPS = ['named']
        self._bad_user_id = 'GEOPM_TEST_INVALID_USER_NAME'
        self._bad_group_id = 'GEOPM_TEST_INVALID_GROUP_NAME'

    def tearDown(self):
        self._CONFIG_PATH.cleanup()
        self._VAR_PATH.cleanup()

    def test_config_parser(self):
        '''
        Tests that the parsing of the config files is resilient to
        comments and spacing oddities.
        '''
        signals_expect = ['geopm', 'signals', 'default', 'energy']
        controls_expect = ['geopm', 'controls', 'default', 'power']
        default_dir = os.path.join(self._CONFIG_PATH.name, '0.DEFAULT_ACCESS')
        os.makedirs(default_dir)
        signal_file = os.path.join(default_dir, 'allowed_signals')
        lines = """# Comment about the file contents
  geopm
energy
\tsignals\t
  # An indented comment
default


"""
        with open(signal_file, 'w') as fid:
            fid.write(lines)
        control_file = os.path.join(default_dir, 'allowed_controls')
        lines = """controls
\t# Inline comment
geopm


# Another comment
default
  power



"""
        with open(control_file, 'w') as fid:
            fid.write(lines)
        signals, controls = self._platform_service.get_group_access('')
        self.assertEqual(set(signals_expect), set(signals))
        self.assertEqual(set(controls_expect), set(controls))

    def test_get_group_access_empty(self):
        signals, controls = self._platform_service.get_group_access('')
        self.assertEqual([], signals)
        self.assertEqual([], controls)

    def test_get_group_access_invalid(self):
        err_msg = 'Linux group name cannot begin with a digit'
        with self.assertRaisesRegex(RuntimeError, err_msg):
            self._platform_service.get_group_access('1' + self._bad_group_id)
        err_msg = 'Linux group is not defined'
        with self.assertRaisesRegex(RuntimeError, err_msg):
            self._platform_service.get_group_access(self._bad_group_id)

    def test_get_group_access_default(self):
        signals_expect = ['geopm', 'signals', 'default', 'energy']
        controls_expect = ['geopm', 'controls', 'default', 'power']
        self._write_group_files_helper('', signals_expect, controls_expect)
        signals, controls = self._platform_service.get_group_access('')
        self.assertEqual(set(signals_expect), set(signals))
        self.assertEqual(set(controls_expect), set(controls))

    def test_get_group_access_named(self):
        controls_expect = ['geopm', 'controls', 'named', 'power']
        self._write_group_files_helper('named', [], controls_expect)
        signals, controls = self._platform_service.get_group_access('named')
        self.assertEqual([], signals)
        self.assertEqual(set(controls_expect), set(controls))

    def test_set_group_access_invalid(self):
        '''
        Test that errors are encountered when there are no
        allowed signals or controls.
        '''
        # Test no pio signals
        signal = ['power']
        err_msg = 'The service does not support any signals that match: "{}"'.format(signal[0])
        with mock.patch('geopmdpy.pio.signal_names', return_value=[]), \
             self.assertRaisesRegex(RuntimeError, err_msg):
            self._platform_service.set_group_access('', signal, [])

        # Test no pio signals or controls
        control = ['power']
        err_msg = 'The service does not support any controls that match: "{}"'.format(control[0])
        with mock.patch('geopmdpy.pio.signal_names', return_value=[]), \
             mock.patch('geopmdpy.pio.control_names', return_value=[]), \
             self.assertRaisesRegex(RuntimeError, err_msg):
            self._platform_service.set_group_access('', [], control)

    def _set_group_access_test_helper(self, group):
        '''
        Test that the files will be created with the proper data.
        '''
        all_signals = ['power', 'energy']
        all_controls = ['power', 'frequency']
        mock_open = mock.mock_open()
        with mock.patch('geopmdpy.pio.signal_names', return_value=all_signals),  \
             mock.patch('geopmdpy.pio.control_names', return_value=all_controls), \
             mock.patch('builtins.open', mock_open):

            self._platform_service.set_group_access(group, ['power'], ['frequency'])

            # Were the files written in the proper order?
            test_dir = os.path.join(self._CONFIG_PATH.name,
                                    '0.DEFAULT_ACCESS' if group == '' else group)
            signal_file = os.path.join(test_dir, 'allowed_signals')
            control_file = os.path.join(test_dir, 'allowed_controls')

            self.assertTrue(os.path.isdir(test_dir),
                            msg = 'Directory does not exist: {}'.format(test_dir))

            calls = [mock.call(signal_file, 'w'),
                     mock.call().__enter__(),
                     mock.call().write('power\n'),
                     mock.call().__exit__(None, None, None),
                     mock.call(control_file, 'w'),
                     mock.call().__enter__(),
                     mock.call().write('frequency\n'),
                     mock.call().__exit__(None, None, None)]
            mock_open.assert_has_calls(calls)

    def test_set_group_access_empty(self):
        self._set_group_access_test_helper('')

    def test_set_group_access_named(self):
        self._set_group_access_test_helper('named')

    def _write_group_files_helper(self, group, signals, controls):
        group_dir = os.path.join(self._CONFIG_PATH.name,
                                 '0.DEFAULT_ACCESS' if group == '' else group)
        os.makedirs(group_dir)
        signal_file = os.path.join(group_dir, 'allowed_signals')
        control_file = os.path.join(group_dir, 'allowed_controls')

        lines = list(controls)
        lines.insert(0, '')
        lines = '\n'.join(lines)
        with open(control_file, 'w') as fid:
            fid.write(lines)

        lines = list(signals)
        lines.insert(0, '')
        lines = '\n'.join(lines)
        with open(signal_file, 'w') as fid:
            fid.write(lines)

    def test_get_user_access_empty(self):
        signals, controls = self._platform_service.get_user_access('')
        self.assertEqual([], signals)
        self.assertEqual([], controls)

    def test_get_user_access_invalid(self):
        bad_user = self._bad_user_id
        err_msg = "Specified user '{}' does not exist.".format(bad_user)
        with self.assertRaisesRegex(RuntimeError, err_msg):
            self._platform_service.get_user_access(bad_user)

    def test_get_user_groups_invalid(self):
        bad_user = self._bad_user_id
        err_msg = r"getpwnam\(\): name not found: '{}'".format(bad_user)
        with self.assertRaisesRegex(KeyError, err_msg):
            self._platform_service._get_user_groups(bad_user)

        bad_gid = 999999
        fake_pwd = pwd.struct_passwd((None, None, None, bad_gid, None, '', None))
        err_msg = r"getgrgid\(\): gid not found: {}".format(bad_gid)
        with mock.patch('pwd.getpwnam', return_value=fake_pwd), \
             self.assertRaisesRegex(KeyError, err_msg):
            self._platform_service._get_user_groups(bad_user)

    def test_get_user_groups_current(self):
        expected_groups = [grp.getgrgid(gid).gr_name for gid in os.getgroups()]
        current_user = pwd.getpwuid(os.getuid()).pw_name
        groups = self._platform_service._get_user_groups(current_user)
        self.assertEqual(set(expected_groups), set(groups))

    def test_get_user_access_default(self):
        signals_default = ['frequency', 'energy']
        controls_default = ['geopm', 'controls', 'named', 'power']
        self._write_group_files_helper('', signals_default, controls_default)

        # Create a group with entries that should NOT be included the default
        signals_named = ['power', 'temperature']
        controls_named = ['frequency_uncore', 'power_uncore']
        self._write_group_files_helper('named', signals_named, controls_named)

        with mock.patch('geopmdpy.service.PlatformService._get_user_groups',
                        return_value=[]):
            signals, controls = self._platform_service.get_user_access('')

        self.assertEqual(set(signals_default), set(signals))
        self.assertEqual(set(controls_default), set(controls))

    def test_get_user_access_valid(self):
        signals_default = ['frequency', 'energy', 'power']
        controls_default = ['frequency', 'power']
        self._write_group_files_helper('', signals_default, controls_default)

        signals_named = ['power', 'temperature']
        controls_named = ['frequency_uncore', 'power_uncore']
        self._write_group_files_helper('named', signals_named, controls_named)

        # The user does not belong to this group, so these should be omitted
        signals_extra = ['extra_power', 'extra_temperature']
        controls_extra = ['extra_frequency_uncore', 'extra_power_uncore']
        self._write_group_files_helper('extra', signals_extra, controls_extra)

        signals_expect = signals_default + signals_named
        controls_expect = controls_default + controls_named

        valid_user = 'val'
        groups=['named']
        with mock.patch('geopmdpy.service.PlatformService._get_user_groups',
                        return_value=groups):
            signals, controls = self._platform_service.get_user_access(valid_user)

        self.assertEqual(set(signals_expect), set(signals))
        self.assertEqual(set(controls_expect), set(controls))

    def test_get_user_access_root(self):
        signals_default = ['frequency', 'energy', 'power']
        controls_default = ['frequency', 'power']
        self._write_group_files_helper('', signals_default, controls_default)

        signals_named = ['power', 'temperature']
        controls_named = ['frequency_uncore', 'power_uncore']
        self._write_group_files_helper('named', signals_named, controls_named)

        all_signals = signals_default + signals_named
        all_controls = controls_default + controls_named

        with mock.patch('geopmdpy.pio.signal_names', return_value=all_signals), \
             mock.patch('geopmdpy.pio.control_names', return_value=all_controls):
            signals, controls = self._platform_service.get_user_access('root')

        self.assertEqual(set(all_signals), set(signals))
        self.assertEqual(set(all_controls), set(controls))

    def test_get_signal_info(self):
        signals = ['energy', 'frequency', 'power']
        descriptions = ['desc0', 'desc1', 'desc2']
        domains = [0, 1, 2]
        infos = [(0, 0, 0), (1, 1, 1), (2, 2, 2)]

        expected_result = list(zip(signals, descriptions, domains))
        for idx in range(len(expected_result)):
            expected_result[idx] = expected_result[idx] + infos[idx]

        with mock.patch('geopmdpy.pio.signal_description', side_effect=descriptions) as mock_desc, \
             mock.patch('geopmdpy.pio.signal_domain_type', side_effect=domains) as mock_dom, \
             mock.patch('geopmdpy.pio.signal_info', side_effect=infos) as mock_inf:

            signal_info = self._platform_service.get_signal_info(signals)
            self.assertEqual(expected_result, signal_info)

            calls = [mock.call(cc) for cc in signals]
            mock_desc.assert_has_calls(calls)
            mock_dom.assert_has_calls(calls)
            mock_inf.assert_has_calls(calls)

    def test_get_control_info(self):
        controls = ['fan', 'frequency', 'power']
        descriptions = ['desc0', 'desc1', 'desc2']
        domains = [0, 1, 2]
        expected_result = list(zip(controls, descriptions, domains))

        with mock.patch('geopmdpy.pio.control_description', side_effect=descriptions) as mock_desc, \
             mock.patch('geopmdpy.pio.control_domain_type', side_effect=domains) as mock_dom:

            control_info = self._platform_service.get_control_info(controls)
            self.assertEqual(expected_result, control_info)

            calls = [mock.call(cc) for cc in controls]
            mock_desc.assert_has_calls(calls)
            mock_dom.assert_has_calls(calls)

    def test_lock_control(self):
        err_msg = 'PlatformService: Implementation incomplete'
        with self.assertRaisesRegex(NotImplementedError, err_msg):
            self._platform_service.lock_control()

    def test_unlock_control(self):
        err_msg = 'PlatformService: Implementation incomplete'
        with self.assertRaisesRegex(NotImplementedError, err_msg):
            self._platform_service.unlock_control()

    def test_open_session_invalid(self):
        client_pid = 999
        with mock.patch('geopmdpy.service.PlatformService._get_user_groups', return_value=[]), \
             mock.patch('geopmdpy.service.PlatformService._watch_client', return_value=[]):
            self._platform_service.open_session('', client_pid)

            session_file = self._platform_service._get_session_file(client_pid)
            self.assertTrue(os.path.isfile(session_file),
                            msg = 'File does not exist:{}'.format(session_file))

            err_msg = 'Session file for connecting process already exists: {}'.format(session_file)
            with self.assertRaisesRegex(RuntimeError, err_msg):
                self._platform_service.open_session('', client_pid)

    def _write_session_files_helper(self):
        signals_default = ['energy', 'frequency']
        controls_default = ['controls', 'geopm', 'named', 'power']
        self._write_group_files_helper('', signals_default, controls_default)

        client_pid = -999 # This should never exist.
        watch_id = 888
        session_data = {'client_pid': client_pid,
                        'mode': 'r',
                        'signals': signals_default,
                        'controls': controls_default,
                        'watch_id': watch_id}
        return session_data

    def test_open_session(self):
        session_data = self._write_session_files_helper()
        client_pid = session_data['client_pid']
        watch_id = session_data['watch_id']
        with mock.patch('geopmdpy.service.PlatformService._get_user_groups', return_value=[]), \
             mock.patch('geopmdpy.service.PlatformService._watch_client', return_value=watch_id):
            self._platform_service.open_session('', client_pid)
            self.assertIn(client_pid, self._platform_service._sessions)
            self.assertEqual(self._platform_service._sessions[client_pid], session_data)

            session_file = self._platform_service._get_session_file(client_pid)
            self.assertTrue(os.path.isfile(session_file),
                            msg = 'File does not exist: {}'.format(session_file))
            with open(session_file, 'r') as fid:
                parsed_session_data = json.load(fid)
            self.assertEqual(session_data, parsed_session_data)

    def test_close_session_invalid(self):
        client_pid = 999
        operation = 'PlatformCloseSession'
        err_msg = "Operation '{}' not allowed without an open session".format(operation)
        with self.assertRaisesRegex(RuntimeError, err_msg):
            self._platform_service.close_session(client_pid)

    def test_close_session_read(self):
        session_data = self._write_session_files_helper()
        client_pid = session_data['client_pid']
        watch_id = session_data['watch_id']
        self._platform_service._sessions[client_pid] = session_data

        with mock.patch('gi.repository.GLib.source_remove', return_value=[]) as gsr, \
             mock.patch('geopmdpy.pio.restore_control', return_value=[]) as prc, \
             mock.patch('os.remove', return_value=[]) as osr, \
             mock.patch('shutil.rmtree', return_value=[]) as srm:
            self._platform_service.close_session(client_pid)
            prc.assert_not_called()
            srm.assert_not_called()
            gsr.assert_called_once_with(watch_id)
            self.assertNotIn(client_pid, self._platform_service._sessions)
            session_file = self._platform_service._get_session_file(client_pid)
            osr.assert_called_once_with(session_file)

    def test_close_session_write(self):
        session_data = self._write_session_files_helper()
        client_pid = session_data['client_pid']
        watch_id = session_data['watch_id']
        self._platform_service._sessions[client_pid] = session_data
        self._platform_service._write_pid = client_pid

        with mock.patch('gi.repository.GLib.source_remove', return_value=[]) as gsr, \
             mock.patch('geopmdpy.pio.restore_control', return_value=[]) as prc, \
             mock.patch('os.remove', return_value=[]) as osr, \
             mock.patch('shutil.rmtree', return_value=[]) as srm:
            self._platform_service.close_session(client_pid)
            prc.assert_called_once()
            save_dir = os.path.join(self._platform_service._VAR_PATH,
                                    self._platform_service._SAVE_DIR)
            srm.assert_called_once_with(save_dir)
            self.assertIsNone(self._platform_service._write_pid)
            gsr.assert_called_once_with(watch_id)
            self.assertNotIn(client_pid, self._platform_service._sessions)
            session_file = self._platform_service._get_session_file(client_pid)
            osr.assert_called_once_with(session_file)

    def test_start_batch_invalid(self):
        session_data = self._write_session_files_helper()
        client_pid = session_data['client_pid']
        watch_id = session_data['watch_id']
        self._platform_service._sessions[client_pid] = session_data

        valid_signals = session_data['signals']
        signal_config = [(0, 0, sig) for sig in valid_signals]

        valid_controls = session_data['controls']
        bogus_controls = [(0, 0, 'frequency'), (0, 0, 'energy')]
        control_config = [(0, 0, con) for con in valid_controls]
        control_config.extend(bogus_controls)

        err_msg = re.escape('Requested controls that are not in allowed list: {}' \
                            .format(sorted({bc[2] for bc in bogus_controls})))
        with self.assertRaisesRegex(RuntimeError, err_msg):
            self._platform_service.start_batch(client_pid, signal_config,
                                               control_config)

        bogus_signals = [(0, 0, 'uncore'), (0, 0, 'power')]
        signal_config.extend(bogus_signals)

        err_msg = re.escape('Requested signals that are not in allowed list: {}' \
                            .format(sorted({bs[2] for bs in bogus_signals})))
        with self.assertRaisesRegex(RuntimeError, err_msg):
            self._platform_service.start_batch(client_pid, signal_config,
                                               control_config)

        # Test if a rw session is already open
        self._platform_service._write_pid = client_pid
        signal_config = [(0, 0, sig) for sig in valid_signals]
        control_config = [(0, 0, con) for con in valid_controls]
        err_msg = 'The geopm service already has a connected "rw" mode client'
        with self.assertRaisesRegex(RuntimeError, err_msg):
            self._platform_service.start_batch(client_pid, signal_config,
                                               control_config)

    def test_start_batch(self):
        session_data = self._write_session_files_helper()
        client_pid = session_data['client_pid']
        watch_id = session_data['watch_id']
        self._platform_service._sessions[client_pid] = session_data

        valid_signals = session_data['signals']
        valid_controls = session_data['controls']
        signal_config = [(0, 0, sig) for sig in valid_signals]
        control_config = [(0, 0, con) for con in valid_controls]

        err_msg = r'pio.start_batch_server\(\) is not yet implemented'
        with self.assertRaisesRegex(NotImplementedError, err_msg), \
             mock.patch('geopmdpy.pio.save_control', return_value=[]):
            self._platform_service.start_batch(client_pid, signal_config,
                                               control_config)
        # Assertions about _write_mode modifying the internal state
        self.assertEqual(self._platform_service._sessions[client_pid]['mode'],
                         'rw')

        # FIXME This is the same as it is for test_open_session.  It should be refactored in
        # the service.py into a helper.  The bit about writing the session data to JSON.
        session_file = self._platform_service._get_session_file(client_pid)
        self.assertTrue(os.path.isfile(session_file),
                        msg = 'File does not exist: {}'.format(session_file))
        with open(session_file, 'r') as fid:
            parsed_session_data = json.load(fid)
        self.assertEqual(session_data, parsed_session_data)
        # END FIXME

        self.assertEqual(self._platform_service._write_pid, client_pid)

        save_dir = os.path.join(self._platform_service._VAR_PATH,
                                self._platform_service._SAVE_DIR)
        self.assertTrue(os.path.isdir(save_dir),
                        msg = 'Directory does not exist: {}'.format(save_dir))

    def test_stop_batch_invalid(self):
        operation = 'StopBatch'
        err_msg = "Operation '{}' not allowed without an open session".format(operation)
        with self.assertRaisesRegex(RuntimeError, err_msg):
            self._platform_service.stop_batch('', '')

    def test_stop_batch(self):
        session_data = self._write_session_files_helper()
        client_pid = session_data['client_pid']
        watch_id = session_data['watch_id']
        self._platform_service._sessions[client_pid] = session_data
        server_pid = 123

        with mock.patch('geopmdpy.pio.stop_batch_server', return_value=[]) as sbd:
            self._platform_service.stop_batch(client_pid, server_pid)
            sbd.assert_called_once_with(server_pid)

    def test_read_signal_invalid(self):
        operation = 'PlatformReadSignal'
        err_msg = "Operation '{}' not allowed without an open session".format(operation)
        with self.assertRaisesRegex(RuntimeError, err_msg):
            self._platform_service.read_signal('', '', '', '')

        session_data = self._write_session_files_helper()
        client_pid = session_data['client_pid']
        self._platform_service._sessions[client_pid] = session_data

        signal_name = 'geopm'
        err_msg = 'Requested signal that is not in allowed list: {}'.format(signal_name)
        with self.assertRaisesRegex(RuntimeError, err_msg):
            self._platform_service.read_signal(client_pid, signal_name, '', '')

    def test_read_signal(self):
        session_data = self._write_session_files_helper()
        client_pid = session_data['client_pid']
        self._platform_service._sessions[client_pid] = session_data

        signal_name = 'energy'
        domain = 7
        domain_idx = 42
        with mock.patch('geopmdpy.pio.read_signal', return_value=[]) as rs:
            self._platform_service.read_signal(client_pid, signal_name, domain, domain_idx)
            rs.assert_called_once_with(signal_name, domain, domain_idx)

    def test_write_control_invalid(self):
        operation = 'PlatformWriteControl'
        err_msg = "Operation '{}' not allowed without an open session".format(operation)
        with self.assertRaisesRegex(RuntimeError, err_msg):
            self._platform_service.write_control('', '', '', '', '')

        session_data = self._write_session_files_helper()
        client_pid = session_data['client_pid']
        self._platform_service._sessions[client_pid] = session_data

        control_name = 'energy'
        err_msg = 'Requested control that is not in allowed list: {}'.format(control_name)
        with self.assertRaisesRegex(RuntimeError, err_msg):
            self._platform_service.write_control(client_pid, control_name, '', '', '')

    def test_write_control(self):
        session_data = self._write_session_files_helper()
        client_pid = session_data['client_pid']
        self._platform_service._sessions[client_pid] = session_data

        control_name = 'geopm'
        domain = 7
        domain_idx = 42
        setting = 777
        with mock.patch('geopmdpy.pio.write_control', return_value=[]) as wc, \
             mock.patch('geopmdpy.pio.save_control', return_value=[]):
            self._platform_service.write_control(client_pid, control_name, domain, domain_idx, setting)
            wc.assert_called_once_with(control_name, domain, domain_idx, setting)

    def test__read_allowed_invalid(self):
        result = self._platform_service._read_allowed('INVALID_PATH')
        self.assertEqual([], result)

    def test__check_client(self):
        result = self._platform_service._check_client('')
        self.assertTrue(result)

        session_data = self._write_session_files_helper()
        client_pid = session_data['client_pid']
        self._platform_service._sessions[client_pid] = session_data

        with mock.patch('geopmdpy.service.PlatformService.close_session',
                        return_value=[]) as pcs:
            result = self._platform_service._check_client(client_pid)
            self.assertFalse(result)
            pcs.assert_called_once_with(client_pid)

    def test_get_cache(self):
        topo = mock.MagicMock()
        topo_service = TopoService(topo=topo)

        mock_open = mock.mock_open(read_data='data')
        cache_file = '/tmp/geopm-topo-cache'
        with mock.patch('builtins.open', mock_open):
            cache_data = topo_service.get_cache()
            self.assertEqual('data', cache_data)
            topo.assert_has_calls([mock.call.create_cache()])
            calls = [mock.call(cache_file),
                     mock.call().__enter__(),
                     mock.call().read(),
                     mock.call().__exit__(None, None, None)]
            mock_open.assert_has_calls(calls)


if __name__ == '__main__':
    unittest.main()
