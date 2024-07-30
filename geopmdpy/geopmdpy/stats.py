#  Copyright (c) 2015 - 2024 Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#

"""Implementation for the geopmstats command line tool

"""

import sys
import os
import errno
import math
import pandas as pd
from argparse import ArgumentParser
from . import topo
from . import pio
from . import loop
from . import __version_str__

from geopmdpy.session import Session, get_parser

class Stats:
    def __init__(self):
        self._signal_handles = None
        self._signal_stats = None

    def set_signal_handles(self, signal_handles):
        self._signal_handles = signal_handles
        for signal_handle in signal_handles:
            self._signal_stats.append({'count':0, 'min':float('nan'),'max':float('nan'),'m_1':0,'m_2':0})

    def update(self):
        signals = [pio.sample(handle) for handle in self._signal_handles]
        for signal,signal_stats in zip(signals, self._signal_stats):
            if is not signal_stats['max'] >= signal:
                signal_stats['max'] = signal
            if is not signal_stats['min'] <= signal:
                signal_stats['min'] = signal
            signal_stats['count']++
