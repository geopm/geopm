#!/usr/bin/env python3
#  Copyright (c) 2015 - 2024, Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#

import matplotlib.pyplot as plt
import sys
import os
import glob

def rapl_pkg_limit_plot(file_name_prefix):
    """Usage: rapl_pkg_limit_plot.py PREFIX

    PREFIX: Path to output files created by running the
    rapl_pkg_limit_test application.  The glob pattern

    PREFIX*.out

    is parsed.  Note that PREFIX may include directory paths.

    """
    for file_name in glob.glob(file_name_prefix + '*.out'):
        limit_key = 'Power limit (Watts): '
        time_key = 'Total time (seconds): '
        meas_key = 'Average power '
        limit_list = []
        time_list = []
        meas_list = []
        with open(file_name) as fid:
            for line in fid.readlines():
                if line.startswith(limit_key):
                    limit_list.append(float(line.split(':')[1]))
                elif line.startswith(time_key):
                    time_list.append(float(line.split(':')[1]))
                elif line.startswith(meas_key):
                    meas_list.append(float(line.split(':')[1]))

        if (len(limit_list) != len(time_list) or
            (len(limit_list) != len(meas_list) and len(limit_list) * 2 != len(meas_list))
           ):
            raise RuntimeError('Failed to parse file {}'.format(file_name))

        # If data is generated from a dual socket system, duplicate the elements in
        # the limit_list for proper plotting.
        if len(limit_list) * 2 == len(meas_list):
            limit_list = [val for val in limit_list for _ in (0, 1)]

        limit_uniq = sorted(list(set(limit_list)))
        base_name = os.path.splitext(os.path.basename(file_name))[0]
        if base_name.startswith('rapl_pkg_limit_test_'):
            base_name = base_name[len('rapl_pkg_limit_test_'):]
        out_name = os.path.splitext(file_name)[0] + '.png'
        plt.plot(limit_uniq, limit_uniq, '-')
        plt.plot(limit_list, meas_list, '.')
        plt.title(base_name)
        plt.xlabel('RAPL setting (Watts)')
        plt.ylabel('RAPL power measured (Watts)')
        plt.savefig(out_name)
        plt.close()

if __name__ == '__main__':
    if len(sys.argv) != 2 or sys.argv[1] == '--help' or sys.argv[1] == '-h':
        sys.stdout.write(rapl_pkg_limit_plot.__doc__)
    else:
        rapl_pkg_limit_plot(sys.argv[1])
