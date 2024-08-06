#  Copyright (c) 2015 - 2024 Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#

"""Wrapper for the geopm_stats_collector_*() C APIs

"""

import sys
import os
import errno
import math
from . import gffi

gffi.gffi.cdef("""

struct geopm_stats_collector_s;

int geopm_stats_collector_create(size_t num_requests, const struct geopm_request_s *requests,
                                 struct geopm_stats_collector_s **collector);

int geopm_stats_collector_update(struct geopm_stats_collector_s *collector);

int geopm_stats_collector_report_yaml(const struct geopm_stats_collector_s *collector,
                                      size_t *max_report_size, char *report_yaml);

int geopm_stats_collector_reset(struct geopm_stats_collector_s *collector);

int geopm_stats_collector_free(struct geopm_stats_collector_s *collector);

""")
_dl = gffi.get_dl_geopmd()

import json
import yaml

class Collector:
    """ Object for aggregating statistics gathered from the PlatformIO interface of GEOPM

    """
    def __init__(self, signal_config):
        """Create stats collector

        Provide a list of read requests for PlatformIO.

        Args:
            signal_config (list((str, int, int))): List of requested
                signals where each tuple represents
                (signal_name, domain_type, domain_idx).

        """
        global _dl
        self._collector_ptr = None
        signal_config = [(rr[0], rr[1], rr[2]) for rr in signal_config]
        num_signal = len(signal_config)
        if num_signal == 0:
            raise RuntimeError('Collector creation failed: length of input is zero')

        signal_config_carr = gffi.gffi.new(f'struct geopm_request_s[{num_signal}]')
        for idx, req in enumerate(signal_config):
            signal_config_carr[idx].domain = req[1]
            signal_config_carr[idx].domain_idx = req[2]
            signal_config_carr[idx].name = req[0].encode()

        collector_ptr = gffi.gffi.new('struct geopm_stats_collector_s **');

        err = _dl.geopm_stats_collector_create(num_signal, signal_config_carr, collector_ptr)
        if err < 0:
            raise RuntimeError('geopm_stats_collector() failed: {}'.format(error.message(err)))
        self._collector_ptr = collector_ptr[0];

    def __del__(self):
        """Free stats collector resources

        """
        if (self._collector_ptr is not None):
            _dl.geopm_stats_collector_free(self._collector_ptr)

    def update(self):
        """Update the collector with new values

        User is expected to call geopmdpy.pio.read_batch() prior to calling this
        interface.  The sampled values will be used to update the report statistics.

        """
        global _dl
        err = _dl.geopm_stats_collector_update(self._collector_ptr)
        if err < 0:
            raise RuntimeError('geopm_stats_collector_update() failed: {}'.format(error.message(err)))

    def report_yaml(self):
        """Create a yaml report

        Return a yaml string containing a report that shows all statistics
        gathered by calls to update().

        """
        global _dl
        report_max = gffi.gffi.new("size_t *", 0)
        _dl.geopm_stats_collector_report_yaml(self._collector_ptr, report_max, gffi.gffi.NULL)
        report_cstr = gffi.gffi.new("char[]", report_max[0])
        err = _dl.geopm_stats_collector_report_yaml(self._collector_ptr, report_max, report_cstr)
        if err < 0:
            raise RuntimeError('geopm_stats_collector_report_yaml() failed: {}'.format(error.message(err)))
        return gffi.gffi.string(report_cstr).decode()

    def report_json(self):
        """Create a json report

        Return a json string containing a report that shows all statistics
        gathered by calls to update().

        """
        result_yaml = self.report_yaml()
        result_obj = yaml.safe_load(result_yaml)
        return json.dumps(result_obj)

    def reset(self):
        """Reset statistics

        Called by user to zero all statistics galthered.  This may be called
        after a call to report_yaml() and before the next call to update() so
        that the next report that is generated is independent of the last.

        """
        global _dl
        err = _dl.geopm_stats_collector_reset(self._collector_ptr)
        if err < 0:
            raise RuntimeError('geopm_stats_collector_reset() failed: {}'.format(error.message(err)))
