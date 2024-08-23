#!/usr/bin/env python3
#  Copyright (c) 2015 - 2024 Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#

from geopmdpy.loop import TimedLoop
import unittest
import subprocess # nosec
import os
import shutil
import pathlib
from tempfile import mkdtemp
from shutil import rmtree
import glob

_do_skip = False
try:
    from geopm_report_cron_plot import CronReport
except ModuleNotFoundError as ex:
    _do_skip = True
_script_dir_name=os.path.dirname(os.path.realpath(__file__))

def skip_unless_import():
    if _do_skip:
        return unittest.skip('geopm_report_cron_plot could not be imported, "pip install plotly dash", skipping test.')
    return lambda func: func

@skip_unless_import()
class TestIntegrationCronReport(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        pids = []
        cls._ret_code = []
        cls._tmp_dir = mkdtemp()
        cls._report_glob = f'{cls._tmp_dir}/geopm-report/*/*'
        # 3 second reports for 30 seconds sampling at 10 ms
        for report_idx in TimedLoop(3, 4):
            pids.append(subprocess.Popen([f'{_script_dir_name}/geopm_report_cron.sh', '3', '0.01', f'{cls._tmp_dir}/geopm-report']))
        for pid in pids:
            cls._ret_code.append(pid.wait())

    @classmethod
    def tearDownClass(cls):
        if 'GEOPM_DEBUG' not in os.environ:
            rmtree(cls._tmp_dir)
        else:
            print(f'Debug output: {cls._tmp_dir}')

    def assertIsFile(self, path):
        if not pathlib.Path(path).resolve().is_file():
            raise AssertionError(f'File does not exist: {path}')

    def test_return_code(self):
        for ret in self._ret_code:
            self.assertEqual(0, ret)

    def test_plots(self):
        cr = CronReport(glob.glob(self._report_glob))
        begin_date, end_date = cr.date_range()
        for domain in ('cpu', 'gpu', 'dram'):
            fig = cr.plot_power(domain, begin_date, end_date)
            fig.write_html(f'{self._tmp_dir}/{domain}_power.html')
            self.assertIsFile(f'{self._tmp_dir}/{domain}_power.html')
        fig = cr.plot_energy(begin_date, end_date)
        fig.write_html(f'{self._tmp_dir}/energy_pie.html')
        self.assertIsFile(f'{self._tmp_dir}/energy_pie.html')

if __name__ == '__main__':
    unittest.main()
