#!/usr/bin/env python3
#
#  Copyright (c) 2015 - 2025 Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#

from unittest import TestCase, mock, main, skipUnless

_prometheus_enabled = False
Gauge = None
Counter = None
Collector = None
PrometheusExporter = None
PrometheusMetricExporter = None
exporter = None
try:
    from prometheus_client import Gauge, Counter
    from geopmdpy.stats import Collector
    from geopmdpy.exporter import PrometheusExporter, PrometheusMetricExporter
    from geopmdpy import exporter
    _prometheus_enabled = True
except Exception as ex:
    pass

import sys
import tempfile

_MOCK_SIGNAL_NAMES = [
    "CPUFREQ::CPUINFO_MAX_FREQ", "CPUFREQ::CPUINFO_MIN_FREQ", "CPUFREQ::SCALING_CUR_FREQ",
    "CPUFREQ::SCALING_MAX_FREQ", "CPUFREQ::SCALING_MIN_FREQ", "CPUINFO::FREQ_MAX", "CPUINFO::FREQ_MIN",
    "CPUINFO::FREQ_STEP", "CPUINFO::FREQ_STICKER", "CPU_CORE_TEMPERATURE", "CPU_CYCLES_REFERENCE",
    "CPU_CYCLES_THREAD", "CPU_ENERGY", "CPU_FREQUENCY_DESIRED_CONTROL", "CPU_FREQUENCY_MAX_AVAIL",
    "CPU_FREQUENCY_MAX_CONTROL", "CPU_FREQUENCY_MIN_AVAIL", "CPU_FREQUENCY_MIN_CONTROL",
    "CPU_FREQUENCY_STATUS", "CPU_FREQUENCY_STEP", "CPU_FREQUENCY_STICKER", "CPU_INSTRUCTIONS_RETIRED",
    "CPU_PACKAGE_TEMPERATURE", "CPU_POWER", "CPU_POWER_LIMIT_CONTROL", "CPU_POWER_LIMIT_DEFAULT",
    "CPU_POWER_MAX_AVAIL", "CPU_POWER_MIN_AVAIL", "CPU_POWER_TIME_WINDOW_CONTROL", "CPU_TIMESTAMP_COUNTER",
    "CPU_UNCORE_FREQUENCY_MAX_CONTROL", "CPU_UNCORE_FREQUENCY_MIN_CONTROL", "CPU_UNCORE_FREQUENCY_STATUS",
    "DRAM_ENERGY", "DRAM_POWER", "GPU_CORE_FREQUENCY_MAX_AVAIL", "GPU_CORE_FREQUENCY_MAX_CONTROL",
    "GPU_CORE_FREQUENCY_MIN_AVAIL", "GPU_CORE_FREQUENCY_MIN_CONTROL", "GPU_CORE_FREQUENCY_STATUS",
    "GPU_CORE_FREQUENCY_STEP", "GPU_ENERGY", "GPU_POWER", "GPU_POWER_LIMIT_CONTROL", "GPU_TEMPERATURE",
    "GPU_UTILIZATION", "MSR::DRAM_ENERGY_STATUS#", "MSR::DRAM_ENERGY_STATUS:ENERGY",
    "MSR::DRAM_PERF_STATUS#", "MSR::DRAM_PERF_STATUS:THROTTLE_TIME", "NVML::GPU_CORE_FREQUENCY_MAX_AVAIL",
    "NVML::GPU_CORE_FREQUENCY_MAX_CONTROL", "NVML::GPU_CORE_FREQUENCY_MIN_AVAIL",
    "NVML::GPU_CORE_FREQUENCY_MIN_CONTROL", "NVML::GPU_CORE_FREQUENCY_RESET_CONTROL",
    "NVML::GPU_CORE_FREQUENCY_STATUS", "NVML::GPU_CORE_FREQUENCY_STEP", "NVML::GPU_CORE_THROTTLE_REASONS",
    "NVML::GPU_CPU_ACTIVE_AFFINITIZATION", "NVML::GPU_ENERGY_CONSUMPTION_TOTAL",
    "NVML::GPU_PCIE_RX_THROUGHPUT", "NVML::GPU_PCIE_TX_THROUGHPUT", "NVML::GPU_PERFORMANCE_STATE",
    "NVML::GPU_POWER", "NVML::GPU_POWER_LIMIT_CONTROL", "NVML::GPU_TEMPERATURE",
    "NVML::GPU_UNCORE_FREQUENCY_STATUS", "NVML::GPU_UNCORE_UTILIZATION", "NVML::GPU_UTILIZATION",
    "SERVICE::CPUFREQ::CPUINFO_MAX_FREQ", "SERVICE::CPUFREQ::CPUINFO_MIN_FREQ",
    "SERVICE::CPUFREQ::CPUINFO_TRANSITION_LATENCY", "SERVICE::CPUFREQ::SCALING_CUR_FREQ",
    "SERVICE::CPUFREQ::SCALING_MAX_FREQ", "SERVICE::CPUFREQ::SCALING_MIN_FREQ",
    "SERVICE::CPUFREQ::SCALING_SETSPEED", "SERVICE::CPUINFO::FREQ_MAX", "SERVICE::CPUINFO::FREQ_MIN",
    "SERVICE::CPUINFO::FREQ_STEP", "SERVICE::CPUINFO::FREQ_STICKER", "SERVICE::CPU_CORE_TEMPERATURE",
    "SERVICE::CPU_CYCLES_REFERENCE", "SERVICE::CPU_CYCLES_THREAD", "SERVICE::CPU_ENERGY", "TIME",
    "TIME::ELAPSED"]

@skipUnless(_prometheus_enabled, "prometheus_client not installed")
class TestPrometheusExporter(TestCase):
    def setUp(self):
        self._mock_collector = mock.create_autospec(Collector)
        self._mock_collector.report_table.return_value = (['TIME-count', 'TIME-mean'], [1, 2])

    def test_run(self):
        period = 0.1
        port = 8000
        num_calls = 10
        with mock.patch('geopmdpy.exporter._create_prom_metric', return_value=mock.create_autospec(Gauge)), \
             mock.patch('geopmdpy.loop.TimedLoop', return_value=range(num_calls)) as mtl, \
             mock.patch('geopmdpy.exporter._start_http_server') as mshs, \
             mock.patch('geopmdpy.pio.read_batch') as mrb:
            prom_exp = PrometheusExporter(self._mock_collector)
            prom_exp.run(period, port, None, None)
            mtl.assert_called_with(period)
            mshs.assert_called_with(port, None, None)
        calls = [mock.call()] * num_calls
        self._mock_collector.update.assert_has_calls(calls)
        mrb.assert_has_calls(calls)

    def test_run_https(self):
        period = 0.1
        port = 8000
        num_calls = 10
        with tempfile.NamedTemporaryFile() as certfile, tempfile.NamedTemporaryFile() as keyfile, \
             mock.patch('geopmdpy.exporter._create_prom_metric', return_value=mock.create_autospec(Gauge)), \
             mock.patch('geopmdpy.loop.TimedLoop', return_value=range(num_calls)) as mtl, \
             mock.patch('geopmdpy.exporter._start_http_server') as mshs, \
             mock.patch('geopmdpy.pio.read_batch') as mrb:
            prom_exp = PrometheusExporter(self._mock_collector)
            prom_exp.run(period, port, certfile.name, keyfile.name)
            mtl.assert_called_with(period)
            mshs.assert_called_with(port, certfile.name, keyfile.name)
        calls = [mock.call()] * num_calls
        self._mock_collector.update.assert_has_calls(calls)
        mrb.assert_has_calls(calls)

    def test_refresh(self):
        with mock.patch('geopmdpy.exporter._create_prom_metric', return_value=mock.create_autospec(Gauge)):
            prom_exp = PrometheusExporter(self._mock_collector)
        prom_exp._get_metric(0)
        self._mock_collector.report_table.assert_has_calls([mock.call()])
        self._mock_collector.reset.assert_has_calls([])
        prom_exp._get_metric(1)
        self._mock_collector.report_table.assert_has_calls([mock.call()])
        self._mock_collector.reset.assert_has_calls([])
        prom_exp._get_metric(0)
        self._mock_collector.report_table.assert_has_calls(2 * [mock.call()])
        self._mock_collector.reset.assert_has_calls([mock.call()])
        prom_exp._get_metric(1)
        self._mock_collector.report_table.assert_has_calls(2 * [mock.call()])
        self._mock_collector.reset.assert_has_calls([mock.call()])

    def test_default_requests(self):
        expected = [('CPU_CORE_TEMPERATURE', 0, 0),
                    ('CPU_ENERGY', 0, 0),
                    ('CPU_FREQUENCY_STATUS', 0, 0),
                    ('CPU_PACKAGE_TEMPERATURE', 0, 0),
                    ('CPU_POWER', 0, 0),
                    ('CPU_UNCORE_FREQUENCY_STATUS', 0, 0),
                    ('DRAM_ENERGY', 0, 0),
                    ('DRAM_POWER', 0, 0),
                    ('GPU_CORE_FREQUENCY_STATUS', 0, 0),
                    ('GPU_ENERGY', 0, 0),
                    ('GPU_POWER', 0, 0),
                    ('GPU_TEMPERATURE', 0, 0)]
        with mock.patch('geopmdpy.pio.signal_names', return_value=_MOCK_SIGNAL_NAMES) as msn:
            selected = exporter.default_requests()
        self.assertEqual(expected, selected)
        with mock.patch('geopmdpy.pio.signal_names', return_value=[]) as msn:
            with self.assertRaisesRegex(RuntimeError,'Failed to find any signals to report'):
                selected = exporter.default_requests()

    def test_sanitize_metric_name(self):
        map_raw2sanitized = {
            'CPU_POWER-package-0' : 'geopm_cpu_power_package_0_watts',
            'CPU_POWER_LIMIT_CONTROL-package-0' : 'geopm_cpu_power_limit_control_package_0_watts',
            'CPU_UNCORE_FREQUENCY_MIN_CONTROL-package-0' : 'geopm_cpu_uncore_frequency_min_control_package_0_hertz',
            'CPU_UNCORE_FREQUENCY_STATUS-package-0' : 'geopm_cpu_uncore_frequency_status_package_0_hertz',
            'DRAM_ENERGY-package-0' : 'geopm_dram_energy_package_0_joules',
            'DRAM_POWER-package-1' : 'geopm_dram_power_package_1_watts',
            'GPU_CORE_FREQUENCY_STATUS-package-0' : 'geopm_gpu_core_frequency_status_package_0_hertz',
            'GPU_CORE_FREQUENCY_STEP-gpu-3' : 'geopm_gpu_core_frequency_step_gpu_3_hertz',
            'GPU_ENERGY-gpu-4' : 'geopm_gpu_energy_gpu_4_joules',
            'GPU_POWER-package-0' : 'geopm_gpu_power_package_0_watts',
            'GPU_TEMPERATURE-package-0' : 'geopm_gpu_temperature_package_0_celcius',
            'GPU_UTILIZATION-gpu-0' : 'geopm_gpu_utilization_gpu_0',
            'MSR::CPU_SCALABILITY_RATIO-package-0' : 'geopm_msr__cpu_scalability_ratio_package_0',
            'MSR::DRAM_ENERGY_STATUS:ENERGY-package-0' : 'geopm_msr__dram_energy_status_energy_package_0_joules',
            'MSR::PACKAGE_THERM_STATUS:CRITICAL_TEMP_STATUS-package-0' : 'geopm_msr__package_therm_status_critical_temp_status_package_0'}
        for metric, expected in map_raw2sanitized.items():
            self.assertEqual(expected, exporter._sanitize_metric_name(metric))


@skipUnless(_prometheus_enabled, "prometheus_client not installed")
class TestPrometheusMetricExporter(TestCase):
    def setUp(self):
        self._requests = [('TIME', 0, 0)]

    def test_run(self):
        prom_exp = PrometheusMetricExporter(self._requests)
        period = 0.1
        port = 8000
        num_calls = 10
        mock_counter = mock.create_autospec(Counter)
        with mock.patch('geopmdpy.exporter._create_prom_metric', return_value=mock_counter), \
             mock.patch('geopmdpy.loop.TimedLoop', return_value=range(num_calls)) as mtl, \
             mock.patch('geopmdpy.exporter._start_http_server') as mshs:
            prom_exp = PrometheusMetricExporter(self._requests)
            prom_exp.run(period, port, None, None)
            mtl.assert_called_with(period)
            mshs.assert_called_with(port, None, None)
            mock_counter.inc.assert_called()

    def test_run_prometheus_metric_exporter_https(self):
        period = 0.1
        port = 8000
        num_calls = 10
        mock_counter = mock.create_autospec(Counter)
        with tempfile.NamedTemporaryFile() as certfile, tempfile.NamedTemporaryFile() as keyfile, \
             mock.patch('geopmdpy.exporter._create_prom_metric', return_value=mock_counter), \
             mock.patch('geopmdpy.loop.TimedLoop', return_value=range(num_calls)) as mtl, \
             mock.patch('geopmdpy.exporter._start_http_server') as mshs:
            prom_exp = PrometheusMetricExporter(self._requests)
            prom_exp.run(period, port, certfile.name, keyfile.name)
            mtl.assert_called_with(period)
            mshs.assert_called_with(port, certfile.name, keyfile.name)
            mock_counter.inc.assert_called()

@skipUnless(_prometheus_enabled, "prometheus_client not installed")
class TestExporterCli(TestCase):
    def test_insecure_http(self):
        sys.argv = ['', '--insecure-http']
        mock_exporter = mock.create_autospec(PrometheusExporter)
        mock_collector = mock.create_autospec(Collector)
        port = 8000
        with mock.patch('geopmdpy.pio.signal_names', return_value=_MOCK_SIGNAL_NAMES), \
             mock.patch('geopmdpy.exporter.PrometheusExporter', return_value=mock_exporter), \
             mock.patch('geopmdpy.stats.Collector', return_value=mock_collector):
            exporter.main()
            mock_exporter.run.assert_called_with(0.1, port, None, None)

    def test_period(self):
        sys.argv = ['', '--period', '1', '--insecure-http']
        mock_exporter = mock.create_autospec(PrometheusExporter)
        mock_collector = mock.create_autospec(Collector)
        with mock.patch('geopmdpy.pio.signal_names', return_value=_MOCK_SIGNAL_NAMES), \
             mock.patch('geopmdpy.exporter.PrometheusExporter', return_value=mock_exporter), \
             mock.patch('geopmdpy.stats.Collector', return_value=mock_collector):
            exporter.main()
            mock_exporter.run.assert_called_with(1, 8000, None, None)

    def test_port(self):
        sys.argv = ['', '--port', '8005', '--insecure-http']
        mock_exporter = mock.create_autospec(PrometheusExporter)
        mock_collector = mock.create_autospec(Collector)
        with mock.patch('geopmdpy.pio.signal_names', return_value=_MOCK_SIGNAL_NAMES), \
             mock.patch('geopmdpy.exporter.PrometheusExporter', return_value=mock_exporter), \
             mock.patch('geopmdpy.stats.Collector', return_value=mock_collector):
            exporter.main()
            mock_exporter.run.assert_called_with(0.1, 8005, None, None)

    def test_signal_config(self):
        with tempfile.NamedTemporaryFile() as tmp:
            tmp.write(b'TIME board 0\n')
            tmp.flush()
            tmp.seek(0)
            sys.argv = ['', '--signal-config', tmp.name, '--insecure-http']
            mock_exporter = mock.create_autospec(PrometheusExporter)
            mock_collector = mock.create_autospec(Collector)
            with mock.patch('geopmdpy.pio.signal_names', return_value=_MOCK_SIGNAL_NAMES), \
                 mock.patch('geopmdpy.exporter.PrometheusExporter', return_value=mock_exporter), \
                 mock.patch('geopmdpy.stats.Collector', return_value=mock_collector) as mock_collector_init:
                exporter.main()
        mock_collector_init.assert_called_once_with([('TIME', 0, 0)])
        mock_exporter.run.assert_called_with(0.1, 8000, None, None)

    def test_summary(self):
        with tempfile.NamedTemporaryFile() as tmp:
            tmp.write(b'TIME board 0\n')
            tmp.flush()
            tmp.seek(0)
            sys.argv = ['', '--signal-config', tmp.name, '--summary', 'prometheus', '--insecure-http']
            mock_exporter = mock.create_autospec(PrometheusMetricExporter)
            with mock.patch('geopmdpy.pio.signal_names', return_value=_MOCK_SIGNAL_NAMES), \
                 mock.patch('geopmdpy.exporter.PrometheusMetricExporter', return_value=mock_exporter) as mock_exporter_init:
                    exporter.main()
        mock_exporter_init.assert_called_once_with([('TIME', 0, 0)])
        mock_exporter.run.assert_called_with(0.1, 8000, None, None)

    def test_https(self):
        with tempfile.NamedTemporaryFile() as certfile, tempfile.NamedTemporaryFile() as keyfile:
            sys.argv = ['', '--certfile', certfile.name, '--keyfile', keyfile.name]
            mock_exporter = mock.create_autospec(PrometheusExporter)
            mock_collector = mock.create_autospec(Collector)
            port = 8000
            with mock.patch('geopmdpy.pio.signal_names', return_value=_MOCK_SIGNAL_NAMES), \
                 mock.patch('geopmdpy.exporter.PrometheusExporter', return_value=mock_exporter), \
                 mock.patch('geopmdpy.stats.Collector', return_value=mock_collector):
                exporter.main()
                mock_exporter.run.assert_called_with(0.1, port, certfile.name, keyfile.name)

    def test_https_prometheus_metric_exporter(self):
        with tempfile.NamedTemporaryFile() as certfile, tempfile.NamedTemporaryFile() as keyfile:
            sys.argv = ['', '--certfile', certfile.name, '--keyfile', keyfile.name, '--summary', 'prometheus']
            mock_exporter = mock.create_autospec(PrometheusMetricExporter)
            with mock.patch('geopmdpy.pio.signal_names', return_value=_MOCK_SIGNAL_NAMES), \
                 mock.patch('geopmdpy.exporter.PrometheusMetricExporter', return_value=mock_exporter):
                exporter.main()
                mock_exporter.run.assert_called_with(0.1, 8000, certfile.name, keyfile.name)

if __name__ == '__main__':
    main()
