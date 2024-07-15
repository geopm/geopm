#!/usr/bin/env python3
#
#  Copyright (c) 2015 - 2024 Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#

"""
This integration test verifies that region classification neural nets and frequency
recommendation maps can be autogenerated and that the ffnet agent is functional.
"""
import json
import sys
import unittest
import os
from pathlib import Path
import pandas as pd
import shutil
from types import SimpleNamespace

import geopmpy.agent
import geopmpy.io

from integration.apps.geopmbench import geopmbench
from integration.apps.parres import parres

from integration.test import util
from integration.test import geopm_test_launcher

from integration.experiment import machine
from integration.experiment.ffnet import ffnet
from integration.experiment.ffnet import neural_net_sweep
from integration.experiment.ffnet import gen_hdf_from_fsweep
from integration.experiment.ffnet import gen_neural_net
from integration.experiment.ffnet import gen_region_parameters

#@util.skip_unless_config_enable('beta')
#@util.skip_unless_do_launch()

class TestIntegration_ffnet(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        """
        Setup applications, execute, and set up class variables.
        """
        mach = machine.init_output_dir('.')
        cls._skip_launch = not util.do_launch()
        cls._test_name = 'test_ffnet_nn_scripts'

        ########################
        # CPU Neural Net Sweep #
        ########################

        # Grabbing system frequency parameters for experiment frequency bounds
        # Choosing the maximum many core frequency to remove redundant frequency sweep values
        cls._cpu_max_freq = mach.frequency_max_many_core()
        cls._cpu_min_freq = mach.frequency_min()
        cls._cpu_freq_step = 2 * mach.frequency_step()

        node_count = 1
        cls._run_count = 0

        # Setup Common Args
        cls._nn_sweep_dir = Path(os.path.join('test_neural_net_sweep_output', 'nn_frequency_sweep'))
        cpu_fsweep_experiment_args = SimpleNamespace(
            output_dir=cls._nn_sweep_dir,
            node_count=node_count,
            max_frequency = cls._cpu_max_freq,
            min_frequency = cls._cpu_min_freq,
            step_frequency = cls._cpu_freq_step,
            trial_count = 1,
            cool_off_time = 3,
            run_max_turbo = False
        )

        experiment_cli_args=['--geopm-ctl=process']

        # Configure the CPU test application - geopmbench
        cpu_test_app_params = {
            'spin_bigo': 0.5,
            'sleep_bigo': 1.0,
            'dgemm_bigo': 1.0,
            'stream_bigo': 2.0,
            'loop_count': 2
        }
        cls._app_regions = {}
        #TODO: Get hashes later from a report and assemble this info
        cls._app_regions['cpu'] = {'spin':"geopmbench-0x120a248f",
                                   'sleep':"geopmbench-0x0f33c2ac",
                                   'dgemm':"geopmbench-0xa12de8ee",
                                   'stream':"geopmbench-0xf0e9be1c"}

        bench_conf = geopmpy.io.BenchConf(cls._test_name + '_app.config')
        bench_conf.set_loop_count(cpu_test_app_params['loop_count'])
        for region in cls._app_regions['cpu']:
            bench_conf.append_region(region, cpu_test_app_params[f"{region}_bigo"])
        bench_conf.write()

        cpu_app_conf = geopmbench.GeopmbenchAppConf(os.path.abspath(bench_conf.get_path()), 1)

        #Launch CPU Frequency Sweeps for NN Generation - geopmbench
        cls.launch_helper(cls, neural_net_sweep, cpu_fsweep_experiment_args, cpu_app_conf, experiment_cli_args)

        ########################
        # GPU Neural Net Sweep #
        ########################

        # Checking if a gpu is present on the system and parres is built. If so, ensuring
        # that a reasonable number of frequency steps is taken for NN frequency sweep
        # as some GPUs have very fine frequency steps

        cls._do_gpu = False
        cls._num_gpu = mach.num_gpu()
        parres_basepath = os.path.join(os.path.dirname(
                              os.path.dirname(os.path.realpath(__file__))),
                                              "apps/parres/Kernels/Cxx11")
        if cls._num_gpu > 0 and os.path.exists(parres_basepath):
            cls._do_gpu = True
            cls._gpu_freq_min = mach.gpu_frequency_min()
            cls._gpu_freq_max = cls._gpu_frequency_max()
            cls._gpu_freq_step = mach.gpu_frequency_step()
            course_step = round((cls._gpu_frequency_max() - mach.gpu_frequency_min())/4)
            if course_step > cls._gpu_freq_step:
                cls._gpu_freq_step = course_step

            #GPU Frequency Sweeps for NN Generation - parres
            gpu_fsweep_experiment_args = SimpleNamespace(
                output_dir=cls._nn_sweep_dir,
                node_count=node_count,
                max_gpu_frequency = cls._gpu_max_freq,
                min_gpu_frequency = cls._gpu_min_freq,
                step_gpu_frequency = cls._gpu_freq_step,
                trial_count = 1,
                run_max_turbo = False
            )

            # Configure the GPU test application - parres dgemm/nstream
            gpu_experiment_args = SimpleNamespace(
                node_count=node_count,
                trial_count = 1,
                cool_off_time=3,
                parres_cores_per_node=None,
                parres_gpus_per_node=None,
                parres_cores_per_rank=1,
                parres_init_setup=None,
                parres_exp_setup=None,
                parres_teardown=None,
                parres_args=None,
                output_dir=cls._nn_sweep_dir,
                min_gpu_frequency = cls._gpu_freq_min,
                max_gpu_frequency = cls._gpu_freq_max,
                step_gpu_frequency = cls._gpu_freq_step
            )

            cls._app_regions['gpu'] = {'dgemm':'parres_dgemm-0xDEADBEEF',
                                       'nstream':'parres_nstream-0xDEADBEEF'}
            #Get correct parres executables and set up app conf
            if util.get_service_config_value('enable_nvml') == '1':
                app_exec_names = ["dgemm-mpi-cublas", "nstream-mpi-cuda"]
                app_confs = [parres.create_dgemm_appconf_cuda(mach, experiment_args),
                             parres.create_nstream_appconf_cuda(mach, experiment_args)]
            elif util.get_service_config_value('enable_levelzero') == '1':
                app_exec_names = ["dgemm-onemkl", "nstream-onemkl"]
                app_confs = [parres.create_dgemm_appconf_oneapi(mach, experiment_args),
                             parres.create_nstream_appconf_oneapi(mach, experiment_args)]
            for app in app_exec_names:
                app_path = os.path.join(parres_basepath, app)
                if os.path.exists(app_path):
                    parres_app_paths.append(app_path)

            #If no parres executables exist, error
            if len(parres_app_paths) == 0:
                self.fail("No parres dgemm/nstream executables were found. Cannot test GPU.")

        #Launch GPU Frequency Sweeps for NN Generation - parres dgemm / nstream
            for parres_app_conf in app_confs:
                cls.launch_helper(neural_net_sweep, gpu_experiment_args, parres_app_conf, [], None)

        # Set up HDF/neural net file info
        cls._nn_output_prefix = "test_nn"
        cls._nn_description = "test description"
        cls._nn_region_ignore = cls._app_regions['cpu']['spin']
        cls._nn_stats_hdf = f"{cls._nn_output_prefix}_stats.h5"
        cls._nn_trace_hdf = f"{cls._nn_output_prefix}_traces.h5"
        cls._nn_out = f"{cls._nn_output_prefix}_nn"
        cls._nn_fmap_out = f"{cls._nn_output_prefix}_fmap"

        #Generate h5s
        gen_hdf_from_fsweep.main(cls._nn_output_prefix, str(cls._nn_sweep_dir))

        # Generate neural nets
        gen_neural_net.main(cls._nn_trace_hdf, cls._nn_out, cls._nn_description, cls._nn_region_ignore)

        gen_region_parameters.main(cls._nn_fmap_out, cls._nn_stats_hdf)

        ###########
        # Helpers #
        ###########

    #Check if JSON is valid (used in tests below)
    def get_json(self, json_file):
        try:
            json_parsed = json.load(json_file)
        except ValueError:
            json_parsed = None
        return json_parsed

    #Launch Helper for multiple job launches
    def launch_helper(self, experiment_type, experiment_args, app_conf, experiment_cli_args):
        if not self._skip_launch:
            self._run_count += 1

            output_dir = experiment_args.output_dir
            if output_dir.exists() and output_dir.is_dir():
                shutil.rmtree(output_dir)

            experiment_type.launch(app_conf=app_conf, args=experiment_args,
                                   experiment_cli_args=experiment_cli_args)


    def test_hdf_generation(self):
        stats_hdf = pd.read_hdf(self._nn_stats_hdf)
        trace_hdf = pd.read_hdf(self._nn_trace_hdf)

        cpu_stats_columns = ["node", "app-config", "runtime (s)",
                             "cpu-frequency", "package-energy (J)"]
        gpu_stats_columns = ["gpu-frequency", "gpu-energy (J)"]

        cpu_trace_columns = ['node', 'app-config', 'TIME', 'CPU_POWER-package-0',
                             'CPU_FREQUENCY_STATUS-package-0',
                             'MSR::UNCORE_PERF_STATUS:FREQ-package-0',
                             'MSR::QM_CTR_SCALED_RATE-package-0',
                             'CPU_INSTRUCTIONS_RETIRED-package-0',
                             'CPU_CYCLES_THREAD-package-0',
                             'CPU_ENERGY-package-0',
                             'MSR::APERF:ACNT-package-0',
                             'MSR::PPERF:PCNT-package-0',
                             'MSR::PPERF:PCNT-package-0']

        gpu_trace_columns = ['GPU_CORE_FREQUENCY_STATUS-gpu-0',
                             'GPU_POWER-gpu-0',
                             'GPU_UTILIZATION-gpu-0',
                             'GPU_CORE_ACTIVITY-gpu-0',
                             'GPU_UNCORE_ACTIVITY-gpu-0']

        #Check for desired stats columns
        for col in cpu_stats_columns:
            self.assertTrue(col in stats_hdf)
        if self._do_gpu is True:
            for col in gpu_stats_columns:
                self.assertTrue(col in stats_hdf)
        else:
            for col in gpu_stats_columns:
                self.assertFalse(col in stats_hdf)

        #Check for desired trace columns
        for col in cpu_trace_columns:
            self.assertTrue(col in trace_hdf)
        if self._do_gpu is True:
            for col in gpu_trace_columns:
                self.assertTrue(col in trace_hdf)
        else:
            for col in gpu_trace_columns:
                self.assertFalse(col in trace_hdf)

    def test_nn_generation(self):
        nn_files = {"cpu":f"{self._nn_out}_cpu.json"}
        if self._do_gpu is True:
            nn_files["gpu"] = f"{self._nn_out}_gpu.json"

        nn_fields = ["description", "delta_inputs", "signal_inputs", "trace_outputs", "layers"]

        nn_jsons = {}
        for domain in nn_files:
            fp = open(nn_files[domain], "r")
            nn_jsons[domain] = self.get_json(fp)

            #Check that the neural net file contains valid json
            self.assertTrue(nn_jsons[domain] is not None)

            #Check that the desired fields are present
            for nn_field in nn_fields:
                self.assertTrue(nn_field in nn_jsons[domain])

            #Check that the appropriate regions are present/ignored in trace_output
            for region in self._app_regions[domain]:
                if self._app_regions[domain][region] == self._nn_region_ignore:
                    self.assertFalse(self._app_regions[domain][region] in nn_jsons[domain]["trace_outputs"])
                else:
                    self.assertTrue(self._app_regions[domain][region] in nn_jsons[domain]["trace_outputs"])
            fp.close()

    def test_freqmap_generation(self):
        #Region frequency map JSONs (with/without GPUs)
        fmap_files = {"cpu":f"{self._nn_fmap_out}_cpu.json"}
        fp = {}
        if self._do_gpu > 0:
            fmap_files["gpu"] = f"{self._nn_fmap_out}_gpu.json"

        fmap_jsons = {}
        for domain in fmap_files:
            fp[domain] = open(fmap_files[domain], "r")
            fmap_jsons[domain] = self.get_json(fp[domain])

            #Check that the region frequency map file contains valid json
            self.assertTrue(fmap_jsons[domain] is not None)

            #Check that frequency values decrease monotonically with phi
            for region in fmap_jsons[domain]:
                freq_list = fmap_jsons[domain][region]
                self.assertTrue(all(x>=y for x,y in zip(freq_list, freq_list[1:])))

            #Check that desired regions are present/ignored in json file
            for region in self._app_regions[domain]:
                self.assertTrue(self._app_regions[domain][region] in fmap_jsons[domain])


        #Check that CPU region frequency for sleep is <= dgemm at phi=0 (could both be fmax)
        self.assertTrue(fmap_jsons["cpu"][self._app_regions['cpu']['sleep']][0] <= fmap_jsons["cpu"][self._app_regions['cpu']['dgemm']][0])
        #Check that CPU region frequency for sleep is < dgemm at phi=1 (strictly)
        self.assertTrue(fmap_jsons["cpu"][self._app_regions['cpu']['sleep']][-1] < fmap_jsons["cpu"][self._app_regions['cpu']['dgemm']][-1])

        #Check that sleep's frequency decrease (phi=0 to phi=1) is
        #greater than dgemm's frequency decrease (phi=0 to phi=1)
        df_sleep = fmap_jsons["cpu"][self._app_regions['cpu']['sleep']][0] - fmap_jsons["cpu"][self._app_regions['cpu']['sleep']][-1]

        df_dgemm = fmap_jsons["cpu"][self._app_regions['cpu']['dgemm']][0] - fmap_jsons["cpu"][self._app_regions['cpu']['dgemm']][-1]
        self.assertTrue(df_sleep >= df_dgemm)
        fp['cpu'].close()

        if self._do_gpu is True:
            #Check that parres nstream's frequency at phi=1 is less than dgemm's frequency at phi=1
            dgemm_freq = fmap_jsons["gpu"][self._app_regions['gpu']['dgemm']][-1]
            nstream_freq = fmap_jsons["gpu"][self._app_regions['gpu']['nstream']][-1]
            assertTrue(dgemm_freq > nstream_freq)
            fp['gpu'].close()

if __name__ == '__main__':
    # Call do_launch to clear non-pyunit command line option
    util.do_launch()
    unittest.main()
