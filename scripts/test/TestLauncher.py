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
import mock
import subprocess
import shlex
import StringIO

import geopmpy.launcher

LSCPU_OUTPUT = '''Architecture:          x86_64
CPU op-mode(s):        32-bit, 64-bit
Byte Order:            Little Endian
CPU(s):                88
On-line CPU(s) list:   0-87
Thread(s) per core:    2
Core(s) per socket:    22
Socket(s):             2
NUMA node(s):          2
Vendor ID:             GenuineIntel
CPU family:            6
Model:                 85
Model name:            Intel(R) Xeon(R) Gold 6152 CPU @ 2.10GHz
Stepping:              4
CPU MHz:               2101.000
CPU max MHz:           2101.0000
CPU min MHz:           1000.0000
BogoMIPS:              4200.00
Virtualization:        VT-x
L1d cache:             32K
L1i cache:             32K
L2 cache:              1024K
L3 cache:              30976K
NUMA node0 CPU(s):     0-21,44-65
NUMA node1 CPU(s):     22-43,66-87
Flags:                 fpu vme de pse tsc msr pae mce cx8 apic sep mtrr pge mca cmov pat pse36 clflush dts acpi mmx fxsr sse sse2 ss ht tm pbe syscall nx pdpe1gb rdtscp lm constant_tsc art arch_perfmon pebs bts rep_good nopl xtopology nonstop_tsc aperfmperf eagerfpu pni pclmulqdq dtes64 monitor ds_cpl vmx smx est tm2 ssse3 sdbg fma cx16 xtpr pdcm pcid dca sse4_1 sse4_2 x2apic movbe popcnt tsc_deadline_timer aes xsave avx f16c rdrand lahf_lm abm 3dnowprefetch epb cat_l3 cdp_l3 intel_pt ssbd mba ibrs ibpb stibp tpr_shadow vnmi flexpriority ept vpid fsgsbase tsc_adjust bmi1 hle avx2 smep bmi2 erms invpcid rtm cqm mpx rdt_a avx512f avx512dq rdseed adx smap clflushopt clwb avx512cd avx512bw avx512vl xsaveopt xsavec xgetbv1 cqm_llc cqm_occup_llc cqm_mbm_total cqm_mbm_local dtherm ida arat pln pts hwp hwp_act_window hwp_epp hwp_pkg_req pku ospke spec_ctrl intel_stibp flush_l1d
'''

SRUN_HELP = '''Usage: srun [OPTIONS...] executable [args...]

Affinity/Multi-core options: (when the task/affinity plugin is enabled)
      --cpu-bind=             Bind tasks to CPUs
                              (see "--cpu-bind=help" for options)
'''

UNITTEST_WORKLOAD_STDOUT = 'Testing, testing, 123'
UNITTEST_WORKLOAD_STDERR = 'Can you hear this? Do, re, mi'

def mock_popen_srun(*args, **kwargs):
    arg_list = shlex.split(args[0]) if kwargs.get('shell', False) else args[0]
    if arg_list[0] == 'srun':
        mock_pid = mock.Mock(returncode=0)
        if '--help' in arg_list:
            mock_pid.communicate.return_value = (SRUN_HELP, 'unittest srun stderr')
        elif 'lscpu' in arg_list:
            mock_pid.communicate.return_value = (LSCPU_OUTPUT, 'unittest lscpu stderr')
        elif 'unittest_workload' in arg_list:
            mock_pid.communicate.return_value = (UNITTEST_WORKLOAD_STDOUT, UNITTEST_WORKLOAD_STDERR)
        else:
            arg_string = ' '.join(arg_list)
            mock_pid.communicate.return_value = ('stdout for {}'.format(arg_string),
                                                 'stderr for {}'.format(arg_string))
        return mock_pid
    else:
        raise NotImplementedError('Popen has not been mocked for {}'.format(' '.join(arg_list)))

class TestLauncher(unittest.TestCase):
    @unittest.skip("msrsave/restore requires changes here")
    @mock.patch('subprocess.Popen', side_effect=mock_popen_srun)
    def test_process_count(self, mock_popen):
        """ Test that geopm requests an additional rank for itself by default.
        """
        launcher = geopmpy.launcher.Factory().create(
                ['unittest_geopm_launcher', 'srun', 'unittest_workload'],
                num_rank = 2, num_node = 1)
        launcher.run()
        srun_args, srun_kwargs = mock_popen.call_args

        if srun_kwargs.get('shell', False):
            srun_args = shlex.split(srun_args[0])

        self.assertIn('-N', srun_args)
        self.assertEqual('1', srun_args[srun_args.index('-N') + 1])

        # Expect an additional rank per node for the geopm process
        self.assertIn('-n', srun_args)
        self.assertEqual('3', srun_args[srun_args.index('-n') + 1])

    @mock.patch('subprocess.Popen', side_effect=mock_popen_srun)
    def test_non_file_output(self, mock_popen):
        """ Test that the launcher can redirect stdout and stderr to a non-file writable object.
        """
        out_stream = StringIO.StringIO()
        error_stream = StringIO.StringIO()

        launcher = geopmpy.launcher.Factory().create(
                ['unittest_geopm_launcher', 'srun', 'unittest_workload'],
                num_rank = 2, num_node = 1)
        launcher.run(stdout=out_stream, stderr=error_stream)

        self.assertIn(UNITTEST_WORKLOAD_STDOUT, out_stream.getvalue())
        self.assertIn(UNITTEST_WORKLOAD_STDERR, error_stream.getvalue())

    @unittest.skip("msrsave/restore requires changes here")
    @mock.patch('subprocess.Popen', side_effect=mock_popen_srun)
    def test_main(self, mock_popen):
        """ Test that the geopm CLI correctly feeds ntasks and nodes to the launcher.
        """
        with mock.patch('sys.argv', ['unittest_geopm_launcher', 'srun',
                                     'unittest_workload', '--ntasks', '4', '--nodes', '2']):
            geopmpy.launcher.main()

        srun_args, srun_kwargs = mock_popen.call_args

        if srun_kwargs.get('shell', False):
            srun_args = shlex.split(srun_args[0])

        self.assertIn('-N', srun_args)
        self.assertEqual('2', srun_args[srun_args.index('-N') + 1])

        # Expect an additional rank per node for the geopm process
        self.assertIn('-n', srun_args)
        self.assertEqual('6', srun_args[srun_args.index('-n') + 1])

if __name__ == '__main__':
    unittest.main()
