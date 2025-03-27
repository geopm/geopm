#!/usr/bin/env python3
#  Copyright (c) 2015 - 2025 Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#

import sys
import os
import re
from time import sleep
from argparse import ArgumentParser
from . import pio, topo, stats, loop, session, system_files, __version_str__

_STARTUP_SLEEP = 0.005

class PrometheusExporter:
    """Prometheus exporter of GEOPM telemetry using the geopm.stats.Collector to
    summarize statistics

    """
    def __init__(self, stats_collector):
        self._stats_collector = stats_collector
        self._stats_collector.report_table()
        sleep(_STARTUP_SLEEP) # Take two samples to enable derivative signals
        self._metric_names, self._metric_data = self._stats_collector.report_table()

        self._gauges = [None if type(metric_val) is str else _create_prom_metric(_sanitize_metric_name(metric_name), metric_name, 'Gauge')
                        for metric_name, metric_val in zip(self._metric_names, self._metric_data)]
        self._num_metric = 0
        for metric_idx, gauge in enumerate(self._gauges):
            if gauge is not None:
                 gauge.set_function(lambda exporter=self, metric_idx=metric_idx: \
                                    exporter._get_metric(metric_idx))
                 self._num_metric += 1
        self._num_fresh = self._num_metric

    def run(self, period, port, certfile, keyfile):
        """Run the GEOPM Prometheus exporter with geopm.stats.Collector

        """
        _start_http_server(port, certfile, keyfile)
        for _ in loop.TimedLoop(period):
            pio.read_batch()
            self._stats_collector.update()

    def _refresh(self):
        """Refresh all values in report cache

        """
        _, self._metric_data = self._stats_collector.report_table()
        self._stats_collector.reset()
        self._num_fresh = self._num_metric

    def _get_metric(self, metric_idx):
        """Get metric from cached report and update cache if all metrics have
        been sampled at least once.

        """
        if self._num_fresh == 0:
            self._refresh()
        self._num_fresh -= 1
        return self._metric_data[metric_idx]

class PrometheusMetricExporter:
    """Prometheus exporter of GEOPM telemetry using the native Prometheus metric
    implementations to summarize statistics.

    """
    def __init__(self, requests):
        self._pio_idx = []
        self._metrics = []
        for rr in requests:
            self._pio_idx.append(pio.push_signal(*rr))
            _, _, behavior = pio.signal_info(rr[0])
            if rr[1] == 0: # Board domain signal
                metric_name = rr[0]
                metric_desc = rr[0]
            else: # Append domain and domain index
                metric_name = f'{rr[0]}_{topo.domain_name(rr[1])}_{rr[2]}'
                metric_desc = f'{rr[0]}-{topo.domain_name(rr[1])}-{rr[2]}'
            metric_name = _sanitize_metric_name(metric_name)

            if behavior == 1: # Use Counter for monotone behavior
                self._metrics.append(_create_prom_metric(metric_name, metric_desc, 'Counter'))
            elif behavior == 2: # Use Summary for variable behavior
                self._metrics.append(_create_prom_metric(metric_name, metric_desc, 'Summary'))
            else: # error condition for constant or label signals
                raise RuntimeError(f'Invalid behavior for signal {metric_name}, '
                                   'only support for monotone or variable signals')

    def run(self, period, port, certfile, keyfile):
        """Run the GEOPM Prometheus exporter with prometheus Summary and
        Counter metrics

        """
        _start_http_server(port, certfile, keyfile)
        sample_last = [None] * len(self._metrics)
        sleep(_STARTUP_SLEEP) # Take two samples to enable derivative signals
        for sample_idx in loop.TimedLoop(period):
            pio.read_batch()
            for metric_idx, metric in enumerate(self._metrics):
                sample = pio.sample(self._pio_idx[metric_idx])
                if hasattr(metric, 'observe'):
                    metric.observe(sample)
                elif hasattr(metric, 'inc'):
                    if sample_last[metric_idx] != None:
                        metric.inc(sample - sample_last[metric_idx])
                    sample_last[metric_idx] = sample


def _sanitize_metric_name(name):
    """Convert the GEOPM metric name into one that adheres
    to Prometheus documentation

    """

    canonical_name = re.sub('[^a-zA-Z0-9_]', '_', name).lower()

    units = ''
    if 'temperature' in canonical_name:
        units = '_celcius'
    elif 'power' in canonical_name:
        units = '_watts'
    elif 'energy' in canonical_name:
        units = '_joules'
    elif 'freq' in canonical_name:
        units = '_hertz'

    name = f'geopm_{canonical_name}{units}'
    return name

_install_prometheus_msg = 'Please install python3-prometheus-client: https://pypi.org/project/prometheus-client/'
def _start_http_server(port, certfile, keyfile):
    """Wrapper to enable easier mocking in unit tests

    """
    try:
        from prometheus_client import start_http_server
    except Exception as ex:
        raise RuntimeError(_install_prometheus_msg) from ex
    if certfile is not None and keyfile is not None:
        try:
            start_http_server(port, certfile=certfile, keyfile=keyfile)
        except TypeError:
            raise RuntimeError(f'{_install_prometheus_msg}: version 0.19.0 required for https support')
    else:
        start_http_server(port)

def _create_prom_metric(name, descr, prom_name):
    """Wrapper to enable easier mocking in unit tests

    """
    try:
        from prometheus_client import Gauge, Counter, Summary
    except Exception as ex:
        raise RuntimeError(_install_prometheus_msg) from ex
    if prom_name == 'Gauge':
        return Gauge(name, descr)
    elif prom_name == 'Summary':
        return Summary(name, descr)
    elif prom_name == 'Counter':
        return Counter(name, descr)
    raise ValueError(f'Unknown prom_name: {prom_name}')


def default_requests():
    """Default configuration exposes all high level non-constant signals related to
    power, energy, frequency, and temperature.

    """
    include_strings = ["POWER", "ENERGY", "FREQ", "TEMPERATURE"]
    exclude_strings = ["::", "CONTROL", "MAX", "MIN", "STEP", "LIMIT", "STICKER"]
    all_signals = pio.signal_names()
    requests = []
    for sig in all_signals:
        if (not any(nn in sig for nn in exclude_strings)
            and any(nn in sig for nn in include_strings)):
            requests.append((sig, 0, 0))
    if len(requests) == 0:
        raise RuntimeError('Failed to find any signals to report')
    return requests

def run(period, port, config_path=None, summary='geopm', certfile=None, keyfile=None, use_insecure_http=False):
    """Run the GEOPM Prometheus exporter

    """
    if use_insecure_http:
        if certfile is not None or keyfile is not None:
            raise ValueError('Do not specify certfile or keyfile when running with insecure http')
    else:
        if certfile is None or keyfile is None:
            raise ValueError('The certfile and keyfile arguments are required unless insecure http is selected')
        if not system_files.is_secure_path(certfile):
            raise ValueError(f'File "{certfile}" is not secure')
        if not system_files.is_secure_path(keyfile):
            raise ValueError(f'File "{keyfile}" is not secure')
    if config_path is None:
        requests = default_requests()
    elif config_path == '-':
        requests = list(session.ReadRequestQueue(sys.stdin))
    else:
        with open(config_path) as fid:
            requests = list(session.ReadRequestQueue(fid))
    if summary == 'geopm':
        with stats.Collector(requests) as stats_collector:
            exporter = PrometheusExporter(stats_collector)
            exporter.run(period, port, certfile, keyfile)
    elif summary == 'prometheus':
        exporter = PrometheusMetricExporter(requests)
        exporter.run(period, port, certfile, keyfile)
    else:
        raise ValueError(f'Unknown summary type: "{summary}".  Must be "geopm" or "prometheus"')

def main():
    """Prometheus exporter for GEOPM metrics

    """
    err = 0
    try:
        parser = ArgumentParser(description=main.__doc__)
        parser.add_argument('-v', '--version', dest='version', action='store_true',
                            help='Print version and exit')
        parser.add_argument('-t', '--period', dest='period', type=float, default=0.1,
                            help='Sample period for fast loop in seconds. Default: %(default)s')
        parser.add_argument('-p', '--port', dest='port', type=int, default=8000,
                            help='Port to publish Prometheus metrics. Default: %(default)s')
        parser.add_argument('-i', '--signal-config', dest='config_path', default=None,
                            help='Input file containing GEOPM signal requests, specify "-" to use '
                                 'standard input. Default: All power, energy, frequency and '
                                 'temperature signals at the board domain')
        parser.add_argument('--summary', dest='summary', default='geopm',
                            help='Summary method, one of "geopm", or "prometheus". Default: %(default)s')
        parser.add_argument('-c', '--certfile',
                            help='Server certificate used during the TLS handshake')
        parser.add_argument('-k', '--keyfile',
                            help='Server certificate private key')
        parser.add_argument('--insecure-http', action='store_true',
                            help='Use http not https to export metrics over TCP/IP')

        args = parser.parse_args()
        if args.version:
            print(__version_str__)
        else:
            run(args.period, args.port, args.config_path, args.summary, args.certfile, args.keyfile, args.insecure_http)
    except Exception as ee:
        if 'GEOPM_DEBUG' in os.environ:
            # Do not handle exception if GEOPM_DEBUG is set
            raise ee
        sys.stderr.write('Error: {}\n\n'.format(ee))
        err = -1
    return err

if __name__ == '__main__':
    sys.exit(main())
