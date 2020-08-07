#  Copyright (c) 2015, 2016, 2017, 2018, 2019, 2020, Intel Corporation
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

'''
Helper functions for running the monitor agent.
'''

import os
import time

from experiment import machine
from experiment import util


def launch_monitor(output_dir, iterations,
                   num_node, app_conf, experiment_cli_args, cool_off_time=60):
    machine.init_output_dir(output_dir)
    agent = 'monitor'
    app_name = app_conf.name()
    rank_per_node = app_conf.get_rank_per_node()
    num_rank = num_node * rank_per_node

    report_sig = ["CYCLES_THREAD@package", "CYCLES_REFERENCE@package",
                  "TIME@package", "ENERGY_PACKAGE@package"]

    for iteration in range(iterations):
        uid = '{}_{}_{}'.format(app_name.lower(), agent, iteration)
        report_path = os.path.join(output_dir, '{}.report'.format(uid))
        trace_path = os.path.join(output_dir, '{}.trace'.format(uid))
        profile_name = 'iteration_{}'.format(iteration)
        log_path = os.path.join(output_dir, '{}.log'.format(uid))

        # TODO: these are not passed to launcher create()
        # some are generic enough they could be, though
        extra_cli_args = ['--geopm-report', report_path,
                          '--geopm-trace', trace_path,
                          '--geopm-profile', profile_name,
                          '--geopm-report-signals=' + ','.join(report_sig)]
        extra_cli_args += experiment_cli_args
        # any arguments after run_args are passed directly to launcher
        util.launch_run(None, app_conf, output_dir, extra_cli_args,
                        num_node=num_node, num_rank=num_rank)  # raw launcher factory args

        # Get app-reported figure of merit
        fom = app_conf.parse_fom(log_path)
        # Append to report????????????????
        with open(report_path, 'a') as report:
            report.write('\nFigure of Merit: {}'.format(fom))

        # rest to cool off between runs
        time.sleep(cool_off_time)
