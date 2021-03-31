#!/usr/bin/env python
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

import sys
import unittest
import os
import subprocess
import io
import time
import signal
from contextlib import contextmanager

import geopm_context
import geopm_test_launcher
import util


@util.skip_unless_do_launch()
class TestIntegrationGeopmio(unittest.TestCase):
    ''' Tests of geopmread and geopmwrite.'''
    def setUp(self):
        self.skip_warning_string = 'Incompatible CPU'

    def check_output(self, args, expected):
        try:
            with subprocess.Popen([self.exec_name] + args,
                                  stdout=subprocess.PIPE, stderr=subprocess.STDOUT) as proc:
                proc.wait()
                for exp in expected:
                    line = proc.stdout.readline()
                    while self.skip_warning_string.encode() in line:
                        line = proc.stdout.readline()
                    self.assertIn(exp.encode(), line)
                for line in proc.stdout:
                    if self.skip_warning_string.encode() not in line:
                        self.assertNotIn(b'Error', line)
        except subprocess.CalledProcessError as ex:
            sys.stderr.write('{} {}\n'.format(str(ex), proc.stderr.getvalue()))

    def check_output_range(self, args, min_exp, max_exp):
        read_value = geopm_test_launcher.geopmread('{}'.format(' '.join(args)))

        self.assertLessEqual(min_exp, read_value, msg="Value read for {} smaller than {}: {}.".format(args, min_exp, read_value))
        self.assertGreaterEqual(max_exp, read_value, msg="Value read for {} larger than {}: {}.".format(args, max_exp, read_value))

    def check_no_error(self, args):
        stdout = io.StringIO()
        stderr = io.StringIO()
        test_exec = 'dummy -- {} {}'.format(self.exec_name, ' '.join(args))
        try:
            geopm_test_launcher.allocation_node_test(test_exec, stdout, stderr)
        except subprocess.CalledProcessError as ex:
            sys.stderr.write('{} {}\n'.format(str(ex), stderr.getvalue()))

        for line in stderr.getvalue().splitlines():
            if self.skip_warning_string not in line:
                self.assertNotIn('Error', line)

    def test_geopmread_command_line(self):
        '''
        Check that geopmread commandline arguments work.
        '''
        self.exec_name = "geopmread"

        # no args
        self.check_no_error([])

        # domain flag
        self.check_output(['--domain'], ['board', 'package', 'core', 'cpu',
                                         'board_memory', 'package_memory',
                                         'board_nic', 'package_nic',
                                         'board_accelerator', 'package_accelerator'])
        # read signal
        self.check_no_error(['TIME', 'board', '0'])

        # info
        self.check_no_error(['--info-all'])

        # errors
        read_err = 'domain type and domain index are required'
        self.check_output(['TIME'], [read_err])
        self.check_output(['TIME', 'board'], [read_err])
        self.check_output(['TIME', 'board', 'bad'], ['invalid domain index'])
        self.check_output(['CPU_FREQUENCY_STATUS', 'package', '111'], ['cannot read signal'])
        self.check_output(['ENERGY_PACKAGE', 'cpu', '0'], ['cannot read signal'])
        self.check_output(['INVALID', 'board', '0'], ['cannot read signal'])
        self.check_output(['--domain', '--info'], ['info about domain not implemented'])

    @util.skip_unless_batch()
    def test_geopmread_all_signal_agg(self):
        '''
        Check that all reported signals can be read for board, aggregating if necessary.
        '''
        self.exec_name = "geopmread"
        stdout = io.StringIO()
        stderr = io.StringIO()
        test_exec = 'dummy -- {}'.format(self.exec_name)
        try:
            geopm_test_launcher.allocation_node_test(test_exec, stdout, stderr)
        except subprocess.CalledProcessError as ex:
            sys.stderr.write('{} {}\n'.format(str(ex), stderr.getvalue()))
            raise

        all_signals = []
        for line in stdout.getvalue().splitlines():
            if self.skip_warning_string not in line:
                all_signals.append(line.strip())

        # signals to be skipped because no application is attached.
        # See issue #1475
        skip_profile_signals = ["EPOCH_COUNT", "EPOCH::EPOCH_COUNT",
                                "REGION_HASH", "PROFILE::REGION_HASH",
                                "REGION_PROGRESS", "PROFILE::REGION_PROGRESS",
                                "REGION_HINT", "PROFILE::REGION_HINT"]
        for sig in all_signals:
            if sig not in skip_profile_signals:
                self.check_no_error([sig, 'board', '0'])

    @util.skip_unless_batch()
    def test_geopmread_signal_value(self):
        '''
        Check that some specific signals give a sane value.
        '''
        self.exec_name = "geopmread"
        signal_range = {
            "POWER_PACKAGE": (20, 400),
            "FREQUENCY": (1.0e8, 5.0e9),
            "TIME": (0, 10),  # time in sec to start geopmread
            "TEMPERATURE_CORE": (0, 100)
        }

        for signal_name, val_range in signal_range.items():
            try:
                self.check_no_error([signal_name, "board", "0"])
            except:
                raise
                pass  # skip missing signals
            else:
                self.check_output_range([signal_name, "board", "0"], *val_range)

    @util.skip_unless_batch()
    def test_geopmread_custom_msr(self):
        '''
        Check that MSRIOGroup picks up additional MSRs in path.
        '''
        self.exec_name = "geopmread"
        path = os.path.join(
           os.path.dirname(
            os.path.dirname(
             os.path.dirname(
              os.path.realpath(__file__)))),
           'examples/custom_msr/')
        custom_env = os.environ.copy()
        custom_env['GEOPM_PLUGIN_PATH'] = path
        all_signals = []
        try:
            with subprocess.Popen([self.exec_name], env=custom_env,
                                  stdout=subprocess.PIPE, stderr=subprocess.STDOUT) as proc:
                proc.wait()
                for line in proc.stdout:
                    if self.skip_warning_string.encode() not in line:
                        all_signals.append(line.strip())
        except subprocess.CalledProcessError as ex:
            sys.stderr.write('{}\n'.format(ex.output))
        self.assertIn(b'MSR::CORE_PERF_LIMIT_REASONS#', all_signals)

    def test_geopmwrite_command_line(self):
        '''
        Check that geopmwrite commandline arguments work.
        '''
        self.exec_name = "geopmwrite"

        # no args
        self.check_no_error([])

        # domain flag
        self.check_output(['--domain'], ['board', 'package', 'core', 'cpu',
                                         'board_memory', 'package_memory',
                                         'board_nic', 'package_nic',
                                         'board_accelerator', 'package_accelerator'])
        # errors
        write_err = 'domain type, domain index, and value are required'
        self.check_output(['CPU_FREQUENCY_CONTROL'], [write_err])
        self.check_output(['CPU_FREQUENCY_CONTROL', 'board'], [write_err])
        self.check_output(['CPU_FREQUENCY_CONTROL', 'board', '0'], [write_err])
        self.check_output(['CPU_FREQUENCY_CONTROL', 'board', 'bad', '0'], ['invalid domain index'])
        self.check_output(['CPU_FREQUENCY_CONTROL', 'board', '0', 'bad'], ['invalid write value'])
        self.check_output(['CPU_FREQUENCY_CONTROL', 'package', '111', '0'], ['cannot write control'])
        self.check_output(['CPU_FREQUENCY_CONTROL', 'board_nic', '0', '0'], ['cannot write control'])
        self.check_output(['INVALID', 'board', '0', '0'], ['cannot write control'])
        self.check_output(['--domain', '--info'], ['info about domain not implemented'])

    @util.skip_unless_batch()
    @util.skip_unless_stressng()
    def test_geopmwrite_set_freq(self):
        '''
        Check that geopmwrite can be used to set frequency.
        '''
        def read_current_freq(domain, signal='FREQUENCY'):
            return geopm_test_launcher.geopmread('{} {} {}'.format(signal, domain, '0'))

        def read_min_sticker_freq():
            return (geopm_test_launcher.geopmread('{} {} {}'.format('CPUINFO::FREQ_MIN', 'board', '0')),
                    geopm_test_launcher.geopmread('{} {} {}'.format('CPUINFO::FREQ_STICKER', 'board', '0')))

        def load_cpu_start():
            self.load_pid = subprocess.Popen('stress-ng --cpu=$(lscpu | grep -e "^CPU(" | cut -d: -f2 | tr -d " ")', shell=True)

        def load_cpu_stop():
            self.load_pid.send_signal(signal.SIGTERM)
            self.load_pid.communicate()

        @contextmanager
        def load_cpu():
            '''
            Context manager to put a load on every CPU on the node
            '''
            try:
                load_cpu_start()
                time.sleep(5)  # Give the load a moment to spin up
                yield
            finally:
                load_cpu_stop()

        read_domain = geopm_test_launcher.geopmread('--info CPU_FREQUENCY_STATUS')['domain']
        write_domain = geopm_test_launcher.geopmread('--info CPU_FREQUENCY_CONTROL')['domain']
        min_freq, sticker_freq = read_min_sticker_freq()

        old_freq = read_current_freq(write_domain, 'CPU_FREQUENCY_CONTROL')
        self.assertLess(old_freq, sticker_freq * 2)
        self.assertGreater(old_freq, min_freq - 1e8)

        with load_cpu():
            # Set to min and check
            geopm_test_launcher.geopmwrite('{} {} {} {}'.format('CPU_FREQUENCY_CONTROL', write_domain, '0', str(min_freq))),
            time.sleep(1)
            result = read_current_freq(read_domain)
            self.assertEqual(min_freq, result)
            # Set to sticker and check
            geopm_test_launcher.geopmwrite('{} {} {} {}'.format('CPU_FREQUENCY_CONTROL', write_domain, '0', str(sticker_freq))),
            time.sleep(1)
            result = read_current_freq(read_domain)
            self.assertEqual(sticker_freq, result)
            # Restore the original frequency
            geopm_test_launcher.geopmwrite('{} {} {} {}'.format('CPU_FREQUENCY_CONTROL', write_domain, '0', str(old_freq))),


if __name__ == '__main__':
    unittest.main()
