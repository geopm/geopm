#!/usr/bin/env python3
#
#  Copyright (c) 2015 - 2025 Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#


import unittest
import os
from unittest import mock
import shlex
try:
    # Test with str StringIO where available, and with unicode StringIO elsewhere
    from StringIO import StringIO
except ImportError:
    from io import StringIO

import geopmpy.launcher

LSCPU_OUTPUT = b'''Architecture:          x86_64
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
LSCPU_STDERR=b'unittest lscpu stderr\n'

SRUN_HELP = b'''Usage: srun [OPTIONS...] executable [args...]

Affinity/Multi-core options: (when the task/affinity plugin is enabled)
      --cpu-bind=             Bind tasks to CPUs
                              (see "--cpu-bind=help" for options)
'''
SRUN_STDERR=b'unittest srun stderr\n'

UNITTEST_WORKLOAD_STDOUT = b'Testing, testing, 123\n'
UNITTEST_WORKLOAD_STDERR = b'Can you hear this? Do, re, mi\n'

def mock_popen_srun(*args, **kwargs):
    arg_list = shlex.split(args[0]) if kwargs.get('shell', False) else args[0]
    if arg_list[0] == 'srun':
        mock_pid = mock.Mock(returncode=0)
        if '--help' in arg_list:
            mock_pid.communicate.return_value = (SRUN_HELP, SRUN_STDERR)
        elif 'lscpu' in arg_list:
            mock_pid.communicate.return_value = (LSCPU_OUTPUT, LSCPU_STDERR)
        elif 'unittest_workload' in arg_list:
            mock_pid.communicate.return_value = (UNITTEST_WORKLOAD_STDOUT, UNITTEST_WORKLOAD_STDERR)
        else:
            arg_string = ' '.join(arg_list)
            stdout_text = 'stdout for {}\n'.format(arg_string)
            stderr_text = 'stderr for {}\n'.format(arg_string)
            mock_pid.communicate.return_value = (stdout_text.encode(), stderr_text.encode())
        return mock_pid
    else:
        raise NotImplementedError('Popen has not been mocked for {}'.format(' '.join(arg_list)))

class TestLauncher(unittest.TestCase):
    @mock.patch('subprocess.Popen', side_effect=mock_popen_srun)
    @mock.patch('geopmpy.launcher._get_cpuset', return_value=list(range(88)))
    def test_process_count(self, mock__get_cpuset, mock_popen):
        """ Test that geopm requests an additional rank for itself by default.
        """
        launcher = geopmpy.launcher.Factory().create(
                ['unittest_geopm_launcher', 'srun', '--geopm-ctl=process', '--geopm-program-filter', 'unittest_workload', 'unittest_workload'],
                num_rank = 2, num_node = 1)
        launcher.run()
        srun_args, srun_kwargs = mock_popen.call_args

        if srun_kwargs.get('shell', False):
            srun_args = shlex.split(srun_args[0])
        else:
            srun_args = srun_args[0]

        self.assertIn('-N', srun_args)
        self.assertEqual('1', srun_args[srun_args.index('-N') + 1])

        # Expect an additional rank per node for the geopm process
        self.assertIn('-n', srun_args)
        self.assertEqual('3', srun_args[srun_args.index('-n') + 1])

    @mock.patch('subprocess.Popen', side_effect=mock_popen_srun)
    @mock.patch('geopmpy.launcher._get_cpuset', return_value=list(range(88)))
    def test_non_file_output(self, mock__get_cpuset, mock_popen):
        """ Test that the launcher can redirect stdout and stderr to a non-file writable object.
        """
        out_stream = StringIO()
        error_stream = StringIO()

        launcher = geopmpy.launcher.Factory().create(
                ['unittest_geopm_launcher', 'srun', '--geopm-ctl=process', '--geopm-program-filter', 'unittest_workload', 'unittest_workload'],
                num_rank = 2, num_node = 1)
        launcher.run(stdout=out_stream, stderr=error_stream)

        self.assertIn(UNITTEST_WORKLOAD_STDOUT.decode(), out_stream.getvalue())
        self.assertIn(UNITTEST_WORKLOAD_STDERR.decode(), error_stream.getvalue())

    @mock.patch('subprocess.Popen', side_effect=mock_popen_srun)
    @mock.patch('geopmpy.launcher._get_cpuset', return_value=list(range(88)))
    def test_main(self, mock__get_cpuset, mock_popen):
        """ Test that the geopm CLI correctly feeds ntasks and nodes to the launcher.
        """
        with mock.patch('sys.argv', ['unittest_geopm_launcher', 'srun', '--geopm-ctl=process', '--geopm-program-filter', 'unittest_workload',
                                     'unittest_workload', '--ntasks', '4', '--nodes', '2']):
            geopmpy.launcher.main()

        srun_args, srun_kwargs = mock_popen.call_args

        if srun_kwargs.get('shell', False):
            srun_args = shlex.split(srun_args[0])
        else:
            srun_args = srun_args[0]

        self.assertIn('-N', srun_args)
        self.assertEqual('2', srun_args[srun_args.index('-N') + 1])

        # Expect an additional rank per node for the geopm process
        self.assertIn('-n', srun_args)
        self.assertEqual('6', srun_args[srun_args.index('-n') + 1])

    @mock.patch('subprocess.Popen', side_effect=mock_popen_srun)
    @mock.patch('geopmpy.launcher._get_cpuset', return_value=list(range(88)))
    def test_quoted_args(self, mock__get_cpuset, mock_popen):
        """ Test that the geopm CLI correctly passes through quoted args.
        """
        workload_command = "unittest_workload 'multiple words'"
        with mock.patch('sys.argv', shlex.split(
            f'unittest_geopm_launcher srun --geopm-ctl=process --ntasks 4 --nodes 2 --geopm-program-filter unittest_workload -- {workload_command}')):
            geopmpy.launcher.main()

        # [0][0] gets the first positional arg to Popen(), which is a string
        # containing the entire srun command
        srun_popen_args = mock_popen.call_args[0][0]
        self.assertEqual(srun_popen_args[-3:], ['--', 'unittest_workload', 'multiple words'])

    @mock.patch('subprocess.Popen', side_effect=mock_popen_srun)
    @mock.patch('geopmpy.launcher._get_cpuset', return_value=list(range(88)))
    def test_affinity_disable(self, mock__get_cpuset, mock_popen):
        """ Test that geopm does not emit affinity related env options when affinity
            is disabled.
        """
        launcher = geopmpy.launcher.Factory().create(
                ['unittest_geopm_launcher', 'srun', '--geopm-ctl=process', 'unittest_workload', '--geopm-program-filter', 'unittest_workload'],
                num_rank = 2, num_node = 1)
        launcher.run()
        srun_args, srun_kwargs = mock_popen.call_args

        self.assertNotIn('--cpu-bind', srun_args[0])
        self.assertNotIn('OMP_PROC_BIND', srun_kwargs['env'].keys())

    @mock.patch('subprocess.Popen', side_effect=mock_popen_srun)
    @mock.patch('geopmpy.launcher._get_cpuset', return_value=list(range(88)))
    def test_affinity_enable(self, mock__get_cpuset, mock_popen):
        """ Test that geopm does emit affinity related env options when affinity
            is enabled.
        """
        launcher = geopmpy.launcher.Factory().create(
                ['unittest_geopm_launcher', 'srun', '--geopm-ctl=process', '--geopm-affinity-enable', '--geopm-program-filter', 'unittest_workload', 'unittest_workload'],
                num_rank = 2, num_node = 1)
        launcher.run()
        srun_args, srun_kwargs = mock_popen.call_args

        self.assertIn('--cpu-bind', srun_args[0])
        self.assertIn('OMP_PROC_BIND', srun_kwargs['env'].keys())

    @mock.patch('subprocess.Popen', side_effect=mock_popen_srun)
    @mock.patch('geopmpy.launcher._get_cpuset', return_value=list(range(2, 88)))
    def test_affinity_enable_cgroup(self, mock__get_cpuset, mock_popen):
        """ Test that geopm emits affinity options respecting cgroup constraints (CPUs 0 and 1 excluded). """
        launcher = geopmpy.launcher.Factory().create(
                ['unittest_geopm_launcher', 'srun', '--geopm-ctl=process', '--geopm-affinity-enable', '--geopm-program-filter', 'unittest_workload', 'unittest_workload'],
                num_rank = 2, num_node = 1)
        launcher.run()
        srun_args, srun_kwargs = mock_popen.call_args

        self.assertIn('--cpu-bind', srun_args[0])
        self.assertIn('OMP_PROC_BIND', srun_kwargs['env'].keys())

        cpu_bind_masks = srun_args[0][6].split(':')[1].split(',')
        self.assertGreater(len(cpu_bind_masks), 0, "No CPU masks found in the '--cpu-bind' argument.")

        disallowed_cpus = 0x3  # Binary: 0011, meaning CPUs 0 and 1 are excluded
        for mask in cpu_bind_masks:
            # Convert the mask from hex to binary and check if it excludes CPUs 0 and 1
            mask_value = int(mask, 16)
            self.assertEqual(mask_value & disallowed_cpus, 0, f"CPU mask {mask} does not exclude CPUs 0 and 1 as expected.")

if __name__ == '__main__':
    unittest.main()
