#!/usr/bin/env python
#
#  Copyright (c) 2015, 2016, 2017, 2018, Intel Corporation
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
import sys
import unittest
from collections import defaultdict
from StringIO import StringIO
from analysis_helper import *


@unittest.skipIf(g_skip_analysis_test, g_skip_analysis_ex)
class TestPowerSweepAnalysis(unittest.TestCase):
    def setUp(self):
        self._name_prefix = 'prof'
        self._use_agent = True
        self._min_power = 160
        self._max_power = 200
        self._step_power = 10
        self._powers = range(self._min_power, self._max_power+self._step_power, self._step_power)
        self._config = {'launcher': 'srun',
                        'profile_prefix': self._name_prefix,
                        'output_dir': '.',
                        'verbose': True,
                        'iterations': 1,
                        'min_power': self._min_power, 'max_power': self._max_power,
                        'step_power': self._step_power}
        self._tmp_files = []
        # default mocked data for each column given power budget
        self._gen_val = {
            'count': (lambda pow: 1),
            'energy_pkg': (lambda pow: 14000.0 + pow),
            'energy_dram': (lambda pow: 2000.0),
            'frequency': (lambda pow: 1.0e9 + (self._max_power/float(pow))*1.0e9),
            'mpi_runtime': (lambda pow: 10),
            'runtime': (lambda pow: 500.0 * (1.0/pow)),
            'id': (lambda pow: 'bad')
        }

    def tearDown(self):
        for ff in self._tmp_files:
            try:
                os.remove(ff)
            except OSError:
                pass

    def make_mock_report_df(self, powers):
        version = '0.3.0'
        agent = 'power_governor'
        node_name = 'mynode'
        region_id = {
            'epoch':  '9223372036854775808',
            'dgemm':  '11396693813',
            'stream': '20779751936'
        }
        start_time = 'Tue Nov  6 08:00:00 2018'

        # for input data frame
        index_names = ['version', 'start_time', 'name', 'agent', 'node_name', 'iteration', 'region']
        numeric_cols = ['count', 'energy_pkg', 'energy_dram', 'frequency', 'mpi_runtime', 'runtime', 'id']

        regions = ['epoch', 'dgemm', 'stream']
        iterations = range(1, 4)

        input_data = {}
        for col in numeric_cols:
            input_data[col] = {}
            for pp in powers:
                prof_name = '{}_{}'.format(self._name_prefix, pp)
                for it in iterations:
                    for region in regions:
                        self._gen_val['id'] = lambda pow: region_id[region]
                        index = (version, start_time, prof_name, agent, node_name, it, region)
                        value = self._gen_val[col](pp)
                        input_data[col][index] = value

        df = pandas.DataFrame.from_dict(input_data)
        df.index.rename(index_names, inplace=True)
        return df

    def make_expected_summary_df(self, powers):
        expected_data = []
        cols = ['count', 'runtime', 'mpi_runtime', 'energy_pkg', 'energy_dram',
                'frequency']
        for pp in powers:
            row = [self._gen_val[col](pp) for col in cols]
            expected_data.append(row)
        index = pandas.Index(powers, name='power cap')
        return pandas.DataFrame(expected_data, index=index, columns=cols)

    def test_power_sweep_summary(self):
        sweep_analysis = geopmpy.analysis.PowerSweepAnalysis(**self._config)
        report_df = self.make_mock_report_df(self._powers)
        parse_output = MockAppOutput(report_df)
        result = sweep_analysis.summary_process(parse_output)
        expected_df = self.make_expected_summary_df(self._powers)

        compare_dataframe(self, expected_df, result)


if __name__ == '__main__':
    unittest.main()
