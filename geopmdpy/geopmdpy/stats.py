#  Copyright (c) 2015 - 2024 Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#

"""Implementation for the geopmstats command line tool

"""

import sys
import os
import errno
import math
from . import gffi

gffi.gffi.cdef("""

int geopm_stats_collector(int num_requests, struct geopm_request_s *requests);
int geopm_stats_collector_update(void);
int geopm_stats_collector_report(size_t max_report_size, char *report);

""")
_dl = gffi.get_dl_geopmd()

def collector(signal_config):
    """Create stats collector

    Args:
        signal_config (list((str, int, int))): List of requested
            signals where each tuple represents
            (signal_name, domain_type, domain_idx).

    """
    global _dl
    num_signal = len(signal_config)
    if num_signal == 0:
        raise RuntimeError('geopm_stats_collector() failed: length of input is zero')

    signal_config_carr = gffi.gffi.new(f'struct geopm_request_s[{num_signal}]')
    for idx, req in enumerate(signal_config):
        signal_config_carr[idx].domain = req[1]
        signal_config_carr[idx].domain_idx = req[2]
        signal_config_carr[idx].name = req[0].encode()

    err = _dl.geopm_stats_collector(num_signal,
                                    signal_config_carr)
    if err < 0:
        raise RuntimeError('geopm_stats_collector() failed: {}'.format(error.message(err)))

def collector_update():
    global _dl
    err = _dl.geopm_stats_collector_update()
    if err < 0:
        raise RuntimeError('geopm_stats_collector_update() failed: {}'.format(error.message(err)))

def collector_report():
    global _dl
    report_max = 262144
    report_cstr = gffi.gffi.new("char[]", report_max)
    err = _dl.geopm_stats_collector_report(report_max, report_cstr)
    if err < 0:
        raise RuntimeError('geopm_stats_collector_report() failed: {}'.format(error.message(err)))
    return gffi.gffi.string(report_cstr).decode()
