#!/usr/bin/python3
#
#  Copyright (c) 2015 - 2025 Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#

"""
Monitor agent for GEOPM Python interface.

This module provides a MonitorAgent class for use with the GEOPM
Python session interface. The MonitorAgent can be used to run
a monitoring session with a default set of signals, and supports
a --hi-res option to sample all signals at their native resolution.

Example usage:
    python -m geopmpy.monitor
    python -m geopmpy.monitor --hi-res
"""

from geopmdpy import pio

from geopmdpy.session import main
from geopmdpy.session import Agent
from geopmdpy.exporter import default_requests

class MonitorAgent(Agent):
    """Agent for monitoring a default set of signals in a GEOPM session.

    The MonitorAgent provides a --hi-res option to sample all signals
    at their native resolution (all domains and indices). By default,
    signals are sampled at the board domain.

    Command-line options:
      --hi-res   Measure signals at native resolution (all domains/indices).

    Example:
        python -m geopmpy.monitor --hi-res
    """
    def __init__(self):
        """Initialize the MonitorAgent."""
        super().__init__()
        self._hi_res = False

    def help(self):
        """Help documentation

        """
        return 'The monitor agent provides a default signal configuration of available metrics relating to power, energy, frequency and temperature.'

    def update_parser(self, parser):
        """Add --hi-res argument to the parser.

        Args:
            parser (argparse.ArgumentParser): The parser to update.

        Returns:
            argparse.ArgumentParser: The updated parser.
        """
        parser.add_argument('--hi-res', action='store_true',
                            help='Measure signals at native resolution (all domains/indices)')
        return parser

    def update_args(self, args):
        """Store the --hi-res argument in the agent.

        Args:
            args (argparse.Namespace): Parsed command-line arguments.

        Returns:
            argparse.Namespace: The (possibly updated) arguments.
        """
        self._hi_res = args.hi_res
        return args

    def signal_config_override(self):
        """Provide a default signal configuration for monitoring.

        Returns:
            str: Signal configuration string for the session.
        """
        if self._hi_res:
            suffix = ' * *\n'
        else:
            suffix = ' board 0\n'
        signals = [dd[0] for dd in default_requests()]
        return 'TIME board 0\n' + suffix.join(signals) + suffix

if __name__ == '__main__':
    main(MonitorAgent())

