#!/usr/bin/env python
#
#  Copyright (c) 2015, 2016, 2017, 2018, 2019, 2020, Intel Corporation
#
#  Redistribution and use in source and binary forms, with or without
#  modification, are permitted provided that the following conditions
#  are met:
#
#      * Redistributions of source code must retain the above copyright
#        notice, this list of conditions and the following disclaimer.
#
#      * Redistributions in binary form must reproduce the above copyright
#        notice, this list of conditions and the following disclaimer in
#        the documentation and/or other materials provided with the
#        distribution.
#
#      * Neither the name of Intel Corporation nor the names of its
#        contributors may be used to endorse or promote products derived
#        from this software without specific prior written permission.
#
#  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
#  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
#  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
#  A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
#  OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
#  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
#  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
#  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
#  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
#  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY LOG OF THE USE
#  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#

"""OUTLIER_DETECTION

The outlier detection integration test is intended to identify outlier nodes
based on time-series statistics gathered from a benchmark run, which it can
also run if desired. The script's behavior is controlled via environment
variables.

Variables that are always used:
    GEOPM_OUTLIER_DIRECTORY - The directory to use for trace and report files
                              If this directory exists, the benchmark will
                              not be run. If it does not exist, create it and
                              run the benchmark, populating it with trace
                              and report files.  "outlier_detection_output" by
                              default.
    GEOPM_OUTLIER_REGION_OF_INTEREST - The name of the region to use for the
                                       analysis.  "dgemm" by default.
    GEOPM_OUTLIER_REGION_OF_INTEREST_HASH - The hash of the region to use for
                                            the analysis.  "0x00000000a74bbf35"
                                            by default (corresponding to dgemm).
    GEOPM_OUTLIER_OUTPUT_PREFIX -
                          The prefix for report and trace filenames. Report
                          files follow the pattern
                          <prefix>_<power limit>_power_governor_<iteration>.report
                          and trace files follow the pattern
                          <prefix>_<power limit>_power_governor_<iteration>.trace-<host>
                          "outlier_detection" by default.

Variables used by the benchmark run:
    GEOPM_OUTLIER_MIN_POWER, GEOPM_OUTLIER_MAX_POWER, GEOPM_OUTLIER_POWER_STEP:
                          These variables control the range of power values
                          that the benchmark is run at. The benchmark run
                          is run for powers between GEOPM_OUTLIER_MIN_POWER
                          and GEOPM_OUTLIER_MAX_POWER (inclusive) at steps
                          GEOPM_OUTLIER_POWER_STEP. The defaults are 0W,
                          1kW and 10W, respectively. WE HIGHLY RECOMMEND
                          SETTING MIN AND MAX POWER.
    GEOPM_OUTLIER_NUM_ITERATIONS: The number of iterations to run at each
                          power limit. 2 by default.
    GEOPM_OUTLIER_NUM_NODES: The number of nodes to run the benchmark on.
                          12 by default.
    GEOPM_OUTLIER_NUM_RANKS: The number of ranks to run the benchmark on
                          (across nodes). 300 by default.
    GEOPM_OUTLIER_BENCH_CONFIG: A geopmbench config file to use for the
                          benchmark runs. By default, this file is generated
                          at runtime in the output directory and is called
                          "short.json". It is configured to run 10 dgemm
                          loops for 30 seconds apiece.

Variables used by statistic 0 alone:
    GEOPM_OUTLIER_PN - The frequency at which voltage begins scaling with
                       frequency in GHz.  Architecture dependent. (Default 1.05)
    GEOPM_OUTLIER_P1 - Sticker frequency in GHz. Architecture dependent.
                       (Default 1.3)

Since the runs tend to be fairly long, this script will not erase its output.
Also, the script will generate cache files and plots in the output directory
that are safe to erase. The cache files have the suffix "_cache.json" and
the plots have the png extension.

The command should be executed in an environment that is prepared to execute
geopm as described in the general README.

If the directory medium14.420 does not exist, the following command-line will
create it, run the benchmark at power limits from 140W to 280W (inclusive) at
10W increments on 14 nodes with 420 ranks, then analyze the output using
statistic 1.

$ GEOPM_OUTLIER_DIRECTORY=medium14.420 GEOPM_OUTLIER_MIN_POWER=140 GEOPM_OULIER_MAX_POWER=280 GEOPM_OUTLIER_NUM_NODES=14 GEOPM_OUTLIER_NUM_RANKS=420 -v TestIntegration_outlier_detection.test_stat1

Later, to analyze the resulting data with statistic 2:

$ GEOPM_OUTLIER_DIRECTORY=medium14.420 -v TestIntegration_outlier_detection.test_stat2

Later, to analyze the resulting data with statistic 0:

$ GEOPM_OUTLIER_DIRECTORY=medium14.420 GEOPM_OUTLIER_PN=1.0 GEOPM_OUTLIER_P1=2.1 -v TestIntegration_outlier_detection.test_stat0

The test will "pass" if no outliers are found. If outliers are found,
the corresponding error message will indicate which node has failed,
the computed value and the deviation from the model. For example:

Test_stat1 (__main__.TestIntegration_outlier_detection) ... (test_outlier_detection.TestIntegration_outlier_detection) ...Extreme energy consumption at TDP: Host mcfly1 reports a value of 12986.7 (2.2 sigmas above the mean 12582.7)
FAIL

======================================================================
FAIL: test_stat1 (__main__.TestIntegration_outlier_detection)
----------------------------------------------------------------------
Traceback (most recent call last):
  File "./test_outlier_detection.py", line 227, in test_stat1
    self.assertEqual(0, len(outliers), "Outlier values of statistic 1 were measured.")
AssertionError: Outlier values of statistic 1 were measured.

----------------------------------------------------------------------
Ran 1 test in 5.280s

FAILED (failures=1)

Plots can be generated using the test_plot0, test_plot1 and test_plot2
tests (which will always pass). These output png files to the output
directory. If the imgcat python module is installed and you are using
a terminal that supports sixel or base64 embedded images, these plots will
also be rendered to the terminal.

The statistics are defined below:

Statistic 0: slope of power/frequency curve at sticker.
Statistic 1: Energy consumption at TDP.
Statistic 2: Minimum energy consumption across power caps.
Statistic 3: Power cap at which energy consumption is minimized.
Statistic 4: Temperature related statistics.  Running test_stat4
             will search for outliers on the basis of:
             4.0: Inverse Processor-Sensor Heat Capacity
             4.1: Thermal Conduction to Cooling Fluid (Fan-Driven Air)
             4.2: Estimated Cooling Fluid Temperature
"""

# Make sure @skip decorator is included here

import sys
import unittest
import os
import glob
import json
import math
import numpy as np
import numpy.matlib as npml

from matplotlib.backends.backend_agg import FigureCanvasAgg as FigureCanvas
from matplotlib.figure import Figure

import geopm_context

import geopmpy.io
import geopmpy.error
import geopmpy.analysis

NAN=float("nan")

_g_skip_launch = False

class TestIntegration_outlier_detection(unittest.TestCase):
    avg_package_energy = None
    power_range = None
    host_range = None
    iteration_range = None

    @classmethod
    def setUpClass(cls):
        """Create launcher, execute benchmark and set up class variables.

        """
        sys.stdout.write('(' + os.path.basename(__file__).split('.')[0] +
                         '.' + cls.__name__ + ') ...')
        test_name = 'outlier_detection'

        cls._output_dir = os.getenv('GEOPM_OUTLIER_DIRECTORY') or 'outlier_detection_output'
        cls._skip_launch = _g_skip_launch
        cls._keep_files = os.getenv('GEOPM_KEEP_FILES') is not None

        if os.path.isdir(cls._output_dir):
            cls._skip_launch = True
            cls._keep_files = True

        cls.region_of_interest = "dgemm"
        # TODO compute the hash
        cls.region_of_interest_hash = "0x00000000a74bbf35"
        cls.output_prefix = test_name

        # TODO use the second arg of getenv to provide the default
        if os.getenv('GEOPM_OUTLIER_REGION_OF_INTEREST'):
            cls.region_of_interest = os.getenv('GEOPM_OUTLIER_REGION_OF_INTEREST')
        if os.getenv('GEOPM_OUTLIER_REGION_OF_INTEREST_HASH'):
            cls.region_of_interest_hash = os.getenv('GEOPM_OUTLIER_REGION_OF_INTEREST_HASH')
        if os.getenv('GEOPM_OUTLIER_OUTPUT_PREFIX'):
            cls.output_prefix = os.getenv('GEOPM_OUTLIER_OUTPUT_PREFIX')

        cls.Pn, cls.P1 = 1.05, 1.3
        if os.getenv('GEOPM_PN'):
            cls.Pn = float(os.getenv('GEOPM_PN'))
        if os.getenv('GEOPM_P1'):
            cls.P1 = float(os.getenv('GEOPM_P1'))

        if not cls._skip_launch:
            # TODO(alawibaba)
            # horrifying workaround since sys_power_avail doesn't seem to work
            geopmpy.analysis.PowerSweepAnalysis.sys_power_avail = staticmethod(lambda self = None: (0, 1e3, 1e3))

            # Default values:
            # default power sweep is to go from min power to tdp in 10W increments
            min_power, max_power, _ = geopmpy.analysis.PowerSweepAnalysis.sys_power_avail()
            power_step = 10
            # default of two iterations, with 12 nodes and 25 ranks per node
            num_iterations = 2
            num_nodes = 12
            num_ranks = 300

            if os.getenv('GEOPM_OUTLIER_MIN_POWER'):
                min_power = int(os.getenv('GEOPM_OUTLIER_MIN_POWER'))
            if os.getenv('GEOPM_OUTLIER_MAX_POWER'):
                max_power = int(os.getenv('GEOPM_OUTLIER_MAX_POWER'))
            if os.getenv('GEOPM_OUTLIER_POWER_STEP'):
                power_step = int(os.getenv('GEOPM_OUTLIER_POWER_STEP'))
            if os.getenv('GEOPM_OUTLIER_NUM_ITERATIONS'):
                num_iterations = int(os.getenv('GEOPM_OUTLIER_NUM_ITERATIONS'))
            if os.getenv('GEOPM_OUTLIER_NUM_NODES'):
                num_nodes = int(os.getenv('GEOPM_OUTLIER_NUM_NODES'))
            if os.getenv('GEOPM_OUTLIER_NUM_RANKS'):
                num_ranks = int(os.getenv('GEOPM_OUTLIER_NUM_RANKS'))
            cls._analysis_obj = geopmpy.analysis.PowerSweepAnalysis(
                    profile_prefix=cls.output_prefix,
                    output_dir=cls._output_dir,
                    verbose=False,
                    iterations=num_iterations,
                    min_power=min_power,
                    max_power=max_power,
                    step_power=power_step
                    )
            bench_config_json = os.path.join(cls._output_dir, "short.json")
            if os.getenv('GEOPM_OUTLIER_BENCH_CONFIG'):
                bench_config_json = os.getenv('GEOPM_OUTLIER_BENCH_CONFIG')
            else:
                benchconf = geopmpy.io.BenchConf(bench_config_json)
                benchconf.set_loop_count(10)
                benchconf.append_region('dgemm', 30)
                benchconf.write()
            cls._analysis_obj.launch(
                    launcher_name = geopm_test_launcher.detect_launcher(),
                    args=['-n', str(num_ranks), '-N', str(num_nodes), 'geopmbench', bench_config_json])
        cls.calculate_ranges()
        cls.compute_package_energy()

    @classmethod
    def tearDownClass(cls):
        pass

    @classmethod
    def find_reports(cls):
        search_pattern = '*report'
        report_glob = os.path.join(cls._output_dir, cls.output_prefix + search_pattern)
        report_files = [os.path.basename(ff) for ff in glob.glob(report_glob)]
        return report_files

    @classmethod
    def guess_power_limit_from_filename(cls, filename):
        # this is only used for old reports (before we started embedding the policy in the report)
        power = filename[len(cls.output_prefix)+1:]
        power = int(power[:power.find("_")])
        return power

    @classmethod
    def calculate_ranges(cls, cache=True):
        cache_path = os.path.join(cls._output_dir, 'range_cache.json')
        if cache and os.path.exists(cache_path):
            with open(cache_path) as cache_file:
                cls.power_range, cls.host_range, cls.iteration_range = json.load(cache_file)
            return
        cls.power_range = set()
        cls.iteration_range = set()
        cls.host_range = set()
        for base_report_filename in cls.find_reports():
            iteration = base_report_filename[base_report_filename.rfind('_')+1:]
            iteration = iteration[:iteration.find('.report')]
            cls.iteration_range.add(int(iteration))
            report = geopmpy.io.RawReport(os.path.join(cls._output_dir, base_report_filename))
            try:
                cls.power_range.add(report.meta_data()['Policy']['POWER_PACKAGE_LIMIT_TOTAL'])
            except KeyError:
                cls.power_range.add(cls.guess_power_limit_from_filename(base_report_filename))
            cls.host_range.update(report.host_names())
        cls.power_range = sorted(list(cls.power_range))
        cls.host_range = sorted(list(cls.host_range))
        cls.iteration_range = sorted(list(cls.iteration_range))
        if cache:
            with open(cache_path, 'w') as cache_file:
                json.dump((cls.power_range, cls.host_range, cls.iteration_range), cache_file)

    @classmethod
    def compute_package_energy(cls, cache=True):
        cache_path = os.path.join(cls._output_dir, 'avg_package_energy_cache.json')
        if cache and os.path.exists(cache_path):
            with open(cache_path) as cache_file:
                avg_package_energy_raw = json.load(cache_file)
                # json doesn't support integers as dictionary keys
                cls.avg_package_energy = {}
                for host in avg_package_energy_raw:
                    cls.avg_package_energy[host] = {}
                    for power_raw in avg_package_energy_raw[host]:
                        cls.avg_package_energy[host][int(power_raw)] = avg_package_energy_raw[host][power_raw]
            return
        cls.calculate_ranges()
        avg = lambda LL: sum(LL)/float(len(LL))

        package_energy = {host: {power: [] for power in cls.power_range} for host in cls.host_range}
        for base_report_filename in cls.find_reports():
            report = geopmpy.io.RawReport(os.path.join(cls._output_dir, base_report_filename))
            try:
                power = report.meta_data()['Policy']['POWER_PACKAGE_LIMIT_TOTAL']
            except KeyError:
                power = cls.guess_power_limit_from_filename(base_report_filename)
            for host in report.host_names():
                package_energy[host][power].append(report.get_field(report.raw_region(host, cls.region_of_interest), 'package-energy', 'joules'))

        cls.avg_package_energy = {host: {power: [] for power in cls.power_range} for host in cls.host_range}
        for host in cls.host_range:
            for power in cls.power_range:
                cls.avg_package_energy[host][power] = avg(package_energy[host][power])

        if cache:
            with open(cache_path, 'w') as cache_file:
                json.dump(cls.avg_package_energy, cache_file)

    def test_run(self):
        "Just collect traces and reports."
        pass

    def test_interpreter(self):
        "Open an interactive session."
        import code
        code.interact(local=dict(globals(), **locals()))

    def test_stat0(self):
        stat0_vals = self.stat0()
        outliers, model = self.outliers(stat0_vals)
        for outlier_info in outliers:
            self.print_outlier_line(outlier_info, model, "Extreme incremental power at sticker")
        self.assertEqual(0, len(outliers), "Outlier values of statistic 0 were measured.")

    def test_stat1(self):
        stat1_vals = self.stat1()
        outliers, model = self.outliers(stat1_vals)
        for outlier_info in outliers:
            self.print_outlier_line(outlier_info, model, "Extreme energy consumption at TDP")
        self.assertEqual(0, len(outliers), "Outlier values of statistic 1 were measured.")

    def test_stat2(self):
        stat2_vals = self.stat2()
        outliers, model = self.outliers(stat2_vals)
        for outlier_info in outliers:
            self.print_outlier_line(outlier_info, model, "Extreme minimum energy consumption")
        self.assertEqual(0, len(outliers), "Outlier values of statistic 2 were measured.")

    def test_stat3(self):
        stat3_vals = self.stat3()
        outliers, model = self.outliers(stat3_vals)
        for outlier_info in outliers:
            self.print_outlier_line(outlier_info, model, "Extreme power limit at which energy consumption is minimized")
        self.assertEqual(0, len(outliers), "Outlier values of statistic 3 were measured.")

    def test_stat4(self):
        stat4_vals = self.stat4()
        outliers0, model0 = self.outliers(stat4_vals, lambda x: x[0]**-1)
        outliers1, model1 = self.outliers(stat4_vals, lambda x: x[1]**-1)
        outliers2, model2 = self.outliers(stat4_vals, lambda x: x[2]**-1)
        for outlier_info in outliers0:
            self.print_outlier_line(outlier_info, model0, "Extreme thermal coefficient")
        for outlier_info in outliers1:
            self.print_outlier_line(outlier_info, model1, "Extreme thermal coefficient")
        for outlier_info in outliers2:
            self.print_outlier_line(outlier_info, model2, "Extreme thermal coefficient")
        self.assertEqual(0, len(outliers0) + len(outliers1) + len(outliers2), "Outlier values of statistic 4 were measured.")

    @classmethod
    def power_vs_freq_by_host(cls, host, min_time=5):
        x1 = [] ; y1 = []
        x2 = [] ; y2 = []
        for power in cls.power_range:
            for iteration in cls.iteration_range:
                file_pointer = os.path.join(cls._output_dir, "%s_%d_power_governor_%d.trace-%s" % (cls.output_prefix, power, iteration, host))
                try:
                    file_pointer = open(file_pointer)
                except:
                    # be robust against iterations not being run on every host
                    sys.stderr.write("File '%s' not found. Continuing.\n" % file_pointer)
                    continue
                for tline in geopmpy.io.RawTraceIterator(file_pointer):
                    if tline["region_hash"] == cls.region_of_interest_hash and tline["time_f"] > min_time:
                        if tline["time_delta"] < 1e-10:
                            continue
                        sample_freq = tline["cycles_thread_delta"]/tline["cycles_reference_delta"]
                        if sample_freq < cls.Pn/float(cls.P1):
                            x1.append(sample_freq)
                            y1.append(tline["energy_package_delta"]/tline["time_delta"])
                        else:
                            x2.append(sample_freq)
                            y2.append(tline["energy_package_delta"]/tline["time_delta"])
        return x1, x2, y1, y2

    @classmethod
    def trimodel_fit(cls, host, datatuple):
        (x1, x2, y1, y2) = datatuple
        best_fit = np.polyfit(x1+x2, y1+y2, deg=3)
        if len(x1) == 0:
            best_fit1 = [NAN, NAN]
        else:
            best_fit1 = np.polyfit(x1, y1, deg=1)
        best_fit2 = np.polyfit(x2, y2, deg=3)
        return {'overall': list(best_fit), 'linear': list(best_fit1), 'cubic': list(best_fit2)}

    @classmethod
    def stat0_by_host(cls, host, min_time=5):
        """Computes power/frequency model for the given host."""
        return cls.trimodel_fit(host, cls.power_vs_freq_by_host(host, min_time))

    @classmethod
    def stat0(cls, cache=True):
        """Computes power/frequency slope at sticker."""
        cache_path = os.path.join(cls._output_dir, 'stat0_cache.json')
        if cache and os.path.exists(cache_path):
            fits = json.load(open(cache_path))
        else:
            fits = {}
            for host in cls.host_range:
                fits[host] = cls.stat0_by_host(host)
            if cache:
                with open(cache_path, 'w') as fp:
                    json.dump(fits, fp)
        slopes = {}
        for host in fits.keys():
            slopes[host] = sum([c*a for c,a in zip([3, 2, 1, 0], fits[host]['cubic'])])
        return slopes

    @classmethod
    def stat1(cls):
        """Computes energy consumption of region of interest at TDP."""
        TDP_POWER = max(cls.power_range)
        rval = {}
        for host in cls.host_range:
            rval[host] = cls.avg_package_energy[host][TDP_POWER]
        return rval

    @classmethod
    def stat2(cls):
        """Computes minimum energy consumption of region of interest across power limits."""
        rval = {}
        for host in cls.host_range:
            rval[host] = min(cls.avg_package_energy[host].values())
        return rval

    @classmethod
    def stat3(cls):
        """Computes power limit at which energy consumption of region of interest is minized."""
        argmin = lambda DD: min([(DD[k],k) for k in DD.keys()])[1]

        rval = {}
        for host in cls.host_range:
            rval[host] = argmin(cls.avg_package_energy[host])
        return rval

    @classmethod
    def stat4_by_host(cls, host, window=300):
        XTXacc = npml.zeros((3,3))
        XTyacc = npml.zeros((3,1))
        for power in cls.power_range:
            for iteration in cls.iteration_range:
                file_pointer = os.path.join(cls._output_dir, "%s_%d_power_governor_%d.trace-%s" % (cls.output_prefix, power, iteration, host))
                try:
                    file_pointer = open(file_pointer)
                except:
                    # be robust against iterations not being run on every host
                    continue
                subt = []
                subdt = []
                subdE = []
                subdTs = []
                subITs = []
                subaTa = []

                datafile = []
                for tline in geopmpy.io.RawTraceIterator(file_pointer):
                    datafile.append([tline['time_f'], tline['energy_package_f'], tline['temperature_core_f']])

                iidx = 0
                fidx = window - 1

                tempsum = 0.5*(datafile[iidx+1][0]-datafile[iidx][0])*datafile[iidx][2] \
                        + 0.5*(datafile[fidx][0]-datafile[fidx-1][0])*datafile[fidx][2]
                for subidx in range(iidx+1,fidx):
                    tempsum += 0.5*(datafile[subidx+1][0]-datafile[subidx-1][0])*datafile[subidx][2]

                for idx in range(1,len(datafile)-window):
                    tempsum += 0.5*(-datafile[iidx+1][0]+datafile[iidx][0])*datafile[iidx][2] \
                            + 0.5*( datafile[iidx][0]-datafile[iidx+1][0])*datafile[iidx+1][2]\
                            + 0.5*(-datafile[fidx][0]  +datafile[fidx+1][0])*datafile[fidx][2]\
                            + 0.5*( datafile[fidx+1][0]-datafile[fidx][0])*datafile[fidx+1][2]

                    iidx = idx
                    fidx = iidx + window - 1

                    t = 0.5*(datafile[iidx][0] + datafile[fidx][0])
                    dt = datafile[fidx][0] - datafile[iidx][0]
                    dE = datafile[fidx][1] - datafile[iidx][1]
                    dTs = datafile[fidx][2] - datafile[iidx][2]
                    ITs = tempsum

                    X = np.matrix([[ \
                            dE,
                            ITs,
                            dt,
                        ]])
                    y = dTs # delta temperature

                    XTXacc += X.T * X
                    XTyacc += X.T * y

        return (XTXacc.I * XTyacc).T.tolist()[0]

    @classmethod
    def stat4(cls):
        rval = {}

        for host in cls.host_range:
            rval[host] = cls.stat4_by_host(host)
        return rval

    @staticmethod
    def outliers(value_dict, stat=lambda x: x):
        # we're putting in an implementation of erfinv
        # so we can avoid requiring another library
        # Adapted from https://people.maths.ox.ac.uk/gilesm/files/gems_erfinv.pdf
        def erfinv(v):
            import math
            w = -math.log((1-v)*(1+v))
            if w < 5:
                w -= 2.5
                p = 2.81022636e-08
                p = 3.43273939e-07 + p*w
                p = -3.5233877e-06 + p*w
                p = -4.39150654e-06 + p*w
                p = 0.00021858087 + p*w
                p = -0.00125372503 + p*w
                p = -0.00417768164 + p*w
                p = 0.246640727 + p*w
                p = 1.50140941 + p*w
            else:
                w = w**.5 - 3
                p = -0.000200214257
                p = 0.000100950558 + p*w
                p = 0.00134934322 + p*w
                p = -0.00367342844 + p*w
                p = 0.00573950773 + p*w
                p = -0.0076224613 + p*w
                p = 0.00943887047 + p*w
                p = 1.00167406 + p*w
                p = 2.83297682 + p*w
            return p
        mean = sum([stat(value) for value in value_dict.values()])/float(len(value_dict))
        stddev = (sum([stat(value)**2 for value in value_dict.values()])/float(len(value_dict)) - mean**2)**0.5
        X = mean + stddev * 2**.5 * erfinv(1-2./len(value_dict))
        return [
                {
                    'key': key,
                    'value': stat(value_dict[key]),
                    'err': (stat(value_dict[key]) - mean)/stddev # number of sigmas above the mean
                    } for key in value_dict.keys() if stat(value_dict[key]) > X], \
                {'mu': mean, 'sigma': stddev}

    @staticmethod
    def print_outlier_line(outlier_info, model, error, output=sys.stdout):
        output.write("%s: Host %s reports a value of %.1f (%.1f sigmas above the mean %.1f)\n" % (error, outlier_info['key'], outlier_info['value'], outlier_info['err'], model['mu']))

    def plot_power_freq_model(self, host):
        x1, x2, y1, y2 = self.power_vs_freq_by_host(host)
        best_fit = self.trimodel_fit(host, (x1, x2, y1, y2))
        fig = Figure()
        FigureCanvas(fig)
        ax = fig.add_subplot(111)

        ax.scatter(x1+x2, y1+y2)
        range1 = np.arange(min(x1), max(x1), 0.02)
        range2 = np.arange(min(x2), max(x2), 0.02)
        range12 = np.arange(min(x1+x2), max(x1+x2), 0.02)
        overallfit = lambda freq: sum([a*freq**c for c,a in zip([3, 2, 1, 0], best_fit['overall'])])
        ax.plot(range12, overallfit(range12))
        if not math.isnan(best_fit['linear'][0]) and not math.isnan(best_fit['linear'][1]):
            linearfit = lambda freq: sum([a*freq**c for c,a in zip([1, 0], best_fit['linear'])])
            cubicfit = lambda freq: sum([a*freq**c for c,a in zip([3, 2, 1, 0], best_fit['cubic'])])
            ax.plot(range1, linearfit(range1))
            ax.plot(range2, cubicfit(range2))
        ax.set_xlabel('frequency (GHz)')
        ax.set_ylabel('power (W)')
        fig.savefig(os.path.join(self._output_dir, "pvsf_%s.png" % host))
        try:
            import imgcat
            sys.stdout.write("\n")
            with open(os.path.join(self._output_dir, "pvsf_%s.png" % host)) as imgfile:
                imgcat.imgcat(imgfile)
        except ImportError:
            pass

    def test_plot0(self, imgcat=True):
        fig = Figure()
        FigureCanvas(fig)
        ax = fig.add_subplot(111)
        stat0out = self.stat0()
        npoints = len(stat0out)
        jon = ax.hist(stat0out.values())
        _, model = self.outliers(stat0out)
        mu = model['mu'] ; sigma = model['sigma']
        erfs = [math.erf(x) for x in (jon[1]-mu)/sigma]
        maxheight = max([b - a for a, b in zip([-1] + erfs, erfs + [1])]) * npoints / 8**.5
        heights = np.arange(min(jon[1]), max(jon[1]), 0.02)
        ax.plot(heights, maxheight * np.exp(-(heights-mu)**2/(2*sigma**2)))
        ax.set_xlabel('Incremental energy consumption at sticker (W/GHz)')
        ax.set_ylabel('count')
        fig.savefig(os.path.join(self._output_dir, "plot0.png"))
        if imgcat:
            try:
                import imgcat
                sys.stdout.write("\n")
                with open(os.path.join(self._output_dir, "plot0.png")) as imgfile:
                    imgcat.imgcat(imgfile)
            except ImportError:
                pass

    def test_plot1(self, imgcat=True):
        fig = Figure()
        FigureCanvas(fig)
        ax = fig.add_subplot(111)
        stat1out = self.stat1()
        npoints = len(stat1out)
        jon = ax.hist(stat1out.values())
        _, model = self.outliers(stat1out)
        mu = model['mu'] ; sigma = model['sigma']
        erfs = [math.erf(x) for x in (jon[1]-mu)/sigma]
        maxheight = max([b - a for a, b in zip([-1] + erfs, erfs + [1])]) * npoints / 8**.5
        heights = np.arange(min(jon[1]), max(jon[1]), 0.02)
        ax.plot(heights, maxheight * np.exp(-(heights-mu)**2/(2*sigma**2)))
        ax.set_xlabel('Energy consumption at TDP (J)')
        ax.set_ylabel('count')
        fig.savefig(os.path.join(self._output_dir, "plot1.png"))
        if imgcat:
            try:
                import imgcat
                sys.stdout.write("\n")
                with open(os.path.join(self._output_dir, "plot1.png")) as imgfile:
                    imgcat.imgcat(imgfile)
            except ImportError:
                pass

    def test_plot2(self, imgcat=True):
        fig = Figure()
        FigureCanvas(fig)
        ax = fig.add_subplot(111)
        stat2out = self.stat2()
        npoints = len(stat2out)
        jon = ax.hist(stat2out.values())
        _, model = self.outliers(stat2out)
        mu = model['mu'] ; sigma = model['sigma']
        erfs = [math.erf(x) for x in (jon[1]-mu)/sigma]
        maxheight = max([b - a for a, b in zip([-1] + erfs, erfs + [1])]) * npoints / 8**.5
        heights = np.arange(min(jon[1]), max(jon[1]), 0.02)
        ax.plot(heights, maxheight * np.exp(-(heights-mu)**2/(2*sigma**2)))
        ax.set_xlabel('Minimum energy consumption (J)')
        ax.set_ylabel('count')
        fig.savefig(os.path.join(self._output_dir, "plot2.png"))
        if imgcat:
            try:
                import imgcat
                sys.stdout.write("\n")
                with open(os.path.join(self._output_dir, "plot2.png")) as imgfile:
                    imgcat.imgcat(imgfile)
            except ImportError:
                pass

def inlineScatterPlot(xdata, ydata):
    import imgcat
    import tempfile
    fig = Figure()
    FigureCanvas(fig)
    ax = fig.add_subplot(111)
    ax.scatter(xdata, ydata)
    _, a = tempfile.mkstemp(suffix='.png')
    fig.savefig(a)
    with open(a) as ff:
        imgcat.imgcat(ff)
    os.remove(a)

if __name__ == '__main__':
    try:
        sys.argv.remove('--skip-launch')
        _g_skip_launch = True
    except ValueError:
        import geopm_test_launcher
        import util
        geopmpy.error.exc_clear()
    unittest.main()
