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

#TODO write some stuff
"""OUTLIER_DETECTION

"""

# Make sure @skip decorator is included here

import sys
import unittest
import os
import glob
import json
import numpy as np
import numpy.matlib as npml

sys.path.append(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
from test_integration import geopm_context
import geopmpy.io
import geopmpy.error

import geopmpy.analysis

NAN=float("nan")

_g_skip_launch = False
try:
    sys.argv.remove('--skip-launch')
    _g_skip_launch = True
except ValueError:
    from test_integration import geopm_test_launcher
    from test_integration import util
    geopmpy.error.exc_clear()

class TestIntegration_outlier_detection(unittest.TestCase):
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
        # Clear out exception record for python 2 support
        geopmpy.error.exc_clear()

        if os.path.isdir(cls._output_dir):
            cls._skip_launch = True
            cls._keep_files = True

        cls.region_of_interest = "dgemm"
        cls.output_prefix = test_name

        # TODO use the second arg of getenv to provide the default
        if os.getenv('GEOPM_REGION_OF_INTEREST'):
            cls.region_of_interest = os.getenv('GEOPM_REGION_OF_INTEREST')
        if os.getenv('GEOPM_OUTPUT_PREFIX'):
            cls.output_prefix = os.getenv('GEOPM_OUTPUT_PREFIX')

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
    def calculate_ranges(cls):
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
                # old reports don't have this
                power = base_report_filename[len(cls.output_prefix + "_power_governor")+1:]
                power = int(power[:power.find("_")])
                cls.power_range.add(power)
            cls.host_range.update(report.host_names())
            cls.region_of_interest_hash = report.region_hash(cls.region_of_interest)
        cls.power_range = sorted(list(cls.power_range))
        cls.host_range = sorted(list(cls.host_range))
        cls.iteration_range = sorted(list(cls.iteration_range))

    @classmethod
    def compute_package_energy(cls):
        avg = lambda LL: sum(LL)/float(len(LL))

        package_energy = {host: {power: [] for power in cls.power_range} for host in cls.host_range}
        for base_report_filename in cls.find_reports():
            report = geopmpy.io.RawReport(os.path.join(cls._output_dir, base_report_filename))
            try:
                power = report.meta_data()['Policy']['POWER_PACKAGE_LIMIT_TOTAL']
            except KeyError:
                power = base_report_filename[len(cls.output_prefix + "_power_governor")+1:]
                power = int(power[:power.find("_")])
            for host in report.host_names():
                package_energy[host][power].append(report.get_field(report.raw_region(host, cls.region_of_interest), 'package-energy', 'joules'))

        cls.avg_package_energy = {host: {power: [] for power in cls.power_range} for host in cls.host_range}
        for host in cls.host_range:
            for power in cls.power_range:
                cls.avg_package_energy[host][power] = avg(package_energy[host][power])


    def test_load_reports(self):
        """Test that the report can be loaded

        """
        reports = [os.path.join(self._output_dir, report) for report in self.find_reports()]
        report = geopmpy.io.RawReport(reports[0])
        import code
        code.interact(local=locals())

    def test_stat0(self):
        stat0_vals = self.stat0()
        outliers = self.outliers(stat0_vals)
        for outlier_info in outliers:
            self.print_outlier_line(outlier_info, "Extreme incremental power at sticker")
        self.assertEqual(0, len(outliers), "Outlier values of statistic 0 were measured.")

    def test_stat1(self):
        stat1_vals = self.stat1()
        outliers = self.outliers(stat1_vals)
        for outlier_info in outliers:
            self.print_outlier_line(outlier_info, "Extreme energy consumption at TDP")
        self.assertEqual(0, len(outliers), "Outlier values of statistic 1 were measured.")

    def test_stat2(self):
        stat2_vals = self.stat2()
        outliers = self.outliers(stat2_vals)
        for outlier_info in outliers:
            self.print_outlier_line(outlier_info, "Extreme minimum energy consumption")
        self.assertEqual(0, len(outliers), "Outlier values of statistic 2 were measured.")

    def test_stat3(self):
        stat3_vals = self.stat3()
        outliers = self.outliers(stat3_vals)
        for outlier_info in outliers:
            self.print_outlier_line(outlier_info, "Extreme power limit at which energy consumption is minimized")
        self.assertEqual(0, len(outliers), "Outlier values of statistic 3 were measured.")

    def test_stat4(self):
        stat4_vals = self.stat4()
        outliers0 = self.outliers(stat4_vals, lambda x: x[0]**-1)
        outliers1 = self.outliers(stat4_vals, lambda x: x[1]**-1)
        outliers2 = self.outliers(stat4_vals, lambda x: x[2]**-1)
        for outlier_info in outliers:
            self.print_outlier_line(outlier_info, "Extreme thermal coefficient")
        for outlier_info in outliers:
            self.print_outlier_line(outlier_info, "Extreme thermal coefficient")
        for outlier_info in outliers:
            self.print_outlier_line(outlier_info, "Extreme thermal coefficient")
        self.assertEqual(0, len(outliers0) + len(outliers1) + len(outliers2), "Outlier values of statistic 4 were measured.")

    @classmethod
    def stat0_by_host(cls, host):
        """Computes power/frequency model for the given host."""
        x1 = [] ; y1 = []
        x2 = [] ; y2 = []
        for power in cls.power_range:
            for iteration in cls.iteration_range:
                file_pointer = os.path.join(cls._output_dir, "%s_%d_power_governor_%d.trace-%s" % (cls.output_prefix, power, iteration, host))
                try:
                    file_pointer = open(file_pointer)
                except:
                    # be robust against iterations not being run on every host
                    # TODO make this an error message
                    print(file_pointer)
                    continue
                for tline in geopmpy.io.RawTraceIterator(file_pointer):
                    if tline["region_hash"] == cls.region_of_interest_hash:
                        if tline["time_delta"] < 1e-10:
                            continue
                        if tline["frequency_f"] < cls.Pn/float(cls.P1):
                            x1.append(tline["frequency_f"])
                            y1.append(tline["energy_package_delta"]/tline["time_delta"])
                        else:
                            x2.append(tline["frequency_f"])
                            y2.append(tline["energy_package_delta"]/tline["time_delta"])
        best_fit = np.polyfit(x1+x2, y1+y2, deg=3)
        if len(x1) == 0:
            best_fit1 = [NAN, NAN]
        else:
            best_fit1 = np.polyfit(x1, y1, deg=1)
        best_fit2 = np.polyfit(x2, y2, deg=3)
        return best_fit, best_fit1, best_fit2

    @classmethod
    def stat0(cls):
        """Computes power/frequency slope at sticker."""
        slopes = {}
        for host in cls.host_range:
            best_fit, best_fit1, best_fit2 = cls.stat0_by_host(host)
            slopes[host] = sum([c*a for c,a in zip([3, 2, 1, 0], best_fit2)])/cls.P1
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
                    'mean': mean,
                    'stddev': stddev,
                    'err': (stat(value_dict[key]) - mean)/stddev # number of sigmas above the mean
                    } for key in value_dict.keys() if stat(value_dict[key]) > X]

    @staticmethod
    def print_outlier_line(outlier_info, error, output=sys.stdout):
        output.write("%s: Host %s reports a value of %.1f (%.1f sigmas above the mean %.1f)\n" % (error, outlier_info['key'], outlier_info['value'], outlier_info['err'], outlier_info['mean']))


if __name__ == '__main__':
    try:
        sys.argv.remove('--skip-launch')
        _g_skip_launch = True
    except ValueError:
        geopmpy.error.exc_clear()
    unittest.main()
