#
#  Copyright (c) 2015 - 2023, Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
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


def add_min_frequency(parser):
    parser.add_argument('--min-frequency', dest='min_frequency',
                        action='store', type=float, default=None,
                        help='bottom core frequency limit for the sweep')


def add_max_frequency(parser):
    parser.add_argument('--max-frequency', dest='max_frequency',
                        action='store', type=float, default=None,
                        help='top core frequency limit for the sweep')


def add_step_frequency(parser):
    parser.add_argument('--step-frequency', dest='step_frequency',
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


def add_performance_metric(parser):
    parser.add_argument('--performance-metric', dest='performance_metric',
                        action='store', type=str, default='FOM',
                        help='metric to use for performance (default: figure of merit)')


def add_analysis_dir(parser):
    parser.add_argument('--analysis-dir', dest='analysis_dir',
                        action='store', default='analysis',
                        help='directory for output analysis files')
