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
import math
import os
import geopmpy.launcher


class Topo():
    def __init__(self, num_socket, core_per_socket, hthread_per_core):
        self._hthread_per_core = hthread_per_core
        self._core_per_socket = core_per_socket
        self._num_socket = num_socket
        self._num_core = self._core_per_socket * self._num_socket
        self._num_linux_cpu = self._hthread_per_core * self._num_core
        # used by tests
        self.core_list = range(self._num_core)
        self.hyperthreads = {}
        for core in self.core_list:
            self.hyperthreads[core] = [core + ht*self._num_core for ht in range(1, self._hthread_per_core)]
        assert math.ceil(self._num_core / self._num_socket) == (self._num_core // self._num_socket)
        self.socket_cores = {}
        for sock in range(self._num_socket):
            self.socket_cores[sock] = [sock*self._core_per_socket + cc for cc in range(self._core_per_socket)]


class TestAffinityLauncher(geopmpy.launcher.Launcher):
    def __init__(self, argv, num_rank, num_node, cpu_per_rank, topo):
        self.topo = topo
        super(TestAffinityLauncher, self).__init__(argv, num_rank, num_node, cpu_per_rank)

    def init_topo(self):
        self.thread_per_core = self.topo._hthread_per_core
        self.core_per_socket = self.topo._core_per_socket
        self.num_socket = self.topo._num_socket
        self.num_linux_cpu = self.topo._num_linux_cpu

    def parse_launcher_argv(self):
        pass

    def msr_save(self):
        pass

    def msr_restore(self):
        pass


class TestAffinity(unittest.TestCase):
    '''
    Expected behavior for pthread is same as process, but GEOPM's
    cores are added to the first app rank.  Expected behavior for
    application is the same pinning for the application, and the same
    pinning for geopm when True is passed to affinity_list().
    '''
    def setUp(self):
        self.maxDiff = 4096
        self.process_argv = ['--geopm-ctl', 'process']
        self.pthread_argv = ['--geopm-ctl', 'pthread']
        self.application_argv = ['--geopm-ctl', 'application']
        self.default_topo = Topo(hthread_per_core=2, core_per_socket=4, num_socket=2)
        self.quartz_topo = Topo(num_socket=2, core_per_socket=18, hthread_per_core=2)
        self.xeon_topo = Topo(num_socket=2, core_per_socket=22, hthread_per_core=2)
        self.knl_topo = Topo(num_socket=1, core_per_socket=64, hthread_per_core=4)
        if os.getenv('OMP_NUM_THREADS') != None:
            self.fail('ERROR: OMP_NUM_THREADS was set in the environment!')
        # TODO: machine with 1 core only, 2 cores, 1 thread per core

    def check_process_mode(self, geopm_cpus, app_cpus, launch_args):
        args = launch_args.copy()
        self.process_argv += args.pop('add_args', [])
        launcher = TestAffinityLauncher(self.process_argv, **args)
        actual = launcher.affinity_list(False)
        expect = [geopm_cpus] + app_cpus
        self.assertEqual(expect, actual)

    def check_pthread_mode(self, geopm_cpus, app_cpus, launch_args):
        args = launch_args.copy()
        self.pthread_argv += args.pop('add_args', [])
        launcher = TestAffinityLauncher(self.pthread_argv, **args)
        actual = launcher.affinity_list(False)
        expect = [geopm_cpus | app_cpus[0]] + app_cpus[1:]
        self.assertEqual(expect, actual)

    def check_application_mode(self, geopm_cpus, app_cpus, launch_args):
        args = launch_args.copy()
        self.application_argv += args.pop('add_args', [])
        launcher = TestAffinityLauncher(self.application_argv, **args)
        actual = launcher.affinity_list(True)
        expect = [geopm_cpus]
        self.assertEqual(expect, actual)
        actual = launcher.affinity_list(False)
        expect = app_cpus
        self.assertEqual(expect, actual)

    def test_affinity_0(self):
        topo = self.xeon_topo
        launch_args = {
            'topo': topo,
            'num_rank': 1,
            'num_node': 1,
            'cpu_per_rank': 1,
        }
        geopm_cpus = {1}
        app_cpus = [{43}]
        self.check_process_mode(geopm_cpus, app_cpus, launch_args)
        self.check_pthread_mode(geopm_cpus, app_cpus, launch_args)
        self.check_application_mode(geopm_cpus, app_cpus, launch_args)

    def test_affinity_1(self):
        topo = self.xeon_topo
        launch_args = {
            'topo': topo,
            'num_rank': 2,
            'num_node': 1,
            'cpu_per_rank': 4,
        }
        geopm_cpus = {1}
        app_cpus = [{21, 20, 19, 18}, {43, 42, 41, 40}]
        self.check_process_mode(geopm_cpus, app_cpus, launch_args)
        self.check_pthread_mode(geopm_cpus, app_cpus, launch_args)
        self.check_application_mode(geopm_cpus, app_cpus, launch_args)

    def test_affinity_4(self):
        '''
        Tests that with 1 cpu per rank, app runs on all the cores, and
        geopm runs on a hyperthread of core 0.
        '''
        topo = self.xeon_topo
        launch_args = {
            'topo': topo,
            'num_rank': 44,
            'num_node': 1,
            'cpu_per_rank': 1,
        }
        geopm_cpus = {44}
        app_cpus = [{ii} for ii in range(44)]
        self.check_process_mode(geopm_cpus, app_cpus, launch_args)
        self.check_pthread_mode(geopm_cpus, app_cpus, launch_args)
        self.check_application_mode(geopm_cpus, app_cpus, launch_args)

    def test_affinity_5(self):
        '''
        Tests that with 2 cpus per rank, app runs on all the cores and
        first hyperthread, and geopm runs on core 0.
        '''
        topo = self.xeon_topo
        launch_args = {
            'topo': topo,
            'num_rank': 44,
            'num_node': 1,
            'cpu_per_rank': 2,
        }
        geopm_cpus = {0}
        app_cpus = [{ii, ii + 44} for ii in range(44)]
        self.check_process_mode(geopm_cpus, app_cpus, launch_args)
        self.check_pthread_mode(geopm_cpus, app_cpus, launch_args)
        self.check_application_mode(geopm_cpus, app_cpus, launch_args)

    def test_affinity_6(self):
        '''
        Tests that with 2 cpus per rank, app runs on all the cores and
        first hyperthread, and geopm runs on core 0.
        '''
        topo = self.default_topo
        launch_args = {
            'topo': topo,
            'num_rank': 8,
            'num_node': 1,
            'cpu_per_rank': 2,
        }
        geopm_cpus = {0}
        app_cpus = [{ii, ii + 8} for ii in range(8)]
        self.check_process_mode(geopm_cpus, app_cpus, launch_args)
        self.check_pthread_mode(geopm_cpus, app_cpus, launch_args)
        self.check_application_mode(geopm_cpus, app_cpus, launch_args)

    def test_affinity_7(self):
        '''
        Tests that the ranks are split evenly between the two sockets and
        fill in cores from the top down, leaving the lower cores on each
        socket free.
        '''
        topo = self.xeon_topo
        launch_args = {
            'topo': topo,
            'num_rank': 40,
            'num_node': 1,
            'cpu_per_rank': 2,
        }
        geopm_cpus = {1}
        app_cpus = [{ii, ii + 44} for ii in range(2, 22)]
        app_cpus += [{ii, ii + 44} for ii in range(24, 44)]
        self.check_process_mode(geopm_cpus, app_cpus, launch_args)
        self.check_pthread_mode(geopm_cpus, app_cpus, launch_args)
        self.check_application_mode(geopm_cpus, app_cpus, launch_args)

    def test_affinity_8(self):
        '''
        Tests that the ranks are split evenly between the two sockets and
        fill in cores from the top down, leaving the lower cores on each
        socket free.
        '''
        topo = self.xeon_topo
        launch_args = {
            'topo': topo,
            'num_rank': 30,
            'num_node': 1,
            'cpu_per_rank': 2,
        }
        geopm_cpus = {1}
        app_cpus = [{ii, ii + 44} for ii in range(7, 22)]
        app_cpus += [{ii, ii + 44} for ii in range(29, 44)]
        self.check_process_mode(geopm_cpus, app_cpus, launch_args)
        self.check_pthread_mode(geopm_cpus, app_cpus, launch_args)
        self.check_application_mode(geopm_cpus, app_cpus, launch_args)

    def test_affinity_9(self):
        '''
        Tests that when all cores and hyperthreads are used by the app,
        GEOPM shares core 0 with the OS.
        '''
        topo = self.knl_topo
        launch_args = {
            'topo': topo,
            'num_rank': 64,
            'num_node': 1,
            'cpu_per_rank': 4,
        }
        geopm_cpus = {0}
        app_cpus = [{ii, ii + 64, ii + 128, ii + 192} for ii in range(64)]
        self.check_process_mode(geopm_cpus, app_cpus, launch_args)
        self.check_pthread_mode(geopm_cpus, app_cpus, launch_args)
        self.check_application_mode(geopm_cpus, app_cpus, launch_args)

    def test_affinity_10(self):
        '''
        Tests that when the app is using 3 out of 4 hyperthreads on every core,
        GEOPM uses the last hyperthread of core 0.
        '''
        topo = self.knl_topo
        launch_args = {
            'topo': topo,
            'num_rank': 64,
            'num_node': 1,
            'cpu_per_rank': 3,
        }
        geopm_cpus = {192}
        app_cpus = [{ii, ii + 64, ii + 128} for ii in range(64)]
        self.check_process_mode(geopm_cpus, app_cpus, launch_args)
        self.check_pthread_mode(geopm_cpus, app_cpus, launch_args)
        self.check_application_mode(geopm_cpus, app_cpus, launch_args)

    def test_affinity_11(self):
        '''
        Tests that when the app requests threads per rank <= the number of
        hyperthreads, and there are not enough cores to satisfy this
        with additional cores per rank, the ranks are assigned
        hyperthreads from the same core.
        '''
        topo = self.knl_topo
        launch_args = {
            'topo': topo,
            'num_rank': 48,
            'num_node': 1,
            'cpu_per_rank': 3,
        }
        geopm_cpus = {1}
        app_cpus = [{ii, ii + 64, ii + 128} for ii in range(16, 64)]
        self.check_process_mode(geopm_cpus, app_cpus, launch_args)
        self.check_pthread_mode(geopm_cpus, app_cpus, launch_args)
        self.check_application_mode(geopm_cpus, app_cpus, launch_args)

    def test_affinity_12(self):
        '''
        Tests that an error is printed when the number of ranks is greater
        than the number of cores.
        '''
        topo = self.knl_topo
        launch_args = {
            'topo': topo,
            'num_rank': 51,
            'num_node': 1,
            'cpu_per_rank': 5,
        }
        launcher = TestAffinityLauncher(self.process_argv, **launch_args)
        err_msg = 'Cores cannot be shared between MPI ranks'
        with self.assertRaisesRegexp(RuntimeError, err_msg):
            launcher.affinity_list(False)

        launcher = TestAffinityLauncher(self.pthread_argv, **launch_args)
        err_msg = 'Cores cannot be shared between MPI ranks'
        with self.assertRaisesRegexp(RuntimeError, err_msg):
            launcher.affinity_list(False)

        launcher = TestAffinityLauncher(self.application_argv, **launch_args)
        err_msg = 'Cores cannot be shared between MPI ranks'
        with self.assertRaisesRegexp(RuntimeError, err_msg):
            launcher.affinity_list(False)

    def test_affinity_13(self):
        """
        Here the app is trying to use num_sockets * num_cores - 1 OMP threads.  This should
        result in the controller getting pinned to core 0's HT, and the app pinned to cores 1-43.
        Core 0 should be left for the OS.
        """
        topo = self.xeon_topo
        launch_args = {
            'topo': topo,
            'num_rank': 1,
            'num_node': 1,
            'cpu_per_rank': 43,  # TODO: topo._num_core - 1 = 43
        }
        geopm_cpus = {44}
        app_cpus = [set(range(1, 44))]
        self.check_process_mode(geopm_cpus, app_cpus, launch_args)
        self.check_pthread_mode(geopm_cpus, app_cpus, launch_args)
        self.check_application_mode(geopm_cpus, app_cpus, launch_args)

    def test_affinity_14(self):
        """
        The mpibind plugin used on TOSS based systems does not yet support pinning to HTs.  When an
        app requests num_sockets * num_cores - 1 OMP threads the controller should be pinned to core 0
        meaning it is shared with the OS.
        """
        topo = self.quartz_topo
        launch_args = {
            'topo': topo,
            'num_rank': 1,
            'num_node': 1,
            'cpu_per_rank': 35,  # topo._num_core - 1
            'add_args': ['--geopm-hyperthreads-disable'],
        }
        app_cpus = [set(range(1, 36))]
        geopm_cpus = {0}
        self.check_process_mode(geopm_cpus, app_cpus, launch_args)
        self.check_pthread_mode(geopm_cpus, app_cpus, launch_args)
        self.check_application_mode(geopm_cpus, app_cpus, launch_args)

    def test_affinity_15(self):
        """
        When the application requests all the physical cores we would normally pin the controller to core
        0's HT.  Since mpibind does not support HT pinning, oversubscribe core 0.
        """
        topo = self.quartz_topo
        launch_args = {
            'topo': topo,
            'num_rank': 1,
            'num_node': 1,
            'cpu_per_rank': 36,
            'add_args': ['--geopm-hyperthreads-disable'],
        }
        app_cpus = [set(range(0, 36))]
        geopm_cpus = {0}
        self.check_process_mode(geopm_cpus, app_cpus, launch_args)
        self.check_pthread_mode(geopm_cpus, app_cpus, launch_args)
        self.check_application_mode(geopm_cpus, app_cpus, launch_args)

    def test_affinity_16(self):
        """
        Similar to test 14, this attempts to utilize 35 execution units (5 ranks * 7 OMP threads).
        Core 0 should be used for the OS and controller.
        """
        topo = self.quartz_topo
        launch_args = {
            'topo': topo,
            'num_rank': 5,
            'num_node': 1,
            'cpu_per_rank': 7,
            'add_args': ['--geopm-hyperthreads-disable'],
        }
        app_cpus = [{1, 2, 3, 4, 5, 6, 7}, {8, 9, 10, 11, 12, 13, 14}, {15, 16, 17, 18, 19, 20, 21},
                    {22, 23, 24, 25, 26, 27, 28}, {29, 30, 31, 32, 33, 34, 35}]
        geopm_cpus = {0}
        self.check_process_mode(geopm_cpus, app_cpus, launch_args)
        self.check_pthread_mode(geopm_cpus, app_cpus, launch_args)
        self.check_application_mode(geopm_cpus, app_cpus, launch_args)

    def test_affinity_17(self):
        """
        Similar to test 14, this attempts to utilize 35 execution units (7 ranks * 5 OMP threads).
        Core 0 should be used for the OS and controller.
        """
        topo = self.quartz_topo
        launch_args = {
            'topo': topo,
            'num_rank': 7,
            'num_node': 1,
            'cpu_per_rank': 5,
            'add_args': ['--geopm-hyperthreads-disable'],
        }
        app_cpus = [{1, 2, 3, 4, 5}, {6, 7, 8, 9, 10}, {11, 12, 13, 14, 15}, {16, 17, 18, 19, 20},
                    {21, 22, 23, 24, 25}, {26, 27, 28, 29, 30}, {31, 32, 33, 34, 35}]
        geopm_cpus = {0}
        self.check_process_mode(geopm_cpus, app_cpus, launch_args)
        self.check_pthread_mode(geopm_cpus, app_cpus, launch_args)
        self.check_application_mode(geopm_cpus, app_cpus, launch_args)

    def test_affinity_18(self):
        """
        Tests trying to use hyperthreads when hyperthreading is disabled.
        """
        topo = self.quartz_topo
        launch_args = {
            'topo': topo,
            'num_rank': 1,
            'num_node': 1,
            'cpu_per_rank': 40,
        }
        add_args = ['--geopm-hyperthreads-disable']
        launcher = TestAffinityLauncher(self.process_argv + add_args, **launch_args)
        err_msg = 'Hyperthreads needed to satisfy ranks/threads configuration, but forbidden by'\
                  ' --geopm-hyperthreads-disable.'
        with self.assertRaisesRegexp(RuntimeError, err_msg):
            launcher.affinity_list(False)

        launcher = TestAffinityLauncher(self.pthread_argv + add_args, **launch_args)
        err_msg = 'Hyperthreads needed to satisfy ranks/threads configuration, but forbidden by'\
                  ' --geopm-hyperthreads-disable.'
        with self.assertRaisesRegexp(RuntimeError, err_msg):
            launcher.affinity_list(False)

        launcher = TestAffinityLauncher(self.application_argv + add_args, **launch_args)
        err_msg = 'Hyperthreads needed to satisfy ranks/threads configuration, but forbidden by'\
                  ' --geopm-hyperthreads-disable.'
        with self.assertRaisesRegexp(RuntimeError, err_msg):
            launcher.affinity_list(False)

    def test_affinity_tutorial_knl(self):
        topo = self.knl_topo
        launch_args = {
            'topo': topo,
            'num_rank': 8,
            'num_node': 2,
            'cpu_per_rank': 63,
        }
        app_cpus = [{jj + 16 * kk
                   for ii in range(4)
                       for jj in range(ii * 64, ii * 64 + 16)}
                  for kk in range(4)]
        geopm_cpus = {0}
        self.check_process_mode(geopm_cpus, app_cpus, launch_args)
        self.check_pthread_mode(geopm_cpus, app_cpus, launch_args)
        self.check_application_mode(geopm_cpus, app_cpus, launch_args)

    def test_1rank_1thread(self):
        topo = self.default_topo
        launch_args = {
            'topo': topo,
            'num_rank': 1,
            'num_node': 1,
            'cpu_per_rank': 1,
        }
        geopm_cpus = {1}                   # leave 0 for OS
        app_cpus = [{topo.core_list[-1]}]  # last real core in the list

        self.check_process_mode(geopm_cpus, app_cpus, launch_args)
        self.check_pthread_mode(geopm_cpus, app_cpus, launch_args)
        self.check_application_mode(geopm_cpus, app_cpus, launch_args)

    def test_1rank_leave_2_cores(self):
        topo = self.default_topo
        app_cores = topo._num_core - 2
        launch_args = {
            'topo': topo,
            'num_rank': 1,
            'num_node': 1,
            'cpu_per_rank': app_cores,
        }
        geopm_cpus = {1}                               # leave 0 for OS
        app_cpus = [set(topo.core_list[-app_cores:])]  # app_core cores from the end

        self.check_process_mode(geopm_cpus, app_cpus, launch_args)
        self.check_pthread_mode(geopm_cpus, app_cpus, launch_args)
        self.check_application_mode(geopm_cpus, app_cpus, launch_args)

    def test_1rank_geopm_os_shared(self):
        topo = self.default_topo
        app_cores = topo._num_core - 1
        launch_args = {
            'topo': topo,
            'num_rank': 1,
            'num_node': 1,
            'cpu_per_rank': app_cores,
        }
        geopm_cpus = {topo.hyperthreads[0][0]}         # core 0's first hyperthread
        app_cpus = [set(topo.core_list[-app_cores:])]  # app_core cores from the end

        self.check_process_mode(geopm_cpus, app_cpus, launch_args)
        self.check_pthread_mode(geopm_cpus, app_cpus, launch_args)
        self.check_application_mode(geopm_cpus, app_cpus, launch_args)

    def test_1rank_geopm_os_app_shared(self):
        topo = self.default_topo
        app_cores = topo._num_core
        launch_args = {
            'topo': topo,
            'num_rank': 1,
            'num_node': 1,
            'cpu_per_rank': app_cores,
        }
        geopm_cpus = {topo.hyperthreads[0][0]}  # core 0's hyperthread, shared
        app_cpus = [set(topo.core_list)]  # app uses all cores
        # TODO: app or geopm could use a hyperthread on core 0
        # geopm could use a different core, still shared with app

        self.check_process_mode(geopm_cpus, app_cpus, launch_args)
        self.check_pthread_mode(geopm_cpus, app_cpus, launch_args)
        self.check_application_mode(geopm_cpus, app_cpus, launch_args)

    def test_1rank_geopm_os_shared_noht(self):
        topo = self.default_topo
        app_cores = topo._num_core - 1
        launch_args = {
            'topo': topo,
            'num_rank': 1,
            'num_node': 1,
            'cpu_per_rank': app_cores,
            'add_args': ['--geopm-hyperthreads-disable'],
        }
        geopm_cpus = {0}  # core 0, shared with OS
        app_cpus = [set(topo.core_list[-app_cores:])]  # app_core cores from the end

        self.check_process_mode(geopm_cpus, app_cpus, launch_args)
        self.check_pthread_mode(geopm_cpus, app_cpus, launch_args)
        self.check_application_mode(geopm_cpus, app_cpus, launch_args)

    def test_1rank_geopm_os_app_shared_noht(self):
        topo = self.default_topo
        app_cores = topo._num_core
        launch_args = {
            'topo': topo,
            'num_rank': 1,
            'num_node': 1,
            'cpu_per_rank': app_cores,
            'add_args': ['--geopm-hyperthreads-disable'],
        }
        geopm_cpus = {0}  # core 0, shared with app and OS
        app_cpus = [set(topo.core_list)]  # app uses all cores

        self.check_process_mode(geopm_cpus, app_cpus, launch_args)
        self.check_pthread_mode(geopm_cpus, app_cpus, launch_args)
        self.check_application_mode(geopm_cpus, app_cpus, launch_args)

    def test_1rank_no_env_threads(self):
        topo = self.default_topo
        launch_args = {
            'topo': topo,
            'num_rank': 1,
            'num_node': 1,
            'cpu_per_rank': None,
        }
        # TODO: add helper function to get set of cores and their hyperthreads
        # or change hyperthreads map to include physical cores
        expected_app = set()
        for core in topo.core_list[2:]:  # cores reserved for OS and geopm
            expected_app.add(core)
            for ht in topo.hyperthreads[core]:
                expected_app.add(ht)
        geopm_cpus = {1}  # leave core 0 for OS
        app_cpus = [expected_app]

        self.check_process_mode(geopm_cpus, app_cpus, launch_args)
        self.check_pthread_mode(geopm_cpus, app_cpus, launch_args)
        self.check_application_mode(geopm_cpus, app_cpus, launch_args)

    def test_1rank_no_env_threads_noht(self):
        topo = self.default_topo
        launch_args = {
            'topo': topo,
            'num_rank': 1,
            'num_node': 1,
            'cpu_per_rank': None,
            'add_args': ['--geopm-hyperthreads-disable']
        }
        geopm_cpus = {1}  # leave core 0 for OS
        app_cpus = [set(topo.core_list[2:])]  # app uses all cores - 2 reserved

        self.check_process_mode(geopm_cpus, app_cpus, launch_args)
        self.check_pthread_mode(geopm_cpus, app_cpus, launch_args)
        self.check_application_mode(geopm_cpus, app_cpus, launch_args)

    def test_per_core_rank_noht(self):
        topo = self.default_topo
        app_cores = topo._num_core
        launch_args = {
            'topo': topo,
            'num_rank': app_cores,
            'num_node': 1,
            'cpu_per_rank': 1,
            'add_args': ['--geopm-hyperthreads-disable']
        }
        geopm_cpus = {0}  # core 0, shared with app and OS
        app_cpus = [{x} for x in topo.core_list]  # app uses all cores

        self.check_process_mode(geopm_cpus, app_cpus, launch_args)
        self.check_pthread_mode(geopm_cpus, app_cpus, launch_args)
        self.check_application_mode(geopm_cpus, app_cpus, launch_args)

    def test_per_core_rank_1reserved_noht(self):
        topo = self.default_topo
        app_cores = topo._num_core - 1
        launch_args = {
            'topo': topo,
            'num_rank': app_cores,
            'num_node': 1,
            'cpu_per_rank': 1,
            'add_args': ['--geopm-hyperthreads-disable']
        }
        geopm_cpus = {0}  # core 0, shared with OS
        app_cpus = [{x} for x in topo.core_list[1:]]  # app uses all cores except 0

        self.check_process_mode(geopm_cpus, app_cpus, launch_args)
        self.check_pthread_mode(geopm_cpus, app_cpus, launch_args)
        self.check_application_mode(geopm_cpus, app_cpus, launch_args)

    def test_per_core_rank_2reserved_noht(self):
        topo = self.default_topo
        app_cores = topo._num_core - 2
        launch_args = {
            'topo': topo,
            'num_rank': app_cores,
            'num_node': 1,
            'cpu_per_rank': 1,
            'add_args': ['--geopm-hyperthreads-disable']
        }
        geopm_cpus = {0}  # shared with OS; TODO: could go on a free core
        app_cpus = [{x} for x in topo.socket_cores[0][1:]]  # app uses 3 cores from each socket
        app_cpus += [{x} for x in topo.socket_cores[1][1:]]
        self.check_process_mode(geopm_cpus, app_cpus, launch_args)
        self.check_pthread_mode(geopm_cpus, app_cpus, launch_args)
        self.check_application_mode(geopm_cpus, app_cpus, launch_args)

    def test_2rank_no_env_threads(self):
        topo = self.default_topo
        launch_args = {
            'topo': topo,
            'num_rank': 2,
            'num_node': 1,
            'cpu_per_rank': None,
        }
        app_cpus = [set(), set()]
        # one rank per socket
        for rank in range(topo._num_socket):
            for core in topo.socket_cores[rank][1:]:
                app_cpus[rank].add(core)
                app_cpus[rank].update(topo.hyperthreads[core])
        geopm_cpus = {0}  # TODO: could use a core on socket 1

        self.check_process_mode(geopm_cpus, app_cpus, launch_args)
        self.check_pthread_mode(geopm_cpus, app_cpus, launch_args)
        self.check_application_mode(geopm_cpus, app_cpus, launch_args)

    def test_2rank_no_env_threads_noht(self):
        topo = self.default_topo
        launch_args = {
            'topo': topo,
            'num_rank': 2,
            'num_node': 1,
            'cpu_per_rank': None,
            'add_args': ['--geopm-hyperthreads-disable']
        }
        geopm_cpus = {0}  # shared with OS; TODO: could use a core on socket 1
        app_cpus = [set(topo.socket_cores[0][1:]),
                    set(topo.socket_cores[1][1:])]

        self.check_process_mode(geopm_cpus, app_cpus, launch_args)
        self.check_pthread_mode(geopm_cpus, app_cpus, launch_args)
        self.check_application_mode(geopm_cpus, app_cpus, launch_args)

    def test_3rank_no_env_threads_noht(self):
        topo = self.default_topo
        launch_args = {
            'topo': topo,
            'num_rank': 3,
            'num_node': 1,
            'cpu_per_rank': None,
            'add_args': ['--geopm-hyperthreads-disable']
        }
        expected_app_cores = 2
        app_cpus = [set(), set(), set()]
        core = 2
        # two cores for each
        for rank in range(len(app_cpus)):
            for cc in range(expected_app_cores):
                app_cpus[rank].add(core)
                core += 1
        geopm_cpus = {1}

        self.check_process_mode(geopm_cpus, app_cpus, launch_args)
        self.check_pthread_mode(geopm_cpus, app_cpus, launch_args)
        self.check_application_mode(geopm_cpus, app_cpus, launch_args)

    def test_3rank_no_env_threads(self):
        topo = self.default_topo
        launch_args = {
            'topo': topo,
            'num_rank': 3,
            'num_node': 1,
            'cpu_per_rank': None,
        }
        expected_app_cores = 2
        app_cpus = [set(), set(), set()]
        core = 2
        # two cores for each with hyperthreads
        for rank in range(len(app_cpus)):
            for cc in range(expected_app_cores):
                app_cpus[rank].add(core)
                app_cpus[rank].update(topo.hyperthreads[core])
                core += 1
        geopm_cpus = {1}

        self.check_process_mode(geopm_cpus, app_cpus, launch_args)
        self.check_pthread_mode(geopm_cpus, app_cpus, launch_args)
        self.check_application_mode(geopm_cpus, app_cpus, launch_args)


if __name__ == '__main__':
    unittest.main()
