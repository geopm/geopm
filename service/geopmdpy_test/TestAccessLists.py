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

from geopmdpy.varrun import ActiveSessions

# Patch dlopen to allow the tests to run when there is no build
with mock.patch('cffi.FFI.dlopen', return_value=mock.MagicMock()):
    from geopmdpy.service import PlatformService
    from geopmdpy.service import TopoService

class TestAccessLists(unittest.TestCase):
    def setUp(self):
        self._test_name = 'TestAccessLists'
        self._CONFIG_PATH = tempfile.TemporaryDirectory('{}_config'.format(self._test_name))
        #  self._platform_service._CONFIG_PATH = self._CONFIG_PATH.name

    def tearDown(self):
        self._CONFIG_PATH.cleanup()
        pass

    def _write_group_files_helper(self, group, signals, controls):
        group_dir = os.path.join(self._CONFIG_PATH.name,
                                 '0.DEFAULT_ACCESS' if group == '' else group)
        os.makedirs(group_dir, exist_ok=True)
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
        with mock.patch('geopmdpy.pio.signal_names', return_value=signals_expect), \
             mock.patch('geopmdpy.pio.control_names', return_value=controls_expect):
            signals, controls = self._platform_service.get_group_access('')
        self.assertEqual(set(signals_expect), set(signals))
        self.assertEqual(set(controls_expect), set(controls))

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
        #  self._write_group_files_helper('', signals_expect, controls_expect)
        with mock.patch('geopmdpy.pio.signal_names', return_value=signals_expect), \
             mock.patch('geopmdpy.pio.control_names', return_value=controls_expect):
            signals, controls = self._platform_service.get_group_access('')
        self.assertEqual(set(signals_expect), set(signals))
        self.assertEqual(set(controls_expect), set(controls))

    def test_get_group_access_named(self):
        signals_expect = ['geopm', 'signals', 'default', 'energy']
        controls_expect = ['geopm', 'controls', 'named', 'power']
        #  self._write_group_files_helper('named', [], controls_expect)
        with mock.patch('geopmdpy.pio.signal_names', return_value=signals_expect), \
             mock.patch('geopmdpy.pio.control_names', return_value=controls_expect):
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
        current_user = pwd.getpwuid(os.getuid()).pw_name
        expected_groups = [grp.getgrgid(gid).gr_name
                           for gid in os.getgrouplist(current_user, os.getgid())]
        groups = self._platform_service._get_user_groups(current_user)
        self.assertEqual(set(expected_groups), set(groups))

    def test_get_user_access_default(self):
        signals_default = ['frequency', 'energy']
        controls_default = ['geopm', 'controls', 'named', 'power']
        #  self._write_group_files_helper('', signals_default, controls_default)

        # Create a group with entries that should NOT be included the default
        signals_named = ['power', 'temperature']
        controls_named = ['frequency_uncore', 'power_uncore']
        #  self._write_group_files_helper('named', signals_named, controls_named)


        with mock.patch('geopmdpy.service.PlatformService._get_user_groups',
                        return_value=[]), \
             mock.patch('geopmdpy.pio.signal_names', return_value=signals_default), \
             mock.patch('geopmdpy.pio.control_names', return_value=controls_default):
            signals, controls = self._platform_service.get_user_access('')

        self.assertEqual(set(signals_default), set(signals))
        self.assertEqual(set(controls_default), set(controls))

    def test_get_user_access_valid(self):
        signals_default = ['frequency', 'energy', 'power']
        controls_default = ['frequency', 'power']
        #  self._write_group_files_helper('', signals_default, controls_default)

        signals_named = ['power', 'temperature', 'not-available-anymore']
        controls_named = ['frequency_uncore', 'power_uncore', 'not-available-anymore-either']
        #  self._write_group_files_helper('named', signals_named, controls_named)

        signals_avail = ['frequency', 'energy', 'power', 'temperature', 'extra_power',
                         'extra_temperature']
        controls_avail = ['frequency', 'power', 'frequency_uncore', 'power_uncore',
                          'extra_frequency_uncore', 'extra_power_uncore']

        # The user does not belong to this group, so these should be omitted
        signals_extra = ['extra_power', 'extra_temperature']
        controls_extra = ['extra_frequency_uncore', 'extra_power_uncore']
        #  self._write_group_files_helper('extra', signals_extra, controls_extra)

        signals_expect = ['frequency', 'energy', 'power', 'power', 'temperature']
        controls_expect = ['frequency', 'power', 'frequency_uncore', 'power_uncore']

        valid_user = 'val'
        groups=['named']
        with mock.patch('geopmdpy.service.PlatformService._get_user_groups',
                        return_value=groups), \
             mock.patch('geopmdpy.pio.signal_names', return_value=signals_avail), \
             mock.patch('geopmdpy.pio.control_names', return_value=controls_avail):
            signals, controls = self._platform_service.get_user_access(valid_user)

        self.assertEqual(set(signals_expect), set(signals))
        self.assertEqual(set(controls_expect), set(controls))

    def test_get_user_access_root(self):
        signals_default = ['frequency', 'energy', 'power']
        controls_default = ['frequency', 'power']
        #  self._write_group_files_helper('', signals_default, controls_default)

        signals_named = ['power', 'temperature']
        controls_named = ['frequency_uncore', 'power_uncore']
        #  self._write_group_files_helper('named', signals_named, controls_named)

        all_signals = signals_default + signals_named
        all_controls = controls_default + controls_named

        with mock.patch('geopmdpy.pio.signal_names', return_value=all_signals), \
             mock.patch('geopmdpy.pio.control_names', return_value=all_controls):
            signals, controls = self._platform_service.get_user_access('root')

        self.assertEqual(set(all_signals), set(signals))
        self.assertEqual(set(all_controls), set(controls))

if __name__ == '__main__':
    unittest.main()
