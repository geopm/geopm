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
Describes the best known configuration for IntelAI.
'''

import os
import textwrap
import subprocess

from .. import apps


class Resnet50AppConf(apps.AppConf):

    @staticmethod
    def name():
        return 'resnet50'

    def __init__(self, mach, num_nodes):
        #TODO: provide options for
        #   1) synthetic vs real
        #   2) training vs inference
        #   3) batch size
        #   4) iterations
        #   5) sockets

        benchmark_dir = os.path.dirname(os.path.abspath(__file__))
        self._IntelAI_path = os.path.join(benchmark_dir, 'IntelAIModel/')
        self._num_nodes = num_nodes
        self._graph = 'resnet50_fp32_pretrained_model.pb'
        self._graph_location = "{}{}{}".format(self._IntelAI_path, '/pretrained_models/', self._graph)
        self._model = 'resnet50'
        self._data_location = "{}{}".format(self._IntelAI_path, 'datasets/coco/tf_records/')
        reserved_cores = 2;

        if num_nodes == 1:
            #TODO: remove the /2 here when enabling multi socket socket-id -1 etc in the launch_benchmark args
            self._cpu_per_rank = (mach.num_core())/2 - reserved_cores
            self._num_rank_per_node = 1
        else:
            raise RuntimeError("An unsupported number of nodes ({}) was provided.  Supported node count: 1".format(num_nodes))

    #Sets OMP Num Threads
    def get_cpu_per_rank(self):
        return self._cpu_per_rank

    #Sets ranks per node
    def get_rank_per_node(self):
        # TODO: use self._machine_file to determine
        return self._num_rank_per_node

    #def get_bash_setup_commands(self):
    #    pass

    def get_bash_exec_path(self):
        binary_name = 'benchmarks/launch_benchmark.py'
        return  "{} {}".format('python3 -m mpi4py ', os.path.join(self._IntelAI_path, binary_name))
        #TODO: replace with print_affinity from examples folder to check pinning

    def get_bash_exec_args(self):
        #Synthetic
        return " --in-graph {}  --model-name {} --framework tensorflow --precision fp32 --mode inference --batch-size 128 --benchmark-only --socket-id 0".format(self._graph_location, self._model)

        #Real data
        #return " --data-location {} --in-graph ${input}  --model-name ${model_name} --framework tensorflow --precision fp32 --mode inference --batch-size 64 --benchmark-only --socket-id 0".format(self._data_location, self._graph_location, self._model)

    def trial_setup(self, run_id, output_dir):
        pass

    def trial_teardown(self, run_id, output_dir):
        self._run_id = run_id
        pass

    def parse_fom(self, log_path):
        #Throughput: ##.### images/sec
        key = "Throughput: "
        #filename =  '{}_*_{}.log'.format(self._model, self._run_id)
        with open(log_path) as fid:
            for line in fid.readlines():
                line = line.rstrip()  # remove '\n' at end of line
                if key in line:
                    throughput = float(line.split()[1])
                    break

        return throughput;
        pass
