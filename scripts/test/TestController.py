#!/usr/bin/env python3
#
#  Copyright (c) 2015 - 2023, Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#

import unittest
import itertools
import math
from unittest import mock
from time import time
import subprocess # nosec

# Patch dlopen to allow the tests to run when there is no build
with mock.patch('cffi.FFI.dlopen', return_value=mock.MagicMock()):
    from geopmpy.runtime import Agent
    from geopmpy.runtime import Controller

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

    def run_begin(self, policy, profile):
        pass

    def update(self, signals):
        return [self._adjust_value] * len(signals)

    def run_end(self):
        pass

    def get_report(self):
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
            ca.get_report()

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

        with mock.patch('geopmpy.pio_rt.push_signal', side_effect = itertools.count()) as pps, \
             mock.patch('geopmpy.pio_rt.push_control', side_effect = itertools.count()) as ppc, \
             mock.patch('geopmpy.pio_rt.restore_control') as prc, \
             mock.patch('geopmpy.pio_rt.sample', side_effect = itertools.count()) as ps, \
             mock.patch('geopmpy.pio_rt.save_control') as psc, \
             mock.patch('geopmpy.pio_rt.read_batch') as prb, \
             mock.patch('geopmpy.pio_rt.write_batch') as pwb, \
             mock.patch('geopmpy.pio_rt.update') as mock_pio_update, \
             mock.patch('geopmpy.reporter.init') as mock_init, \
             mock.patch('geopmpy.reporter.update') as mock_reporter_update, \
             mock.patch('geopmpy.pio_rt.adjust') as pad, \
             mock.patch('geopmdpy.loop.PIDTimedLoop', return_value=list(range(loops))) as rtl, \
             mock.patch('subprocess.Popen') as sp:

            con = Controller(pa)

            sp().poll.return_value = None
            sp().returncode = return_code
            sp().wait.raiseError.side_effect = mock.Mock(side_effect=subprocess.TimeoutExpired)
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
                rtl.assert_called_with(sp(), period, None)
                sp().poll.assert_has_calls([mock.call()] * loops)

                if return_code is not None:
                    loops = 0 # Poll is called once in this case then the break is hit

                prb.assert_has_calls([mock.call()] * loops)
                calls = [mock.call(ii, pa._adjust_value) for ii in range(len(controls))] * loops
                pad.assert_has_calls(calls)
                pwb.assert_has_calls([mock.call()] * loops)
                prc.assert_called_once()
                mock_init.assert_called_once()
                mock_reporter_update.assert_has_calls([mock.call()] * loops)
                mock_pio_update.assert_has_calls([mock.call()] * loops)
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
