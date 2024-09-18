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
from . import error

gffi.gffi.cdef("""

struct geopm_stats_collector_s;

struct geopm_metric_stats_s {
    char name[255];
    double stats[7];
};

struct geopm_report_s {
    char host[255];
    char sample_time_first[255];
    double sample_stats[4];
    size_t num_metric;
    struct geopm_metric_stats_s *metric_stats;
};

int geopm_stats_collector_create(size_t num_requests, const struct geopm_request_s *requests,
                                 struct geopm_stats_collector_s **collector);

int geopm_stats_collector_update(struct geopm_stats_collector_s *collector);

int geopm_stats_collector_report_yaml(const struct geopm_stats_collector_s *collector,
                                      size_t *max_report_size, char *report_yaml);

int geopm_stats_collector_report(const struct geopm_stats_collector_s *collector,
                                 size_t num_requests, struct geopm_report_s *report);

int geopm_stats_collector_reset(struct geopm_stats_collector_s *collector);

int geopm_stats_collector_free(struct geopm_stats_collector_s *collector);



""")
_dl = gffi.get_dl_geopmd()


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
        self._num_signal = len(signal_config)
        if self._num_signal == 0:
            raise ValueError('Collector creation failed: length of input is zero')

        signal_config_carr = gffi.gffi.new('struct geopm_request_s[]', self._num_signal)
        for idx, req in enumerate(signal_config):
            signal_config_carr[idx].domain = req[1]
            signal_config_carr[idx].domain_idx = req[2]
            signal_config_carr[idx].name = req[0].encode()

        collector_ptr = gffi.gffi.new('struct geopm_stats_collector_s **');

        err = _dl.geopm_stats_collector_create(self._num_signal, signal_config_carr, collector_ptr)
        if err < 0:
            raise RuntimeError('geopm_stats_collector_create() failed: {}'.format(error.message(err)))
        self._collector_ptr = collector_ptr[0];
        self._is_reset = True

    def __enter__(self):
        return self

    def __exit__(self, type, value, traceback):
        self.close()

    def close(self):
        """Free all resources by deleting the StatsCollector

        The object cannot be used after this call.  Must be called on
        all Collector objects created without context management when
        they are no longer in use.

        """
        global _dl
        if (self._collector_ptr is not None):
            collector_ptr = self._collector_ptr
            self._collector_ptr = None
            _dl.geopm_stats_collector_free(collector_ptr)

    def _check_ptr(self, func_name):
        if self._collector_ptr is None:
            raise RuntimeError(f'Called Collector.{func_name}() after calling Collector.close()')

    def update(self):
        """Update the collector with new values

        User is expected to call geopmdpy.pio.read_batch() prior to calling this
        interface.  The sampled values will be used to update the report statistics.

        """
        global _dl
        self._check_ptr('update')
        err = _dl.geopm_stats_collector_update(self._collector_ptr)
        if err < 0:
            raise RuntimeError('geopm_stats_collector_update() failed: {}'.format(error.message(err)))
        self._is_reset = False

    def report_yaml(self):
        """Create a yaml report

        Return a yaml string containing a report that shows all statistics
        gathered by calls to update().

        """
        global _dl
        self._check_ptr('report_yaml')
        report_max = gffi.gffi.new('size_t *')
        report_max[0] = 0
        _dl.geopm_stats_collector_report_yaml(self._collector_ptr, report_max, gffi.gffi.NULL)
        report_cstr = gffi.gffi.new('char[]', report_max[0])
        err = _dl.geopm_stats_collector_report_yaml(self._collector_ptr, report_max, report_cstr)
        if err < 0:
            raise RuntimeError('geopm_stats_collector_report_yaml() failed: {}'.format(error.message(err)))
        return gffi.gffi.string(report_cstr).decode()

    def report(self):
        global _dl
        self._check_ptr('report')
        report_ptr = gffi.gffi.new('struct geopm_report_s *')
        metric_stats = gffi.gffi.new('struct geopm_metric_stats_s[]', self._num_signal)
        report_ptr.metric_stats = metric_stats
        err = _dl.geopm_stats_collector_report(self._collector_ptr, self._num_signal, report_ptr)
        if err < 0:
            raise RuntimeError('geopm_stats_collector_report() failed: {}'.format(error.message(err)))
        result = dict()
        result['host'] = gffi.gffi.string(report_ptr.host).decode()
        result['sample-time-first'] = gffi.gffi.string(report_ptr.sample_time_first).decode()
        result['sample-time-total'] = report_ptr.sample_stats[0]
        result['sample-count'] = int(report_ptr.sample_stats[1])
        result['sample-period-mean'] = report_ptr.sample_stats[2]
        result['sample-period-std'] = report_ptr.sample_stats[3]
        result['metrics'] = dict()
        for metric_idx in range(report_ptr.num_metric):
            name = gffi.gffi.string(report_ptr.metric_stats[metric_idx].name).decode()
            result['metrics'][name] = {"count": int(report_ptr.metric_stats[metric_idx].stats[0]),
                                       "first": report_ptr.metric_stats[metric_idx].stats[1],
                                       "last": report_ptr.metric_stats[metric_idx].stats[2],
                                       "min": report_ptr.metric_stats[metric_idx].stats[3],
                                       "max": report_ptr.metric_stats[metric_idx].stats[4],
                                       "mean-arithmetic": report_ptr.metric_stats[metric_idx].stats[5],
                                       "std": report_ptr.metric_stats[metric_idx].stats[6]}
        return result

    def report_csv(self, delimiter, print_header):
        report = self.report()
        result = []
        header = ['host', 'sample-time-first', 'sample-time-total', 'sample-count', 'sample-period-mean', 'sample-period-std']
        data = [f'"{report[kk]}"' if type(report[kk]) is str else str(report[kk]) for kk in header]

        metric_stat_names = ['count', 'first', 'last', 'min', 'max', 'mean-arithmetic', 'std']
        for metric_name in sorted(report['metrics'].keys()):
            for stat_name in metric_stat_names:
                data.append(str(report['metrics'][metric_name][stat_name]))
                if print_header:
                    header.append(f'{metric_name}-{stat_name}')
        if print_header:
            result.append(delimiter.join(header))
        result.append(delimiter.join(data))
        result.append('')
        return '\n'.join(result)

    def reset(self):
        """Reset statistics

        Called by user to zero all statistics gathered.  This may be called
        after a call to report_yaml() and before the next call to update() so
        that the next report that is generated is independent of the last.

        """
        global _dl
        self._check_ptr('reset')
        err = _dl.geopm_stats_collector_reset(self._collector_ptr)
        self._is_reset = True
        if err < 0:
            raise RuntimeError('geopm_stats_collector_reset() failed: {}'.format(error.message(err)))

    def is_reset(self):
        return self._is_reset
