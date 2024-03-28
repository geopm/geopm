#!/usr/bin/env python3
#  Copyright (c) 2015 - 2024, Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#

from geopmdpy import pio
from time import sleep
from os import getpid

def main():
    cpu_frequency_value = pio.read_signal("CPU_FREQUENCY_STICKER", "board", 0) - 100e6
    pio.write_control("SERVICE::MSR::PERF_CTL:FREQ", "core", 0, cpu_frequency_value)
    print(getpid())
    sleep(5)
    pio.write_control("SERVICE::MSR::PERF_CTL:FREQ", "core", 0, cpu_frequency_value)
    sleep(5)

if __name__ == '__main__':
    main()
