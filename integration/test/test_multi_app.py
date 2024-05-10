#!/usr/bin/env python3
#
#  Copyright (c) 2015 - 2024 Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#

"""Test the use case of running multiple apps running simultaneously.
The test will launch an instance of geopmbench and a non-MPI app and
verify they're tracked/profiled correctly.

"""

import sys
import unittest
import subprocess
import os
from socket import gethostname

import geopmpy.io
from integration.test import util


class TestIntegration_multi_app(unittest.TestCase):
    TEST_NAME = 'test_multi_app'
    TIME_LIMIT = 60
    NUM_NODE = 1
    EXPECTED_REGIONS = {'model-init', 'stream', 'dgemm',
                        'MPI_Init_thread'}

    @classmethod
    def setUpClass(cls):
        sys.stdout.write('(' + os.path.basename(__file__).split('.')[0] +
                         '.' + cls.__name__ + ') ...')
        script_dir = os.path.dirname(os.path.realpath(__file__))
        script_path = os.path.join(script_dir,'test_multi_app.sh')
        subprocess.run(['/bin/bash', script_path],
                       timeout=cls.TIME_LIMIT, check=True)

        cls._report_path = f'{cls.TEST_NAME}_report.yaml-{gethostname()}'
        cls._report = geopmpy.io.RawReport(cls._report_path)
        cls._node_names = cls._report.host_names()

    def test_meta_data(self):
        self.assertEqual(len(self._node_names), self.NUM_NODE)

    def test_expected_regions_exist(self):
        for node in self._node_names:
            regions = set(self._report.region_names(node))
            self.assertEqual(regions, self.EXPECTED_REGIONS)

    def test_regions_valid(self):
        for node in self._node_names:
            for region in self._report.region_names(node):
                region_data = self._report.raw_region(node, region)
                if region in ('model-init', 'MPI_Init_thread'):
                    self.assertEqual(region_data['count'], 0.5)
                else:
                    self.assertEqual(region_data['count'], 1)
                if region != 'MPI_Init_thread':
                    # We do not sample during PMPI_Init_thread call
                    # but other regions should have non-zero sample time
                    self.assertGreater(region_data['TIME@package-0'], 0)
                self.assertEqual(region_data['TIME@package-1'], 0)

    def test_non_mpi_app_tracked(self):
        for node in self._node_names:
            unmarked_data = self._report.raw_unmarked(node)
            self.assertGreater(unmarked_data['TIME@package-1'], 0)

    def test_runtime(self):
        for node in self._node_names:
            total_runtime = 0
            for region in self._report.region_names(node):
                region_data = self._report.raw_region(node, region)
                total_runtime += region_data['runtime (s)']
            unmarked_data = self._report.raw_unmarked(node)
            total_runtime += unmarked_data['runtime (s)']
            app_totals = self._report.raw_totals(node)
            util.assertNear(self, total_runtime, app_totals['runtime (s)'])


if __name__ == '__main__':
    unittest.main()
