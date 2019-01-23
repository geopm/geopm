#!/usr/bin/env python
#
#  Copyright (c) 2015, 2016, 2017, 2018, 2019, Intel Corporation
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
import shlex
import geopm_context
import geopmpy.launcher


class TestSubsetOptionParser(unittest.TestCase):
    def test_all_param_known(self):
        parser = geopmpy.launcher.SubsetOptionParser()
        parser.add_argument('--long-form0', dest='lf0', type=str)
        parser.add_argument('--long-form1', dest='lf1', type=str)
        parser.add_argument('-a', dest='sfa', type=str)
        parser.add_argument('-b', dest='sfb', type=str)

        opts, unparsed = parser.parse_args(['-aone', '-b', 'two', '--long-form0=three', '--long-form1', 'four'])
        self.assertEqual('one', opts.sfa)
        self.assertEqual('two', opts.sfb)
        self.assertEqual('three', opts.lf0)
        self.assertEqual('four', opts.lf1)
        self.assertEqual(0, len(unparsed))

    def test_all_param_unknown(self):
        parser = geopmpy.launcher.SubsetOptionParser()
        opts, unparsed = parser.parse_args(['-aone', '-b', 'two', '--long-form0=three', '--long-form1', 'four'])
        self.assertEqual(['-aone', '-b', 'two', '--long-form0=three', '--long-form1', 'four'], unparsed)

    def test_some_param_known(self):
        parser = geopmpy.launcher.SubsetOptionParser()
        parser.add_argument('--long-form0', dest='lf0', type=str)
        parser.add_argument('--long-form1', dest='lf1', type=str)
        parser.add_argument('-a', dest='sfa', type=str)
        parser.add_argument('-b', dest='sfb', type=str)

        opts, unparsed = parser.parse_args(['-aone', '-b' 'two', '--long-form0=three', '-c', 'five', '--long-form2=six', '--long-form1', 'four'])
        self.assertEqual('one', opts.sfa)
        self.assertEqual('two', opts.sfb)
        self.assertEqual('three', opts.lf0)
        self.assertEqual('four', opts.lf1)
        self.assertEqual(['-c', 'five', '--long-form2=six'], unparsed)

    def test_geopm_srun_mix_arg_overlap(self):
        gparser = geopmpy.launcher.SubsetOptionParser()
        gparser.add_argument('--geopm-ctl', dest='ctl', type=str)
        gparser.add_argument('--geopm-policy', dest='policy', type=str)
        gparser.add_argument('--geopm-report', dest='report', type=str)
        gparser.add_argument('--geopm-trace', dest='trace', type=str)
        gparser.add_argument('--geopm-profile', dest='profile', type=str)
        gparser.add_argument('--geopm-shmkey', dest='shmkey', type=str)
        gparser.add_argument('--geopm-timeout', dest='timeout', type=str)
        gparser.add_argument('--geopm-plugin-path', dest='plugin', type=str)
        gparser.add_argument('--geopm-debug-attach', dest='debug_attach', type=str)
        gparser.add_argument('--geopm-region-barrier', dest='barrier', action='store_true', default=False)
        gparser.add_argument('--geopm-preload', dest='preload', action='store_true', default=False)
        gparser.add_argument('-n', '--ntasks', dest='num_rank', type=int)
        sparser = geopmpy.launcher.SubsetOptionParser()
        sparser.add_argument('-N', '--nodes', dest='num_node', type=int)
        sparser.add_argument('-c', '--cpus-per-task', dest='cpu_per_rank', type=int)
        sparser.add_argument('-I', '--immediate', dest='timeout', type=int)
        sparser.add_argument('-t', '--time', dest='time_limit', type=int)
        sparser.add_argument('-J', '--job-name', dest='job_name', type=str)
        sparser.add_argument('-w', '--nodelist', dest='node_list', type=str)
        sparser.add_argument('--ntasks-per-node', dest='rank_per_node', type=int)

        argv = shlex.split('-N 2 --geopm-policy policy_file  -n100 --undef-arg --geopm-ctl=process -c 3 --immediate 5 "app_name --arg 1 -N 5 -c -w"')

        gopts, unparsed = gparser.parse_args(argv)
        sopts, unparsed = sparser.parse_args(unparsed)
        self.assertEqual(2, sopts.num_node)
        self.assertEqual('policy_file', gopts.policy)
        self.assertEqual(100, gopts.num_rank)
        self.assertEqual('process', gopts.ctl)
        self.assertEqual(3, sopts.cpu_per_rank)
        self.assertEqual(5, sopts.timeout)
        self.assertTrue('app_name --arg 1 -N 5 -c -w' in unparsed)
        self.assertTrue('--undef-arg' in unparsed)
        self.assertEqual(False, gopts.barrier)
        self.assertEqual(False, gopts.preload)

    def test_geopm_srun_mix_no_arg_overlap(self):
        gparser = geopmpy.launcher.SubsetOptionParser()
        gparser.add_argument('--geopm-ctl', dest='ctl', type=str)
        gparser.add_argument('--geopm-policy', dest='policy', type=str)
        gparser.add_argument('--geopm-report', dest='report', type=str)
        gparser.add_argument('--geopm-trace', dest='trace', type=str)
        gparser.add_argument('--geopm-profile', dest='profile', type=str)
        gparser.add_argument('--geopm-shmkey', dest='shmkey', type=str)
        gparser.add_argument('--geopm-timeout', dest='timeout', type=str)
        gparser.add_argument('--geopm-plugin-path', dest='plugin', type=str)
        gparser.add_argument('--geopm-debug-attach', dest='debug_attach', type=str)
        gparser.add_argument('--geopm-region-barrier', dest='barrier', action='store_true', default=False)
        gparser.add_argument('--geopm-preload', dest='preload', action='store_true', default=False)
        gparser.add_argument('-n', '--ntasks', dest='num_rank', type=int)
        sparser = geopmpy.launcher.SubsetOptionParser()
        sparser.add_argument('-N', '--nodes', dest='num_node', type=int)
        sparser.add_argument('-c', '--cpus-per-task', dest='cpu_per_rank', type=int)
        sparser.add_argument('-I', '--immediate', dest='timeout', type=int)
        sparser.add_argument('-t', '--time', dest='time_limit', type=int)
        sparser.add_argument('-J', '--job-name', dest='job_name', type=str)
        sparser.add_argument('-w', '--nodelist', dest='node_list', type=str)
        sparser.add_argument('--ntasks-per-node', dest='rank_per_node', type=int)

        argv = shlex.split('-N 2 --geopm-policy policy_file  -n100 --undef-arg --geopm-ctl=process -c 3 --immediate 5 -- app_name --arg 1 -A 5 -B -C')

        gopts, unparsed = gparser.parse_args(argv)
        sopts, unparsed = sparser.parse_args(unparsed)
        self.assertEqual(2, sopts.num_node)
        self.assertEqual('policy_file', gopts.policy)
        self.assertEqual(100, gopts.num_rank)
        self.assertEqual('process', gopts.ctl)
        self.assertEqual(3, sopts.cpu_per_rank)
        self.assertEqual(5, sopts.timeout)
        self.assertTrue(['app_name', '--arg', '1', '-A',  '5',  '-B', '-C'] == unparsed[-7:])
        self.assertTrue('--undef-arg' in unparsed)
        self.assertEqual(False, gopts.barrier)
        self.assertEqual(False, gopts.preload)

    def test_geopm_srun_mix_no_arg(self):
        gparser = geopmpy.launcher.SubsetOptionParser()
        gparser.add_argument('--geopm-ctl', dest='ctl', type=str)
        gparser.add_argument('--geopm-policy', dest='policy', type=str)
        gparser.add_argument('--geopm-report', dest='report', type=str)
        gparser.add_argument('--geopm-trace', dest='trace', type=str)
        gparser.add_argument('--geopm-profile', dest='profile', type=str)
        gparser.add_argument('--geopm-shmkey', dest='shmkey', type=str)
        gparser.add_argument('--geopm-timeout', dest='timeout', type=str)
        gparser.add_argument('--geopm-plugin-path', dest='plugin', type=str)
        gparser.add_argument('--geopm-debug-attach', dest='debug_attach', type=str)
        gparser.add_argument('--geopm-region-barrier', dest='barrier', action='store_true', default=False)
        gparser.add_argument('--geopm-preload', dest='preload', action='store_true', default=False)
        gparser.add_argument('-n', '--ntasks', dest='num_rank', type=int)
        sparser = geopmpy.launcher.SubsetOptionParser()
        sparser.add_argument('-N', '--nodes', dest='num_node', type=int)
        sparser.add_argument('-c', '--cpus-per-task', dest='cpu_per_rank', type=int)
        sparser.add_argument('-I', '--immediate', dest='timeout', type=int)
        sparser.add_argument('-t', '--time', dest='time_limit', type=int)
        sparser.add_argument('-J', '--job-name', dest='job_name', type=str)
        sparser.add_argument('-w', '--nodelist', dest='node_list', type=str)
        sparser.add_argument('--ntasks-per-node', dest='rank_per_node', type=int)

        argv = shlex.split('-N 2 --geopm-policy policy_file --geopm-region-barrier --geopm-preload -n100 --undef-arg --geopm-ctl=process -c 3 --immediate 5 -- app_name --arg 1 -A 5 -B -C')

        gopts, unparsed = gparser.parse_args(argv)
        sopts, unparsed = sparser.parse_args(unparsed)
        self.assertEqual(2, sopts.num_node)
        self.assertEqual('policy_file', gopts.policy)
        self.assertEqual(True, gopts.barrier)
        self.assertEqual(True, gopts.preload)
        self.assertEqual(100, gopts.num_rank)
        self.assertEqual('process', gopts.ctl)
        self.assertEqual(3, sopts.cpu_per_rank)
        self.assertEqual(5, sopts.timeout)
        self.assertTrue(['app_name', '--arg', '1', '-A',  '5',  '-B', '-C'] == unparsed[-7:])
        self.assertTrue('--undef-arg' in unparsed)


if __name__ == '__main__':
    unittest.main()
