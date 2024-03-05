#!/usr/bin/env python3
#
#  Copyright (c) 2015 - 2024, Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#

"""This integration test verifies that the frequency_balancer agent can improve
efficiency of an imbalanced application. If only P-State controls are available,
it aims to achieve efficiency gains through energy reduction. If SST-TF is also
available, the test aims to improve both energy and performance on an imbalanced
workload.

"""

import sys
import unittest
import os
from pathlib import Path
import shutil
from experiment import machine
from types import SimpleNamespace

import geopmpy.agent
from yaml import load
try:
    from yaml import CSafeLoader as SafeLoader
except ImportError:
    from yaml import SafeLoader
from collections import defaultdict

from integration.test import util
from integration.test import geopm_test_launcher
from experiment.sst_evaluation import sst_evaluation
from apps.arithmetic_intensity import arithmetic_intensity

@util.skip_unless_do_launch()
class TestIntegration_frequency_balancer(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        try:
            cls._do_use_sst = geopm_test_launcher.geopmread("SST::TURBOFREQ_SUPPORT:SUPPORTED board 0")
        except:
            cls._do_use_sst = False

    def tearDown(self):
        if sys.exc_info() != (None, None, None):
            TestIntegration_frequency_balancer._keep_files = True

    def test_cores_rebalance(self):
        """
        An imbalanced application gets rebalanced by the frequency_balancer agent.
        """
        output_dir = Path(__file__).resolve().parent.joinpath('test_cores_rebalance_output')
        if output_dir.exists() and output_dir.is_dir():
            shutil.rmtree(output_dir)

        mach = machine.init_output_dir(output_dir)

        node_count = 1
        ranks_per_node = mach.num_core() - mach.num_package()
        ranks_per_package = ranks_per_node // mach.num_package()

        try:
            slow_ranks_per_package = int(geopm_test_launcher.geopmread("SST::HIGHPRIORITY_NCORES:0 package 0"))
        except Exception:
            slow_ranks_per_package = ranks_per_package // 4

        # Configure the test application
        app_conf = arithmetic_intensity.ArithmeticIntensityAppConf(
            ['--slowdown=2.5',
             f'--slow-ranks-per-imbalanced-group={slow_ranks_per_package}',
             f'--ranks-per-imbalanced-group={ranks_per_package}',
             '--base-internal-iterations=2',
             '--iterations=100',
             f'--floats={1<<22}',
             '--benchmarks=32'],
            mach,
            run_type='avx2',
            ranks_per_node=ranks_per_node)
        experiment_args = SimpleNamespace(
            output_dir=output_dir,
            agent_list=None,
            node_count=node_count,
            trial_count=1,
            cool_off_time=10,
            enable_traces=False,
            enable_profile_traces=False,
            power_cap=None,
        )

        sst_evaluation.launch(app_conf, args=experiment_args, experiment_cli_args=[])

        results = defaultdict(dict)
        for report_path in output_dir.glob('*.report'):
            with open(report_path) as f:
                report = load(f, Loader=SafeLoader)
                agent_variant = report['Agent']
                if agent_variant != 'monitor':
                    if report['Policy'].get('USE_FREQUENCY_LIMITS', 0):
                        agent_variant += '_pstate'
                    agent_variant += '(used)' if report.get('Agent uses frequency control', 0) else '(unused)'
                    if report['Policy'].get('USE_SST_TF', 0):
                        agent_variant += '_ssttf'
                    agent_variant += '(used)' if report.get('Agent uses SST-TF', 0) else '(unused)'

                results[agent_variant]['FoM'] = report['Figure of Merit']
                results[agent_variant]['package-energy (J)'] = sum(
                    host_data['Epoch Totals']['package-energy (J)']
                    for host_data in report['Hosts'].values())
                results[agent_variant]['time-hint-network (s)'] = max(
                    host_data['Epoch Totals']['time-hint-network (s)']
                    for host_data in report['Hosts'].values())

        # Save both energy and time when SST-TF is present
        for agent_variant in results:
            if 'pstate(used)' in agent_variant or 'ssttf(used)' in agent_variant:
                self.assertLess(results[agent_variant]['package-energy (J)'],
                                results['monitor']['package-energy (J)'],
                                msg=f'{agent_variant} reduce energy')
                self.assertLess(results[agent_variant]['time-hint-network (s)'],
                                results['monitor']['time-hint-network (s)'],
                                msg=f'{agent_variant} should reduce time spent waiting in network routines')
                self.assertGreater(results[agent_variant]['FoM'],
                                   results['monitor']['FoM'] * 0.95,
                                   msg=f'{agent_variant} should not decrease figure of merit by a lot')
            if 'ssttf(used)' in agent_variant:
                self.assertGreater(results[agent_variant]['FoM'],
                                   results['monitor']['FoM'],
                                   msg=f'{agent_variant} should increase figure of merit')

if __name__ == '__main__':
    # Call do_launch to clear non-pyunit command line option
    util.do_launch()
    unittest.main()
