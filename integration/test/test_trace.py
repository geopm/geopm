#!/usr/bin/env python3
#
#  Copyright (c) 2015 - 2024 Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#

import sys
import unittest
import os
import glob

import geopmpy.io
import geopmpy.agent
import geopmdpy.error
import geopmdpy.topo
from integration.test import util
from integration.test import geopm_test_launcher
from integration.test import check_trace


class TestIntegration_trace(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        sys.stdout.write('(' + os.path.basename(__file__).split('.')[0] +
                         '.' + cls.__name__ + ') ...')
        cls._test_name = 'test_trace'
        cls._report_path = '{}.report'.format(cls._test_name)
        cls._trace_path_prefix = '{}.trace'.format(cls._test_name)

        cls._agent_conf_path = cls._test_name + '-agent-config.json'

        num_node = util.get_num_node()
        num_rank = num_node
        time_limit = 600
        app_conf = geopmpy.io.BenchConf(cls._test_name + '_app.config')
        app_conf.set_loop_count(1)
        app_conf.append_region('sleep', 1.0)
        app_conf.append_region('dgemm', 1.0)
        app_conf.append_region('all2all', 1.0)

        agent_conf = geopmpy.agent.AgentConf(cls._agent_conf_path)

        launcher = geopm_test_launcher.TestLauncher(app_conf,
                                                    agent_conf,
                                                    cls._report_path,
                                                    cls._trace_path_prefix,
                                                    time_limit=time_limit)
        launcher.set_num_node(num_node)
        launcher.set_num_rank(num_rank)
        geopm_args = ['--geopm-period=0.005']
        launcher.run(cls._test_name, add_geopm_args=geopm_args)

    def test_sample_rate(self):
        """ Test that the sample rate is regular and fast"""
        traces = glob.glob(self._trace_path_prefix + "*")
        if len(traces) == 0:
            raise RuntimeError("No traces found with prefix: {}".format(self._trace_path_prefix))
        for tt in traces:
            check_trace.check_sample_rate(tt, 0.005)

    def test_trace_runtimes(self):
        """Test that the sync-runtime matches the runtime from the trace."""
        report_output = geopmpy.io.RawReport(self._report_path)
        trace_output = geopmpy.io.AppOutput(traces=self._trace_path_prefix + '*')
        node_names = report_output.host_names()
        for nn in node_names:
            trace = trace_output.get_trace_data(node_name=nn)
            # Check that app total runtime matches the timestamps from trace
            app_totals = report_output.raw_totals(host_name=nn)
            util.assertNear(self, trace.iloc[-1]['TIME'] - trace.iloc[0]['TIME'],
                            app_totals['sync-runtime (s)'],
                            msg='Application runtime failure, node_name={}.'.format(nn))
            # Calculate runtime totals for each region in each trace, compare to report
            tt = trace.reset_index(level='index')  # move 'index' field from multiindex to columns
            tt = tt.set_index(['REGION_HASH'], append=True)  # add region_hash column to multiindex
            tt_reg = tt.groupby(level=['REGION_HASH'])
            regions = report_output.region_names(host_name=nn)
            for region_name in regions:
                region_data = report_output.raw_region(host_name=nn, region_name=region_name)
                if region_data['sync-runtime (s)'] != 0:
                    region_hash = '0x{:08x}'.format(region_data['hash'])
                    trace_data = tt_reg.get_group(region_hash)
                    start_idx = trace_data.iloc[0]['index']
                    end_idx = trace_data.iloc[-1]['index'] + 1  # use time from sample after exiting region
                    start_time = tt.loc[tt['index'] == start_idx]['TIME'].item()
                    end_time = tt.loc[tt['index'] == end_idx]['TIME'].item()
                    trace_elapsed_time = end_time - start_time
                    msg = 'for region {rn} on node {nn}'.format(rn=region_name, nn=nn)
                    util.assertNear(self, trace_elapsed_time, region_data['sync-runtime (s)'], msg=msg)
            # check the final epoch count and runtime
            epoch_data = report_output.raw_epoch(host_name=nn)
            trace_elapsed_time = trace.iloc[-1]['TIME'] - trace['TIME'].loc[trace['EPOCH_COUNT'] == 1].iloc[0]
            msg = 'for epoch on node {nn}'.format(nn=nn)
            util.assertNear(self, trace_elapsed_time, epoch_data['runtime (s)'], msg=msg)
            self.assertEqual(trace.iloc[-1]['EPOCH_COUNT'], epoch_data['count'])


if __name__ == '__main__':
    unittest.main()
