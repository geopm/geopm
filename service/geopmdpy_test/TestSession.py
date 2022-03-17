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
    from geopmdpy.session import WriteRequestQueue

class TestSession(unittest.TestCase):
    def setUp(self):
        self._test_name = 'TestSession'
        self._geopm_proxy = mock.MagicMock()
        self._session = Session(self._geopm_proxy)

    def tearDown(self):
        self._geopm_proxy.reset_mock(return_value=True, side_effect=True)

    def test_create_session_invalid(self):
        err_msg = "'NoneType' object has no attribute 'PlatformOpenSession'"
        with self.assertRaisesRegex(AttributeError, err_msg):
            Session(None)

        # This must NOT be a mock so that the geopm_proxy.PlatformOpenSession
        # check in the constructor is made without parenthesis.
        class geopm_proxy:
            def __getattr__(self, name):
                raise DBusError('io.github.geopm was not provided')

        err_msg = """The geopm systemd service is not enabled.
    Install geopm service and run 'systemctl start geopm'"""
        with self.assertRaisesRegex(RuntimeError, err_msg):
            Session(geopm_proxy())

        class geopm_proxy:
            def __getattr__(self, name):
                raise DBusError('Other DBusError')

        err_msg = "Other DBusError"
        with self.assertRaisesRegex(DBusError, err_msg):
            Session(geopm_proxy())

    def test_create_session(self):
        # Constructed in setUp
        self.assertIs(self._geopm_proxy, self._session._geopm_proxy)

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
        pio_requests = [('SERVICE::power', 0, 0), ('SERVICE::energy', 1, 1), ('SERVICE::frequency', 2, 2)]

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

    def test_run_write(self):
        requests = [('one', 2, 3, 1.0), ('four', 5, 6, 4.0)]
        duration = 42
        with mock.patch('time.sleep') as ts:
            self._session.run_write(requests, duration)

            self._geopm_proxy.PlatformOpenSession.assert_called_once_with()
            calls = [mock.call(*cc) for cc in requests]
            self._geopm_proxy.PlatformWriteControl.assert_has_calls(calls)
            ts.assert_called_once_with(duration)
            self._geopm_proxy.PlatformCloseSession.assert_called_once_with()

    def test_check_read_args(self):
        err_msg = 'Specified a period that is greater than the total run time'
        with self.assertRaisesRegex(RuntimeError, err_msg):
            self._session.check_read_args(1, 2)

        err_msg = 'Specified a negative run time or period'
        with self.assertRaisesRegex(RuntimeError, err_msg):
            self._session.check_read_args(-1, -1)

        err_msg = 'Specified a negative run time or period'
        with self.assertRaisesRegex(RuntimeError, err_msg):
            self._session.check_read_args(1, -1)

        self._session.check_read_args(1, 1)

    def test_check_write_args(self):
        err_msg = 'When opening a write mode session, a time greater than zero must be specified'
        with self.assertRaisesRegex(RuntimeError, err_msg):
            self._session.check_write_args(-1, 2)

        err_msg = 'Cannot specify period with write mode session'
        with self.assertRaisesRegex(RuntimeError, err_msg):
            self._session.check_write_args(1, 2)

        self._session.check_write_args(10, 0)

    def test_run(self):
        is_write = True
        runtime = 42
        period = 0
        request_stream = [1, 2, 3, 4, 5]
        out_stream = mock.MagicMock()
        wrq_return_value = [0, 1, 2]
        rrq_return_value = [4, 5, 6]
        with mock.patch('geopmdpy.session.WriteRequestQueue',
                        return_value=wrq_return_value) as swrq, \
             mock.patch('geopmdpy.session.ReadRequestQueue',
                        return_value=rrq_return_value) as srrq, \
             mock.patch('geopmdpy.session.Session.check_write_args') as scwa, \
             mock.patch('geopmdpy.session.Session.run_write') as srw:
            self._session.run(is_write, runtime, period, request_stream, out_stream)

            swrq.assert_called_once_with(request_stream)
            scwa.assert_called_once_with(runtime, period)
            srw.assert_called_once_with(wrq_return_value, runtime)
            srrq.assert_not_called()

        is_write = False
        period = 7
        with mock.patch('geopmdpy.session.WriteRequestQueue',
                        return_value=wrq_return_value) as swrq, \
             mock.patch('geopmdpy.session.ReadRequestQueue',
                        return_value=rrq_return_value) as srrq, \
             mock.patch('geopmdpy.session.Session.check_read_args') as scra, \
             mock.patch('geopmdpy.session.Session.run_read') as srr:
            self._session.run(is_write, runtime, period, request_stream, out_stream)

            srrq.assert_called_once_with(request_stream, self._geopm_proxy)
            scra.assert_called_once_with(runtime, period)
            srr.assert_called_once_with(rrq_return_value, runtime, period, out_stream)
            swrq.assert_not_called()


if __name__ == '__main__':
    unittest.main()
