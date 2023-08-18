#!/usr/bin/env python3
#
#  Copyright (c) 2015 - 2023, Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#
"""Run through the python agents tutorial as a user would.
"""

import sys
import unittest
import os
import subprocess
import yaml
from integration.test import geopm_test_launcher
from integration.test import util


@util.skip_unless_do_launch()
@util.skip_unless_stressng()
class TestIntegration_tutorial_python_agents(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        sys.stdout.write('(' + os.path.basename(__file__).split('.')[0] +
                         '.' + cls.__name__ + ') ...')
        cls._test_name = 'python_agents_tutorial'
        cls._skip_launch = not util.do_launch()
        cls._script_dir = os.path.dirname(os.path.realpath(__file__))
        cls._base_dir = os.path.dirname(os.path.dirname(cls._script_dir))
        cls._tutorial_dir = os.path.join(cls._base_dir, 'tutorial', 'python_agents')
        cls._readme_path = os.path.join(cls._tutorial_dir, 'README.rst')
        cls._expected_stress_ng_timeout_seconds = 5
        cls._expected_frequency_limit = 1.5e9

        if not cls._skip_launch:
            cls.launch()

    def tearDown(self):
        if sys.exc_info() != (None, None, None):
            TestIntegration_tutorial_python_agents._keep_files = True

    @classmethod
    def launch(cls):
        "Run the tutorial scripts"
        script_bodies = util.get_scripts_from_readme(cls._readme_path)

        cls._initial_frequency_control = geopm_test_launcher.geopmread(
            'CPU_FREQUENCY_MAX_CONTROL board 0')

        for script_body in script_bodies:
            print('Executing:', script_body)
            subprocess.check_call(script_body, shell=True, cwd=cls._tutorial_dir)

        cls._final_frequency_control = geopm_test_launcher.geopmread(
            'CPU_FREQUENCY_MAX_CONTROL board 0')

    def test_monitor_report(self):
        with open(os.path.join(self._tutorial_dir, 'stress-monitor.report')) as f:
            report = yaml.load(f, Loader=yaml.SafeLoader)
            self.assertEqual(dict(), report['Policy']['Initial Controls'])
            host_data = next(iter(report['Hosts'].values()))
            self.assertAlmostEqual(
                self._expected_stress_ng_timeout_seconds,
                host_data['Application Totals']['runtime (s)'],
                places=0)

    def test_package_energy_report(self):
        with open(os.path.join(self._tutorial_dir, 'stress-package-energy.report')) as f:
            report = yaml.load(f, Loader=yaml.SafeLoader)
            self.assertEqual(dict(), report['Policy']['Initial Controls'])
            host_data = next(iter(report['Hosts'].values()))
            self.assertAlmostEqual(
                self._expected_stress_ng_timeout_seconds,
                host_data['Application Totals']['runtime (s)'],
                places=0)
            self.assertGreater(
                host_data['Application Totals']['CPU_ENERGY@package-0'],
                host_data['Application Totals']['CPU_ENERGY@package-1'])

    def test_frequency_limit_report(self):
        with open(os.path.join(self._tutorial_dir, 'stress-frequency-limit.report')) as f:
            report = yaml.load(f, Loader=yaml.SafeLoader)
            self.assertAlmostEqual(
                self._expected_frequency_limit,
                report['Policy']['Initial Controls']['CPU_FREQUENCY_MAX_CONTROL'],
                places=0)
            host_data = next(iter(report['Hosts'].values()))

            self.assertAlmostEqual(
                self._expected_stress_ng_timeout_seconds,
                host_data['Application Totals']['runtime (s)'],
                places=0)
            self.assertGreater(
                host_data['Application Totals']['CPU_ENERGY@package-0'],
                host_data['Application Totals']['CPU_ENERGY@package-1'])

            # Test that the frequency control had an effect for the duration of
            # the reported application. This loosely tests that the next P-state
            # above the request was not achieved.
            self.assertLess(
                host_data['Application Totals']['frequency (Hz)'],
                self._expected_frequency_limit + 1e8)

            # Test that the agent correctly applies controls between
            # the controller's save/restore events.
            self.assertEqual(self._initial_frequency_control, self._final_frequency_control)


if __name__ == '__main__':
    # Call do_launch to clear non-pyunit command line option
    util.do_launch()
    unittest.main()
