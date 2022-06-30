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
This package provides a BaseAgent class for monitoring jobs. One can also
extends this class to implement more functionality around the convenience
provided by it.

In addition, the package provides parse_common_args and run_controller
convenience methods for easily creating command line for classes that extend
BaseAgent.
'''

import sys
import os
import argparse
from collections import OrderedDict
import socket
import yaml

import geopmdpy
import geopmdpy.runtime
import geopmpy.reporter


CSV_SEP_CHAR = "|"

DOMAIN_SEP_CHAR = "-"

TIME_SIGNAL = ("TIME", geopmdpy.topo.DOMAIN_BOARD, 0, "-")

DEFAULT_SIGNAL_LIST = [
    ("INSTRUCTIONS_RETIRED",         geopmdpy.topo.DOMAIN_BOARD,        0, "incr"),
    ("POWER_PACKAGE",                geopmdpy.topo.DOMAIN_BOARD,        0, "avg"),
    ("POWER_DRAM",                   geopmdpy.topo.DOMAIN_BOARD_MEMORY, 0, "avg"),
    ("FREQUENCY",                    geopmdpy.topo.DOMAIN_BOARD,        0, "avg"),
    ("MSR::UNCORE_PERF_STATUS:FREQ", geopmdpy.topo.DOMAIN_BOARD,        0, "avg"),
    ("TEMPERATURE_CORE",             geopmdpy.topo.DOMAIN_BOARD,        0, "avg"),
    ("ENERGY_DRAM",                  geopmdpy.topo.DOMAIN_BOARD_MEMORY, 0, "incr"),
    ("ENERGY_PACKAGE",               geopmdpy.topo.DOMAIN_BOARD,        0, "incr"),
]


class BaseAgent(geopmdpy.runtime.Agent):
    ''' Simple GEOPM python agent that only monitors jobs. It is similar to but not an
    equivalent of the GEOPM C++ monitor agent. In the general scheme, one can use this agent to
    monitor a running job or use it as a base class (or template) for agents with more
    functionality.

    The monitored signals are passed via the signal_list argument of the class constructor. This
    list defines the signal name, domain, instance, signal type (monotonically increasing or not)
    and the line format in the report for each signal. TIME signal is added by this class by
    default and need not to be defined by the user. An extending class should not override the
    get_signals method and instead should pass the monitored signal list via the constructor.
    '''

    def __init__(self, signal_list=DEFAULT_SIGNAL_LIST, period=1):
        ''' Initialize the base agent.

        Args

        signal_list (list of tuples): Each element of the list is a 5 element tuple that
        represents one signal: signal text, domain from geopmdpy.topo, domain
        instance, and an 'incr' or 'avg' string representing the type of signal.

        The 'incr' and 'avg' signal types determine if a signal is monotonically increasing or if it
        is an averaged value such as frequency or power. Monotonically increasing signals will
        appear in the final report as the diff of the values at the first and last update calls.
        Averaged signals will appear in the final report as the time weighted average among all
        samples.

        period (float): Agent sampling period, in seconds.
        '''
        self._signal_list = list(signal_list) + [TIME_SIGNAL]
        self._control_period = period

        # List of variables used in this class for documentation purposes and keeping lint happy.
        # These signals are initialized per run in run_begin.
        self._loop_idx = None
        self._signals_last = None
        self._last_time = None
        self._start_time = None
        self._signal_acc = None
        self._tfile = None
        self._hostname = socket.gethostname()
        geopmpy.reporter.init()

    def run_begin(self, policy):
        '''
        Args

        policy (tuple(str, bool)): A tuple of trace file name and True if the output should be
        appended to the file or if it should overwrite the file.
        '''
        if policy is None:
            tracefile, trace_append = None, False
        else:
            tracefile, trace_append = policy
        self._loop_idx = 0
        self._signals_last = None
        self._last_time = None
        self._start_time = None
        self._signal_acc = [0] * len(self._signal_list)

        self._tfile = None
        if tracefile is not None:
            if isinstance(tracefile, str):
                tfile_mode = "w"
                if trace_append:
                    tfile_mode = "a"
                self._tfile = open(tracefile, tfile_mode)
            else:
                self._tfile = tracefile
            self._tfile.write(CSV_SEP_CHAR.join(self.get_trace_header()) + "\n")

    def policy_repr(self, policy):
        ''' Create a string representation of a policy suitable for printing.
        ''' 
        return f"Trace file: {policy[0]} Append to trace: {policy[1]}"

    def get_signals(self):
        '''
        Extending class should not override this method.
        '''
        return [(signal[0], signal[1], signal[2]) for signal in self._signal_list]

    def get_debug_fields(self):
        return []

    def get_controls(self):
        return []

    def update(self, signals):
        '''
        Extending class should not override this method. Instead override agent_update.
        '''
        if self._loop_idx == 0:
            self._start_time = signals[-1]
            self._signals_last = list(signals[:-1])
            delta_time = 0
        elif self._loop_idx != 0:
            delta_time = signals[-1] - self._last_time

        # This variable contains delta_time multiplied versions of "avg" signals and delta versions
        # of the "incr" signals.
        processed_sigs = list(signals[:-1])
        # This variable contains the delta versions of the "incr" signals whereas "avg" signals are
        # kept as is.
        delta_sigs = list(signals[:-1])
        for idx, signal in enumerate(self._signal_list[:-1]):
            if signal[3] == "incr":
                processed_sigs[idx] = signals[idx] - self._signals_last[idx]
                delta_sigs[idx] = signals[idx] - self._signals_last[idx]
            elif signal[3] == "avg":
                if delta_time == 0:
                    # Explicitly doing this allows the final value to be 0 even if
                    # processed_sigs[idx] was 0 (i.e. first sample).
                    processed_sigs[idx] = 0
                else:
                    processed_sigs[idx] *= delta_time

        self._signal_acc = [aa + bb for (aa, bb) in zip(self._signal_acc, processed_sigs)]

        ret = self.agent_update(delta_sigs, delta_time, signals[:-1], signals[-1])
        geopmpy.reporter.update()

        if len(ret) == 2:
            controls, trace_fields = ret
            debug_fields = []
        elif len(ret) == 3:
            controls, trace_fields, debug_fields = ret

        if self._tfile is not None and trace_fields is not None:
            self._tfile.write(
                CSV_SEP_CHAR.join(
                    [str(signal) for signal in trace_fields] +
                    [str(control) for control in controls] +
                    [str(field) for field in debug_fields]) +
                "\n")

        self._signals_last = list(signals[:-1])
        self._last_time = signals[-1]
        self._loop_idx += 1

        return controls

    def loop_idx(self):
        '''
        Retuns

        The count of how many times update method was called on the agent by the controller.
        '''
        return self._loop_idx

    def agent_update(self, delta_signals, delta_time, signals, time):
        ''' This method is called once per update after the diff (delta) values are computed for
        signals. The default implementation is a noop but an extending class can make decisions
        on what controls to apply and what to print in the trace line. If this method returns
        control signal values, get_controls method need to be overridden appropriately.

        Args

        delta_signals (list of float): Diffed version of the signals that are marked as incr,
        actual signal value for signals marked as avg. In the same order as the signals
        in signal_list passed to the class constructor.

        delta_time (float): Diffed version of the time signal, in seconds.

        signals (list of float): Signal values as they are passed to the update method of BaseAgent.
        In the same order as the signals in signal_list passed to the class constructor.

        time (float): TIME signal value as it is passed to the update method of BaseAgent.

        Returns

        A tuple of controls and trace values.

        controls (list of float): Value per signal in get_controls method.

        trace_values (list of float): Value per signal in get_signals method prepended by the value
        for the TIME column.
        '''
        return [], [delta_time] + delta_signals

    def get_trace_header(self):
        '''
        Returns the header line for the trace output. An extending class should not have a need
        to override this.
        '''
        return (["TIME"] + 
                [sig[0] + DOMAIN_SEP_CHAR + geopmdpy.topo.domain_name(sig[1]) + "-" + str(sig[2])
                 for sig in self.get_signals()[:-1]] +
                ['control{sep}{name}{sep}{domain}{sep}{domain_idx}'.format(
                    sep=DOMAIN_SEP_CHAR,
                    name=control[0],
                    domain=geopmdpy.topo.domain_name(control[1]),
                    domain_idx=control[2])
                 for control in self.get_controls()] +
                self.get_debug_fields()
                )

    def get_period(self):
        return self._control_period

    def run_end(self):
        if self._tfile is not None and self._tfile is not sys.stdout:
            self._tfile.close()

    def get_report(self, profile):
        '''
        Returns a report dictionary specific to the run from the call to
        run_start to run_end. The output contains a line For each signal passed
        to the class constructor in signal_list. Avg signals are time averaged
        in the final value whereas the delta from run_end to run_start is
        reported for incr signals.

        Extending classes can override this method to print out the report in a
        different format.

        Returns

        Report dictionary.
        '''
        report = yaml.load(geopmpy.reporter.generate(profile, self.__class__.__name__),
                           Loader=yaml.SafeLoader)
        sig_list_w_time = [self._signal_list[-1]] + self._signal_list[:-1]
        for idx, val in enumerate(self.get_report_values().values()):
            signal_name = (
                sig_list_w_time[idx][0] + DOMAIN_SEP_CHAR
                + geopmdpy.topo.domain_name(sig_list_w_time[idx][1])
                + DOMAIN_SEP_CHAR + str(sig_list_w_time[idx][2])
            )

            report['Hosts'][self._hostname]['Application Totals'][signal_name] = val
        return yaml.dump(report, default_flow_style=False, sort_keys=False)

    def get_report_values(self):
        ''' Calculate the final report value of each monitored signal.

        Returns

        Dict of signal name to its final reported value.
        '''
        if self._loop_idx == 1:
            return {"TIME": 0}

        total_time = self._last_time - self._start_time

        modified_vals = list(self._signal_acc)
        for idx, _ in enumerate(modified_vals):
            if self._signal_list[idx][3] == "avg":
                modified_vals[idx] /= total_time

        headers = ["TIME"] + \
            [sig[0] + DOMAIN_SEP_CHAR + geopmdpy.topo.domain_name(sig[1]) + "-" + str(sig[2])
             for sig in self.get_signals()[:-1]]

        return OrderedDict(zip(headers, [total_time] + modified_vals))


def parse_common_args(help_text=None, parser=None, parse_known_args=True):
    '''
    Parses the command arguments that should be common to all classes that expand BaseAgent.

    Args

    help_text (str): Help text for the parser. Ignored if parser argument is defined. Default help
    text, if not defined.

    parser (argparse.ArgumentParser): Use an already defined argument parser. Allows for user to
    add more commands.

    parse_known_args (bool): If True (default), command line will expect additional arguments to
    be passed to the app and will return them. For safe parsing app arguments can be passed after
    a '--', which will not be returned.

    Returns

    A tuple of ArgumentParser and list of str:

    ArgumentParser object is for the agent/experiment. The second value (list of str) are the
    command line for running the app. This value is None if parse_known_args is False.
    '''

    if help_text is None:
        help_text = "Run GEOPM base agent with an app for monitoring. App arguments should come " \
                    "after -- for safe execution.\nExample:\n" \
                    "    ./{os.path.basename(__file__)}} srun -N 1 -n 4 -- echo Hello World"

    if parser is None:
        parser = argparse.ArgumentParser(description=help_text,
                                         formatter_class=argparse.RawTextHelpFormatter)
    parser.add_argument('--control-period', '-p', dest="control_period", default=1, type=float,
                        help='GEOPM Agent control period in secs.')
    parser.add_argument('--trace', '-t', nargs='?', const="",
                        help='Enable trace output. Input needs to be a filename or no argument.')
    parser.add_argument('--append-trace', dest="append_trace", action="store_true",
                        help='If trace is being written to a file, append instead of oevrwriting.')
    parser.add_argument('--report', '-r', nargs='?', const="",
                        help='Enable report output. Input needs to be a filename or no argument.')
    parser.add_argument('--profile',
                        help='Override the report profile name. Otherwise, the launched command is used')

    app_args = None
    if parse_known_args:
        args, app_args = parser.parse_known_args()

        if len(app_args) == 0:
            sys.stderr.write("ERROR: No command given to run.\n")
            sys.exit(1)

        if app_args[0] != "--" and "--" in app_args:
            sys.stderr.write("ERROR: Argument not known: " + app_args[0] + "\n")
            sys.exit(1)

        # -- puts everything into positional args then to app_args. For some reason, when there is
        # no positional arguments -- is passed to app_args (not the case if there were any
        # positional arguments).
        app_args = app_args[1:]
    else:
        args = parser.parse_args()

    if args.trace is not None:
        if args.trace != "":
            if os.path.isfile(args.trace):
                print("ERROR: File already exists: " + args.trace)
                sys.exit(1)
        else:
            args.trace = sys.stdout

    if args.report is not None and args.report != "":
        if os.path.isfile(args.report):
            print("ERROR: File already exists: " + args.report)
            sys.exit(1)

    return args, app_args


def run_controller(agent, args, app_args):
    '''
    Convenience method for running an agent via geopmdpy.runtime.Controller. Meant for classes
    that extend BaseAgent and use base_agent.parse_common_args to create a command line.

    Args

    agent (Instance of BaseAgent): Instance of BaseAgent or a class that extends it.

    args (argparse.ArgumentParser): Typically from the output of parse_common_args. Values from here
    are passed to controller run policy.

    app_args (list of str): Typically from the output of parse_common_args. Command line for running
    the app, split to individual strings.
    '''
    controller = geopmdpy.runtime.Controller(agent)
    report = controller.run(app_args, (args.trace, args.append_trace))

    if args.report is not None:
        if args.report != "":
            with open(args.report, 'w') as rpt:
                print(report, file=rpt)
        else:
            print(report)


def main():
    ''' Default main method for monitor runs.
    '''
    args, app_args = parse_common_args()
    agent = BaseAgent(period=args.control_period)

    run_controller(agent, args, app_args)


if __name__ == '__main__':
    main()
