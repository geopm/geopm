#!/usr/bin/env python3
#  Copyright (c) 2015 - 2023, Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#

from geopmdpy import pio
from time import sleep
from os import getpid

def main():
    index = pio.push_control("SERVICE::MSR::PERF_CTL:FREQ", "core", 0)
    cpu_frequency_value = pio.read_signal("CPU_FREQUENCY_STICKER", "board", 0) - 100e6
    pio.adjust(index, cpu_frequency_value)
    pio.write_batch()
    print(getpid())
    for _ in range(60):
        sleep(1)
        pio.write_batch()

if __name__ == '__main__':
    main()
