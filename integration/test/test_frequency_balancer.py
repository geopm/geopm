#!/usr/bin/env python3
#
#  Copyright (c) 2015 - 2025, Intel Corporation
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
from pathlib import Path
import shutil
from types import SimpleNamespace
from collections import defaultdict

import geopmpy.agent
from geopmpy.io import RawReport

from integration.test import util
from integration.test import geopm_test_launcher
from integration.experiment import machine
from integration.experiment.sst_evaluation import sst_evaluation
from integration.apps.arithmetic_intensity import arithmetic_intensity


@util.skip_unless_do_launch()
@util.skip_unless_workload_exists("apps/arithmetic_intensity/ARITHMETIC_INTENSITY/bench_avx2")
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
        output_dir = Path('test_cores_rebalance_output')
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
             '--base-internal-iterations=4',
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
            trial_count=3,
            cool_off_time=10,
            enable_traces=False,
            enable_profile_traces=False,
            power_cap=None,
        )

        sst_evaluation.launch(app_conf, args=experiment_args, experiment_cli_args=['--geopm-ctl=process'])

        results = defaultdict(lambda: defaultdict(list))
        for report_path in output_dir.glob('*.report'):
            report = RawReport(report_path).raw_report()
            agent_variant = report['Agent']
            if agent_variant != 'monitor':
                if report['Policy'].get('USE_FREQUENCY_LIMITS', 0):
                    agent_variant += '_pstate'
                    agent_variant += '(used)' if report.get('Agent uses frequency control', 0) else '(unused)'
                if report['Policy'].get('USE_SST_TF', 0):
                    agent_variant += '_ssttf'
                    agent_variant += '(used)' if report.get('Agent uses SST-TF', 0) else '(unused)'

            results[agent_variant]['FoM'].append(report['Figure of Merit'])
            results[agent_variant]['package-energy (J)'].append(sum(
                host_data['Epoch Totals']['package-energy (J)']
                for host_data in report['Hosts'].values()))
            results[agent_variant]['time-hint-network (s)'].append(max(
                network_time
                for host_data in report['Hosts'].values()
                for network_time_field, network_time in host_data['Epoch Totals'].items()
                if network_time_field.startswith('TIME_HINT_NETWORK@core')))
        # Record the meai across trials
        summary_results = {agent: {k: (sum(v) / len(v)) for k, v in agent_results.items()}
                           for agent, agent_results in results.items()}

        # Save both energy and time when SST-TF is present
        for agent_variant in summary_results:
            if 'pstate(used)' in agent_variant or 'ssttf(used)' in agent_variant:
                self.assertLess(summary_results[agent_variant]['package-energy (J)'],
                                summary_results['monitor']['package-energy (J)'],
                                msg=f'{agent_variant} reduce energy')
                self.assertLess(summary_results[agent_variant]['time-hint-network (s)'],
                                summary_results['monitor']['time-hint-network (s)'],
                                msg=f'{agent_variant} should reduce time spent waiting in network routines')
                self.assertGreater(summary_results[agent_variant]['FoM'],
                                   summary_results['monitor']['FoM'] * 0.95,
                                   msg=f'{agent_variant} should not decrease figure of merit by a lot')
            if 'ssttf(used)' in agent_variant:
                self.assertGreater(summary_results[agent_variant]['FoM'],
                                   summary_results['monitor']['FoM'],
                                   msg=f'{agent_variant} should increase figure of merit')


if __name__ == '__main__':
    # Call do_launch to clear non-pyunit command line option
    util.do_launch()
    unittest.main()
