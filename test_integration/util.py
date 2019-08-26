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
import os
import unittest
import subprocess

from . import geopm_test_launcher

def skip_unless_platform_bdx():
    fam, mod = geopm_test_launcher.get_platform()
    if fam != 6 or mod not in (45, 47, 79):
        return unittest.skip("Performance test is tuned for BDX server, The family {}, model {} is not supported.".format(fam, mod))
    return lambda func: func


def skip_unless_config_enable(feature):
    path = os.path.join(
           os.path.dirname(
            os.path.dirname(
             os.path.realpath(__file__))),
           'config.log')
    with open(path) as fid:
        for line in fid.readlines():
            if line.startswith("enable_{}='0'".format(feature)):
                return unittest.skip("Feature: {feature} is not enabled, configure with --enable-{feature} to run this test.".format(feature=feature))
            elif line.startswith("enable_{}='1'".format(feature)):
                break
    return lambda func: func


def skip_unless_optimized():
    path = os.path.join(
           os.path.dirname(
            os.path.dirname(
             os.path.realpath(__file__))),
           'config.log')
    with open(path) as fid:
        for line in fid.readlines():
            if line.startswith("enable_debug='1'"):
                return unittest.skip("This performance test cannot be run when GEOPM is configured with --enable-debug")
    return lambda func: func


def skip_unless_slurm_batch():
    if 'SLURM_NODELIST' not in os.environ:
        return unittest.skip('Requires SLURM batch session.')
    return lambda func: func


def skip_unless_run_long_tests():
    if 'GEOPM_RUN_LONG_TESTS' not in os.environ:
        return unittest.skip("Define GEOPM_RUN_LONG_TESTS in your environment to run this test.")
    return lambda func: func


def skip_unless_cpufreq():
    try:
        test_exec = "dummy -- stat /sys/devices/system/cpu/cpu0/cpufreq/cpuinfo_min_freq \
                     && stat /sys/devices/system/cpu/cpu0/cpufreq/cpuinfo_max_freq"
        dev_null = open('/dev/null', 'w')
        geopm_test_launcher.allocation_node_test(test_exec, dev_null, dev_null)
        dev_null.close()
    except subprocess.CalledProcessError:
        return unittest.skip("Could not determine min and max frequency, enable cpufreq driver to run this test.")
    return lambda func: func
