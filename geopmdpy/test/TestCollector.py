#!/usr/bin/env python3
#
#  Copyright (c) 2015 - 2025 Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#


from socket import gethostname
import unittest
from geopmdpy import loop
from geopmdpy import pio
from geopmdpy.stats import Collector
from io import StringIO
try:
    from yaml import SafeLoader, load as yaml_load
    _skip_yaml = False
except ModuleNotFoundError:
    _skip_yaml = True

class TestCollector(unittest.TestCase):
    def test_smoke(self):
        """Simple test: run for 0.1 second sample every 0.01 seconds and print report
        """
        config = [('TIME', 0, 0)]
        with Collector(config) as coll:
            for loop_idx in loop.TimedLoop(0.01, num_period=10):
                pio.read_batch()
                coll.update()
            report = coll.report()
        self.assertEqual(['TIME'], list(report['metrics'].keys()))
        self.assertEqual(gethostname(), report['host'])

    @unittest.skipIf(_skip_yaml, 'yaml module could not be imported, to resolve "pip install yaml"')
    def test_yaml(self):
        """Check that yaml and object reports have the same data structure and
        close values.

        """
        config = [('TIME', 0, 0)]
        with Collector(config) as coll:
            for loop_idx in loop.TimedLoop(0.01, num_period=10):
                pio.read_batch()
                coll.update()
            report = coll.report()
            report_yaml = coll.report_yaml()
        report_yaml_obj = yaml_load(report_yaml, Loader=SafeLoader)
        self.assertEqual(report_yaml_obj.keys(), report.keys())
        self.assertEqual(report_yaml_obj['metrics'].keys(), report['metrics'].keys())
        for kk in report.keys():
            if type(report[kk]) is float:
                self.assertAlmostEqual(report[kk], report_yaml_obj[kk], places=5)
            elif type(report[kk]) is int:
                self.assertEqual(report[kk], report_yaml_obj[kk])
        for kk in report['metrics']['TIME'].keys():
            if type(report['metrics']['TIME'][kk]) is float:
                self.assertAlmostEqual(report['metrics']['TIME'][kk], report_yaml_obj['metrics']['TIME'][kk], places=5)
            elif type(report['metrics']['TIME'][kk]) is int:
                self.assertEqual(report['metrics']['TIME'][kk], report_yaml_obj['metrics']['TIME'][kk])

if __name__ == '__main__':
    unittest.main()
