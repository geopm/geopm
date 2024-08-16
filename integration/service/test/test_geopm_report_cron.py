#!/usr/bin/env python3
#  Copyright (c) 2015 - 2024 Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#

from geopm_report_cron_plot import CronReport
from geopmdpy.loop import TimedLoop
import unittest
import subprocess # nosec
import os
import shutil
import pathlib

class TestIntegraitonCronReport(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        pids = []
        cls._ret_code = []
        for report_idx in TimedLoop(3, 10):
            pids.append(subprocess.Popen('./geopm_report_cron.sh 3 0.01 geopm-report &>> geopm-report.log', shell=True))
        for pid in pids:
            cls._ret_code.append(pid.wait())

    def assertIsFile(self, path):
        if not pathlib.Path(path).resolve().is_file():
            raise AssertionError(f'File does not exist: {path}')

    def test_return_code(self):
        for ret in self._ret_code:
            self.assertEqual(0, ret)

    def test_plots(self):
        cr = CronReport('./geopm-report/*/*')
        begin_date, end_date = cr.date_range()
        fig = cr.plot_power('cpu', begin_date, end_date)
        fig.write_html('geopm-report/cpu_power.html')
        self.assertIsFile('geopm-report/cpu_power.html')
        fig = cr.plot_energy(begin_date, end_date)
        fig.write_html('geopm-report/energy_pie.html')
        self.assertIsFile('geopm-report/energy_pie.html')

if __name__ == '__main__':
    unittest.main()
