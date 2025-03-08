#  Copyright (c) 2015 - 2025 Intel Corporation
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

SAMPLE_TIME_TOTAL = gffi.dl_geopmd.GEOPM_SAMPLE_TIME_TOTAL
SAMPLE_COUNT = gffi.dl_geopmd.GEOPM_SAMPLE_COUNT
SAMPLE_PERIOD_MEAN = gffi.dl_geopmd.GEOPM_SAMPLE_PERIOD_MEAN
SAMPLE_PERIOD_STD = gffi.dl_geopmd.GEOPM_SAMPLE_PERIOD_STD
NUM_SAMPLE_STATS = gffi.dl_geopmd.GEOPM_NUM_SAMPLE_STATS
METRIC_COUNT = gffi.dl_geopmd.GEOPM_METRIC_COUNT
METRIC_FIRST = gffi.dl_geopmd.GEOPM_METRIC_FIRST
METRIC_LAST = gffi.dl_geopmd.GEOPM_METRIC_LAST
METRIC_MIN = gffi.dl_geopmd.GEOPM_METRIC_MIN
METRIC_MAX = gffi.dl_geopmd.GEOPM_METRIC_MAX
METRIC_MEAN = gffi.dl_geopmd.GEOPM_METRIC_MEAN
METRIC_STD = gffi.dl_geopmd.GEOPM_METRIC_STD
NUM_METRIC_STATS = gffi.dl_geopmd.GEOPM_NUM_METRIC_STATS

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
        self._collector_ptr = None
        self._num_signal = len(signal_config)
        if self._num_signal == 0:
            raise ValueError('Collector creation failed: length of input is zero')

        signal_config_carr = gffi.gffi.new('struct geopm_request_s[]', self._num_signal)
        for idx, req in enumerate(signal_config):
            signal_config_carr[idx].domain_type = req[1]
            signal_config_carr[idx].domain_idx = req[2]
            signal_config_carr[idx].name = req[0].encode()

        collector_ptr = gffi.gffi.new('struct geopm_stats_collector_s **');

        err = gffi.dl_geopmd.geopm_stats_collector_create(self._num_signal, signal_config_carr, collector_ptr)
        if err < 0:
            raise RuntimeError('geopm_stats_collector_create() failed: {}'.format(error.message(err)))
        self._collector_ptr = collector_ptr[0];

    def __enter__(self):
        """Enter context management for allocated geopm_stats_collector_s

        """
        return self

    def __exit__(self, type, value, traceback):
        """Exit context management for allocated geopm_stats_collector_s

        """
        self.close()

    def close(self):
        """Free all resources by deleting the StatsCollector

        The object cannot be used after this call.  Must be called on
        all Collector objects created without context management when
        they are no longer in use.

        """
        if (self._collector_ptr is not None):
            collector_ptr = self._collector_ptr
            self._collector_ptr = None
            gffi.dl_geopmd.geopm_stats_collector_free(collector_ptr)

    def _check_ptr(self, func_name):
        if self._collector_ptr is None:
            raise RuntimeError(f'Called Collector.{func_name}() after calling Collector.close()')

    def update(self):
        """Update the collector with new values

        User is expected to call geopmdpy.pio.read_batch() prior to calling this
        interface.  The sampled values will be used to update the report statistics.

        """
        self._check_ptr('update')
        err = gffi.dl_geopmd.geopm_stats_collector_update(self._collector_ptr)
        if err < 0:
            raise RuntimeError('geopm_stats_collector_update() failed: {}'.format(error.message(err)))

    def update_count(self):
        """Number of updates since last reset

        Get number of calls to the update() method since object construction, or
        last call to the reset() method.

        """
        self._check_ptr('update_count')
        result = gffi.gffi.new('size_t *')
        err = gffi.dl_geopmd.geopm_stats_collector_update_count(self._collector_ptr, result)
        if err < 0:
            raise RuntimeError('geopm_stats_collector_update_count() failed: {}'.format(error.message(err)))
        return result[0]

    def report_yaml(self):
        """Create a yaml report

        Return a yaml string containing a report that shows all statistics
        gathered by calls to update() since last reset().  This gives a more
        efficient way to create a yaml report than converting a report object
        with the pyyaml module.  The structure of the ``report_a`` and
        ``report_b`` yaml strings is the same in the following example.

        >>> import yaml
        >>> report_a = stats.report_yaml()
        >>> report_b = yaml.dump(stats.report())

        """
        self._check_ptr('report_yaml')
        report_max = gffi.gffi.new('size_t *')
        report_max[0] = 0
        gffi.dl_geopmd.geopm_stats_collector_report_yaml(self._collector_ptr, report_max, gffi.gffi.NULL)
        report_cstr = gffi.gffi.new('char[]', report_max[0])
        err = gffi.dl_geopmd.geopm_stats_collector_report_yaml(self._collector_ptr, report_max, report_cstr)
        if err < 0:
            raise RuntimeError('geopm_stats_collector_report_yaml() failed: {}'.format(error.message(err)))
        return gffi.gffi.string(report_cstr).decode()

    def report(self):
        """Create a report object

        Return a dictionary containing a report that shows all statistics
        gathered by calls to update() since last reset().  This gives more
        efficient access to the full resolution data than the report_yaml()
        method.  The structure of the ``report_a`` and ``report_b`` objects is
        the same in the following example:

        >>> import yaml
        >>> report_a = yaml.safe_load(stats.report_yaml())
        >>> report_b = stats.report()

        """
        self._check_ptr('report')
        report_ptr = gffi.gffi.new('struct geopm_report_s *')
        metric_stats = gffi.gffi.new('struct geopm_metric_stats_s[]', self._num_signal)
        report_ptr.metric_stats = metric_stats
        err = gffi.dl_geopmd.geopm_stats_collector_report(self._collector_ptr, self._num_signal, report_ptr)
        if err < 0:
            raise RuntimeError('geopm_stats_collector_report() failed: {}'.format(error.message(err)))
        result = dict()
        result['host'] = gffi.gffi.string(report_ptr.host).decode()
        result['sample-time-first'] = gffi.gffi.string(report_ptr.sample_time_first).decode()
        result['sample-time-total'] = report_ptr.sample_stats[SAMPLE_TIME_TOTAL]
        result['sample-count'] = int(report_ptr.sample_stats[SAMPLE_COUNT])
        result['sample-period-mean'] = report_ptr.sample_stats[SAMPLE_PERIOD_MEAN]
        result['sample-period-std'] = report_ptr.sample_stats[SAMPLE_PERIOD_STD]
        result['metrics'] = dict()
        for metric_idx in range(report_ptr.num_metric):
            name = gffi.gffi.string(report_ptr.metric_stats[metric_idx].name).decode()
            result['metrics'][name] = {"count": int(report_ptr.metric_stats[metric_idx].stats[METRIC_COUNT]),
                                       "first": report_ptr.metric_stats[metric_idx].stats[METRIC_FIRST],
                                       "last": report_ptr.metric_stats[metric_idx].stats[METRIC_LAST],
                                       "min": report_ptr.metric_stats[metric_idx].stats[METRIC_MIN],
                                       "max": report_ptr.metric_stats[metric_idx].stats[METRIC_MAX],
                                       "mean": report_ptr.metric_stats[metric_idx].stats[METRIC_MEAN],
                                       "std": report_ptr.metric_stats[metric_idx].stats[METRIC_STD]}
        return result

    def report_csv(self, delimiter=',', print_header=True):
        """Create a report CSV string

        Flatten the report structure into tabular data in CSV format.  The
        output is a single line of CSV with or without a header line depending
        on the value of ``print_header`` parameter.

        The hierarchical report is flattened by combining the metric name and
        the metric statistic name into a single column header, for example a
        metric may appear in a report with name 'CPU_FREQUENCY_STATUS'.  In the
        CSV representation this metric will have a column for each statistic:

        ``...,"CPU_FREQUENCY_STATUS-count","CPU_FREQUENCY_STATUS-first","CPU_FREQUENCY_STATUS-last",...``

        Args:
            delimiter (str): Delimiter used to separate values in CSV output.

            print_header (bool): If true output will include a first line with
                                 name of each column as a string.

        """
        header, data = self.report_table()
        header = [f'"{name}"' for name in header]
        data = [f'"{dd}"' if type(dd) is str else str(dd) for dd in data]
        result = []
        if print_header:
            result.append(delimiter.join(header))
        result.append(delimiter.join(data))
        result.append('')
        return '\n'.join(result)

    def report_table(self):
        """Create report in tabular data format

        Flatten the report structure into tabular data.  Returns a tuple of
        (header, data) where the header is a list of names for each of the
        values stored in the list data.

        The hierarchical report is flattened by combining the metric name and
        the metric statistic name into a single column header, for example a
        metric may appear in a report with name 'CPU_FREQUENCY_STATUS'.  In the
        CSV representation this metric will have a column for each statistic:

        ``...,"CPU_FREQUENCY_STATUS-count","CPU_FREQUENCY_STATUS-first","CPU_FREQUENCY_STATUS-last",...``

        Returns:
            (list, list): List of data names, and data values

        """
        report = self.report()
        header = ['host', 'sample-time-first', 'sample-time-total', 'sample-count', 'sample-period-mean', 'sample-period-std']
        data = [report[kk] for kk in header]

        metric_stat_names = ['count', 'first', 'last', 'min', 'max', 'mean', 'std']
        for metric_name in sorted(report['metrics'].keys()):
            for stat_name in metric_stat_names:
                header.append(f'{metric_name}-{stat_name}')
                data.append(report['metrics'][metric_name][stat_name])
        return header, data

    def reset(self):
        """Reset statistics

        Called by user to zero all statistics gathered.  This may be called
        after a call to report_yaml() and before the next call to update() so
        that the next report that is generated is independent of the last.

        """
        self._check_ptr('reset')
        err = gffi.dl_geopmd.geopm_stats_collector_reset(self._collector_ptr)
        if err < 0:
            raise RuntimeError('geopm_stats_collector_reset() failed: {}'.format(error.message(err)))
