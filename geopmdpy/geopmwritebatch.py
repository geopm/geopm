#!/usr/bin/env python3
import sys
from geopmdpy import pio

# Use batch interface to write controls
#
# Example:
#
# printf 'CPU_FREQUENCY_MAX_CONTROL board 0 2e9\nCPU_FREQUENCY_MIN_CONTROL board 0 2e9\n' | ./geopmwritebatch.py
#
requests = [line.split() for line in sys.stdin.readlines()]
ctl_idx = [pio.push_control(rr[0], rr[1], int(rr[2])) for rr in requests]
for ii, rr in enumerate(requests):
    pio.adjust(ctl_idx[ii], float(rr[3]))
pio.write_batch()
