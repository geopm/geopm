#!/usr/bin/env python3
#
#  Copyright (c) 2015 - 2022, Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#
"""
This package provides a StaticAgent class to monitor processes and optionally
apply GEOPM control settings that last the duration of an application run.
"""

import os
import argparse
import yaml

import geopmdpy
import geopmdpy.runtime
import geopmdpy.pio
import geopmpy.reporter


class StaticAgent(geopmdpy.runtime.Agent):
    """GEOPM python agent that monitors an application and optionally writes
    one or more controls that last the duration of the application run.
    """

    def __init__(self, period_seconds=1, initial_controls=None):
        """Initialize the agent.

        period_seconds (float): Agent sampling period, in seconds.
        initial_controls (dict[str, float]): Dictionary mapping CONTROL_NAME to initial value
        """
        self._control_period = period_seconds
        self._initial_controls = dict() if initial_controls is None else initial_controls
        self._report = None
        self._profile = None
        geopmpy.reporter.init()

    def run_begin(self, policy, profile):
        # The GEOPM runtime already saved controls by now. It will take care of
        # restoring the original values of these controls after our run finishes.
        self._profile = profile
        for control_name, value in self._initial_controls.items():
            geopmdpy.pio.write_control(control_name, 'board', 0, value)

    def run_end(self):
        # Handle anything that should be managed after PlatformIO restores the
        # platform's pre-launch state.
        self._report = yaml.load(geopmpy.reporter.generate(self._profile, self.__class__.__name__),
                                 Loader=yaml.SafeLoader)
        self._report['Policy']['Initial Controls'] = self._initial_controls

    def get_signals(self):
        # This agent does not consume any PlatformIO signals.
        # Otherwise, this would return a list of names of the signals that the
        # agent needs to receive in its update() function.
        return []

    def update(self, signals):
        # This agent doesn't dynamically make decisions based on any signals
        # signals is a list of values corresponding to get_signals().
        # E.g., to get a dict[signal_name, value], use dict(zip(self.get_signals(), signals))

        # We only need to tell our reporter to update its snapshot of the
        # state of PlatformIO.
        geopmpy.reporter.update()

        # This agent does not dynamically modify any controls.
        # Otherwise, we would return a list of their *values* here.
        return []

    def get_controls(self):
        # This agent does not dynamically modify any controls.
        # Otherwise, we would return a list of their *names* here.
        return []

    def get_period(self):
        return self._control_period

    def get_report(self):
        # Optionally add more agent-specific contents to the report here. For
        # example, if the agent makes any dynamic decisions at execution time,
        # it might report a summary of its decisions or algorithmic state.
        result = None
        if self._report is not None:
            result = yaml.dump(self._report, default_flow_style=False, sort_keys=False)
        return result

def main():
    """Launch an application under the StaticAgent.
    """
    parser = argparse.ArgumentParser(
        description="Launch an application with a static agent. All positional "
                    "arguments and unrecognized options are forwarded to the "
                    "launched application's argv. Every option after '--' is "
                    "treated as an option for the wrapped application.")
    parser.add_argument('--geopm-control-period', default=1, type=float,
                        help='GEOPM Agent control period, in seconds.')
    parser.add_argument('--geopm-report', metavar='REPORT_FILE',
                        help='Write report output to REPORT_FILE.')
    parser.add_argument('--geopm-report-signals',
                        help='Additional signals to include in the report.')
    parser.add_argument('--geopm-initialize-control',
                        action='append',
                        help='Initialize a control to a given value, separated by an "=" '
                             'symbol. e.g., to run at a cpu frequency of 2 GHz: '
                             '--initialize-control CPU_FREQUENCY_MAX_CONTROL=2e9')
    parser.add_argument('--geopm-profile',
                        help='Override the report profile name. Default: the launched command')

    args, app_args = parser.parse_known_args()

    if len(app_args) == 0:
        parser.error("Must specify something to execute.")

    if app_args[0] == "--":
        app_args = app_args[1:]

    if len(app_args) == 0:
        parser.error("Must specify something to execute.")

    if args.geopm_report_signals:
        os.environ['GEOPM_REPORT_SIGNALS'] = args.geopm_report_signals

    initial_controls = dict()
    if args.geopm_initialize_control is not None:
        try:
            # Convert the list of CONTROL=value pairs to a dict of CONTROL->value
            for control_specification in args.geopm_initialize_control:
                control_name, value = control_specification.split('=')
                initial_controls[control_name] = float(value)
        except ValueError as e:
            parser.error(f'Invalid control specification for {control_specification}. Reason: {e}')

    agent = StaticAgent(period_seconds=args.geopm_control_period,
                        initial_controls=initial_controls)

    controller = geopmdpy.runtime.Controller(agent)
    report = controller.run(app_args, None)

    if args.geopm_report:
        with open(args.geopm_report, 'w') as f:
            print(report, file=f)


if __name__ == '__main__':
    main()
