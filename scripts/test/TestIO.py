#!/usr/bin/env python
#
#  Copyright (c) 2015, 2016, 2017, 2018, 2019, Intel Corporation
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

from __future__ import absolute_import

import unittest
import os
import tempfile
import shutil
import mock
from collections import Counter
from contextlib import contextmanager

import geopmpy.io

def touch_file(file_path):
    """ Set a file's last-modified time to now.
    Create the file if it doesn't exist.
    """
    with open(file_path, 'a'):
        os.utime(file_path, None)

@contextmanager
def self_cleaning_app_output(*args, **kwargs):
    """ Create an AppOutput that cleans up its cache files when it goes out of
    scope.
    """
    app_output = geopmpy.io.AppOutput(*args, **kwargs)
    try:
        yield app_output
    finally:
        app_output.remove_files()

test_report_data = """##### geopm 1.0.0+dev30g4cccfda #####
Start Time: Thu May 30 14:38:17 2019
Profile: test_ee_stream_dgemm_mix
Agent: energy_efficient

Host: mcfly11
Final online freq map:
    0x000000000db5f27a: 1000000000.000000
    0x000000003a6c47e3: 1000000000.000000
    0x000000003c627f60: 1400000000.000000
    0x00000000536c798f: 1000000000.000000
    0x00000000725e8066: 2100000000.000000
    0x0000000076244144: 1100000000.000000
    0x000000009851de3f: 1000000000.000000
    0x00000000addfa74f: 1600000000.000000
    0x00000000af4cafa3: 1700000000.000000
    0x00000000ce08ae24: 1300000000.000000
    0x00000000e1242325: 1700000000.000000
    0x00000000e50a9187: 1600000000.000000
    0x00000000f9d11bbd: 1500000000.000000

Region sleep (0x00000000536c798f):
    runtime (sec): 275.998
    sync-runtime (sec): 276.581
    package-energy (joules): 33573.9
    dram-energy (joules): 2835.43
    power (watts): 121.389
    frequency (%): 50.0846
    frequency (Hz): 1.05178e+09
    network-time (sec): 0
    count: 5500
    requested-online-frequency: 1000000000.000000
Region stream-0.35-dgemm-1.80 (0x0000000076244144):
    runtime (sec): 28.5021
    sync-runtime (sec): 28.4395
    package-energy (joules): 4804.24
    dram-energy (joules): 598.129
    power (watts): 168.928
    frequency (%): 53.9602
    frequency (Hz): 1.13317e+09
    network-time (sec): 0
    count: 500
    requested-online-frequency: 1100000000.000000
Region stream-0.40-dgemm-1.20 (0x000000000db5f27a):
    runtime (sec): 28.2568
    sync-runtime (sec): 28.2187
    package-energy (joules): 4575.15
    dram-energy (joules): 639.775
    power (watts): 162.132
    frequency (%): 50.303
    frequency (Hz): 1.05636e+09
    network-time (sec): 0
    count: 500
    requested-online-frequency: 1000000000.000000
Region stream-0.45-dgemm-0.60 (0x000000009851de3f):
    runtime (sec): 27.1528
    sync-runtime (sec): 27.1159
    package-energy (joules): 4430.24
    dram-energy (joules): 667.228
    power (watts): 163.382
    frequency (%): 50.5233
    frequency (Hz): 1.06099e+09
    network-time (sec): 0
    count: 500
    requested-online-frequency: 1000000000.000000
Region stream-0.30-dgemm-2.40 (0x00000000ce08ae24):
    runtime (sec): 25.6523
    sync-runtime (sec): 25.5885
    package-energy (joules): 4517.88
    dram-energy (joules): 540.39
    power (watts): 176.559
    frequency (%): 61.4774
    frequency (Hz): 1.29102e+09
    network-time (sec): 0
    count: 500
    requested-online-frequency: 1300000000.000000
Region stream-0.50-dgemm-0.00 (0x000000003a6c47e3):
    runtime (sec): 25.3236
    sync-runtime (sec): 25.2728
    package-energy (joules): 4308.43
    dram-energy (joules): 685.24
    power (watts): 170.477
    frequency (%): 50.8315
    frequency (Hz): 1.06746e+09
    network-time (sec): 0
    count: 500
    requested-online-frequency: 1000000000.000000
Region stream-0.25-dgemm-3.00 (0x000000003c627f60):
    runtime (sec): 24.4374
    sync-runtime (sec): 24.3823
    package-energy (joules): 4606.69
    dram-energy (joules): 491.049
    power (watts): 188.936
    frequency (%): 65.2223
    frequency (Hz): 1.36967e+09
    network-time (sec): 0
    count: 500
    requested-online-frequency: 1400000000.000000
Region stream-0.20-dgemm-3.60 (0x00000000f9d11bbd):
    runtime (sec): 23.2674
    sync-runtime (sec): 23.2105
    package-energy (joules): 4591.63
    dram-energy (joules): 443.931
    power (watts): 197.826
    frequency (%): 68.9304
    frequency (Hz): 1.44754e+09
    network-time (sec): 0
    count: 500
    requested-online-frequency: 1500000000.000000
Region stream-0.15-dgemm-4.20 (0x00000000addfa74f):
    runtime (sec): 20.85
    sync-runtime (sec): 20.7729
    package-energy (joules): 4424.92
    dram-energy (joules): 383.237
    power (watts): 213.014
    frequency (%): 72.3136
    frequency (Hz): 1.51858e+09
    network-time (sec): 0
    count: 500
    requested-online-frequency: 1600000000.000000
Region stream-0.10-dgemm-4.80 (0x00000000e50a9187):
    runtime (sec): 19.8318
    sync-runtime (sec): 19.7529
    package-energy (joules): 4429.42
    dram-energy (joules): 328.406
    power (watts): 224.242
    frequency (%): 72.2398
    frequency (Hz): 1.51704e+09
    network-time (sec): 0
    count: 500
    requested-online-frequency: 1600000000.000000
Region stream-0.05-dgemm-5.40 (0x00000000e1242325):
    runtime (sec): 18.2642
    sync-runtime (sec): 18.1456
    package-energy (joules): 4319.39
    dram-energy (joules): 280.249
    power (watts): 238.041
    frequency (%): 75.8382
    frequency (Hz): 1.5926e+09
    network-time (sec): 0
    count: 500
    requested-online-frequency: 1700000000.000000
Region stream-0.00-dgemm-6.00 (0x00000000af4cafa3):
    runtime (sec): 17.8807
    sync-runtime (sec): 17.8327
    package-energy (joules): 4367.48
    dram-energy (joules): 252.722
    power (watts): 244.914
    frequency (%): 75.7953
    frequency (Hz): 1.5917e+09
    network-time (sec): 0
    count: 500
    requested-online-frequency: 1700000000.000000
Region unmarked-region (0x00000000725e8066):
    runtime (sec): 6.07865
    sync-runtime (sec): 6.13216
    package-energy (joules): 1043.75
    dram-energy (joules): 72.5349
    power (watts): 170.209
    frequency (%): 97.2362
    frequency (Hz): 2.04196e+09
    network-time (sec): 0
    count: 0
    requested-online-frequency: 2100000000.000000
Epoch Totals:
    runtime (sec): 0
    sync-runtime (sec): 0
    package-energy (joules): 0
    dram-energy (joules): 0
    power (watts): 0
    frequency (%): 0
    frequency (Hz): 0
    network-time (sec): 0
    count: 0
    epoch-runtime-ignore (sec): 0
Application Totals:
    runtime (sec): 541.507
    package-energy (joules): 83998.9
    dram-energy (joules): 8218.69
    power (watts): 155.121
    network-time (sec): 0
    ignore-time (sec): 0
    geopmctl memory HWM: 144472 kB
    geopmctl network BW (B/sec): 0.0443208

Host: mcfly12
Final online freq map:
    0x000000000db5f27a: 1000000000.000000
    0x000000003a6c47e3: 1000000000.000000
    0x000000003c627f60: 1400000000.000000
    0x00000000536c798f: 1000000000.000000
    0x00000000725e8066: 2200000000.000000
    0x0000000076244144: 1100000000.000000
    0x000000009851de3f: 1000000000.000000
    0x00000000addfa74f: 1500000000.000000
    0x00000000af4cafa3: 1700000000.000000
    0x00000000ce08ae24: 1300000000.000000
    0x00000000e1242325: 1700000000.000000
    0x00000000e50a9187: 1600000000.000000
    0x00000000f9d11bbd: 1500000000.000000

Region sleep (0x00000000536c798f):
    runtime (sec): 275.837
    sync-runtime (sec): 276.56
    package-energy (joules): 34545.2
    dram-energy (joules): 3811.41
    power (watts): 124.91
    frequency (%): 50.0117
    frequency (Hz): 1.05024e+09
    network-time (sec): 0
    count: 5500
    requested-online-frequency: 1000000000.000000
Region stream-0.35-dgemm-1.80 (0x0000000076244144):
    runtime (sec): 29.1273
    sync-runtime (sec): 29.0865
    package-energy (joules): 4968.14
    dram-energy (joules): 713.884
    power (watts): 170.806
    frequency (%): 53.9922
    frequency (Hz): 1.13384e+09
    network-time (sec): 0
    count: 500
    requested-online-frequency: 1100000000.000000
Region stream-0.40-dgemm-1.20 (0x000000000db5f27a):
    runtime (sec): 28.4557
    sync-runtime (sec): 28.3904
    package-energy (joules): 4697.92
    dram-energy (joules): 745.835
    power (watts): 165.476
    frequency (%): 50.3009
    frequency (Hz): 1.05632e+09
    network-time (sec): 0
    count: 500
    requested-online-frequency: 1000000000.000000
Region stream-0.45-dgemm-0.60 (0x000000009851de3f):
    runtime (sec): 27.2367
    sync-runtime (sec): 27.1905
    package-energy (joules): 4478.41
    dram-energy (joules): 767.496
    power (watts): 164.705
    frequency (%): 50.3711
    frequency (Hz): 1.05779e+09
    network-time (sec): 0
    count: 500
    requested-online-frequency: 1000000000.000000
Region stream-0.30-dgemm-2.40 (0x00000000ce08ae24):
    runtime (sec): 25.8974
    sync-runtime (sec): 25.8082
    package-energy (joules): 4672.56
    dram-energy (joules): 636.411
    power (watts): 181.05
    frequency (%): 61.493
    frequency (Hz): 1.29135e+09
    network-time (sec): 0
    count: 500
    requested-online-frequency: 1300000000.000000
Region stream-0.50-dgemm-0.00 (0x000000003a6c47e3):
    runtime (sec): 25.2686
    sync-runtime (sec): 25.2523
    package-energy (joules): 4412.83
    dram-energy (joules): 776.741
    power (watts): 174.75
    frequency (%): 50.9256
    frequency (Hz): 1.06944e+09
    network-time (sec): 0
    count: 500
    requested-online-frequency: 1000000000.000000
Region stream-0.25-dgemm-3.00 (0x000000003c627f60):
    runtime (sec): 24.5979
    sync-runtime (sec): 24.5117
    package-energy (joules): 4732.35
    dram-energy (joules): 583.029
    power (watts): 193.065
    frequency (%): 65.2297
    frequency (Hz): 1.36982e+09
    network-time (sec): 0
    count: 500
    requested-online-frequency: 1400000000.000000
Region stream-0.20-dgemm-3.60 (0x00000000f9d11bbd):
    runtime (sec): 23.2737
    sync-runtime (sec): 23.2129
    package-energy (joules): 4694.3
    dram-energy (joules): 527.13
    power (watts): 202.228
    frequency (%): 68.9091
    frequency (Hz): 1.44709e+09
    network-time (sec): 0
    count: 500
    requested-online-frequency: 1500000000.000000
Region stream-0.15-dgemm-4.20 (0x00000000addfa74f):
    runtime (sec): 22.3063
    sync-runtime (sec): 22.1766
    package-energy (joules): 4650.85
    dram-energy (joules): 466.899
    power (watts): 209.719
    frequency (%): 68.7558
    frequency (Hz): 1.44387e+09
    network-time (sec): 0
    count: 500
    requested-online-frequency: 1500000000.000000
Region stream-0.10-dgemm-4.80 (0x00000000e50a9187):
    runtime (sec): 20.3869
    sync-runtime (sec): 20.3101
    package-energy (joules): 4646.27
    dram-energy (joules): 410.599
    power (watts): 228.767
    frequency (%): 72.2666
    frequency (Hz): 1.5176e+09
    network-time (sec): 0
    count: 500
    requested-online-frequency: 1600000000.000000
Region stream-0.05-dgemm-5.40 (0x00000000e1242325):
    runtime (sec): 19.2864
    sync-runtime (sec): 19.1575
    package-energy (joules): 4618.09
    dram-energy (joules): 352.889
    power (watts): 241.059
    frequency (%): 76.1363
    frequency (Hz): 1.59886e+09
    network-time (sec): 0
    count: 500
    requested-online-frequency: 1700000000.000000
Region stream-0.00-dgemm-6.00 (0x00000000af4cafa3):
    runtime (sec): 18.6718
    sync-runtime (sec): 18.6886
    package-energy (joules): 4600.85
    dram-energy (joules): 321.137
    power (watts): 246.185
    frequency (%): 75.8795
    frequency (Hz): 1.59347e+09
    network-time (sec): 0
    count: 500
    requested-online-frequency: 1700000000.000000
Region unmarked-region (0x00000000725e8066):
    runtime (sec): 1.15034
    sync-runtime (sec): 1.11497
    package-energy (joules): 226.452
    dram-energy (joules): 28.9149
    power (watts): 203.102
    frequency (%): 95.0914
    frequency (Hz): 1.99692e+09
    network-time (sec): 0
    count: 0
    requested-online-frequency: 2200000000.000000
Epoch Totals:
    runtime (sec): 0
    sync-runtime (sec): 0
    package-energy (joules): 0
    dram-energy (joules): 0
    power (watts): 0
    frequency (%): 0
    frequency (Hz): 0
    network-time (sec): 0
    count: 0
    epoch-runtime-ignore (sec): 0
Application Totals:
    runtime (sec): 541.505
    package-energy (joules): 85946.8
    dram-energy (joules): 10142.6
    power (watts): 158.718
    network-time (sec): 0
    ignore-time (sec): 0
    geopmctl memory HWM: 142300 kB
    geopmctl network BW (B/sec): 0
"""

# First lines from a test_trace_runtimes integration test run
test_trace_data = """# geopm_version: 1.0.0+dev209g77e1ebb8
# start_time: Thu Oct 03 08:19:34 2019
# profile_name: test_trace_runtimes
# node_name: mcfly1
# agent: power_governor
TIME|EPOCH_COUNT|REGION_HASH|REGION_HINT|REGION_PROGRESS|REGION_COUNT|REGION_RUNTIME|ENERGY_PACKAGE|ENERGY_DRAM|POWER_PACKAGE|POWER_DRAM|FREQUENCY|CYCLES_THREAD|CYCLES_REFERENCE|TEMPERATURE_CORE|POWER_BUDGET
0.204296343|-1|0x00000000725e8066|0x0000000100000000|0|0|0|242598.262512207|31537.68629558909|nan|nan|2763636363.636364|44306314967139|44889410342036|60.18181818181818|0
0.2223164129999999|-1|0x00000000644f9787|0x0000000100000000|0|0|0|242603.1904296875|31538.00337322582|273.4682762313778|17.59580494025078|2800000000|44310710666495|44892733038308|61.72727272727273|150
0.2235088959999999|-1|0x00000000644f9787|0x0000000100000000|0|0|0|242603.6185302734|31538.02900799145|276.6301227683065|17.73233606304858|2800000000|44311004818789|44892952655048|61.77272727272727|150
0.228532226|-1|0x00000000644f9787|0x0000000100000000|0|0|0|242604.9693603516|31538.11309917797|276.9201297420731|17.65826611195596|2100000000|44312147188306|44893868186252|61.43181818181818|150
0.233763619|-1|0x00000000644f9787|0x0000000100000000|0|0|0|242605.6206665039|31538.18380840649|213.1977333899407|17.13213273088159|776136363.6363636|44312679661043|44894805921596|59.79545454545455|150
0.238758759|-1|0x00000000644f9787|0x0000000100000000|0|0|0|242606.1339111328|31538.24949749341|160.7312347320962|16.51782487831938|800000000|44313012842482|44895724971644|59.13636363636363|150
0.2436604579999999|-1|0x00000000644f9787|0x0000000100000000|0|0|0|242606.7144165039|31538.31727703442|114.1336448920131|16.01735740105445|1450000000|44313477045974|44896618887716|58.97727272727273|150
0.248572409|-1|0x00000000644f9787|0x0000000100000000|0|0|0|242607.4743041992|31538.39257915845|124.4617940659552|15.71872562446383|1877272727.272727|44314215214920|44897517408080|59.15909090909091|150
0.253558066|-1|0x00000000644f9787|0x0000000100000000|0|0|0|242608.3024291992|31538.46925457349|147.3800125521751|14.56509260230485|1850000000|44315033338888|44898435409136|59.29545454545455|150
0.2585734129999999|-1|0x00000000644f9787|0x0000000100000000|0|0|0|242609.1127319336|31538.54991253248|161.3588539038742|14.57640738621386|1650000000|44315800167450|44899353800036|59.13636363636363|150
0.263574746|-1|0x00000000644f9787|0x0000000100000000|0|0|0|242609.8388671875|31538.61998089185|158.0069444604364|14.60065683114463|1550000000|44316493094098|44900272907876|59.02272727272727|150
0.268616921|-1|0x00000000644f9787|0x0000000100000000|0|0|0|242610.5656738281|31538.70031841627|149.7814626529688|14.94329910436242|1600000000|44317174126049|44901195205784|58.93181818181818|150
"""

class TestIO(unittest.TestCase):
    def setUp(self):
        if 'assertCountEqual' not in dir(self):
            # Python 3 replaces assertItemsEqual with assertCountEqual
            self.assertCountEqual = self.assertItemsEqual
        self._test_directory = tempfile.mkdtemp()
        self._report_path = os.path.join(self._test_directory, 'geopmpy-io-test-raw-report')
        with open(self._report_path, 'w') as fid:
            fid.write(test_report_data)
        self._trace_path = os.path.join(self._test_directory, 'geopmpy-io-test-trace')
        with open(self._trace_path, 'w') as fid:
            fid.write(test_trace_data)

    def tearDown(self):
        shutil.rmtree(self._test_directory)

    def test_requested_online_frequency(self):
        report = geopmpy.io.RawReport(self._report_path)
        host_names = report.host_names()
        for nn in report.region_names(host_names[0]):
            if 'dgemm-0.00' in nn:
                stream_region_name = nn
            if 'stream-0.00' in nn:
                dgemm_region_name = nn
        stream_region = report.raw_region(host_names[0], stream_region_name)
        dgemm_region = report.raw_region(host_names[0], dgemm_region_name)
        self.assertLess(stream_region['requested-online-frequency'], dgemm_region['requested-online-frequency'])

        raw = report.raw_report()
        json_path = self._report_path + '.json'
        report.dump_json(json_path)
        meta = report.meta_data()
        hosts = report.host_names()
        region_names = report.region_names(hosts[0])
        hash = report.region_hash(region_names[0])
        region = report.raw_region(hosts[1], region_names[1])
        region_runtime = region['runtime (sec)']
        count = region['count']
        epoch = report.raw_epoch(hosts[0])
        runtime = epoch['runtime (sec)']
        self.assertEqual(0, runtime)
        totals = report.raw_totals(hosts[1])
        total_runtime = totals['runtime (sec)']
        self.assertLess(region_runtime, total_runtime)
        field_runtime = report.get_field(epoch, 'runtime')
        self.assertEqual(runtime, field_runtime)

    def test_report(self):
        """ Test that a file of concatenated reports can be extracted to
        a dataframe.
        """
        with self_cleaning_app_output(reports=self._report_path) as app_output:
            self.assertCountEqual(['mcfly11', 'mcfly12'], app_output.get_node_names())
            start_time = app_output.get_report_data().index.get_level_values('start_time').unique()
            self.assertEqual(1, len(start_time))
            self.assertEqual('Thu May 30 14:38:17 2019', start_time[0])
            self.assertEqual(27.1528, app_output.get_report_data(node_name='mcfly11', region='stream-0.45-dgemm-0.60').iloc[0]['runtime'])
            self.assertEqual(27.2367, app_output.get_report_data(node_name='mcfly12', region='stream-0.45-dgemm-0.60').iloc[0]['runtime'])

    def test_report_cache(self):
        """ Test that a report is not read when it is cached.
        """
        spy_open = mock.Mock(wraps=open)
        def count_open(path):
            """ Count the number of times spy_open() has been called with path
            """
            return Counter([c[0][0] for c in spy_open.call_args_list])[path]

        with mock.patch('geopmpy.io.open', spy_open), self_cleaning_app_output(reports=self._report_path):
            initial_call_count = count_open(self._report_path)

            geopmpy.io.AppOutput(reports=self._report_path)
            self.assertEqual(initial_call_count, count_open(self._report_path))

            touch_file(self._report_path)
            geopmpy.io.AppOutput(reports=self._report_path)
            self.assertEqual(initial_call_count * 2, count_open(self._report_path))

    def test_trace(self):
        """ Test that a trace file can be extracted to a dataframe.
        """
        with self_cleaning_app_output(traces=self._trace_path) as app_output:
            trace_df = app_output.get_trace_data(node_name='mcfly1')
            start_time = trace_df.index.get_level_values('start_time').unique()
            self.assertEqual(1, len(start_time))
            self.assertEqual('Thu Oct 03 08:19:34 2019', start_time[0])
            self.assertAlmostEqual(0.204296343, trace_df.iloc[0]['TIME'])
            self.assertAlmostEqual(242598.262512207, trace_df.iloc[0]['ENERGY_PACKAGE'])
            self.assertAlmostEqual(0.268616921, trace_df.iloc[-1]['TIME'])
            self.assertAlmostEqual(242610.5656738281, trace_df.iloc[-1]['ENERGY_PACKAGE'])

if __name__ == '__main__':
    unittest.main()
