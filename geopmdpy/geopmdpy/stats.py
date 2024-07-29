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

from session import Session, get_parser

class Stats:
    def update_dataframe(signals):
        df = pd.DataFrame(signals)

    def main():
        err = 0
        args = get_parser().parse_args()
        try:
            if args.version:
                print(__version_str__)
                return 0
            sess = Session()
            sess.run(run_time=args.time, period=args.period, pid=args.pid, print_header=args.print_header, update_dataframe, True)
        except RuntimeError as ee:
            if 'GEOPM_DEBUG' in os.environ:
                # Do not handle exception if GEOPM_DEBUG is set
                raise ee
            sys.stderr.write('Error: {}\n\n'.format(ee))
            err = -1
        return err

