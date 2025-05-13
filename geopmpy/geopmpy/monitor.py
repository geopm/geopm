#!/usr/bin/python3
#
#  Copyright (c) 2015 - 2025 Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#
from geopmdpy import pio

from geopmdpy.session import main
from geopmdpy.session import Agent
from geopmdpy.exporter import default_requests

class MonitorAgent(Agent):
    def __init__(self):
        pass

    def update_parser(self, parser):
        parser.add_argument('--hi-res', action='store_true',
                            help='Mesure signals at native resolution')
        return parser

    def update_args(self, args):
        self._hi_res = args.hi_res
        return args

    def signal_config_override(self):
        if self._hi_res:
            suffix = ' * *\n'
        else:
            suffix = ' board 0\n'
        signals = [dd[0] for dd in default_requests()]
        return 'TIME board 0\n' + suffix.join(signals) + suffix

if __name__ == '__main__':
    main(MonitorAgent())

