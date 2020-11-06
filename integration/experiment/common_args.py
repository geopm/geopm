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
    add_enable_traces(parser)
    add_enable_profile_traces(parser)


def add_output_dir(parser):
    parser.add_argument('--output-dir', dest='output_dir',
                        action='store', default='.',
                        help='location for reports and other output files')


def add_trial_count(parser):
    parser.add_argument('--trial-count', dest='trial_count',
                        action='store', type=int, default=2,
                        help='number of experiment trials to launch')


def add_node_count(parser):
    parser.add_argument('--node-count', dest='node_count',
                        default=1, type=int,
                        help='number of nodes to use for launch')


def add_show_details(parser):
        parser.add_argument('--show-details', dest='show_details',
                            action='store_true', default=False,
                            help='print additional data analysis details')


def add_min_power(parser):
    parser.add_argument('--min-power', dest='min_power',
                        action='store', type=float, default=None,
                        help='bottom power limit for the sweep')


def add_max_power(parser):
    parser.add_argument('--max-power', dest='max_power',
                        action='store', type=float, default=None,
                        help='top power limit for the sweep')


def add_step_power(parser):
    parser.add_argument('--step-power', dest='step_power',
                        action='store', type=float, default=10,
                        help='increment between power steps for sweep')


def add_label(parser):
    parser.add_argument('--label', action='store', default="APP",
                        help='name of the application to use for plot titles')


def add_min_core_frequency(parser):
    parser.add_argument('--min-core-frequency', dest='min_core_frequency',
                        action='store', type=float, default=None,
                        help='bottom core frequency limit for the sweep')


def add_max_core_frequency(parser):
    parser.add_argument('--max-core_frequency', dest='max_core_frequency',
                        action='store', type=float, default=None,
                        help='top core frequency limit for the sweep')


def add_step_core_frequency(parser):
    parser.add_argument('--step-core-frequency', dest='step_core_frequency',
                        action='store', type=float, default=None,
                        help='increment between core frequency steps for sweep')


def add_run_max_turbo(parser):
    parser.add_argument("--run-max-turbo", dest="run_max_turbo",
                        action='store_true', default=False,
                        help='add extra run to the experiment at maximum turbo frequency')


def add_use_stdev(parser):
    parser.add_argument('--use-stdev', dest='use_stdev',
                        action='store_true', default=False,
                        help='use standard deviation instead of min-max spread for error bars')


def add_cool_off_time(parser):
    parser.add_argument('--cool-off-time', dest='cool_off_time',
                        action='store', type=float, default=60,
                        help='wait time between workload execution for cool down')


def add_agent_list(parser):
    parser.add_argument('--agent-list', dest='agent_list',
                        action='store', type=str, default=None,
                        help='comma separated list of agents to be compared')


def add_enable_traces(parser):
    parser.add_argument('--enable-traces', dest='enable_traces',
                         action='store_const', const=True,
                         default=False, help='Enable trace generation')
    parser.add_argument('--disable-traces', dest='enable_traces',
                        action='store_const', const=False,
                        help='Disable trace generation')


def add_disable_traces(parser):
    add_enable_traces(parser)
    parser.set_defaults(enable_traces=True)


def add_enable_profile_traces(parser):
    parser.add_argument('--enable-profile-traces', dest='enable_profile_traces',
                         action='store_const', const=True,
                         default=False, help='Enable profile trace generation')
    parser.add_argument('--disable-profile-traces', dest='enable_profile_traces',
                        action='store_const', const=False,
                        help='Disable profile trace generation')


def add_disable_profile_traces(parser):
    add_enable_profile_traces(parser)
    parser.set_defaults(enable_profile_traces=True)
