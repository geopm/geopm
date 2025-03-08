#!/usr/bin/env python3
#
#  Copyright (c) 2015 - 2025 Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#

import os
import unittest
from unittest import mock
from io import StringIO
from dasbus.error import DBusError
import errno
import itertools

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
        signal_expect = (1 + num_period) * [1.234, 2.345, 3.456]

        with mock.patch('geopmdpy.loop.TimedLoop', return_value=[0, 1]) as mock_timed_loop, \
             mock.patch('geopmdpy.pio.push_signal', side_effect=signal_handle) as mock_push_signal, \
             mock.patch('geopmdpy.pio.read_batch') as mock_read_batch, \
             mock.patch('geopmdpy.pio.sample', side_effect=signal_expect) as mock_sample, \
             mock.patch('geopmdpy.session.Session.format_signals', return_value=format_return_value):
            # Call tested method
            self._session.run_read(mock_requests, duration, period, pid=None, out_stream=out_stream)
            # Check mocks calls
            calls = [mock.call(*req) for req in pio_requests]
            mock_push_signal.assert_has_calls(calls)
            calls = num_period * [mock.call()]
            mock_read_batch.assert_has_calls(calls)
            calls = num_period * [mock.call(idx) for idx in signal_handle]
            mock_sample.assert_has_calls(calls)
            calls = num_period * [mock.call(format_return_value)]
            out_stream.write.assert_has_calls(calls)

        # Check that resample occurs when first returned value is nan
        signal_expect[1] = float('nan')
        with mock.patch('geopmdpy.loop.TimedLoop', return_value=[0, 1]) as mock_timed_loop, \
             mock.patch('geopmdpy.pio.push_signal', side_effect=signal_handle) as mock_push_signal, \
             mock.patch('geopmdpy.pio.read_batch') as mock_read_batch, \
             mock.patch('geopmdpy.pio.sample', side_effect=signal_expect) as mock_sample, \
             mock.patch('geopmdpy.session.Session.format_signals', return_value=format_return_value):
            # Call tested method
            self._session.run_read(mock_requests, duration, period, pid=None, out_stream=out_stream)
            # Check mocks calls
            calls = [mock.call(*req) for req in pio_requests]
            mock_push_signal.assert_has_calls(calls)
            calls = (1 + num_period) * [mock.call()]
            mock_read_batch.assert_has_calls(calls)
            calls = num_period * [mock.call(idx) for idx in signal_handle]
            mock_sample.assert_has_calls(calls)
            calls = num_period * [mock.call(format_return_value)]
            out_stream.write.assert_has_calls(calls)

    def test_run_read_pid_exit(self):
        """Geopmsession exits before all loops have finished if watching a terminated PID."""
        duration = 10
        period = 1
        termination_time = 2
        out_stream = mock.MagicMock()

        requests = [('power', 0, 0), ('SERVICE::energy', 1, 1), ('frequency', 2, 2)]
        mock_requests = mock.MagicMock()
        mock_requests.__iter__.return_value = requests
        format_return_value = "1.234, 2.345, 3.456"
        mock_requests.get_formats.return_value = format_return_value
        signal_handle = list(range(len(requests)))
        signal_expect = itertools.cycle([1.234, 2.345, 3.456])

        with mock.patch('geopmdpy.loop.TimedLoop', return_value=list(range(duration))), \
             mock.patch('geopmdpy.pio.push_signal', side_effect=signal_handle), \
             mock.patch('geopmdpy.pio.read_batch'), \
             mock.patch('geopmdpy.pio.sample', side_effect=signal_expect), \
             mock.patch('geopmdpy.session.os.kill', side_effect=[0, 0, OSError(errno.ESRCH, 'Fault Injection')]), \
             mock.patch('geopmdpy.session.Session.format_signals', return_value=format_return_value):
            self._session.run_read(mock_requests, duration, period, pid=12345, out_stream=out_stream)

            # Output should be limited by the shorter PID termination_time
            # instead of being limited by the longer loop duration
            calls = termination_time * [mock.call(format_return_value)]
            out_stream.write.assert_has_calls(calls)

    def test_run_read_pid_perm(self):
        """Geopmsession can wait for PID without kill permissions."""
        duration = 10
        period = 1
        out_stream = mock.MagicMock()

        requests = [('power', 0, 0), ('SERVICE::energy', 1, 1), ('frequency', 2, 2)]
        mock_requests = mock.MagicMock()
        mock_requests.__iter__.return_value = requests
        format_return_value = "1.234, 2.345, 3.456"
        mock_requests.get_formats.return_value = format_return_value
        signal_handle = list(range(len(requests)))
        signal_expect = itertools.cycle([1.234, 2.345, 3.456])

        with mock.patch('geopmdpy.loop.TimedLoop', return_value=list(range(duration))), \
             mock.patch('geopmdpy.pio.push_signal', side_effect=signal_handle), \
             mock.patch('geopmdpy.pio.read_batch'), \
             mock.patch('geopmdpy.pio.sample', side_effect=signal_expect), \
             mock.patch('geopmdpy.session.os.kill', side_effect=itertools.repeat(OSError(errno.EPERM, 'Fault Injection'))), \
             mock.patch('geopmdpy.session.Session.format_signals', return_value=format_return_value):
            self._session.run_read(mock_requests, duration, period, pid=12345, out_stream=out_stream)

            # Output should be limited by the loop duration since the PID
            # exists (albeit without kill permission) the whole time.
            calls = duration * [mock.call(format_return_value)]
            out_stream.write.assert_has_calls(calls)

    def test_check_read_args(self):
        day = 24 * 60 * 60
        err_msg = 'Specified a period greater than 24 hours'
        with self.assertRaisesRegex(RuntimeError, err_msg):
            self._session.check_read_args(7 * day, day + 1, None, None)

        err_msg = 'Specified a negative run time or period'
        with self.assertRaisesRegex(RuntimeError, err_msg):
            self._session.check_read_args(-1, -1, None, None)

        err_msg = 'Specified a negative run time or period'
        with self.assertRaisesRegex(RuntimeError, err_msg):
            self._session.check_read_args(1, -1, None, None)

        err_msg = 'Specified report samples is negative'
        with self.assertRaisesRegex(RuntimeError, err_msg):
            self._session.check_read_args(1, 0.01, -10, None)

        self._session.check_read_args(1, 1, None, None)
        self._session.check_read_args(1, .01, 10, None)

    def test_run(self):
        period = 7
        runtime = 42
        period = 0
        request_stream = [1, 2, 3, 4, 5]
        out_stream = mock.MagicMock()
        rrq_return_value = [('signal1', 'board', 0), ('signal2', 'package', 1)]
        with mock.patch('geopmdpy.session.ReadRequestQueue',
                        return_value=rrq_return_value) as srrq, \
             mock.patch('geopmdpy.session.Session.check_read_args') as scra, \
             mock.patch('geopmdpy.session.Session.run_read') as srr, \
             mock.patch('geopmdpy.topo.num_domain', side_effect=lambda x: {'board': 1, 'package': 2}.get(x)), \
             mock.patch('geopmdpy.pio.signal_domain_type',
                        side_effect=lambda lookup_name: next(domain for name, domain, idx in rrq_return_value if name == lookup_name)):
            self._session.run(runtime, period, None, False, request_stream, out_stream)

            srrq.assert_called_once_with(request_stream)
            scra.assert_called_once_with(runtime, period, None, None)
            srr.assert_called_once_with(rrq_return_value, runtime, period, None, out_stream, None, None, None)

    def test_run_with_header(self):
        """geopmsession prints signal-domain-idx header fields."""
        period = 7
        runtime = 42
        period = 0
        request_stream = [1, 2, 3, 4, 5]
        out_stream = StringIO()
        rrq_return_value = [('signal1', 'board', 0), ('signal2', 'package', 1)]
        with mock.patch('geopmdpy.session.ReadRequestQueue',
                        return_value=rrq_return_value) as srrq, \
             mock.patch('geopmdpy.session.Session.check_read_args') as scra, \
             mock.patch('geopmdpy.topo.domain_name', side_effect=lambda x: x), \
             mock.patch('geopmdpy.session.Session.run_read') as srr, \
             mock.patch('geopmdpy.topo.num_domain', side_effect=lambda x: {'board': 1, 'package': 2}.get(x)), \
             mock.patch('geopmdpy.pio.signal_domain_type',
                        side_effect=lambda lookup_name: next(domain for name, domain, idx in rrq_return_value if name == lookup_name)):
            self._session.run(runtime, period, None, True, request_stream, out_stream)

            srrq.assert_called_once_with(request_stream)
            scra.assert_called_once_with(runtime, period, None, None)
            srr.assert_called_once_with(rrq_return_value, runtime, period, None, out_stream, None, None, None)
        self.assertEqual('"signal1","signal2-package-1"\n', out_stream.getvalue())

    def test_run_with_bad_request(self):
        period = 7
        runtime = 42
        period = 0
        request_stream = [1, 2, 3, 4, 5]
        out_stream = mock.MagicMock()
        with mock.patch('geopmdpy.session.ReadRequestQueue',
                        return_value=[('signal', 'package', 1)]), \
             mock.patch('geopmdpy.session.Session.check_read_args'), \
             mock.patch('geopmdpy.session.Session.run_read'), \
             mock.patch('geopmdpy.pio.signal_domain_type', return_value='board'):
            # Invalid due to bad domain
            self.assertRaises(ValueError, self._session.run,
                              runtime, period, None, False,
                              request_stream=request_stream, out_stream=out_stream)

        with mock.patch('geopmdpy.session.ReadRequestQueue',
                        return_value=[('signal', 'package', -1)]), \
             mock.patch('geopmdpy.session.Session.check_read_args'), \
             mock.patch('geopmdpy.session.Session.run_read'), \
             mock.patch('geopmdpy.pio.signal_domain_type', return_value='package'):
            # Invalid due to bad index
            self.assertRaises(ValueError, self._session.run,
                              runtime, period, None, False,
                              request_stream=request_stream, out_stream=out_stream)


if __name__ == '__main__':
    unittest.main()
