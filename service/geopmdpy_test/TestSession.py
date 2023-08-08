#!/usr/bin/env python3
#
#  Copyright (c) 2015 - 2023, Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#

import os
import unittest
from unittest import mock
from io import StringIO
from dasbus.error import DBusError

# Patch dlopen to allow the tests to run when there is no build
with mock.patch('cffi.FFI.dlopen', return_value=mock.MagicMock()):
    from geopmdpy.session import Session
    from geopmdpy.session import RequestQueue
    from geopmdpy.session import ReadRequestQueue

class TestSession(unittest.TestCase):
    def setUp(self):
        self._test_name = 'TestSession'
        self._session = Session()

    def test_format_signals_invalid(self):
        err_msg = 'Number of signal values does not match the number of requests'
        with self.assertRaisesRegex(RuntimeError, err_msg):
            self._session.format_signals([], [1])

    def test_format_signals(self):
        signals = [123, 456]
        signal_format = [789, 101112]
        return_value = 'formatted'
        with mock.patch('geopmdpy.pio.format_signal', return_value=return_value) as pfs:
            result = self._session.format_signals(signals, signal_format)

            self.assertEqual('{}\n'.format(','.join([return_value] * len(signals))), result)
            calls = [mock.call(*ii) for ii in list(zip(signals, signal_format))]
            pfs.assert_has_calls(calls)

    def test_run_read(self):
        duration = 2
        period = 1
        num_period = int(duration / period)
        out_stream = mock.MagicMock()

        user_requests = [('power', 0, 0), ('SERVICE::energy', 1, 1), ('frequency', 2, 2)]
        pio_requests = [('power', 0, 0), ('SERVICE::energy', 1, 1), ('frequency', 2, 2)]

        mock_requests = mock.MagicMock()
        mock_requests.__iter__.return_value = user_requests
        format_return_value = "1.234, 2.345, 3.456"
        mock_requests.get_formats.return_value = format_return_value
        signal_handle = list(range(3))
        signal_expect = num_period * [1.234, 2.345, 3.456]

        with mock.patch('geopmdpy.runtime.TimedLoop', return_value=[0, 1]) as mock_timed_loop, \
             mock.patch('geopmdpy.pio.push_signal', side_effect=signal_handle) as mock_push_signal, \
             mock.patch('geopmdpy.pio.read_batch') as mock_read_batch, \
             mock.patch('geopmdpy.pio.sample', side_effect=signal_expect) as mock_sample, \
             mock.patch('geopmdpy.session.Session.format_signals', return_value=format_return_value):
            # Call tested method
            self._session.run_read(mock_requests, duration, period, out_stream)
            # Check mocks calls
            calls = [mock.call(*req) for req in pio_requests]
            mock_push_signal.assert_has_calls(calls)
            calls = num_period * [mock.call()]
            mock_read_batch.assert_has_calls(calls)
            calls = num_period * [mock.call(idx) for idx in signal_handle]
            mock_sample.assert_has_calls(calls)
            calls = num_period * [mock.call(format_return_value)]
            out_stream.write.assert_has_calls(calls)

    def test_check_read_args(self):
        err_msg = 'Specified a period that is greater than the total run time'
        with self.assertRaisesRegex(RuntimeError, err_msg):
            self._session.check_read_args(1, 2)

        day = 24 * 60 * 60
        err_msg = 'Specified a period greater than 24 hours'
        with self.assertRaisesRegex(RuntimeError, err_msg):
            self._session.check_read_args(7 * day, day + 1,)

        err_msg = 'Specified a negative run time or period'
        with self.assertRaisesRegex(RuntimeError, err_msg):
            self._session.check_read_args(-1, -1)

        err_msg = 'Specified a negative run time or period'
        with self.assertRaisesRegex(RuntimeError, err_msg):
            self._session.check_read_args(1, -1)

        self._session.check_read_args(1, 1)

    def test_run(self):
        period = 7
        runtime = 42
        period = 0
        request_stream = [1, 2, 3, 4, 5]
        out_stream = mock.MagicMock()
        rrq_return_value = [4, 5, 6]
        with mock.patch('geopmdpy.session.ReadRequestQueue',
                        return_value=rrq_return_value) as srrq, \
             mock.patch('geopmdpy.session.Session.check_read_args') as scra, \
             mock.patch('geopmdpy.session.Session.run_read') as srr:
            self._session.run(runtime, period, request_stream, out_stream)

            srrq.assert_called_once_with(request_stream)
            scra.assert_called_once_with(runtime, period)
            srr.assert_called_once_with(rrq_return_value, runtime, period, out_stream)


if __name__ == '__main__':
    unittest.main()
