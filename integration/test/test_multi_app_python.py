#!/usr/bin/env python3
#
#  Copyright (c) 2015 - 2025 Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#

"""Test the use case of running multiple apps running simultaneously.
The test will launch an instance of geopmbench and python and
verify they're tracked/profiled correctly.

"""

import sys
import unittest
import subprocess # nosec
import os
from socket import gethostname

import geopmpy.io
from integration.test import util


class TestIntegration_multi_app_python(unittest.TestCase):
    TEST_NAME = 'test_multi_app_python'
    TIME_LIMIT = 480
    NUM_NODE = int(os.environ.get("GEOPM_NUM_NODE", 1))
    EXPECTED_REGIONS = {'MPI_Init_thread', 'model-init', 'stream', 'dgemm'}

    @classmethod
    def setUpClass(cls):
        sys.stdout.write('(' + os.path.basename(__file__).split('.')[0] +
                         '.' + cls.__name__ + ') ...')
        script_dir = os.path.dirname(os.path.realpath(__file__))
        script_path = os.path.join(script_dir,'test_multi_app_python.sh')
        if util.do_launch():
            proc = subprocess.Popen(['/bin/bash', script_path])
            try:
                proc.wait(timeout=cls.TIME_LIMIT)
            except subprocess.TimeoutExpired:
                util.terminate_process_and_children(proc.pid)
                raise
            if proc.returncode != 0:
                raise subprocess.CalledProcessError(proc.returncode, proc.args)

        cls._report_path = f'{cls.TEST_NAME}_report.yaml'
        cls._report_collection = geopmpy.io.RawReportCollection(f'{cls.TEST_NAME}_report*')
        cls._node_names = cls._report_collection.get_df()['host'].unique().tolist()

    def test_meta_data(self):
        self.assertEqual(len(self._node_names), self.NUM_NODE)

    def test_expected_regions_exist(self):
        for node in self._node_names:
            rdf = self._report_collection.get_df()
            regions = rdf[rdf['host'] == node]['region'].unique().tolist()
            for rr in self.EXPECTED_REGIONS:
                self.assertIn(rr, regions, msg=node)

    def test_regions_valid(self):
        for node in self._node_names:
            rdf = self._report_collection.get_df()
            regions = rdf[rdf['host'] == node]['region'].unique().tolist()
            for region in regions:
                region_data = rdf[(rdf['host'] == node) & (rdf['region'] == region)].to_dict(orient='records')[0]

                if region in ('model-init', 'MPI_Init_thread'):
                    self.assertEqual(region_data['count'], 0.5)
                else:
                    self.assertEqual(region_data['count'], 25, msg=f'{node} - {region}')
                if region != 'MPI_Init_thread':
                    # We do not sample during PMPI_Init_thread call
                    # but other regions should have non-zero sample time
                    self.assertGreater(region_data['TIME@package-0'], 0)
                self.assertEqual(region_data['TIME@package-1'], 0)

    def test_non_mpi_app_tracked(self):
        for node in self._node_names:
            udf = self._report_collection.get_unmarked_df()
            unmarked_data = udf[udf['host'] == node].to_dict(orient='records')[0]

            self.assertGreater(unmarked_data['TIME@package-1'], 0, msg=node)

    def test_runtime(self):
        for node in self._node_names:
            total_runtime = 0
            rdf = self._report_collection.get_df()
            regions = rdf[rdf['host'] == node]['region'].unique().tolist()
            for region in regions:
                region_data = rdf[(rdf['host'] == node) & (rdf['region'] == region)].to_dict(orient='records')[0]
                total_runtime += region_data['runtime (s)']
            udf = self._report_collection.get_unmarked_df()
            unmarked_data = udf[udf['host'] == node].to_dict(orient='records')[0]
            total_runtime += unmarked_data['runtime (s)']
            adf = self._report_collection.get_app_df()
            app_totals = adf[adf['host'] == node].to_dict(orient='records')[0]
            util.assertNear(self, total_runtime, app_totals['runtime (s)'])


if __name__ == '__main__':
    unittest.main()
