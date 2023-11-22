#!/usr/bin/env python3
#
#  Copyright (c) 2015 - 2023, Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#


import os
from pathlib import Path
from types import SimpleNamespace

import geopmpy.io

from apps.parres import parres
from experiment import machine
from experiment import util as exp_util
from experiment.gpu_frequency_sweep import gpu_frequency_sweep
from experiment.gpu_frequency_sweep import gen_gpu_activity_constconfig_recommendation

from integration.test import util as test_util


class GPUCACharacterization(object):
    def __init__(self, base_dir='gpu_characterization'):
        self._mach = machine.init_output_dir('.')
        self._base_dir = base_dir

        self._cpu_max_freq = exp_util.geopmread('CPU_FREQUENCY_MAX_AVAIL board 0')
        self._uncore_max_freq = exp_util.geopmread('CPU_UNCORE_FREQUENCY_MAX_CONTROL board 0')
        self._uncore_min_freq = exp_util.geopmread('CPU_UNCORE_FREQUENCY_MIN_CONTROL board 0')

        self._gpu_max_freq = exp_util.geopmread('GPU_CORE_FREQUENCY_MAX_AVAIL board 0')
        self._gpu_min_freq = exp_util.geopmread('GPU_CORE_FREQUENCY_MIN_AVAIL board 0')

    def do_characterization(self):
        dgemm_freq_sweep_dir = Path(self._base_dir, 'dgemm_gpu_freq_sweep')
        exp_util.prep_experiment_output_dir(dgemm_freq_sweep_dir)
        app_conf, experiment_args = self.get_app_conf(dgemm_freq_sweep_dir)
        experiment_cli_args = ['--geopm-ctl-local']

        gpu_frequency_sweep.launch(app_conf=app_conf, args=experiment_args,
                                   experiment_cli_args=experiment_cli_args)
        # Parse data
        df_frequency_sweep = geopmpy.io.RawReportCollection('*report',
                                                            dir_name=dgemm_freq_sweep_dir).get_df()
        gpu_config = gen_gpu_activity_constconfig_recommendation \
                        .get_config_from_frequency_sweep(df_frequency_sweep, self._mach, 0, True)
        # TODO: consider False

        return gpu_config

    def get_app_conf(self, output_dir):
        experiment_args = self._get_experiment_args(output_dir)
        #if test_util.is_nvml_enabled():
        #    app_path = os.path.join(os.path.dirname(os.path.dirname(os.path.realpath(__file__))),
        #                            "apps/parres/Kernels/Cxx11/nstream-mpi-cuda")
        #    app_conf = parres.create_dgemm_appconf_cuda(self._mach, experiment_args)
        #elif test_util.is_oneapi_enabled():
        app_path = os.path.join(os.path.dirname(os.path.dirname(os.path.realpath(__file__))),
                                "apps/parres/Kernels/Cxx11/nstream-onemkl")
        app_conf = parres.create_dgemm_appconf_oneapi(self._mach, experiment_args)
        #if not os.path.exists(app_path):
        #    raise Exception("Support for neither NVML or OneAPI was found")
        return app_conf, experiment_args

    def get_machine_config(self):
        return self._mach

    def _get_experiment_args(self, output_dir):
        experiment_args = SimpleNamespace(
            output_dir=output_dir,
            node_count=1,
            trial_count=1,
            cool_off_time=3,
            parres_cores_per_node=None,
            parres_gpus_per_node=None,
            parres_cores_per_rank=1,
            parres_init_setup=None,
            parres_exp_setup=None,
            parres_teardown=None,
            init_control=None,
            parres_args=None,
            enable_traces=False,
            enable_profile_traces=False,
            verbose=False,
            min_frequency = self._cpu_max_freq,
            max_frequency = self._cpu_max_freq,
            min_uncore_frequency = self._uncore_max_freq,
            max_uncore_frequency = self._uncore_max_freq,
            step_frequency = 1e8,
            step_uncore_frequency = 1e8,
            run_max_turbo = True,
            min_gpu_frequency = self._gpu_min_freq,
            max_gpu_frequency = self._gpu_max_freq,
            step_gpu_frequency = 1e8)
        return experiment_args
