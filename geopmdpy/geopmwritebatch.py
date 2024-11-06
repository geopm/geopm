#!/usr/bin/env python3
#  Copyright (c) 2015 - 2024 Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#

"""Use batch interface to write controls
    Examples:

        printf 'CPU_FREQUENCY_MAX_CONTROL board 0 2e9\\nCPU_FREQUENCY_MIN_CONTROL board 0 2e9\\n' > batch-config.txt
        ./geopmwritebatch.py batch-config.txt

            or

        printf 'CPU_FREQUENCY_MAX_CONTROL board 0 2e9\\nCPU_FREQUENCY_MIN_CONTROL board 0 2e9\\n' | ./geopmwritebatch.py
"""

import sys
from geopmdpy import pio
from geopmdpy import __version_str__


def run(input_stream):
    requests = [line.split() for line in input_stream.readlines()]
    ctl_idx = [pio.push_control(rr[0], rr[1], int(rr[2])) for rr in requests]
    for ii, rr in enumerate(requests):
        pio.adjust(ctl_idx[ii], float(rr[3]))
    pio.write_batch()

def main():
    if len(sys.argv) > 1:
        if sys.argv[1] == '--help':
            print(__doc__)
            return 0
        if sys.argv[1] == '--version':
            print(__version_str__)
            return 0
        with open(sys.argv[1]) as input_stream:
            run(input_stream)
    else:
        run(sys.stdin)

if __name__ == '__main__':
    main()
