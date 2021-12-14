#!/usr/bin/env python3
#
#  Copyright (c) 2015 - 2022, Intel Corporation
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

    def run_begin(self, policy):
        pass

    def update(self, signals):
        return [self._adjust_value] * len(signals)

    def run_end(self):
        pass

    def get_report(self, profile):
        return self._report_data


class ConstructorAgent(Agent):
    def __init__(self):
        pass


class TestController(unittest.TestCase):
    def setUp(self):
        self._test_name = 'TestController'

    def tearDown(self):
        pass

    def test_agent(self):
        err_msg = 'Agent is an abstract base class'
        with self.assertRaisesRegex(NotImplementedError, err_msg):
            Agent()

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
            ca.get_report('test_profile')

    def test_controller_construction_invalid(self):
        err_msg = 'agent must be a subclass of Agent.'
        with self.assertRaisesRegex(ValueError, err_msg):
            con = Controller('abc')

        err_msg = 'timeout must be >= 0'
        with self.assertRaisesRegex(ValueError, err_msg):
            con = Controller(ConstructorAgent(), -5)

    def test_controller_construction(self):
        signals = [('power', 1, 2), ('energy', 3, 4)]
        controls = [('frequency', 5, 6), ('power', 7, 8)]
        period = 42

        argv = 'data'
        with mock.patch('geopmdpy.topo.num_domain', return_value=1) as pnd:
            pa = PassthroughAgent(signals, controls, period)
            con = Controller(pa) # No timeout

            self.assertIs(pa, con._agent)
            self.assertEqual(period, con._update_period)
            self.assertIsNone(con._num_update)
            self.assertIsNone(con._returncode)

        # Again, with timeout, same expectations otherwise
        timeout = 10
        con = Controller(pa, timeout)
        self.assertIs(pa, con._agent)
        self.assertEqual(period, con._update_period)
        self.assertEqual(math.ceil(timeout / period), con._num_update)
        self.assertIsNone(con._returncode)

    def _controller_run_helper(self, return_code = None, subprocess_error = False):
        signals = [('power', 1, 2), ('energy', 3, 4)]
        controls = [('frequency', 5, 6), ('power', 7, 8)]
        period = 42

        argv = 'data'
        pa = PassthroughAgent(signals, controls, period)
        loops = 5 if return_code is None else 1

        with mock.patch('geopmdpy.pio.push_signal', side_effect = itertools.count()) as pps, \
             mock.patch('geopmdpy.pio.push_control', side_effect = itertools.count()) as ppc, \
             mock.patch('geopmdpy.pio.restore_control') as prc, \
             mock.patch('geopmdpy.pio.sample', side_effect = itertools.count()) as ps, \
             mock.patch('geopmdpy.pio.save_control') as psc, \
             mock.patch('geopmdpy.pio.read_batch') as prb, \
             mock.patch('geopmdpy.pio.write_batch') as pwb, \
             mock.patch('geopmdpy.pio.adjust') as pad, \
             mock.patch('geopmdpy.runtime.TimedLoop', return_value=list(range(loops))) as rtl, \
             mock.patch('subprocess.Popen') as sp:

            con = Controller(pa)

            sp().poll.return_value = return_code
            sp().returncode = return_code

            if subprocess_error:
                sp_err_msg = 'An error has occured'
                sp.side_effect = RuntimeError(sp_err_msg)
                with self.assertRaisesRegex(RuntimeError, sp_err_msg):
                    con.run(argv)
                psc.assert_called_once()
            else:
                result = con.run(argv)
                psc.assert_called_once()

                calls = [mock.call(argv)] + [mock.call().poll()] * loops
                sp.assert_has_calls(calls)
                rtl.assert_called_with(period, None)
                sp().poll.assert_has_calls([mock.call()] * loops)

                if return_code is not None:
                    loops = 0 # Poll is called once in this case then the break is hit

                prb.assert_has_calls([mock.call()] * loops)
                calls = [mock.call(ii, pa._adjust_value) for ii in range(len(controls))] * loops
                pad.assert_has_calls(calls)
                pwb.assert_has_calls([mock.call()] * loops)
                prc.assert_called_once()
                self.assertEqual(pa._report_data, result)

                if return_code is None:
                    err_msg = 'App process is still running'
                    with self.assertRaisesRegex(RuntimeError, err_msg):
                        con.returncode()
                else:
                    self.assertEqual(return_code, con.returncode())

    def test_controller_run(self):
        self._controller_run_helper()

    def test_controller_run_app_rc(self):
        self._controller_run_helper(42)

    def test_controller_run_app_error(self):
        self._controller_run_helper(subprocess_error = True)


if __name__ == '__main__':
    unittest.main()
