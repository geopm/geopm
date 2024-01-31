#!/usr/bin/env python3
#
#  Copyright (c) 2015 - 2024, Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#

"""Basic sanity checks of trace files.  These methods can be used in
other tests, or this script can be run against a set of trace files
given as input.

"""

import sys
import glob
import unittest
import pandas

from integration.test import util


def read_meta_data(trace_file):
    agent = None
    with open(trace_file) as infile:
        for line in infile:
            if agent is None and line.startswith('#') and 'agent' in line:
                agent = line.split(': ')[-1]
            if agent is not None:
                break
    return agent


def check_sample_rate(trace_file, expected_sample_rate, verbose=False):
    """Check that sample rate is regular and fast.

    """
    print(trace_file)
    test = unittest.TestCase()

    trace_data = pandas.read_csv(trace_file, delimiter='|', comment='#')
    tt = trace_data
    max_mean = 0.01  # 10 millisecond max sample period
    max_nstd = 0.1   # 10% normalized standard deviation (std / mean)
    delta_t = tt['TIME'].diff()
    if verbose:
        sys.stdout.write('sample rates:\n{}\n'.format(delta_t.describe()))
    delta_t = delta_t.loc[delta_t != 0]
    test.assertGreater(max_mean, delta_t.mean())
    test.assertGreater(max_nstd, delta_t.std() / delta_t.mean())

    util.assertNear(test, delta_t.mean(), expected_sample_rate)

    # find outliers
    delta_t_out = delta_t[(delta_t - delta_t.mean()) >= 3*delta_t.std()]
    if verbose:
        sys.stdout.write('outliers (>3*stdev):\n{}\n'.format(delta_t_out.describe()))
    num_samples = len(delta_t)
    num_out = len(delta_t_out)
    # check that less than 1% of the samples are outliers
    test.assertLess(num_out, num_samples * 0.01)


if __name__ == '__main__':
    if len(sys.argv) < 2:
        sys.stderr.write('Usage: {} <trace file name or glob pattern>\n'.format(sys.argv[0]))
        sys.exit(1)

    trace_pattern = sys.argv[1]
    traces = glob.glob(trace_pattern)
    if len(traces) == 0:
        sys.stderr.write('No trace files found for pattern {}.\n'.format(trace_pattern))
        sys.exit(1)

    default_sample_rate = 0.005

    for tt in traces:
        agent = read_meta_data(tt)
        # TODO: check these for all agents, or just make this a CLI
        # option? what if different agent traces are in this glob?
        if agent in ['frequency_map']:
            sample_rate = 0.002
        else:
            sample_rate = default_sample_rate

        check_sample_rate(tt, sample_rate, verbose=True)
