#!/usr/bin/env python
#
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

from experiment import launch_util
from experiment.power_sweep import power_sweep
from experiment.monitor import monitor


def launch_configs(app_conf_ref, app_conf, min_power, max_power, step_power):
    launch_configs = [launch_util.LaunchConfig(app_conf=app_conf_ref,
                                               agent_conf=None,
                                               name="reference")]

    launch_configs += power_sweep.launch_configs(app_conf=app_conf,
                                                 agent='power_balancer',
                                                 min_power=min_power,
                                                 max_power=max_power,
                                                 step_power=step_power)
    return launch_configs


def report_signals():
    report_sig = set(monitor.report_signals())
    report_sig.update(power_sweep.report_signals())
    report_sig = sorted(list(report_sig))
    return report_sig


def trace_signals():
    return power_sweep.trace_signals()


def launch(output_dir, iterations,
           min_power, max_power, step_power,
           num_nodes, app_conf_ref, app_conf,
           experiment_cli_args, cool_off_time=60):

    extra_cli_args = list(experiment_cli_args)
    extra_cli_args += launch_util.geopm_signal_args(report_signals=report_signals(),
                                                    trace_signals=trace_signals())
    targets = launch_configs(app_conf_ref, app_conf, min_power, max_power, step_power)

    launch_util.launch_all_runs(targets=targets,
                                num_nodes=num_nodes,
                                iterations=iterations,
                                extra_cli_args=extra_cli_args,
                                output_dir=output_dir,
                                cool_off_time=cool_off_time)
