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
import numpy
import socket

sys.path.append(os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__)))))
from integration.test import geopm_context
from integration.test import util
import geopmpy.agent
import geopmpy.io
import geopmpy.hash
from integration.experiment import machine
import geopm_test_launcher


class AppConf(object):
    """Class that is used by the test launcher in place of a
    geopmpy.io.BenchConf when running the progress benchmark.

    """
    def write(self):
        """Called by the test launcher prior to executing the test application
        to write any files required by the application.

        """
        pass

    def get_exec_path(self):
        """Path to benchmark filled in by template automatically.

        """
        return util.get_exec_path('test_progress')

    def get_exec_args(self):
        """Returns a list of strings representing the command line arguments
        to pass to the test-application for the next run.  This is
        especially useful for tests that execute the test-application
        multiple times.

        """
        return []


class TestIntegration_progress(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        """Create launcher, execute benchmark and set up class variables.

        """
        sys.stdout.write('(' + os.path.basename(__file__).split('.')[0] +
                         '.' + cls.__name__ + ') ...')
        cls._test_name = 'test_progress'
        cls._report_path = '{}.report'.format(cls._test_name)
        cls._trace_path = '{}.trace'.format(cls._test_name)
        cls._agent_conf_path = cls._test_name + '-agent-config.json'
        # Clear out exception record for python 2 support
        geopmpy.error.exc_clear()

        # Create machine
        cls._machine = machine.try_machine('.', cls._test_name)

        # Set the job size parameters
        cls._num_node = 1
        cls._num_rank = 10
        cls._num_active_cpu = cls._machine.num_core() - 2 * cls._machine.num_package()
        cls._thread_per_rank = int(cls._num_active_cpu / cls._num_rank)

        app_conf = AppConf()

        trace_signals = 'REGION_PROGRESS@cpu,REGION_HASH@cpu'
        report_signals = 'TIME@cpu'
        agent_conf = geopmpy.agent.AgentConf(cls._test_name + '_agent.config')

        # Create the test launcher with the above configuration
        launcher = geopm_test_launcher.TestLauncher(app_conf=app_conf,
                                                    agent_conf=agent_conf,
                                                    report_path=cls._report_path,
                                                    report_signals=report_signals,
                                                    trace_path=cls._trace_path,
                                                    trace_signals=trace_signals)
        launcher.set_num_node(cls._num_node)
        launcher.set_num_rank(cls._num_rank)
        # Run the test application
        launcher.run(cls._test_name)

        # Output to be reused by all tests
        cls._report = geopmpy.io.RawReport(cls._report_path)
        cls._trace = geopmpy.io.AppOutput(cls._trace_path + '*')

    def get_hash(self, function_name):
        """Get hash value from function name.

        Return the hash value associated with the first region name in
        the report that contains the substring.

        Args:
            function_name: The substring to match

        Returns:
            A string of the hex representation of the region hash
            suitable for matching from the data frame.

        """
        host = self._report.host_names()[0]
        region_names = self._report.region_names(host)
        full_name = [rn for rn in region_names if function_name in rn]
        if len(full_name) != 1:
            raise RuntimeError("Single match not found for {}".format(function_name))
        return hex(self._report.raw_region(host, full_name[0])['hash'])

    def check_progress(self, time, progress, expected_max, msg):
        """Check a time series of progress values from a region.

        Checks that the progress contains linearly increasing values
        from zero to expected max.  Assert that a linear fit to the
        data is a good one.

        Args:
            time: A sequence of time values

            progress: A sequence of progress values

            expected_max: The expected maximum progress

            msg: Error message to include with any failures

        """
        epsilon = 0.05
        abs_epsilon = epsilon * expected_max
        # Get rid of nan values
        good_idx = numpy.isfinite(progress)
        max_prog = max(progress[good_idx])
        util.assertNear(self, expected_max, max_prog, epsilon=epsilon,
                        msg='{}: maximum value'.format(msg))
        # Get rid of values at expected_max to avoid bad fit error
        # when completed threads stop increasing progress
        good_idx = good_idx & ~numpy.isin(progress, expected_max)
        # Make sure there are at least 10 samples to over-constrain
        # the two parameter linear fit.
        self.assertLess(10, sum(good_idx))
        progress = progress[good_idx]
        time = time[good_idx]
        poly_out = numpy.polyfit(time,
                                 progress,
                                 1, full=True)
        min_prog = min(progress)
        self.assertLessEqual(0.0, min_prog)
        util.assertNear(self, 0.0, min_prog, epsilon=abs_epsilon,
                        msg='{}: minimum value'.format(msg))
        # Assert that the expected deviation from the fit is is no
        # larger than 1%.
        rr = max_prog - min_prog
        self.assertLess(0.0, poly_out[0][0])
        error = (poly_out[1][0] / len(progress)) ** 0.5 / rr
        self.assertLess(error, 0.01, msg)

    def test_num_host(self):
        """Check the number of hosts in the report

        """
        host_names = self._report.host_names()
        self.assertEqual(len(host_names), self._num_node)

    def get_cpu_with_progress(self):
        """Find all CPUs for which region progress is not NAN

        """
        df = self._trace.get_trace_df()
        result = set()
        for cpu in range(self._machine.num_cpu()):
            name = 'REGION_PROGRESS-cpu-{}'.format(cpu)
            if len(df[name].dropna()) > 0:
                result.add(cpu)
        return result

    def test_num_active_cpu(self):
        """Test that the correct number of CPUs report progress

        """
        cpu_with_progress = self.get_cpu_with_progress()
        err_msg ='CPUs with progress:\n    {}\n'.format(cpu_with_progress)
        self.assertEqual(self._num_active_cpu, len(cpu_with_progress),
                         msg=err_msg)

    def check_monotone(self, progress, err_msg):
        """Check that progress does not decrease over time.

        """
        progress = progress.dropna()
        self.assertEqual(progress[0], min(progress), err_msg)
        self.assertEqual(progress[-1], max(progress), err_msg)
        self.assertLessEqual(0, min(progress.diff()[1:]), err_msg)

    def test_linear_progress_triad(self):
        """Test that the triad region with the geopm_tprof_post() call reports
        progress that is well behaved for all CPUs and also
        collectively.

        """
        triad_post_hash = self.get_hash('triad_with_post')
        df = self._trace.get_trace_df()
        max_progress = 1.0 / self._thread_per_rank
        for cpu in self.get_cpu_with_progress():
            group_name = 'REGION_HASH-cpu-{}'.format(cpu)
            grouped_df = df.groupby(group_name)
            triad_post_df = grouped_df.get_group(triad_post_hash)
            name = 'REGION_PROGRESS-cpu-{}'.format(cpu)
            err_msg = 'Bad fit for triad cpu {} progress'.format(cpu)
            self.check_monotone(triad_post_df[name], err_msg)
            self.check_progress(triad_post_df['TIME'],
                                triad_post_df[name],
                                max_progress,
                                err_msg)

    def test_linear_progress_dgemm(self):
        """Test that the dgemm region with the calls to geopm_tprof_post()
        reports progress that is well behaved for all CPUs and also
        collectively.

        """
        dgemm_post_hash = self.get_hash('dgemm_with_post')
        df = self._trace.get_trace_df()
        max_progress = 1.0 / self._thread_per_rank
        for cpu in self.get_cpu_with_progress():
            group_name = 'REGION_HASH-cpu-{}'.format(cpu)
            grouped_df = df.groupby(group_name)
            dgemm_post_df = grouped_df.get_group(dgemm_post_hash)
            name = 'REGION_PROGRESS-cpu-{}'.format(cpu)
            err_msg = 'Bad fit for dgemm cpu {} progress'.format(cpu)
            self.check_monotone(dgemm_post_df[name], err_msg)
            self.check_progress(dgemm_post_df['TIME'],
                                dgemm_post_df[name],
                                max_progress,
                                err_msg)

    def test_zero_progress(self):
        """Test that all regions that do not contain the geopm_tprof_post()
        call report zero or no progress.

        """
        df = self._trace.get_trace_df()
        grouped_df = df.groupby('REGION_HASH')

        triad_post_hash = self.get_hash('triad_with_post')
        dgemm_post_hash = self.get_hash('dgemm_with_post')
        unmarked_hash = hex(geopmpy.hash.crc32_str('GEOPM_REGION_HASH_UNMARKED'))
        other_groups = [gg for gg in list(grouped_df.groups)
                        if gg not in (triad_post_hash,
                                      dgemm_post_hash,
                                      unmarked_hash,
                                      'NAN')]
        for gg in other_groups:
           for cpu in self.get_cpu_with_progress():
               name = 'REGION_PROGRESS-cpu-{}'.format(cpu)
               uu = grouped_df.get_group(gg)[name].dropna().unique()
               if len(uu) != 0:
                   err_msg = 'Expected all zero, group {} had {}'.format(gg, uu)
                   self.assertEqual(1, len(uu), msg=err_msg)
                   self.assertEqual(0.0, uu[0], msg=err_msg)

        self.assertLessEqual(0, min(df['REGION_PROGRESS'].dropna()))
        self.assertGreaterEqual(1.0, max(df['REGION_PROGRESS'].dropna()))

    def check_overhead(self, key, epsilon):
        """Compare the regions where geopm_tprof_post() was and was not called
        and check that the relative difference in time and energy is
        in bounds.

        Args:
            key: Name of the region to match on (e.g. 'dgemm')

            epsilon: Maximum relative overhead for time and energy

        """
        for host in self._report.host_names():
            region_names = self._report.region_names(host)
            ref_energy = None
            ref_time = None
            actual_energy = None
            actual_time = None
            for rn in region_names:
                if '{}_no_post'.format(key) in rn:
                    raw_region = self._report.raw_region(host, rn)
                    ref_energy = raw_region['package-energy (J)']
                    ref_time = raw_region['runtime (s)']
                elif '{}_with_post'.format(key) in rn:
                    raw_region = self._report.raw_region(host, rn)
                    actual_energy = raw_region['package-energy (J)']
                    actual_time = raw_region['runtime (s)']
            self.assertTrue(ref_energy is not None,
                            'Reference energy not found for {}'.format(key))
            self.assertTrue(ref_time is not None,
                            'Reference time not found for {}'.format(key))
            self.assertTrue(actual_energy is not None,
                            'Actual energy not found for {}'.format(key))
            self.assertTrue(actual_time is not None,
                            'Actual time not found for {}'.format(key))
            util.assertNear(self, ref_energy, actual_energy, epsilon=epsilon,
                            msg='Energy overhead of post for {} is too high'.format(key))
            util.assertNear(self, ref_time, actual_time, epsilon=epsilon,
                            msg='Time overhead of post for {} is too high'.format(key))

    def test_overhead_dgemm(self):
        """Test that the overhead is small for dgemm which does many
        operations for each work chunk.

        """
        self.check_overhead('dgemm', 0.05)

    def test_overhead_triad(self):
        """Test that the overhead is not huge for triad which does only 3
        operations per work chunk (FMA, add, and store).

        """
        self.check_overhead('triad', 2.0)

if __name__ == '__main__':
    unittest.main()
