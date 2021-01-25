#!/usr/bin/env python
#
#  Copyright (c) 2015, 2016, 2017, 2018, 2019, 2020, Intel Corporation
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

import geopm_context
import geopmpy.io

import util
import geopm_test_launcher


class TestIntegration_power_governor(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        """Create launcher, execute benchmark and set up class variables.

        """
        sys.stdout.write('(' + os.path.basename(__file__).split('.')[0] +
                         '.' + cls.__name__ + ') ...')
        test_name = 'test_power_governor'
        cls._report_path = '{}.report'.format(test_name)
        cls._trace_path = '{}.trace'.format(test_name)
        cls._agent_conf_path = test_name + '-agent-config.json'
        # Clear out exception record for python 2 support
        geopmpy.error.exc_clear()
        # Set the job size parameters
        cls._num_node = 4
        num_rank = 16
        loop_count = 500
        app_conf = geopmpy.io.BenchConf(test_name + '_app.config')
        app_conf.append_region('dgemm', 8.0)
        app_conf.set_loop_count(loop_count)

        cls._agent = 'power_governor'
        cls._options = dict()
        fam, mod = geopm_test_launcher.get_platform()
        if fam == 6 and mod == 87:
            # budget for KNL
            cls._options['power_budget'] = 130
        else:
            cls._options['power_budget'] = 200
        agent_conf = geopmpy.io.AgentConf(test_name + '_agent.config', cls._agent, cls._options)

        # Create the test launcher with the above configuration
        launcher = geopm_test_launcher.TestLauncher(app_conf,
                                                    agent_conf,
                                                    cls._report_path,
                                                    cls._trace_path)
        launcher.set_num_node(cls._num_node)
        launcher.set_num_rank(num_rank)
        # Run the test application
        launcher.run(test_name)

        # Output to be reused by all tests
        cls._report = geopmpy.io.RawReport(cls._report_path)
        cls._trace = geopmpy.io.AppOutput(cls._trace_path + '*')
        cls._node_names = cls._report.host_names()

    @util.skip_unless_run_long_tests()
    def test_power_consumption(self):
        self.assertEqual(self._num_node, len(self._node_names))
        all_power_data = dict()
        for nn in self._node_names:
            tt = self._trace.get_trace_data(node_name=nn)

            first_epoch_index = tt.loc[tt['EPOCH_COUNT'] == 0][:1].index[0]
            epoch_dropped_data = tt[first_epoch_index:]  # Drop all startup data

            power_data = epoch_dropped_data.filter(regex='ENERGY')
            power_data['TIME'] = epoch_dropped_data['TIME']
            power_data = power_data.diff().dropna()
            power_data.rename(columns={'TIME': 'ELAPSED_TIME'}, inplace=True)
            power_data = power_data.loc[(power_data != 0).all(axis=1)]  # Will drop any row that is all 0's

            pkg_energy_cols = [s for s in power_data.keys() if 'ENERGY_PACKAGE' in s]
            dram_energy_cols = [s for s in power_data.keys() if 'ENERGY_DRAM' in s]
            power_data['SOCKET_POWER'] = power_data[pkg_energy_cols].sum(axis=1) / power_data['ELAPSED_TIME']

            pandas.set_option('display.width', 100)

            all_power_data[nn] = power_data

        for node_name, power_data in all_power_data.items():
            # Allow for overages of 2% at the 75th percentile.
            self.assertGreater(self._options['power_budget'] * 1.02, power_data['SOCKET_POWER'].quantile(.75))

            # TODO Checks on the maximum power computed during the run?
            # TODO Checks to see how much power was left on the table?


if __name__ == '__main__':
    unittest.main()
