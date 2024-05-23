#!/usr/bin/env python3
#
#  Copyright (c) 2015 - 2024, Intel Corporation
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
import pandas as pd

import geopmpy.agent
import geopmpy.io

from apps.geopmbench import geopmbench
from apps.parres import parres

from integration.test import util
from integration.test import geopm_test_launcher

from experiment.ffnet import ffnet
from experiment.ffnet import neural_net_sweep
from experiment.ffnet import gen_hdf_from_fsweep
from experiment.ffnet import gen_neural_net
from experiment.ffnet import gen_region_parameters

@util.skip_unless_config_enable('beta')
@util.skip_unless_do_launch()

class TestIntegration_ffnet(unittest.TestCas):
    @classmethod
    def setUpClass(cls):
        """
        Setup applications, execute, and set up class variables.
        """
        mach = machine.init_output_dir('.')
        cls._skip_launch = not util.do_launch()

        ########################
        # CPU Neural Net Sweep #
        ########################

        # Grabbing system frequency parameters for experiment frequency bounds
        # Choosing the maximum many core frequency to remove redundant frequency sweep values
        cls._cpu_max_freq = mach.frequency_max_many_core()
        cls._cpu_min_freq = mach.frequency_min()
        cls._cpu_freq_step = 2 * cls._machine.frequency_step()

        # The minimum and maximum values used rely on the system controls being properly configured
        # as there is no other mechanism to confirm uncore min & max at this time.  These values are
        # guaranteed to be correct after a system reset, but could have been modified by the administrator
        cls._uncore_min_freq = mach.frequency_uncore_min()
        cls._uncore_max_freq = mach.frequency_uncore_max()


        node_count = 1

        cls._run_count = 0

        #--Setup Common Args--#

        #TODO: Figure out if you need to enable traces explicitly
        cls._nn_sweep_dir = Path(os.path.join('test_neural_net_sweep_output', 'gpu_frequency_sweep'))
        cpu_fsweep_experiment_args = SimpleNamespace(
            output_dir=cls._nn_sweep_dir,
            node_count=node_count,
            max_frequency = cls._cpu_max_freq,
            min_frequency = cls._cpu_min_freq,
            step_frequency = cls._cpu_freq_step,
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
        cls._cpu_app_regions = {'spin':"geopmbench-0xeebad5f7",
                            'sleep':"geopmbench-0x536c798f",
                            'dgemm':"geopmbench-0xa74bbf35",
                            'stream':"geopmbench-0xd691da00"}

        cpu_app_conf = geopmpy.io.BenchConf(cls._test_name + '_app.config')
        cpu_app_conf.set_loop_count(test_app_params['loop_count'])
        for region in cls._cpu_app_regions:
            cpu_app_conf.append_region(region, test_app_params[f"{region}_bigo"])
        cpu_app_conf.write()

        #TODO: What
        geopmbench_app_conf = geopmbench.GeopmbenchAppConf(cls._test_name = '_app.config', num_rank)

        #Launch CPU Frequency Sweeps for NN Generation - geopmbench
        cls.launch_helper(cls, neural_net_sweep, cpu_fsweep_experiment_args, geopmbench_app_conf, experiment_cli_args)

        ########################
        # GPU Neural Net Sweep #
        ########################

        # Checking if a gpu is present on the system and parres is built. If so, ensuring
        # that a reasonable number of frequency steps is taken for NN frequency sweep
        # as some GPUs have very fine frequency steps

        cls._do_gpu = False
        cls._num_gpu = cls._machine.num_gpu()
        parres_basepath = os.path.join(os.path.dirname(
                              os.path.dirname(os.path.realpath(__file__))),
                                              "apps/parres/Kernels/Cxx11")
        if cls._num_gpu > 0 and os.path.exists(parres_basepath):
            cls._do_gpu = True
            cls._gpu_freq_min = cls._machine.gpu_frequency_min()
            cls._gpu_freq_max = cls._gpu_frequency_max()
            cls._gpu_freq_step = cls._machine_gpu_frequency_step()
            course_step = round((cls._gpu_frequency_max() - cls._machine.gpu_frequency_min())/10)
            if course_step > cls._gpu_freq_step:
                cls._gpu_freq_step = course_step

            #GPU Frequency Sweeps for NN Generation - parres
            gpu_fsweep_experiment_args = SimpleNamespace(
                output_dir=cls._nn_sweep_dir,
                node_count=node_count,
                max_gpu_frequency = cls._gpu_max_freq,
                min_gpu_frequency = cls._gpu_min_freq,
                step_gpu_frequency = cls._gpu_freq_step,
                run_max_turbo = False
            )

            # Configure the GPU test application - parres dgemm/nstream
            gpu_experiment_args = SimpleNamespace(
                node_count=node_count,
                parres_cores_per_node=None,
                parres_gpus_per_node=None,
                parres_cores_per_rank=1,
                parres_init_setup=None,
                parres_exp_setup=None,
                parres_teardown=None,
                parres_args=None,
                output_dir=cls._nn_sweep_dir,
                node_count=cls._num_node,
                min_gpu_frequency = cls._gpu_freq_min,
                max_gpu_frequency = cls._gpu_freq_max,
                step_gpu_frequency = cls._gpu_freq_step
            )

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
                app_path = os.path.join(parres_basepath, app))
                if os.path.exists(app_path):
                    parres_app_paths.append(app_path)

            #If no parres executables exist, error
            if len(parres_app_paths) == 0:
                self.fail("No parres dgemm/nstream executables were found. Cannot test GPU.")

        #Launch GPU Frequency Sweeps for NN Generation - parres dgemm / nstream
            for parres_app_conf in app_confs:
                launch_helper(neural_net_sweep, gpu_experiment_args, parres_app_conf, [], None)

        ##TODO: Pick up here
        ##############
        # Parse data #
        ##############

        #Check if JSON is valid (used in tests below)
        def is_valid_json(json_file):
            try:
                json.loads(json_file)
            except ValueError:
                return False
            return True

        #Launch Helper for multiple job launches
        def launch_helper(experiment_type, experiment_args, app_conf, experiment_cli_args, init_control_cfg=None):
            if not cls._skip_launch:
                if init_control_cfg is not None:
                    init_control_path = 'init_control_{}'.format(cls._run_count)
                    cls._run_count += 1
                    with open(init_control_path, 'w') as outfile:
                        outfile.write(init_control_cfg)
                    experiment_args.init_control = os.path.realpath(init_control_path)

                output_dir = experiment_args.output_dir
                if output_dir.exists() and output_dir.is_dir():
                    shutil.rmtree(output_dir)

                experiment_type.launch(app_conf=app_conf, args=experiment_args,
                                       experiment_cli_args=experiment_cli_args)


        cls._do_gpu = False
        cls._num_gpu = cls._machine.num_gpu()
        if cls._num_gpu > 0
            cls._do_gpu = True
            cls._gpu_freq_min = cls._machine.gpu_frequency_min()
                              + 2 * cls._machine.gpu_frequency_step()
            cls._gpu_freq_max = cls._gpu_frequency_max() + 2 * cls._machine_gpu_frequency_step()
            cls._gpu_freq_step = cls._machine_gpu_frequency_step()

        cls._agent = 'frequency_map'

        # Set up HDF/neural net file info
        cls._nn_sweep_dir = "test_neural_net_sweep_output"

        cls._nn_output_prefix = "test_nn"
        cls._nn_description = "test description"
        cls._nn_region_ignore = "geopmbench-0x644f9787"
        cls._nn_stats_hdf = f"{cls._nn_output_prefix}_stats.h5"
        cls._nn_trace_hdf = f"{cls._nn_output_prefix}_traces.h5"
        cls._nn_out = f"{cls._nn_output_prefix}_nn"
        cls._nn_fmap_out = f"{cls._fmap_output_prefix}_fmap"

        #Parameters for neural net frequency sweeps
        #CPU
        launch_helper(neural_net_sweep, cpu_experiment_args, geopmbench_app_conf, [], None)

        #GPU
        if cls._do_gpu:
            #Check if parres is compiled in integration apps directory
            parres_basepath = os.path.join(os.path.dirname(
                              os.path.dirname(os.path.realpath(__file__))),
                                              "apps/parres/Kernels/Cxx11")
            cls._do_gpu = os.path.exists(parres_basepath)
            parres_app_paths = []
            parres_app_confs = []
        if cls._do_gpu:
            gpu_experiment_args = SimpleNamespace(
                node_count=node_count,
                parres_cores_per_node=None,
                parres_gpus_per_node=None,
                parres_cores_per_rank=1,
                parres_init_setup=None,
                parres_exp_setup=None,
                parres_teardown=None,
                parres_args=None,
                output_dir=cls._nn_sweep_dir,
                node_count=cls._num_node,
                min_gpu_frequency = cls._gpu_freq_min,
                max_gpu_frequency = cls._gpu_freq_max,
                step_gpu_frequency = cls._gpu_freq_step
            )
            #Get correct parres executables
            if util.get_service_config_value('enable_nvml') == '1':
                app_exec_names = ["dgemm-mpi-cublas", "nstream-mpi-cuda"]:
            elif util.get_service_config_value('enable_levelzero') == '1':
                app_exec_names = ["dgemm-onemkl", "nstream-onemkl"]
            for app in app_exec_names:
                app_path = os.path.join(parres_basepath, app))
                if os.path.exists(app_path):
                    parres_app_paths.append(app_path)
            #If no parres executables exist, error
            if len(parres_app_paths) == 0:
                self.fail("No parres dgemm/nstream executables were found. Cannot test GPU.")
            launch_helper(neural_net_sweep, gpu_experiment_args, parres_app_conf, [], None)

        # Output to be reused by all tests
        cls._trace_output = geopmpy.io.AppOutput(traces=cls._trace_path + '*')
        cls._report = geopmpy.io.RawReport(cls._report_path)
        cls._node_names = cls._report.host_names()


        # Generate h5s
        gen_hdf_from_fsweep.main(cls._trace_output, cls._nn_sweep_dir)

        # Generate neural nets
        #TODO: Add a region to ignore for testing
        gen_neural_net.main(cls._nn_trace_hdf, cls._nn_out, cls._nn_description, cls._nn_region_ignore)

        gen_region_parameters.main(cls._nn_fmap_out, cls._nn_stats_hdf)
        #TODO: Add region-frequency map generation for CPU (and GPU if on gpu-enabled system)


    def test_frequency_range(self):
        #TODO: Check start/stop at correct frequencies
        for node in self._node_names:
            trace_data = self._trace_output.get_trace_data(node_name=node)
            dgemm_data = self._report.raw_region(node, 'dgemm')
            epoch_data = self._report.raw_epoch(node)

    def test_report_signals(self):
        #TODO: Check for desired report signals (CPU-only run)
        #      CPU_CYCLES_THREAD, CPU_CYCLES_REFERENCE, TIME, CPU_ENERGY

        #TODO: Check that there are no GPU signals (CPU-only run)

        #TODO: Check for desired report signals (CPU+GPU run)
        #      GPU: GPU_CORE_FREQUENCY_STATUS, GPU_CORE_FREQUENCY_MIN_CONTROL,
        #           GPU_CORE_FREQUENCY_MAX_CONTROL

    def test_trace_signals(self):
        #TODO: Check for desired trace signals:
        #      CPU-only: CPU_POWER,DRAM_POWER, CPU_FREQUENCY_STATUS,CPU_PACKAGE_TEMPERATURE
        #               ,MSR::UNCORE_PERF_STATUS:FREQ,MSR::QM_CTR_SCALED_RATE
        #               ,CPU_INSTRUCTIONS_RETIRED,CPU_CYCLES_THREAD,CPU_ENERGY
        #               ,MSR::APERF:ACNT,MSR::MPERF:MCNT,MSR::PPERF:PCNT

    def test_hdf_generation(self):
        stats_hdf = pd.read_hdf(self._nn_stats_hdf)
        trace_hdf = pd.read_hdf(self._nn_trace_hdf)

        cpu_stats_columns = ["node", "hash", "region", "app-config", "runtime (s)",
                         "cpu-frequency", "package-energy (J)"]
        gpu_stats_columns = ["gpu-frequency", "gpu-energy (J)"]

        cpu_trace_columns = ['region-id', 'TIME', 'CPU_POWER-package-0',
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
            util.assertTrue(col in stats_hdf)
        if self._num_gpu > 0:
            for col in gpu_stats_columns:
                util.assertTrue(col in stats_hdf)
        else:
            for col in gpu_stats_columns:
                util.assertFalse(col in stats_hdf)

        #Check for desired trace columns
        for col in cpu_trace_columns:
            util.assertTrue(col in trace_hdf)
        if self._num_gpu > 0:
            for col in gpu_trace_columns:
                util.assertTrue(col in trace_hdf)
        else:
            for col in gpu_trace_columns:
                util.assertFalse(col in trace_hdf)

    def test_nn_generation(self):
        nn_files = [f"{cls._nn_out}_cpu.json"]
        if self._num_gpu > 0:
            nn_files.append(f"{cls._nn_out}_gpu.json")

        nn_fields = ["description", "delta_inputs", "signal_inputs", "trace_outputs", "layers"]

        for nn in nn_files:
            nn_json = json.load(open(nn, "r"))
            #Check that the neural net file contains valid json
            util.assertTrue(is_valid_json(nn_json))
            #Check that the desired fields are present
            for nn_field in nn_fields:
                util.assertTrue(nn_field in nn_json)
            #TODO: Check that the appropriate region is ignored in trace_output
            util.assertFalse(self._nn_region_ignore in nn_json["trace_outputs"])


    def test_freqmap_generation(self):
        #Region frequency map JSONs (with/without GPUs)
        fmap_files = [f"{cls._nn_fmap_out}_cpu.json"]
        if self._num_gpu > 0:
            fmap_files.append(f"{cls._nn_fmap_out}_gpu.json")


        for fmap in fmap_files:
            fmap_json = json.load(open(fmap, "r"))
            #Check that the region frequency map file contains valid json
            util.assertTrue(is_valid_json(fmap_json))
            #Check that frequency values decrease monotonically with phi
            for region,freqlist in fmap_json:
                for idx in len(freqlist)-1:
                    util.assertTrue(freqlist[idx] <= freqlist[idx+1])

        fmap_cpu_json = json.load(open(fmap_files[0], "r"))
        #Check that CPU region frequency for sleep at phi=0 is
        #less than frequency for dgemm at phi=0 (in json)
        assertTrue(fmap_cpu_json[self._app_regions['sleep']][0] <= fmap_cpu_json[self._app_regions['dgemm']][0])

        #Check that sleep's frequency increase (phi=0 to phi=1) is
        #less than dgemm's frequency increase (phi=0 to phi=1)
            df_sleep = fmap_cpu_json[self._app_regions['sleep']][0]
                     - fmap_cpu_json[self._app_regions['sleep']][-1]

            df_dgemm = fmap_cpu_json[self._app_regions['dgemm']][0]
                     - fmap_cpu_json[self._app_regions['dgemm']][-1]
            assertTrue(df_sleep <= df_dgemm)


    def test_ffnet_agent(self):


if __name__ == '__main__':
    # Call do_launch to clear non-pyunit command line option
    util.do_launch()
    unittest.main()
