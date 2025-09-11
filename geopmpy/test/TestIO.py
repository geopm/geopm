#!/usr/bin/env python3
#
#  Copyright (c) 2015 - 2025 Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#


import unittest
import os
import tempfile
import shutil
from unittest import mock
from collections import Counter
from contextlib import contextmanager

# test_report_cache requires the use of read_hdf() from pandas
os.environ["GEOPM_USE_UNSAFE_HDF5"] = "1"
import geopmpy.io


def touch_file(file_path):
    """ Set a file's last-modified time to now.
    Create the file if it doesn't exist.
    """
    with open(file_path, 'a'):
        os.utime(file_path, None)


@contextmanager
def self_cleaning_raw_report_collection(path):
    """Create an raw report that will remove the cache after it goes out
    of scope.

    """
    raw_report = geopmpy.io.RawReportCollection(path)
    try:
        yield raw_report
    finally:
        raw_report.remove_cache()


test_report_data_old = """\
##### geopm 1.0.0+dev30g4cccfda #####
Start Time: Thu May 30 14:38:17 2019
Profile: test_fm_stream_dgemm_mix
Agent: frequency_map
Policy: {}


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
    frequency (Hz): 1e+09
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
Region with some spaces (0x00000000af000fa3):
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
    network-time (sec): 12
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
    network-time (sec): 14
    count: 0
    epoch-runtime-ignore (sec): 0
Application Totals:
    runtime (sec): 541.507
    package-energy (joules): 83998.9
    dram-energy (joules): 8218.69
    power (watts): 155.121
    network-time (sec): 15
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
Region with some spaces (0x00000000af000fa3):
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
    runtime (sec): 1.15034
    sync-runtime (sec): 1.11497
    package-energy (joules): 226.452
    dram-energy (joules): 28.9149
    power (watts): 203.102
    frequency (%): 95.0914
    frequency (Hz): 1.99692e+09
    network-time (sec): 12
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
    network-time (sec): 14
    count: 0
    epoch-runtime-ignore (sec): 0
Application Totals:
    runtime (sec): 541.505
    package-energy (joules): 85946.8
    dram-energy (joules): 10142.6
    power (watts): 158.718
    network-time (sec): 15
    ignore-time (sec): 0
    geopmctl memory HWM: 142300 kB
    geopmctl network BW (B/sec): 0
"""

test_report_data = """\
GEOPM Version: 1.0.0+dev30g4cccfda
Start Time: Thu May 30 14:38:17 2019
Profile: test_fm_stream_dgemm_mix
Agent: frequency_map
Policy: {}

Hosts:
  mcfly11:
    Final online freq map:
      0x0db5f27a: 1000000000.000000
      0x3a6c47e3: 1000000000.000000
      0x3c627f60: 1400000000.000000
      0x536c798f: 1000000000.000000
      0x725e8066: 2100000000.000000
      0x76244144: 1100000000.000000
      0x9851de3f: 1000000000.000000
      0xaddfa74f: 1600000000.000000
      0xaf4cafa3: 1700000000.000000
      0xce08ae24: 1300000000.000000
      0xe1242325: 1700000000.000000
      0xe50a9187: 1600000000.000000
      0xf9d11bbd: 1500000000.000000

    Regions:
    -
      region: "sleep"
      hash: 0x536c798f
      runtime (s): 275.998
      sync-runtime (s): 276.581
      package-energy (J): 33573.9
      dram-energy (J): 2835.43
      power (W): 121.389
      frequency (%): 50.0846
      frequency (Hz): 1e+09
      time-hint-network (s): 0
      time-hint-ignore (s): 0
      time-hint-compute (s): 0
      time-hint-memory (s): 0
      time-hint-io (s): 0
      time-hint-serial (s): 0
      time-hint-parallel (s): 0
      time-hint-unknown (s): 0
      time-hint-unset (s): 0
      count: 5500
      requested-online-frequency: 1000000000.000000
    -
      region: "stream-0.35-dgemm-1.80"
      hash: 0x76244144
      runtime (s): 28.5021
      sync-runtime (s): 28.4395
      package-energy (J): 4804.24
      dram-energy (J): 598.129
      power (W): 168.928
      frequency (%): 53.9602
      frequency (Hz): 1.13317e+09
      time-hint-network (s): 0
      time-hint-ignore (s): 0
      time-hint-compute (s): 0
      time-hint-memory (s): 0
      time-hint-io (s): 0
      time-hint-serial (s): 0
      time-hint-parallel (s): 0
      time-hint-unknown (s): 0
      time-hint-unset (s): 0
      count: 500
      requested-online-frequency: 1100000000.000000
    -
      region: "stream-0.40-dgemm-1.20"
      hash: 0x0db5f27a
      runtime (s): 28.2568
      sync-runtime (s): 28.2187
      package-energy (J): 4575.15
      dram-energy (J): 639.775
      power (W): 162.132
      frequency (%): 50.303
      frequency (Hz): 1.05636e+09
      time-hint-network (s): 0
      time-hint-ignore (s): 0
      time-hint-compute (s): 0
      time-hint-memory (s): 0
      time-hint-io (s): 0
      time-hint-serial (s): 0
      time-hint-parallel (s): 0
      time-hint-unknown (s): 0
      time-hint-unset (s): 0
      count: 500
      requested-online-frequency: 1000000000.000000
    -
      region: "stream-0.45-dgemm-0.60"
      hash: 0x9851de3f
      runtime (s): 27.1528
      sync-runtime (s): 27.1159
      package-energy (J): 4430.24
      dram-energy (J): 667.228
      power (W): 163.382
      frequency (%): 50.5233
      frequency (Hz): 1.06099e+09
      time-hint-network (s): 0
      time-hint-ignore (s): 0
      time-hint-compute (s): 0
      time-hint-memory (s): 0
      time-hint-io (s): 0
      time-hint-serial (s): 0
      time-hint-parallel (s): 0
      time-hint-unknown (s): 0
      time-hint-unset (s): 0
      count: 500
      requested-online-frequency: 1000000000.000000
    -
      region: "stream-0.30-dgemm-2.40"
      hash: 0xce08ae24
      runtime (s): 25.6523
      sync-runtime (s): 25.5885
      package-energy (J): 4517.88
      dram-energy (J): 540.39
      power (W): 176.559
      frequency (%): 61.4774
      frequency (Hz): 1.29102e+09
      time-hint-network (s): 0
      time-hint-ignore (s): 0
      time-hint-compute (s): 0
      time-hint-memory (s): 0
      time-hint-io (s): 0
      time-hint-serial (s): 0
      time-hint-parallel (s): 0
      time-hint-unknown (s): 0
      time-hint-unset (s): 0
      count: 500
      requested-online-frequency: 1300000000.000000
    -
      region: "stream-0.50-dgemm-0.00"
      hash: 0x3a6c47e3
      runtime (s): 25.3236
      sync-runtime (s): 25.2728
      package-energy (J): 4308.43
      dram-energy (J): 685.24
      power (W): 170.477
      frequency (%): 50.8315
      frequency (Hz): 1.06746e+09
      time-hint-network (s): 0
      time-hint-ignore (s): 0
      time-hint-compute (s): 0
      time-hint-memory (s): 0
      time-hint-io (s): 0
      time-hint-serial (s): 0
      time-hint-parallel (s): 0
      time-hint-unknown (s): 0
      time-hint-unset (s): 0
      count: 500
      requested-online-frequency: 1000000000.000000
    -
      region: "stream-0.25-dgemm-3.00"
      hash: 0x3c627f60
      runtime (s): 24.4374
      sync-runtime (s): 24.3823
      package-energy (J): 4606.69
      dram-energy (J): 491.049
      power (W): 188.936
      frequency (%): 65.2223
      frequency (Hz): 1.36967e+09
      time-hint-network (s): 0
      time-hint-ignore (s): 0
      time-hint-compute (s): 0
      time-hint-memory (s): 0
      time-hint-io (s): 0
      time-hint-serial (s): 0
      time-hint-parallel (s): 0
      time-hint-unknown (s): 0
      time-hint-unset (s): 0
      count: 500
      requested-online-frequency: 1400000000.000000
    -
      region: "stream-0.20-dgemm-3.60"
      hash: 0xf9d11bbd
      runtime (s): 23.2674
      sync-runtime (s): 23.2105
      package-energy (J): 4591.63
      dram-energy (J): 443.931
      power (W): 197.826
      frequency (%): 68.9304
      frequency (Hz): 1.44754e+09
      time-hint-network (s): 0
      time-hint-ignore (s): 0
      time-hint-compute (s): 0
      time-hint-memory (s): 0
      time-hint-io (s): 0
      time-hint-serial (s): 0
      time-hint-parallel (s): 0
      time-hint-unknown (s): 0
      time-hint-unset (s): 0
      count: 500
      requested-online-frequency: 1500000000.000000
    -
      region: "stream-0.15-dgemm-4.20"
      hash: 0xaddfa74f
      runtime (s): 20.85
      sync-runtime (s): 20.7729
      package-energy (J): 4424.92
      dram-energy (J): 383.237
      power (W): 213.014
      frequency (%): 72.3136
      frequency (Hz): 1.51858e+09
      time-hint-network (s): 0
      time-hint-ignore (s): 0
      time-hint-compute (s): 0
      time-hint-memory (s): 0
      time-hint-io (s): 0
      time-hint-serial (s): 0
      time-hint-parallel (s): 0
      time-hint-unknown (s): 0
      time-hint-unset (s): 0
      count: 500
      requested-online-frequency: 1600000000.000000
    -
      region: "stream-0.10-dgemm-4.80"
      hash: 0xe50a9187
      runtime (s): 19.8318
      sync-runtime (s): 19.7529
      package-energy (J): 4429.42
      dram-energy (J): 328.406
      power (W): 224.242
      frequency (%): 72.2398
      frequency (Hz): 1.51704e+09
      time-hint-network (s): 0
      time-hint-ignore (s): 0
      time-hint-compute (s): 0
      time-hint-memory (s): 0
      time-hint-io (s): 0
      time-hint-serial (s): 0
      time-hint-parallel (s): 0
      time-hint-unknown (s): 0
      time-hint-unset (s): 0
      count: 500
      requested-online-frequency: 1600000000.000000
    -
      region: "stream-0.05-dgemm-5.40"
      hash: 0xe1242325
      runtime (s): 18.2642
      sync-runtime (s): 18.1456
      package-energy (J): 4319.39
      dram-energy (J): 280.249
      power (W): 238.041
      frequency (%): 75.8382
      frequency (Hz): 1.5926e+09
      time-hint-network (s): 0
      time-hint-ignore (s): 0
      time-hint-compute (s): 0
      time-hint-memory (s): 0
      time-hint-io (s): 0
      time-hint-serial (s): 0
      time-hint-parallel (s): 0
      time-hint-unknown (s): 0
      time-hint-unset (s): 0
      count: 500
      requested-online-frequency: 1700000000.000000
    -
      region: "stream-0.00-dgemm-6.00"
      hash: 0xaf4cafa3
      runtime (s): 17.8807
      sync-runtime (s): 17.8327
      package-energy (J): 4367.48
      dram-energy (J): 252.722
      power (W): 244.914
      frequency (%): 75.7953
      frequency (Hz): 1.5917e+09
      time-hint-network (s): 0
      time-hint-ignore (s): 0
      time-hint-compute (s): 0
      time-hint-memory (s): 0
      time-hint-io (s): 0
      time-hint-serial (s): 0
      time-hint-parallel (s): 0
      time-hint-unknown (s): 0
      time-hint-unset (s): 0
      count: 500
      requested-online-frequency: 1700000000.000000
    -
      region: "with some spaces"
      hash: 0xaf000fa3
      runtime (s): 17.8807
      sync-runtime (s): 17.8327
      package-energy (J): 4367.48
      dram-energy (J): 252.722
      power (W): 244.914
      frequency (%): 75.7953
      frequency (Hz): 1.5917e+09
      time-hint-network (s): 0
      time-hint-ignore (s): 0
      time-hint-compute (s): 0
      time-hint-memory (s): 0
      time-hint-io (s): 0
      time-hint-serial (s): 0
      time-hint-parallel (s): 0
      time-hint-unknown (s): 0
      time-hint-unset (s): 0
      count: 500
      requested-online-frequency: 1700000000.000000
    Unmarked Totals:
      runtime (s): 6.07865
      sync-runtime (s): 6.13216
      package-energy (J): 1043.75
      dram-energy (J): 72.5349
      power (W): 170.209
      frequency (%): 97.2362
      frequency (Hz): 2.04196e+09
      time-hint-network (s): 12
      time-hint-ignore (s): 0
      time-hint-compute (s): 0
      time-hint-memory (s): 0
      time-hint-io (s): 0
      time-hint-serial (s): 0
      time-hint-parallel (s): 0
      time-hint-unknown (s): 0
      time-hint-unset (s): 0
      count: 0
      requested-online-frequency: 2100000000.000000
    Epoch Totals:
      runtime (s): 0
      sync-runtime (s): 0
      package-energy (J): 0
      dram-energy (J): 0
      power (W): 0
      frequency (%): 0
      frequency (Hz): 0
      time-hint-network (s): 14
      time-hint-compute (s): 0
      time-hint-memory (s): 0
      time-hint-io (s): 0
      time-hint-serial (s): 0
      time-hint-parallel (s): 0
      time-hint-unknown (s): 0
      time-hint-unset (s): 0
      count: 0
      time-hint-ignore (s): 0
    Application Totals:
      runtime (s): 541.507
      sync-runtime (s): 541.507
      package-energy (J): 83998.9
      dram-energy (J): 8218.69
      power (W): 155.121
      frequency (%): 0
      frequency (Hz): 0
      time-hint-network (s): 15
      time-hint-compute (s): 0
      time-hint-memory (s): 0
      time-hint-io (s): 0
      time-hint-serial (s): 0
      time-hint-parallel (s): 0
      time-hint-unknown (s): 0
      time-hint-unset (s): 0
      time-hint-ignore (s): 0
      geopmctl memory HWM (B): 147939328
      geopmctl network BW (B/sec): 0.0443208

  mcfly12:
    Final online freq map:
      0x0db5f27a: 1000000000.000000
      0x3a6c47e3: 1000000000.000000
      0x3c627f60: 1400000000.000000
      0x536c798f: 1000000000.000000
      0x725e8066: 2200000000.000000
      0x76244144: 1100000000.000000
      0x9851de3f: 1000000000.000000
      0xaddfa74f: 1500000000.000000
      0xaf4cafa3: 1700000000.000000
      0xce08ae24: 1300000000.000000
      0xe1242325: 1700000000.000000
      0xe50a9187: 1600000000.000000
      0xf9d11bbd: 1500000000.000000

    Regions:
    -
      region: "sleep"
      hash: 0x536c798f
      runtime (s): 275.837
      sync-runtime (s): 276.56
      package-energy (J): 34545.2
      dram-energy (J): 3811.41
      power (W): 124.91
      frequency (%): 50.0117
      frequency (Hz): 1.05024e+09
      time-hint-network (s): 0
      time-hint-ignore (s): 0
      time-hint-compute (s): 0
      time-hint-memory (s): 0
      time-hint-io (s): 0
      time-hint-serial (s): 0
      time-hint-parallel (s): 0
      time-hint-unknown (s): 0
      time-hint-unset (s): 0
      count: 5500
      requested-online-frequency: 1000000000.000000
    -
      region: "stream-0.35-dgemm-1.80"
      hash: 0x76244144
      runtime (s): 29.1273
      sync-runtime (s): 29.0865
      package-energy (J): 4968.14
      dram-energy (J): 713.884
      power (W): 170.806
      frequency (%): 53.9922
      frequency (Hz): 1.13384e+09
      time-hint-network (s): 0
      time-hint-ignore (s): 0
      time-hint-compute (s): 0
      time-hint-memory (s): 0
      time-hint-io (s): 0
      time-hint-serial (s): 0
      time-hint-parallel (s): 0
      time-hint-unknown (s): 0
      time-hint-unset (s): 0
      count: 500
      requested-online-frequency: 1100000000.000000
    -
      region: "stream-0.40-dgemm-1.20"
      hash: 0x0db5f27a
      runtime (s): 28.4557
      sync-runtime (s): 28.3904
      package-energy (J): 4697.92
      dram-energy (J): 745.835
      power (W): 165.476
      frequency (%): 50.3009
      frequency (Hz): 1.05632e+09
      time-hint-network (s): 0
      time-hint-ignore (s): 0
      time-hint-compute (s): 0
      time-hint-memory (s): 0
      time-hint-io (s): 0
      time-hint-serial (s): 0
      time-hint-parallel (s): 0
      time-hint-unknown (s): 0
      time-hint-unset (s): 0
      count: 500
      requested-online-frequency: 1000000000.000000
    -
      region: "stream-0.45-dgemm-0.60"
      hash: 0x9851de3f
      runtime (s): 27.2367
      sync-runtime (s): 27.1905
      package-energy (J): 4478.41
      dram-energy (J): 767.496
      power (W): 164.705
      frequency (%): 50.3711
      frequency (Hz): 1.05779e+09
      time-hint-network (s): 0
      time-hint-ignore (s): 0
      time-hint-compute (s): 0
      time-hint-memory (s): 0
      time-hint-io (s): 0
      time-hint-serial (s): 0
      time-hint-parallel (s): 0
      time-hint-unknown (s): 0
      time-hint-unset (s): 0
      count: 500
      requested-online-frequency: 1000000000.000000
    -
      region: "stream-0.30-dgemm-2.40"
      hash: 0xce08ae24
      runtime (s): 25.8974
      sync-runtime (s): 25.8082
      package-energy (J): 4672.56
      dram-energy (J): 636.411
      power (W): 181.05
      frequency (%): 61.493
      frequency (Hz): 1.29135e+09
      time-hint-network (s): 0
      time-hint-ignore (s): 0
      time-hint-compute (s): 0
      time-hint-memory (s): 0
      time-hint-io (s): 0
      time-hint-serial (s): 0
      time-hint-parallel (s): 0
      time-hint-unknown (s): 0
      time-hint-unset (s): 0
      count: 500
      requested-online-frequency: 1300000000.000000
    -
      region: "stream-0.50-dgemm-0.00"
      hash: 0x3a6c47e3
      runtime (s): 25.2686
      sync-runtime (s): 25.2523
      package-energy (J): 4412.83
      dram-energy (J): 776.741
      power (W): 174.75
      frequency (%): 50.9256
      frequency (Hz): 1.06944e+09
      time-hint-network (s): 0
      time-hint-ignore (s): 0
      time-hint-compute (s): 0
      time-hint-memory (s): 0
      time-hint-io (s): 0
      time-hint-serial (s): 0
      time-hint-parallel (s): 0
      time-hint-unknown (s): 0
      time-hint-unset (s): 0
      count: 500
      requested-online-frequency: 1000000000.000000
    -
      region: "stream-0.25-dgemm-3.00"
      hash: 0x3c627f60
      runtime (s): 24.5979
      sync-runtime (s): 24.5117
      package-energy (J): 4732.35
      dram-energy (J): 583.029
      power (W): 193.065
      frequency (%): 65.2297
      frequency (Hz): 1.36982e+09
      time-hint-network (s): 0
      time-hint-ignore (s): 0
      time-hint-compute (s): 0
      time-hint-memory (s): 0
      time-hint-io (s): 0
      time-hint-serial (s): 0
      time-hint-parallel (s): 0
      time-hint-unknown (s): 0
      time-hint-unset (s): 0
      count: 500
      requested-online-frequency: 1400000000.000000
    -
      region: "stream-0.20-dgemm-3.60"
      hash: 0xf9d11bbd
      runtime (s): 23.2737
      sync-runtime (s): 23.2129
      package-energy (J): 4694.3
      dram-energy (J): 527.13
      power (W): 202.228
      frequency (%): 68.9091
      frequency (Hz): 1.44709e+09
      time-hint-network (s): 0
      time-hint-ignore (s): 0
      time-hint-compute (s): 0
      time-hint-memory (s): 0
      time-hint-io (s): 0
      time-hint-serial (s): 0
      time-hint-parallel (s): 0
      time-hint-unknown (s): 0
      time-hint-unset (s): 0
      count: 500
      requested-online-frequency: 1500000000.000000
    -
      region: "stream-0.15-dgemm-4.20"
      hash: 0xaddfa74f
      runtime (s): 22.3063
      sync-runtime (s): 22.1766
      package-energy (J): 4650.85
      dram-energy (J): 466.899
      power (W): 209.719
      frequency (%): 68.7558
      frequency (Hz): 1.44387e+09
      time-hint-network (s): 0
      time-hint-ignore (s): 0
      time-hint-compute (s): 0
      time-hint-memory (s): 0
      time-hint-io (s): 0
      time-hint-serial (s): 0
      time-hint-parallel (s): 0
      time-hint-unknown (s): 0
      time-hint-unset (s): 0
      count: 500
      requested-online-frequency: 1500000000.000000
    -
      region: "stream-0.10-dgemm-4.80"
      hash: 0xe50a9187
      runtime (s): 20.3869
      sync-runtime (s): 20.3101
      package-energy (J): 4646.27
      dram-energy (J): 410.599
      power (W): 228.767
      frequency (%): 72.2666
      frequency (Hz): 1.5176e+09
      time-hint-network (s): 0
      time-hint-ignore (s): 0
      time-hint-compute (s): 0
      time-hint-memory (s): 0
      time-hint-io (s): 0
      time-hint-serial (s): 0
      time-hint-parallel (s): 0
      time-hint-unknown (s): 0
      time-hint-unset (s): 0
      count: 500
      requested-online-frequency: 1600000000.000000
    -
      region: "stream-0.05-dgemm-5.40"
      hash: 0xe1242325
      runtime (s): 19.2864
      sync-runtime (s): 19.1575
      package-energy (J): 4618.09
      dram-energy (J): 352.889
      power (W): 241.059
      frequency (%): 76.1363
      frequency (Hz): 1.59886e+09
      time-hint-network (s): 0
      time-hint-ignore (s): 0
      time-hint-compute (s): 0
      time-hint-memory (s): 0
      time-hint-io (s): 0
      time-hint-serial (s): 0
      time-hint-parallel (s): 0
      time-hint-unknown (s): 0
      time-hint-unset (s): 0
      count: 500
      requested-online-frequency: 1700000000.000000
    -
      region: "stream-0.00-dgemm-6.00"
      hash: 0xaf4cafa3
      runtime (s): 18.6718
      sync-runtime (s): 18.6886
      package-energy (J): 4600.85
      dram-energy (J): 321.137
      power (W): 246.185
      frequency (%): 75.8795
      frequency (Hz): 1.59347e+09
      time-hint-network (s): 0
      time-hint-ignore (s): 0
      time-hint-compute (s): 0
      time-hint-memory (s): 0
      time-hint-io (s): 0
      time-hint-serial (s): 0
      time-hint-parallel (s): 0
      time-hint-unknown (s): 0
      time-hint-unset (s): 0
      count: 500
      requested-online-frequency: 1700000000.000000
    -
      region: "with some spaces"
      hash: 0xaf000fa3
      runtime (s): 17.8807
      sync-runtime (s): 17.8327
      package-energy (J): 4367.48
      dram-energy (J): 252.722
      power (W): 244.914
      frequency (%): 75.7953
      frequency (Hz): 1.5917e+09
      time-hint-network (s): 0
      time-hint-ignore (s): 0
      time-hint-compute (s): 0
      time-hint-memory (s): 0
      time-hint-io (s): 0
      time-hint-serial (s): 0
      time-hint-parallel (s): 0
      time-hint-unknown (s): 0
      time-hint-unset (s): 0
      count: 500
      requested-online-frequency: 1700000000.000000
    Unmarked Totals:
      runtime (s): 1.15034
      sync-runtime (s): 1.11497
      package-energy (J): 226.452
      dram-energy (J): 28.9149
      power (W): 203.102
      frequency (%): 95.0914
      frequency (Hz): 1.99692e+09
      time-hint-network (s): 12
      time-hint-ignore (s): 0
      time-hint-compute (s): 0
      time-hint-memory (s): 0
      time-hint-io (s): 0
      time-hint-serial (s): 0
      time-hint-parallel (s): 0
      time-hint-unknown (s): 0
      time-hint-unset (s): 0
      count: 0
      requested-online-frequency: 2200000000.000000
    Epoch Totals:
      runtime (s): 0
      sync-runtime (s): 0
      package-energy (J): 0
      dram-energy (J): 0
      power (W): 0
      frequency (%): 0
      frequency (Hz): 0
      time-hint-network (s): 14
      time-hint-compute (s): 0
      time-hint-memory (s): 0
      time-hint-io (s): 0
      time-hint-serial (s): 0
      time-hint-parallel (s): 0
      time-hint-unknown (s): 0
      time-hint-unset (s): 0
      count: 0
      time-hint-ignore (s): 0
    Application Totals:
      runtime (s): 541.505
      sync-runtime (s): 541.505
      package-energy (J): 85946.8
      dram-energy (J): 10142.6
      power (W): 158.718
      frequency (%): 0
      frequency (Hz): 0
      time-hint-network (s): 15
      time-hint-compute (s): 0
      time-hint-memory (s): 0
      time-hint-io (s): 0
      time-hint-serial (s): 0
      time-hint-parallel (s): 0
      time-hint-unknown (s): 0
      time-hint-unset (s): 0
      time-hint-ignore (s): 0
      geopmctl memory HWM (B): 145715200
      geopmctl network BW (B/sec): 0
"""

# First lines from a test_trace_runtimes integration test run
test_trace_data = """# geopm_version: 1.0.0+dev209g77e1ebb8
# start_time: Thu Oct 03 08:19:34 2019
# profile_name: test_trace_runtimes
# node_name: mcfly1
# agent: power_governor
TIME|EPOCH_COUNT|REGION_HASH|REGION_HINT|REGION_PROGRESS|REGION_COUNT|REGION_RUNTIME|CPU_ENERGY|DRAM_ENERGY|CPU_POWER|DRAM_POWER|FREQUENCY|CPU_CYCLES_THREAD|CPU_CYCLES_REFERENCE|CPU_CORE_TEMPERATURE|POWER_BUDGET
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

        hosts = report.host_names()
        region_names = report.region_names(hosts[0])
        region = report.raw_region(hosts[1], region_names[1])
        region_runtime = region['runtime (s)']
        epoch = report.raw_epoch(hosts[0])
        runtime = epoch['runtime (s)']
        self.assertEqual(0, runtime)
        totals = report.raw_totals(hosts[1])
        total_runtime = totals['runtime (s)']
        self.assertLess(region_runtime, total_runtime)
        field_runtime = report.get_field(epoch, 'runtime')
        self.assertEqual(runtime, field_runtime)
        region = report.raw_region('mcfly11', 'sleep')
        self.assertTrue(type(region['frequency (Hz)']) is float)
        self.assertEqual(1e9, region['frequency (Hz)'])

    def test_report_cache(self):
        """ Test that a report is not read when it is cached.
        """
        spy_open = mock.Mock(wraps=open)

        def count_open(path):
            """ Count the number of times spy_open() has been called with path
            """
            return Counter([c[0][0] for c in spy_open.call_args_list])[path]

        with mock.patch('geopmpy.io.open', spy_open), self_cleaning_raw_report_collection(self._report_path):
            initial_call_count = count_open(self._report_path)

            geopmpy.io.RawReportCollection(self._report_path)
            self.assertEqual(initial_call_count, count_open(self._report_path))

            touch_file(self._report_path)
            geopmpy.io.RawReportCollection(self._report_path)
            self.assertEqual(initial_call_count * 2, count_open(self._report_path))

    def test_trace(self):
        """ Test that a trace file can be extracted to a dataframe.
        """
        app_output = geopmpy.io.AppOutput(traces=self._trace_path, do_cache=False)
        trace_df = app_output.get_trace_data(node_name='mcfly1')
        start_time = trace_df.index.get_level_values('start_time').unique()
        self.assertEqual(1, len(start_time))
        self.assertEqual('Thu Oct 03 08:19:34 2019', start_time[0])
        self.assertAlmostEqual(0.204296343, trace_df.iloc[0]['TIME'])
        self.assertAlmostEqual(242598.262512207, trace_df.iloc[0]['CPU_ENERGY'])
        self.assertAlmostEqual(0.268616921, trace_df.iloc[-1]['TIME'])
        self.assertAlmostEqual(242610.5656738281, trace_df.iloc[-1]['CPU_ENERGY'])

    def test_figure_of_merit(self):
        fom_report_path = os.path.join(os.path.dirname(__file__), 'test_io_experiment.report')

        # test that RawReport can find FOM
        rr = geopmpy.io.RawReport(path=fom_report_path)
        self.assertAlmostEqual(327780.0, rr.figure_of_merit())
        rrc = geopmpy.io.RawReportCollection(fom_report_path, do_cache=False)
        app_data = rrc.get_app_df()
        self.assertAlmostEqual(327780.0, app_data['FOM'].mean())

        # test that RawReport works when FOM is missing
        report = geopmpy.io.RawReport(self._report_path)
        self.assertTrue(report.figure_of_merit() is None)
        rrc = geopmpy.io.RawReportCollection(self._report_path, do_cache=False)
        self.assertFalse('FOM' in rrc.get_app_df().columns)

    def test_h5_name(self):
        # check that a set of reports produces the same hdf5 cache name
        # from any output dir relative path

        cwd = os.getcwd()
        basenames = ['file_a', 'file_b', 'file_c']
        for name in basenames:
            with open(name, 'w'):
                # touch file
                pass
        rel_set = [os.path.join('.', ff) for ff in basenames]
        abs_set = [os.path.join(cwd, ff) for ff in basenames]

        outdir = cwd
        rel_h5 = geopmpy.io.RawReportCollection.make_h5_name(rel_set, outdir)
        abs_h5 = geopmpy.io.RawReportCollection.make_h5_name(abs_set, outdir)
        self.assertEqual(rel_h5, abs_h5)

        for ff in basenames:
            os.unlink(ff)

    def test_raw_report_collection_region(self):
        rrc = geopmpy.io.RawReportCollection(self._report_path, do_cache=False)
        df = rrc.get_df()
        sleep_region_mcfly11 = {'GEOPM Version': '1.0.0+dev30g4cccfda',
                                'Start Time': 'Thu May 30 14:38:17 2019',
                                'Profile': 'test_fm_stream_dgemm_mix',
                                'Agent': 'frequency_map',
                                'host': 'mcfly11',
                                'region': 'sleep',
                                'hash': 0x536c798f,
                                'runtime (s)': 275.998,
                                'sync-runtime (s)': 276.581,
                                'package-energy (J)': 33573.9,
                                'dram-energy (J)': 2835.43,
                                'power (W)': 121.389,
                                'frequency (%)': 50.0846,
                                'frequency (Hz)': 1e+09,
                                'time-hint-network (s)': 0,
                                'time-hint-ignore (s)': 0,
                                'time-hint-compute (s)': 0,
                                'time-hint-memory (s)': 0,
                                'time-hint-io (s)': 0,
                                'time-hint-serial (s)': 0,
                                'time-hint-parallel (s)': 0,
                                'time-hint-unknown (s)': 0,
                                'time-hint-unset (s)': 0,
                                'count': 5500,
                                'requested-online-frequency': 1000000000.000000}
        self.assertEqual(set(sleep_region_mcfly11.keys()), set(df.columns))
        actual_sleep_region = df.loc[(df['host'] == 'mcfly11') &
                                     (df['region'] == 'sleep')].to_dict('records')[0]
        self.assertEqual(sleep_region_mcfly11, actual_sleep_region)

    def test_raw_report_collection_app_totals(self):
        rrc = geopmpy.io.RawReportCollection(self._report_path, do_cache=False)
        df = rrc.get_app_df()
        app_totals_mcfly12 = {'GEOPM Version': '1.0.0+dev30g4cccfda',
                              'Start Time': 'Thu May 30 14:38:17 2019',
                              'Profile': 'test_fm_stream_dgemm_mix',
                              'Agent': 'frequency_map',
                              'host': 'mcfly12',
                              'runtime (s)': 541.505,
                              'sync-runtime (s)': 541.505,
                              'package-energy (J)': 85946.8,
                              'dram-energy (J)': 10142.6,
                              'power (W)': 158.718,
                              'frequency (%)': 0,
                              'frequency (Hz)': 0,
                              'time-hint-network (s)': 15,
                              'time-hint-compute (s)': 0,
                              'time-hint-memory (s)': 0,
                              'time-hint-io (s)': 0,
                              'time-hint-serial (s)': 0,
                              'time-hint-parallel (s)': 0,
                              'time-hint-unknown (s)': 0,
                              'time-hint-unset (s)': 0,
                              'time-hint-ignore (s)': 0,
                              'geopmctl memory HWM (B)': 145715200,
                              'geopmctl network BW (B/sec)': 0}
        self.assertEqual(set(app_totals_mcfly12.keys()), set(df.columns))
        actual_totals= df.loc[(df['host'] == 'mcfly12')].to_dict('records')[0]
        self.assertEqual(app_totals_mcfly12, actual_totals)

    def test_raw_report_collection_epoch_totals(self):
        rrc = geopmpy.io.RawReportCollection(self._report_path, do_cache=False)
        df = rrc.get_epoch_df()
        epoch_totals_mcfly12 = {'GEOPM Version': '1.0.0+dev30g4cccfda',
                                'Start Time': 'Thu May 30 14:38:17 2019',
                                'Profile': 'test_fm_stream_dgemm_mix',
                                'Agent': 'frequency_map',
                                'host': 'mcfly12',
                                'runtime (s)': 0,
                                'sync-runtime (s)': 0,
                                'package-energy (J)': 0,
                                'dram-energy (J)': 0,
                                'power (W)': 0,
                                'frequency (%)': 0,
                                'frequency (Hz)': 0,
                                'time-hint-network (s)': 14,
                                'time-hint-compute (s)': 0,
                                'time-hint-memory (s)': 0,
                                'time-hint-io (s)': 0,
                                'time-hint-serial (s)': 0,
                                'time-hint-parallel (s)': 0,
                                'time-hint-unknown (s)': 0,
                                'time-hint-unset (s)': 0,
                                'count': 0,
                                'time-hint-ignore (s)': 0}
        self.assertEqual(set(epoch_totals_mcfly12.keys()), set(df.columns))
        actual_totals= df.loc[(df['host'] == 'mcfly12')].to_dict('records')[0]
        self.assertEqual(epoch_totals_mcfly12, actual_totals)

    def test_raw_report_collection_unmarked_totals(self):
        rrc = geopmpy.io.RawReportCollection(self._report_path, do_cache=False)
        df = rrc.get_unmarked_df()
        unmarked_mcfly11 = {'GEOPM Version': '1.0.0+dev30g4cccfda',
                            'Start Time': 'Thu May 30 14:38:17 2019',
                            'Profile': 'test_fm_stream_dgemm_mix',
                            'Agent': 'frequency_map',
                            'host': 'mcfly11',
                            'runtime (s)': 6.07865,
                            'sync-runtime (s)': 6.13216,
                            'package-energy (J)': 1043.75,
                            'dram-energy (J)': 72.5349,
                            'power (W)': 170.209,
                            'frequency (%)': 97.2362,
                            'frequency (Hz)': 2.04196e+09,
                            'time-hint-network (s)': 12,
                            'time-hint-ignore (s)': 0,
                            'time-hint-compute (s)': 0,
                            'time-hint-memory (s)': 0,
                            'time-hint-io (s)': 0,
                            'time-hint-serial (s)': 0,
                            'time-hint-parallel (s)': 0,
                            'time-hint-unknown (s)': 0,
                            'time-hint-unset (s)': 0,
                            'count': 0,
                            'requested-online-frequency': 2100000000.000000}
        self.assertEqual(set(unmarked_mcfly11.keys()), set(df.columns))
        actual = df.loc[(df['host'] == 'mcfly11')].to_dict('records')[0]
        self.assertEqual(unmarked_mcfly11, actual)


if __name__ == '__main__':
    unittest.main()
