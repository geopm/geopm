#!/usr/bin/env python3
#
#  Copyright (c) 2015 - 2021, Intel Corporation
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
import itertools
import math
from unittest import mock
from time import time

# Patch dlopen to allow the tests to run when there is no build
with mock.patch('cffi.FFI.dlopen', return_value=mock.MagicMock()):
    from geopmdpy.runtime import TimedLoop
    from geopmdpy.runtime import Agent
    from geopmdpy.runtime import Controller

class PassthroughAgent(Agent):
    def __init__(self, signals, controls, period):
        self._signals = signals
        self._controls = controls
        self._period = period
        self._adjust_value = 7
        self._report_data = 'report data'

    def get_signals(self):
        return self._signals

    def get_controls(self):
        return self._controls

    def get_period(self):
        return self._period

    def update(self, signals):
        return [self._adjust_value] * len(signals)

    def get_report(self):
        return self._report_data

class TestRuntime(unittest.TestCase):
    def setUp(self):
        self._test_name = 'TestRuntime'

    def tearDown(self):
        pass

    def test_timed_loop_invalid(self):
        err_msg = 'Specified period is invalid.  Must be > 0.'
        with self.assertRaisesRegex(RuntimeError, err_msg):
            TimedLoop(0, 0)

        err_msg = 'Specified num_period is invalid.  Must be > 0.'
        with self.assertRaisesRegex(RuntimeError, err_msg):
            TimedLoop(1, 0)

        err_msg = 'num_period must be a whole number.'
        with self.assertRaisesRegex(ValueError, err_msg):
            TimedLoop(1.5, 1.5)

        err_msg = "'<=' not supported between instances of 'str' and 'float'"
        with self.assertRaisesRegex(TypeError, err_msg):
            TimedLoop('asd', 'dsa')

    def test_timed_loop_infinite(self):
        period = 0.01
        tl = TimedLoop(period) # Infinte loop

        self.assertEqual(period, tl._period)
        self.assertIsNone(tl._num_loop)

        t1 = None
        for index in tl:
            if t1 is not None:
                elapsed = time() - t1
                self.assertAlmostEqual(period, elapsed, delta = 0.001)
            t1 = time()
            if index == 50: # Break after 50 iterations
                break

    def test_timed_loop_fixed(self):
        period = 0.01
        num_period = 10
        tl = TimedLoop(period, num_period)

        self.assertEqual(period, tl._period)
        self.assertEqual(num_period + 1, tl._num_loop)

        it = iter(tl)
        for ii in range(num_period + 1):
            # Start checking at 2 since 0 is really setup and the 1st iteration is not delayed
            if ii == 2:
                elapsed = time() - t1
                self.assertAlmostEqual(period, elapsed, delta = 0.001)
            t1 = time()
            self.assertEqual(ii, next(it))

        with self.assertRaisesRegex(StopIteration, ''):
            next(it)

    def test_agent(self):
        err_msg = 'Agent is an abstract base class'
        with self.assertRaisesRegex(NotImplementedError, err_msg):
            Agent()

        class ConstructorAgent(Agent):
            def __init__(self):
                pass

        ca = ConstructorAgent()
        with self.assertRaisesRegex(NotImplementedError, err_msg):
            ca.get_signals()
        with self.assertRaisesRegex(NotImplementedError, err_msg):
            ca.get_controls()
        with self.assertRaisesRegex(NotImplementedError, err_msg):
            ca.update(None)
        with self.assertRaisesRegex(NotImplementedError, err_msg):
            ca.get_period()
        with self.assertRaisesRegex(NotImplementedError, err_msg):
            ca.get_report()

    # TODO Negative controller tests

    def test_controller_construction(self):
        signals = [('power', 1, 2), ('energy', 3, 4)]
        controls = [('frequency', 5, 6), ('power', 7, 8)]
        period = 42

        argv = 'data'
        pa = PassthroughAgent(signals, controls, period)
        with mock.patch('geopmdpy.pio.push_signal', side_effect = itertools.count()) as pps, \
             mock.patch('geopmdpy.pio.push_control', side_effect = itertools.count()) as ppc:
            con = Controller(pa, argv) # No timeout

            self.assertIs(pa, con._agent)
            self.assertEqual(signals, con._signals)
            self.assertEqual(controls, con._controls)
            self.assertEqual(list(range(len(signals))), con._signals_idx)
            self.assertEqual(list(range(len(controls))), con._controls_idx)
            self.assertEqual(period, con._update_period)
            self.assertIsNone(con._num_update)
            self.assertEqual(argv, con._argv)

            calls = [mock.call(*cc) for cc in signals]
            pps.assert_has_calls(calls)
            calls = [mock.call(*cc) for cc in controls]
            ppc.assert_has_calls(calls)

        # Again, with timeout, same expectations otherwise
        timeout = 10
        with mock.patch('geopmdpy.pio.push_signal', side_effect = itertools.count()) as pps, \
             mock.patch('geopmdpy.pio.push_control', side_effect = itertools.count()) as ppc:
            con = Controller(pa, argv, timeout)

            self.assertIs(pa, con._agent)
            self.assertEqual(signals, con._signals)
            self.assertEqual(controls, con._controls)
            self.assertEqual(list(range(len(signals))), con._signals_idx)
            self.assertEqual(list(range(len(controls))), con._controls_idx)
            self.assertEqual(period, con._update_period)
            self.assertEqual(math.ceil(timeout / period), con._num_update)
            self.assertEqual(argv, con._argv)

            calls = [mock.call(*cc) for cc in signals]
            pps.assert_has_calls(calls)
            calls = [mock.call(*cc) for cc in controls]
            ppc.assert_has_calls(calls)

    def test_controller_run(self):
        signals = [('power', 1, 2), ('energy', 3, 4)]
        controls = [('frequency', 5, 6), ('power', 7, 8)]
        period = 42

        argv = 'data'
        pa = PassthroughAgent(signals, controls, period)
        loops = 5

        with mock.patch('geopmdpy.pio.push_signal', side_effect = itertools.count()) as pps, \
             mock.patch('geopmdpy.pio.push_control', side_effect = itertools.count()) as ppc, \
             mock.patch('geopmdpy.pio.save_control') as psc, \
             mock.patch('geopmdpy.pio.restore_control') as prc, \
             mock.patch('geopmdpy.pio.sample', side_effect = itertools.count()) as ps, \
             mock.patch('geopmdpy.pio.read_batch') as prb, \
             mock.patch('geopmdpy.pio.write_batch') as pwb, \
             mock.patch('geopmdpy.pio.adjust') as pad, \
             mock.patch('geopmdpy.runtime.TimedLoop', return_value=list(range(loops))) as rtl, \
             mock.patch('subprocess.Popen') as sp:

            con = Controller(pa, argv)
            self.assertEqual(list(range(len(signals))), con.read_all_signals())

            sp().poll.return_value = None # Fake the process to be alive
            result = con.run()

            psc.assert_called_once()
            calls = [mock.call(argv)] + [mock.call().poll()] * loops
            sp.assert_has_calls(calls)
            rtl.assert_called_with(period, None)
            sp().poll.assert_has_calls([mock.call()] * loops)
            prb.assert_has_calls([mock.call()] * loops)
            calls = [mock.call(ii, pa._adjust_value) for ii in range(len(controls))] * loops
            pad.assert_has_calls(calls)
            pwb.assert_has_calls([mock.call()] * loops)
            prc.assert_called_once()
            self.assertEqual(pa._report_data, result)


if __name__ == '__main__':
    unittest.main()
