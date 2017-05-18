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
import geopm_test_path
import geopm.launcher

class TestAffinityLauncher(geopm.launcher.Launcher):
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
        expect = [{1}, {2}]
        self.assertEqual(expect, actual)

    def test_affinity_1(self):
        launcher = XeonAffinityLauncher(['--geopm-ctl', 'process'], 2, 1, 4)
        actual = launcher.affinity_list(False)
        expect = [{1}, {2, 3, 4, 5}, {6, 7, 8, 9}]
        self.assertEqual(expect, actual)

    def test_affinity_2(self):
        launcher = XeonAffinityLauncher(['--geopm-ctl', 'pthread'], 2, 1, 4)
        actual = launcher.affinity_list(False)
        expect = [{1, 2, 3, 4, 5}, {6, 7, 8, 9}]
        self.assertEqual(expect, actual)

    def test_affinity_3(self):
        launcher = XeonAffinityLauncher(['--geopm-ctl', 'application'], 2, 1, 4)
        actual = launcher.affinity_list(False)
        expect = [{2, 3, 4, 5}, {6, 7, 8, 9}]
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
        expect.extend([{ii, ii + 44} for ii in range(2, 42)])
        self.assertEqual(expect, actual)

    def test_affinity_8(self):
        launcher = XeonAffinityLauncher(['--geopm-ctl', 'process'], 30, 1, 2)
        actual = launcher.affinity_list(False)
        expect = [{1}]
        expect.extend([{ii, ii + 44} for ii in range(2, 17)])
        expect.extend([{ii, ii + 44} for ii in range(22, 37)])
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
        expect.extend([{ii, ii + 64, ii + 128} for ii in range(2, 50)])
        self.assertEqual(expect, actual)

    def test_affinity_12(self):
        launcher = KNLAffinityLauncher(['--geopm-ctl', 'process'], 51, 1, 5)
        err_msg = 'Cores cannot be shared between MPI ranks'
        with self.assertRaisesRegexp(RuntimeError, err_msg) as cm:
            launcher.affinity_list(False)

if __name__ == '__main__':
    unittest.main()
