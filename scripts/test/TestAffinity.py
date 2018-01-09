#!/usr/bin/env python
#
#  Copyright (c) 2015, 2016, 2017, Intel Corporation
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

import unittest
import geopm_context
import geopmpy.launcher

class TestAffinityLauncher(geopmpy.launcher.Launcher):
    def __init__(self, argv, num_rank, num_node, cpu_per_rank):
        super(TestAffinityLauncher, self).__init__(argv, num_rank, num_node, cpu_per_rank)

    def init_topo(self):
        raise NotImplementedError('UnitTestLauncher base class does not define init_topo()')

    def parse_mpiexec_argv(self):
        pass


class XeonAffinityLauncher(TestAffinityLauncher):
    def __init__(self, argv, num_rank, num_node, cpu_per_rank):
        super(XeonAffinityLauncher, self).__init__(argv, num_rank, num_node, cpu_per_rank)

    def init_topo(self):
        self.num_linux_cpu = 88
        self.thread_per_core = 2
        self.core_per_socket = 22
        self.num_socket = 2

class QuartzAffinityLauncher(TestAffinityLauncher):
    def __init__(self, argv, num_rank, num_node, cpu_per_rank):
        argv.append('--geopm-disable-hyperthreads')
        super(QuartzAffinityLauncher, self).__init__(argv, num_rank, num_node, cpu_per_rank)

    def init_topo(self):
        self.num_linux_cpu = 72
        self.thread_per_core = 2
        self.core_per_socket = 18
        self.num_socket = 2

class KNLAffinityLauncher(TestAffinityLauncher):
    def __init__(self, argv, num_rank, num_node, cpu_per_rank):
        super(KNLAffinityLauncher, self).__init__(argv, num_rank, num_node, cpu_per_rank)

    def init_topo(self):
        self.num_linux_cpu = 256
        self.thread_per_core = 4
        self.core_per_socket = 64
        self.num_socket = 1


class ToyAffinityLauncher(TestAffinityLauncher):
    def __init__(self, argv, num_rank, num_node, cpu_per_rank):
        super(ToyAffinityLauncher, self).__init__(argv, num_rank, num_node, cpu_per_rank)

    def init_topo(self):
        self.num_linux_cpu = 16
        self.thread_per_core = 2
        self.core_per_socket = 4
        self.num_socket = 2

class TestAffinity(unittest.TestCase):
    def setUp(self):
        self.maxDiff = 4096

    def test_affinity_0(self):
        launcher = XeonAffinityLauncher(['--geopm-ctl', 'process'], 1, 1, 1)
        actual = launcher.affinity_list(False)
        expect = [{1}, {43}]
        self.assertEqual(expect, actual)

    def test_affinity_1(self):
        launcher = XeonAffinityLauncher(['--geopm-ctl', 'process'], 2, 1, 4)
        actual = launcher.affinity_list(False)
        expect = [{1}, {21, 20, 19, 18}, {43, 42, 41, 40}]
        self.assertEqual(expect, actual)

    def test_affinity_2(self):
        launcher = XeonAffinityLauncher(['--geopm-ctl', 'pthread'], 2, 1, 4)
        actual = launcher.affinity_list(False)
        expect = [{1, 21, 20, 19, 18}, {43, 42, 41, 40}]
        self.assertEqual(expect, actual)

    def test_affinity_3(self):
        launcher = XeonAffinityLauncher(['--geopm-ctl', 'application'], 2, 1, 4)
        actual = launcher.affinity_list(False)
        expect = [{21, 20, 19, 18}, {43, 42, 41, 40}]
        self.assertEqual(expect, actual)
        actual = launcher.affinity_list(True)
        expect = [{1}]
        self.assertEqual(expect, actual)

    def test_affinity_4(self):
        launcher = XeonAffinityLauncher(['--geopm-ctl', 'process'], 44, 1, 1)
        actual = launcher.affinity_list(False)
        expect = [{44}]
        expect.extend([{ii} for ii in range(44)])
        self.assertEqual(expect, actual)

    def test_affinity_5(self):
        launcher = XeonAffinityLauncher(['--geopm-ctl', 'process'], 44, 1, 2)
        actual = launcher.affinity_list(False)
        expect = [{0}]
        expect.extend([{ii, ii + 44} for ii in range(44)])
        self.assertEqual(expect, actual)

    def test_affinity_6(self):
        launcher = ToyAffinityLauncher(['--geopm-ctl', 'process'], 8, 1, 2)
        actual = launcher.affinity_list(False)
        expect = [{0}]
        expect.extend([{ii, ii + 8} for ii in range(8)])
        self.assertEqual(expect, actual)

    def test_affinity_7(self):
        launcher = XeonAffinityLauncher(['--geopm-ctl', 'process'], 40, 1, 2)
        actual = launcher.affinity_list(False)
        expect = [{1}]
        expect.extend([{ii, ii + 44} for ii in range(2, 22)])
        expect.extend([{ii, ii + 44} for ii in range(24, 44)])
        self.assertEqual(expect, actual)

    def test_affinity_8(self):
        launcher = XeonAffinityLauncher(['--geopm-ctl', 'process'], 30, 1, 2)
        actual = launcher.affinity_list(False)
        expect = [{1}]
        expect.extend([{ii, ii + 44} for ii in range(7, 22)])
        expect.extend([{ii, ii + 44} for ii in range(29, 44)])
        self.assertEqual(expect, actual)

    def test_affinity_9(self):
        launcher = KNLAffinityLauncher(['--geopm-ctl', 'process'], 64, 1, 4)
        actual = launcher.affinity_list(False)
        expect = [{0}]
        expect.extend([{ii, ii + 64, ii + 128, ii + 192} for ii in range(64)])
        self.assertEqual(expect, actual)

    def test_affinity_10(self):
        launcher = KNLAffinityLauncher(['--geopm-ctl', 'process'], 64, 1, 3)
        actual = launcher.affinity_list(False)
        expect = [{192}]
        expect.extend([{ii, ii + 64, ii + 128} for ii in range(64)])
        self.assertEqual(expect, actual)

    def test_affinity_11(self):
        launcher = KNLAffinityLauncher(['--geopm-ctl', 'process'], 48, 1, 3)
        actual = launcher.affinity_list(False)
        expect = [{1}]
        expect.extend([{ii, ii + 64, ii + 128} for ii in range(16, 64)])
        self.assertEqual(expect, actual)

    def test_affinity_12(self):
        launcher = KNLAffinityLauncher(['--geopm-ctl', 'process'], 51, 1, 5)
        err_msg = 'Cores cannot be shared between MPI ranks'
        with self.assertRaisesRegexp(RuntimeError, err_msg) as cm:
            launcher.affinity_list(False)

    def test_affinity_13(self):
        """
        Here the app is trying to use num_sockets * num_cores - 1 OMP threads.  This should
        result in the controller getting pinned to core 0's HT, and the app pinned to cores 1-43.
        Core 0 should be left for the OS.
        """
        launcher = XeonAffinityLauncher(['--geopm-ctl', 'process'], 1, 1, 43)
        actual = launcher.affinity_list(False)
        expect = [{44}]
        expect.extend([set(range(1, 44))])
        self.assertEqual(expect, actual)

    def test_affinity_14(self):
        """
        The mpibind plugin used on TOSS based systems does not yet support pinning to HTs.  When an
        app requests num_sockets * num_cores - 1 OMP threads the controller should be pinned to core 0
        meaning it is shared with the OS.
        """
        launcher = QuartzAffinityLauncher(['--geopm-ctl', 'application'], 1, 1, 35)
        actual = launcher.affinity_list(False)
        expect = [set(range(1, 36))]
        self.assertEqual(expect, actual)
        actual = launcher.affinity_list(True)
        expect = [{0}]
        self.assertEqual(expect, actual)

    def test_affinity_15(self):
        """
        When the application requests all the physical cores we would normally pin the controller to core
        0's HT.  Since mpibind does not support HT pinning, oversubscribe core 0.
        """
        launcher = QuartzAffinityLauncher(['--geopm-ctl', 'application'], 1, 1, 36)
        actual = launcher.affinity_list(False)
        expect = [set(range(0, 36))]
        self.assertEqual(expect, actual)
        actual = launcher.affinity_list(True)
        expect = [{0}]
        self.assertEqual(expect, actual)

    def test_affinity_16(self):
        """
        Similar to test 14, this attempts to utilize 35 execution units (5 ranks * 7 OMP threads).
        Core 0 should be used for the OS and controller.
        """
        launcher = QuartzAffinityLauncher(['--geopm-ctl', 'application'], 5, 1, 7)
        actual = launcher.affinity_list(False)
        expect = [{1, 2, 3, 4, 5, 6, 7}, {8, 9, 10, 11, 12, 13, 14}, {15, 16, 17, 18, 19, 20, 21},
                  {22, 23, 24, 25, 26, 27, 28}, {29, 30, 31, 32, 33, 34, 35}]
        self.assertEqual(expect, actual)
        actual = launcher.affinity_list(True)
        expect = [{0}]
        self.assertEqual(expect, actual)

    def test_affinity_17(self):
        """
        Similar to test 14, this attempts to utilize 35 execution units (7 ranks * 5 OMP threads).
        Core 0 should be used for the OS and controller.
        """
        launcher = QuartzAffinityLauncher(['--geopm-ctl', 'application'], 7, 1, 5)
        actual = launcher.affinity_list(False)
        expect = [{1, 2, 3, 4, 5}, {6, 7, 8, 9, 10}, {11, 12, 13, 14, 15}, {16, 17, 18, 19, 20},
                  {21, 22, 23, 24, 25}, {26, 27, 28, 29, 30}, {31, 32, 33, 34, 35}]
        self.assertEqual(expect, actual)
        actual = launcher.affinity_list(True)
        expect = [{0}]
        self.assertEqual(expect, actual)

if __name__ == '__main__':
    unittest.main()
