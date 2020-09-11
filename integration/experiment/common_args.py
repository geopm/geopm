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

'''
Common command line arguments for experiments.
'''

def setup_run_args(parser):
    """Add common arguments for all run scripts:
       --output-dir --node-count --trial-count --cool-off-time
    """
    add_output_dir(parser)
    add_node_count(parser)
    add_trial_count(parser)
    add_cool_off_time(parser)


def add_output_dir(parser, default='.'):
    parser.add_argument('--output-dir', dest='output_dir',
                        action='store', default=default,
                        help='location for reports and other output files')


def add_trial_count(parser, default=2):
    parser.add_argument('--trial-count', dest='trial_count',
                        action='store', type=int, default=default,
                        help='number of experiment trials to launch')


def add_node_count(parser, default=1):
    parser.add_argument('--node-count', dest='node_count',
                        default=default, type=int,
                        help='number of nodes to use for launch')


def add_show_details(parser, default=False):
        parser.add_argument('--show-details', dest='show_details',
                            action='store_true', default=default,
                            help='print additional data analysis details')


def add_min_power(parser, default=None):
    parser.add_argument('--min-power', dest='min_power',
                        action='store', type=float, default=default,
                        help='bottom power limit for the sweep')


def add_max_power(parser, default=None):
    parser.add_argument('--max-power', dest='max_power',
                        action='store', type=float, default=default,
                        help='top power limit for the sweep')


def add_step_power(parser, default=10):
    parser.add_argument('--step-power', dest='step_power',
                        action='store', type=float, default=default,
                        help='increment between power steps for sweep')


def add_label(parser, default="APP"):
    parser.add_argument('--label', action='store', default=default,
                        help='name of the application to use for plot titles')


def add_min_frequency(parser, default=None):
    parser.add_argument('--min-frequency', dest='min_frequency',
                        action='store', type=float, default=default,
                        help='bottom frequency limit for the sweep')


def add_max_frequency(parser, default=None):
    parser.add_argument('--max-frequency', dest='max_frequency',
                        action='store', type=float, default=default,
                        help='top frequency limit for the sweep')


def add_step_frequency(parser, default=None):
    parser.add_argument('--step-frequency', dest='step_frequency',
                        action='store', type=float, default=default,
                        help='increment between frequency steps for sweep')


def add_use_stdev(parser, default=False):
    parser.add_argument('--use-stdev', dest='use_stdev',
                        action='store_true', default=default,
                        help='use standard deviation instead of min-max spread for error bars')


def add_cool_off_time(parser, default=60):
    parser.add_argument('--cool-off-time', dest='cool_off_time',
                        action='store', type=float, default=default,
                        help='wait time between workload execution for cool down')


def add_agent_list(parser, default=None):
    parser.add_argument('--agent-list', dest='agent_list',
                        action='store', type=str, default=default,
                        help='comma separated list of agents to be compared')
