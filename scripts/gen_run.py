#!/usr/bin/env python

#  Copyright (c) 2015, 2016, 2017, 2018, 2019, Intel Corporation
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

# Used to generate GEOPM run scripts for SLURM

import os
import sys
import textwrap
import argparse


class DgemmBenchmark:
    @staticmethod
    def name():
        return 'dgemm'

    def __init__(self, num_nodes, ranks_per_node, threads_per_rank):
        self.num_nodes = num_nodes
        self.ranks_per_node = 2 if not ranks_per_node else ranks_per_node
        self.threads_per_rank = 17 if not threads_per_rank else threads_per_rank
        self.config_file = 'dgemm.config'

    def setup(self):
        return '''echo "{\"loop-count\":100, \"region\": [\"dgemm\"], \"big-o\": [28.0]}" > ''' + self.config_file

    def executable(self):
        return 'geopmbench'

    def exec_params(self):
        return self.config_file


class MinifeBenchmark:
    @staticmethod
    def name():
        return 'minife'

    def __init__(self, num_nodes, ranks_per_node, threads_per_rank):
        self.num_nodes = num_nodes
        # todo: change this to method providing the default; let caller do this override
        self.ranks_per_node = 1 if not ranks_per_node else ranks_per_node
        self.threads_per_rank = 34 if not threads_per_rank else threads_per_rank
        problem_sizes = {
            1: '"-nx=264 -ny=256 -nz=256"',
            64: '"-nx=1056 -ny=1024 -nz=1024"',
            128: '"-nx=1330 -ny=1290 -nz=1290"',
            256: '"-nx=1676 -ny=1625 -nz=1625"',
            512: '"-nx=2112 -ny=2048 -nz=2048"',  # scale each dimension of 1-node size by 512^(1/3)=8
        }
        if self.num_nodes not in problem_sizes:
            raise RuntimeError("No input size defined for minife on {} nodes".format(self.num_nodes))
        self.app_params = problem_sizes[self.num_nodes]
        self.exe_path = '/p/lustre2/guttman1/benchmarks/minife/miniFE_openmp-2.0-rc3/src/miniFE.x'

    def setup(self):
        return ''

    def executable(self):
        return self.exe_path

    def exec_params(self):
        return self.app_params


class NekboneBenchmark:
    @staticmethod
    def name():
        return 'nekbone'

    def __init__(self, num_nodes, ranks_per_node, threads_per_rank):
        self.num_nodes = num_nodes
        # todo: change this to method providing the default; let caller do this override
        self.ranks_per_node = 1 if not ranks_per_node else ranks_per_node
        self.threads_per_rank = 34 if not threads_per_rank else threads_per_rank

        nekbone_path = '/p/lustre2/guttman1/benchmarks/nekbone/nekbone-2.3.4/test/example1/'
        self.exe_path = os.path.join(nekbone_path, 'nekbone')
        self.app_params = os.path.join(nekbone_path, 'data.rea')

    def setup(self):
        return 'cp {} .'.format(self.app_params)

    def executable(self):
        return self.exe_path

    def exec_params(self):
        return 'ex1'


class AmgBenchmark:
    @staticmethod
    def name():
        return 'amg'

    def __init__(self, num_nodes, ranks_per_node, threads_per_rank):
        self.num_nodes = num_nodes
        self.ranks_per_node = 16 if not ranks_per_node else ranks_per_node
        self.threads_per_rank = 2 if not threads_per_rank else threads_per_rank
        if self.num_nodes == 128 and self.ranks_per_node == 16:
            self.app_params = '-problem 1 -n 96 96 96 -P 16 16 8'  # total product of -P must == total ranks
        else:
            raise RuntimeError("No input size defined for amg on {} nodes with {} ranks per node".format(self.num_nodes, self.ranks_per_node))
        self.exe_path = '/p/lustre2/guttman1/benchmarks/amg/AMG-master/test/amg'

    def setup(self):
        return ''

    def executable(self):
        return self.exe_path

    def exec_params(self):
        return self.app_params


if __name__ == '__main__':

    benchmarks = {
        'dgemm': DgemmBenchmark,
        'minife': MinifeBenchmark,
        'nekbone': NekboneBenchmark,
        'amg': AmgBenchmark,
        # todo: hacc
    }

    # todo: number of iterations
    parser = argparse.ArgumentParser()
    parser.add_argument('--benchmark', '-b', action='store', required=True,
                        help='Name of the benchmark to run.  One of: {}'.format(', '.join(benchmarks.keys())))
    parser.add_argument('--analysis-type', '-a', dest='analysis_type', action='store', default=None)
    parser.add_argument('--agent', '-A', action='store', default=None)
    parser.add_argument('--num-nodes', '-n', dest='num_nodes', type=int,
                        action='store', default=1)
    parser.add_argument('--ranks-per-node', '-r', dest='ranks_per_node', type=int,
                        action='store', default=None)
    parser.add_argument('--threads-per-rank', '-t', dest='threads_per_rank', type=int,
                        action='store', default=None)
    parser.add_argument('--do-debug', dest='do_debug', action='store_true', default=False,
                        help='Run a test size on a few nodes in the debug queue.')

    args = parser.parse_args(sys.argv[1:])

    if not (args.analysis_type or args.agent) or (args.analysis_type and args.agent):
        raise RuntimeError('Only one of --analysis-type or --agent may be set')

    if args.benchmark not in benchmarks:
        raise RuntimeError('No run parameters available for {}'.format(args.benchmark))

    # options
    benchmark = benchmarks[args.benchmark](args.num_nodes,
                                           args.ranks_per_node,
                                           args.threads_per_rank)
    num_nodes = benchmark.num_nodes
    ranks_per_node = benchmark.ranks_per_node
    threads_per_rank = benchmark.threads_per_rank

    do_debug = args.do_debug
    analysis_type = args.analysis_type
    agent = args.agent

    # common
    results_dir = '/p/lustre2/guttman1/analysis'

    # job config
    time_limit = '08:00:00'  # todo: depends on app
    if do_debug:
        queue = 'pdebug'
        time_limit = '00:30:00'
        if num_nodes > 4:
            raise RuntimeError('Too many nodes for debug queue')
    else:
        queue = 'pbatch'

    # app config
    app_name = benchmark.name()
    job_name = app_name  # TODO

    if analysis_type:
        outfile_name = '{}_{}node_{}rank_{}thread_{}.slurm'.format(app_name,
                                                                   num_nodes,
                                                                   ranks_per_node,
                                                                   threads_per_rank,
                                                                   analysis_type)
    elif agent:
        outfile_name = '{}_{}node_{}rank_{}thread_{}_agent.slurm'.format(app_name,
                                                                         num_nodes,
                                                                         ranks_per_node,
                                                                         threads_per_rank,
                                                                         agent)

    slurm_header = textwrap.dedent('''\
    #!/bin/bash
    #SBATCH -N {num_nodes}
    #SBATCH -J {job_name}
    #SBATCH -t {time_limit}
    #SBATCH -p {queue}
    #SBATCH --mail-type=END,FAIL
    #SBATCH -o {results_dir}/%j_{app_name}.out

    '''.format(num_nodes=num_nodes,
               job_name=job_name,
               time_limit=time_limit,
               queue=queue,
               results_dir=results_dir,
               app_name=app_name))

    # TODO: somewhat app dependent
    setup_steps = textwrap.dedent('''\
    set -x

    OUTDIR={results_dir}/${{SLURM_JOB_ID}}_{app_name}
    mkdir -p $OUTDIR
    cd $OUTDIR

    NUM_NODES=$SLURM_NNODES
    RANKS_PER_NODE={ranks_per_node}
    export OMP_NUM_THREADS={omp_threads}

    {app_input_create}
    APP_EXECUTABLE={app_exec}
    APP_PARAMS={app_params}

    export MPLBACKEND='Agg'

    module load mkl
    source ~/env.sh
    source ~/pyenv/geopm/bin/activate

    APP_NAME={app_name}
    PROFILE_NAME=$APP_NAME

    '''.format(num_nodes=num_nodes,
               ranks_per_node=ranks_per_node,
               omp_threads=threads_per_rank,
               job_name=job_name,
               time_limit=time_limit,
               queue=queue,
               results_dir=results_dir,
               app_input_create=benchmark.setup(),
               app_exec=benchmark.executable(),
               app_params=benchmark.exec_params(),
               app_name=app_name))

    if analysis_type:
        run_commands = textwrap.dedent('''\
        geopmanalysis {analysis_type} \\
            --geopm-analysis-launcher=srun \\
            -N $NUM_NODES -n $(($RANKS_PER_NODE * $NUM_NODES)) \\
            --geopm-analysis-summary \\
            --geopm-analysis-profile-prefix=$PROFILE_NAME \\
            -- $APP_EXECUTABLE $APP_PARAMS

        '''.format(analysis_type=analysis_type,
               num_nodes=num_nodes))
    elif agent:
        # todo: this is messy
        nans=''
        if agent == 'energy_efficient' or agent == 'frequency_map':
            nans = '-p NAN,NAN'
        elif agent == 'power_governor':
            nans = '-p NAN'
        elif agent == 'power_balancer':
            nans = '-p NAN,NAN,NAN,NAN'

        run_commands = textwrap.dedent('''\
        geopmagent -a {agent} {nans} > {agent}_policy.json
        geopmlaunch srun -N $NUM_NODES -n $(($RANKS_PER_NODE * $NUM_NODES)) \\
            --geopm-agent={agent} \\
            --geopm-policy={agent}_policy.json \\
            --geopm-report=$PROFILE_NAME.report \\
            -- $APP_EXECUTABLE $APP_PARAMS

        '''.format(agent=agent, nans=nans))
    else:
        assert analysis_type or agent, 'should not get here'

    with open(outfile_name, 'w') as outfile:
        outfile.write(slurm_header)
        outfile.write(setup_steps)
        outfile.write(run_commands)

    sys.stdout.write('Wrote script to {}\n'.format(outfile_name))
