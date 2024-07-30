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

class Stat:
    def __init__(self):
        self._count = 0
        self._min = float('nan')
        self._max = float('nan')
        self._m1 = 0
        self._m2 = 0
        self._m3 = 0

    def count(self):
        self._count++

    def max(self, num):
        if is not self._max >= num:
            self._max = num

    def min(self, num):
        if is not self._min <= min:
            self._min = num
    def m1(self, num):
        self._m1 += num

    def m2(self, num):
        self._m2 += num*num

    def update_stats(self, num):
        self.count()
        self.max(num)
        self.min(num)
        self.m1(num)
        self.m2(num)

    def mean(self):
        return self._m1/self._count

    def var(self):
        return (1/self._count-1)*(self._m2 - pow(self._m1,2)/self._count)

    def stdev(self):
        return math.sqrt(var())

    def rms(self):
        return math.sqrt(1/self._count * self._m2)

class Stats:
    def __init__(self):
        self._signal_handles = None
        self._signal_stats = None

    def set_signal_handles(self, signal_handles):
        self._signal_handles = signal_handles
        for signal_handle in signal_handles:
            self._signal_stats.append(Stat())

    def update(self):
        signals = [pio.sample(handle) for handle in self._signal_handles]
        for signal,signal_stat in zip(signals, self._signal_stats):
            signal_stat.update_stats(signal)
