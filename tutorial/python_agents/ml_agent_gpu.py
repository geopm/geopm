#!/usr/bin/env python3

#  Copyright (c) 2015 - 2021, Intel Corporation
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
Defines a GEOPM agent that sets GPU frequency using a neural network.
'''

import os.path
import argparse
import geopmdpy.runtime
import geopmdpy.topo
import collections
from statistics import mean, stdev
import math
import numpy as np
import tensorflow as tf

import base_agent

# Used for a heuristic in the GEOPM report. The first and last time that GPU
# SM active exceeds this value are used as endpoints for an estimated
# region of interest. The agent tracks some of the metrics within this
# region and adds them to the report.
GPU_ACTIVITY_CUTOFF = 0.05

DEFAULT_INITIAL_GPU_FREQUENCY = 1e9

SIGNAL_LIST = \
    [('POWER_PACKAGE', geopmdpy.topo.DOMAIN_BOARD,
      0, "avg", 'Avg package power:    {:.1f}W'),
     ('POWER_DRAM', geopmdpy.topo.DOMAIN_BOARD_MEMORY,
      0, "avg", 'Avg dram power:       {:.1f}W'),
     ('FREQUENCY', geopmdpy.topo.DOMAIN_BOARD,
      0, "avg", lambda x: f'Avg core frequency:   {x / 1e9:.2f}GHz'),
     ('MSR::UNCORE_PERF_STATUS:FREQ', geopmdpy.topo.DOMAIN_BOARD,
      0, "avg", lambda x: f'Avg uncore frequency: {x / 1e9:.2f}GHz'),
     ('TEMPERATURE_CORE', geopmdpy.topo.DOMAIN_BOARD,
      0, "avg", 'Avg core temperature:   {:.1f}C'),
     ('ENERGY_DRAM', geopmdpy.topo.DOMAIN_BOARD_MEMORY,
      0, "incr", 'Total DRAM energy:    {:.1f}j'),
     ('ENERGY_PACKAGE', geopmdpy.topo.DOMAIN_BOARD,
      0, "incr", 'Total package energy: {:.1f}j'),
    ]

class MLAgentGPU(base_agent.BaseAgent):
    '''
    This GEOPM Agent controls the GPU frequency based on a neural net provided
    as a user input.

    This agent uses and requires NN trained via TensorFlow Keras. The input NN
    is read via tensorflow.keras.models.load_model(). Input should be a
    directory, not an HDF compressed file.
    '''
    def __init__(self, model, period, freq_controls=True, freq_gpu=None,
                 phi=0.0):
        '''
        Initialize the ML agent. Setting the initial frequency to a value
        covered by NN training may help converge to the target output.

        Args:
        model (str): Path to the saved NN.
        period (double): Agent sampling and control period.
        freq_controls (bool): Enforce NN output frequencies if True. If False,
                              monitor only.
        freq_gpu (float): Initial GPU frequency in GHz. GPU frequency will not
                          be set if this is left undefined. Set at the
                          beginning of each run in run_begin.
        phi (float): Balance of importance between performance and energy.
                     0 indicates that the agent should focus on improving
                     performance. 1 indicates that the agent should focus on
                     improving energy consumption.
        '''
        tf.config.experimental.set_visible_devices(devices=[], device_type='GPU')
        with tf.device("/cpu:0"):
            self._model = tf.keras.models.load_model(model)
        self._freq_controls = freq_controls

        self._gpu_count = geopmdpy.topo.num_domain(geopmdpy.topo.DOMAIN_BOARD_ACCELERATOR)
        if (self._gpu_count == 0):
            print('Warning: Running a GEOPM GPU agent, but no GPUs are seen by GEOPM',
                  sys.stderr)
        self._freq_gpu = freq_gpu

        self._frequency_control_accumulator = None
        self._phi = phi

        # Raw signals used in neural network tensors
        self._model_signal_idx_by_gpu = list()

        # Signals that indicate gpu activity
        self._gpu_activity_signal_idx = list()
        self._gpu_frequency_signal_idx = list()
        self._gpu_energy_signal_idx = list()

        signal_list = SIGNAL_LIST
        self._cpu_energy_signal_idx = [[element[0] for element in signal_list].index('ENERGY_PACKAGE')]
        signal_list.extend([
            ('INSTRUCTIONS_RETIRED', geopmdpy.topo.DOMAIN_BOARD,
             0, "incr", lambda x: f'Total Instructions:   {int(x):.2e}'),
            ('CYCLES_REFERENCE', geopmdpy.topo.DOMAIN_BOARD,
             0, "incr", lambda x: f'Reference Cycles:   {int(x):.2e}'),
        ])

        for gpu_idx in range(self._gpu_count):
            old_len = len(signal_list)
            signal_list.extend([
                ('NVML::TOTAL_ENERGY_CONSUMPTION', geopmdpy.topo.DOMAIN_BOARD_ACCELERATOR, gpu_idx, "incr",
                 f'Total GPU-{gpu_idx} energy:     {{:.1f}}j'),
                ('NVML::FREQUENCY', geopmdpy.topo.DOMAIN_BOARD_ACCELERATOR, gpu_idx, "avg",
                 lambda x, g=gpu_idx: 'Avg GPU-{} Frequency:     {:.3f}GHz'.format(g, x / 1e9)),
                ('NVML::POWER', geopmdpy.topo.DOMAIN_BOARD_ACCELERATOR, gpu_idx, "avg",
                 f"Avg GPU-{gpu_idx} Power:     {{:.1f}}W"),
                ('NVML::UTILIZATION_ACCELERATOR', geopmdpy.topo.DOMAIN_BOARD_ACCELERATOR, gpu_idx, "avg",
                 f"Avg GPU-{gpu_idx} Utilization: {{:.3f}}"),
                ('DCGM::SM_ACTIVE', geopmdpy.topo.DOMAIN_BOARD_ACCELERATOR, gpu_idx, "avg",
                 f"Avg SM Active-{gpu_idx}: {{:.3f}}"),
                ('DCGM::DRAM_ACTIVE', geopmdpy.topo.DOMAIN_BOARD_ACCELERATOR, gpu_idx, "avg",
                 f"Avg DRAM Active-{gpu_idx}: {{:.3f}}"),
            ])

            # Add the geopm signal indices of the inputs for our tensorflow
            # model for this GPU. These must be in the same order as the
            # signals used when training the model.
            self._model_signal_idx_by_gpu.append([
                len(signal_list) - 5, # NVML::FREQUENCY
                len(signal_list) - 4, # NVML::POWER
                len(signal_list) - 3, # NVML::UTILIZATION_ACCELERATOR
                len(signal_list) - 2, # SM_ACTIVE
                len(signal_list) - 1, # DRAM_ACTIVE
            ])
            self._gpu_activity_signal_idx.append(
                len(signal_list) - 2 # SM_ACTIVE
            )
            self._gpu_frequency_signal_idx.append(
                len(signal_list) - 5 # NVML::FREQUENCY
            )
            self._gpu_energy_signal_idx.append(
                len(signal_list) - 6 # NVML::TOTAL_ENERGY_CONSUMPTION
            )

        super().__init__(period=period, signal_list=signal_list)

    def run_begin(self, policy):
        self._frequency_control_accumulator = [0] * self._gpu_count
        self._is_roi = False
        self._roi_start_time = None
        self._roi_end_time = None
        self._last_time = None
        self._roi_start_gpu_energies = None
        self._roi_end_gpu_energies = None
        self._roi_start_cpu_energies = None
        self._roi_end_cpu_energies = None

        # Track both the frequencies between start and end of ROI as well
        # as the frequencies from the start until now. The latter may be 
        # needed since we don't know if current inactivity is post-ROI or just
        # a period of non-utilization within the ROI.
        self._gpu_cycle_sums_since_start_of_roi = [0] * self._gpu_count # Sum of cycles since the start of the ROI
        self._roi_gpu_cycle_sums = None # Sum of cycles in the ROI

        if self._freq_gpu is not None:
            os.system(f'geopmwrite NVML::FREQUENCY_CONTROL board 0 {self._freq_gpu}')

        super().run_begin(policy)

    def get_controls(self):
        if self._freq_controls:
            return [('NVML::FREQUENCY_CONTROL', geopmdpy.topo.DOMAIN_BOARD_ACCELERATOR, gpu_idx)
                    for gpu_idx in range(self._gpu_count)]
        else:
            return []

    def agent_update(self, delta_signals, delta_time, signals, time):
        if self._freq_controls:
            # Defaults for the first update loop.
            if self._freq_gpu is not None:
                controls = [self._freq_gpu] * self._gpu_count
            else:
                controls = [DEFAULT_INITIAL_GPU_FREQUENCY] * self._gpu_count
        else:
            controls = []

        trace_fields = []
        debug_fields = []

        if self.loop_idx() > 0:
            gpu_activities = [signals[activity_idx] for activity_idx in self._gpu_activity_signal_idx]
            gpu_energies = [signals[energy_idx] for energy_idx in self._gpu_energy_signal_idx]
            cpu_energies = [signals[energy_idx] for energy_idx in self._cpu_energy_signal_idx]
            gpu_cycles = [signals[frequency_idx] * delta_time for frequency_idx in self._gpu_frequency_signal_idx]
            self._last_time = time

            if self._roi_start_time is not None:
                # We already encountered the start of the estimated ROI, so
                # accumulate on the GPU cycle counter.
                self._gpu_cycle_sums_since_start_of_roi = [
                    original + additional
                    for original, additional
                    in zip(self._gpu_cycle_sums_since_start_of_roi, gpu_cycles)]

            if any(activity >= GPU_ACTIVITY_CUTOFF for activity in gpu_activities):
                # We are definitely within the estimated ROI at this time.
                # If we previously hit a maybe-the-end, clear it.
                self._roi_end_time = None
                self._roi_gpu_cycle_sums = None

                if self._roi_start_time is None:
                    self._roi_start_time = time
                    self._roi_start_gpu_energies = gpu_energies
                    self._roi_start_cpu_energies = cpu_energies
            elif all(activity < GPU_ACTIVITY_CUTOFF for activity in gpu_activities):
                # We might not be in the ROI any more. Update the end-of-roi
                # metrics, but keep accumulating ROI counters just in case we
                # later detect that this was just a low activity phase.
                if self._roi_end_time is None:
                    self._roi_end_time = time
                    self._roi_end_gpu_energies = gpu_energies
                    self._roi_end_cpu_energies = cpu_energies
                if self._roi_gpu_cycle_sums is None:
                    self._roi_gpu_cycle_sums = self._gpu_cycle_sums_since_start_of_roi

            for gpu_idx in range(self._gpu_count):
                tensor = ([signals[signal_idx] for signal_idx in self._model_signal_idx_by_gpu[gpu_idx]] +
                          [self._phi])
                if self._freq_controls and not len(tensor) == 0:
                    with tf.device("/cpu:0"):
                        desired_freq = 1e9 * self._model(np.array([tensor])).numpy().tolist()[0][0]
                        controls[gpu_idx] = desired_freq
                        debug_fields.append(desired_freq)
                    self._frequency_control_accumulator[gpu_idx] += controls[gpu_idx]
            trace_fields = signals

        return controls, [delta_time] + trace_fields, debug_fields


    def get_debug_fields(self):
        # Add trace columns to indicate which frequencies are requested by the
        # neural network. These could be greater than the frequencies actually
        # achieved by the GPU.
        return [f'DBG::FREQ-board_accelerator-{gpu}' for gpu in range(self._gpu_count)]

    def get_report(self):
        report = [super().get_report()]

        if self._roi_start_time is not None:
            if self._roi_end_time is None:
                self._roi_end_time = self._last_time
            time_in_roi = self._roi_end_time - self._roi_start_time
            report.append(f'Time in ROI: {time_in_roi}')

            gpu_energies = [e2 - e1
                for e1, e2 in zip(self._roi_start_gpu_energies, self._roi_end_gpu_energies)]
            for gpu_idx, gpu_roi_energy in enumerate(gpu_energies):
                report.append(f'GPU-{gpu_idx} Energy in ROI: {gpu_roi_energy}')

            cpu_energies = [e2 - e1
                for e1, e2 in zip(self._roi_start_cpu_energies, self._roi_end_cpu_energies)]
            for cpu_roi_energy in cpu_energies:
                report.append(f'CPU Energy in ROI: {cpu_roi_energy}')

            if self._roi_gpu_cycle_sums is not None:
                gpu_frequencies = [cycles / time_in_roi
                                   for cycles in self._roi_gpu_cycle_sums]
                for gpu_idx, gpu_frequency in enumerate(gpu_frequencies):
                    report.append(f'GPU-{gpu_idx} Average Frequency in ROI: {gpu_frequency}')

        if self._freq_controls:
            report.extend([
                'Avg requested GPU-{} freq: {:.2f}GHz'.format(
                    gpu_idx,
                    self._frequency_control_accumulator[gpu_idx] / (self.loop_idx() - 1) / 1e9)
                for gpu_idx in range(self._gpu_count)])

        return '\n'.join(report)

def add_args(parser):
    parser.add_argument('--model', '-m', default="nn_mixbench",
                        help='Path to the tensorflow model directory.')
    parser.add_argument('--no-freq-controls', '-n', dest='no_freq_controls', action='store_true',
                        help='Turn off frequency controls (only monitoring).')
    parser.add_argument('--phi', type=float, default=0.0,
                        help='Balance between energy and performance importance.')
    parser.add_argument('--freq-gpu', '-g', dest='freq_gpu', type=float,
                        help='Set GPU frequency in GHz at run startup.')

def parse_common_args(parser=None):
    if parser is None:
        help_text = (
            "Run GEOPM ML agent controller with an app. App arguments should come after "
            "-- safe execution.\nExample:\n"
            f"    ./{os.path.basename(__file__)} srun -N 1 -n 4 -- echo Hello World")
        parser = argparse.ArgumentParser(
            description=help_text,
            formatter_class=argparse.RawTextHelpFormatter)
    add_args(parser)
    return base_agent.parse_common_args(parser=parser)


def create_instance(args):
    return MLAgentGPU(model=args.model, period=args.control_period,
                      freq_controls=not args.no_freq_controls,
                      freq_gpu=args.freq_gpu, phi=args.phi)


def main():
    ''' Default main method for ML agent runs.
    '''
    args, app_args = parse_common_args()
    agent = create_instance(args)
    base_agent.run_controller(agent, args, app_args)


if __name__ == '__main__':
    main()
