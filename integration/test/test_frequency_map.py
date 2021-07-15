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

import sys
import unittest
import os
import pandas
import socket

import geopm_context
import geopmpy.io
import geopmpy.hash

import util
import experiment.machine as machine
import geopm_test_launcher


class TestIntegration_frequency_map(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        """Create launcher, execute benchmark and set up class variables.

        """
        sys.stdout.write('(' + os.path.basename(__file__).split('.')[0] +
                         '.' + cls.__name__ + ') ...')
        cls._test_name = 'test_frequency_map'
        cls._report_path = '{}.report'.format(cls._test_name)
        cls._trace_path = '{}.trace'.format(cls._test_name)
        cls._agent_conf_path = cls._test_name + '-agent-config.json'
        # Set the job size parameters
        cls._num_node = 1
        num_rank = 4
        loop_count = 5
        dgemm_bigo = 15.0
        stream_bigo = 1.0
        dgemm_bigo_jlse = 35.647
        dgemm_bigo_quartz = 29.12
        stream_bigo_jlse = 1.6225
        stream_bigo_quartz = 1.7941
        hostname = socket.gethostname()
        if hostname.endswith('.alcf.anl.gov'):
            dgemm_bigo = dgemm_bigo_jlse
            stream_bigo = stream_bigo_jlse
        elif hostname.startswith('mcfly'):
            dgemm_bigo = 28.0
            stream_bigo = 1.5
        elif hostname.startswith('quartz'):
            dgemm_bigo = dgemm_bigo_quartz
            stream_bigo = stream_bigo_quartz

        app_conf = geopmpy.io.BenchConf(cls._test_name + '_app.config')
        app_conf.set_loop_count(loop_count)
        app_conf.append_region('barrier-unmarked', 1.0)
        app_conf.append_region('dgemm', dgemm_bigo)
        app_conf.append_region('barrier-unmarked', 1.0)
        app_conf.append_region('stream', stream_bigo)
        app_conf.append_region('barrier-unmarked', 1.0)
        app_conf.append_region('all2all', 1.0)
        app_conf.write()

        cls._machine = machine.Machine()
        try:
            cls._machine.load()
            sys.stderr.write('Warning: {}: using existing file "machine.json", delete if invalid\n'.format(cls._test_name))
        except RuntimeError:
            cls._machine.save()

        cls._freq_map = {}
        cls._freq_map['dgemm'] = cls._machine.frequency_min() + 2 * cls._machine.frequency_step()
        cls._freq_map['stream'] = cls._machine.frequency_sticker() - 2 * cls._machine.frequency_step()
        cls._freq_map['all2all'] = cls._machine.frequency_min()
        cls._options = cls.create_frequency_map_policy(cls._machine.frequency_sticker(),
                                                       cls._freq_map)
        cls._agent = 'frequency_map'
        trace_signals = 'REGION_HASH@core,MSR::PERF_CTL:FREQ@core'
        agent_conf = geopmpy.agent.AgentConf(cls._test_name + '_agent.config',
                                          cls._agent, cls._options)

        # Create the test launcher with the above configuration
        launcher = geopm_test_launcher.TestLauncher(app_conf=app_conf,
                                                    agent_conf=agent_conf,
                                                    report_path=cls._report_path,
                                                    trace_path=cls._trace_path,
                                                    trace_signals=trace_signals)
        launcher.set_num_node(cls._num_node)
        launcher.set_num_rank(num_rank)
        # Run the test application
        launcher.run(cls._test_name)

        # Output to be reused by all tests
        cls._report = geopmpy.io.RawReport(cls._report_path)
        cls._trace = geopmpy.io.AppOutput('{}*'.format(cls._trace_path))

    @classmethod
    def create_frequency_map_policy(cls, default_freq, frequency_map):
        """Create a frequency map to be consumed by the frequency map agent.

        Arguments:
        default_freq: Ceiling frequency for the agent
        frequency_map: Dictionary mapping region names to frequencies
        """
        policy = {'FREQ_DEFAULT': default_freq, 'FREQ_UNCORE': float('nan')}
        for i, (region_name, frequency) in enumerate(frequency_map.items()):
            policy['HASH_{}'.format(i)] = geopmpy.hash.crc32_str(region_name)
            policy['FREQ_{}'.format(i)] = frequency

        return policy

    def expected_frequency(self, target):
        """Adjust the expected average frequency across the board to account
        for unused cores to be set to sticker.

        """
        total = self._machine.num_core()
        # Note two cores are left idle per package by default
        num_unused = self._machine.num_package() * 2
        num_used = total - num_unused
        # Default frequency for the policy is sticker
        sticker = self._machine.frequency_sticker()
        return (num_used * target + num_unused * sticker) / total

    def test_agent_frequency_map(self):
        host_names = self._report.host_names()
        self.assertEqual(len(host_names), self._num_node)
        for host in host_names:
            for region_name in self._report.region_names(host):
                raw_region = self._report.raw_region(host_name=host,
                                                     region_name=region_name)
                if (region_name in ['dgemm', 'stream', 'all2all']):
                    #todo verify trace frequencies
                    #todo verify agent report augment frequecies
                    msg = region_name + ": frequency should be near assigned map frequency"
                    actual = raw_region['frequency (Hz)']
                    expect = self.expected_frequency(self._freq_map[region_name])
                    util.assertNear(self, expect, actual, msg=msg)

#    def test_agent_frequency_map_per_core(self):
#        """
#        Some preliminary interactive testing to look at traces used commands below
#        Leaving these here to help address todo's above
#        """
#        hash = [int(ii, 16) if ii != 'NAN' else None for ii in list(df['REGION_HASH-core-43'])]
#        expect = [fmap[hh] if hh in fmap.keys() else None for hh in hash]
#        fmap = {policy['HASH_{}'.format(ii)]: policy['FREQ_{}'.format(ii)] for ii in range(3)}


if __name__ == '__main__':
    unittest.main()
