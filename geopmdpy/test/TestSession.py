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
import signal
from contextlib import contextmanager

# Patch dlopen to allow the tests to run when there is no build
with mock.patch('cffi.FFI.dlopen', return_value=mock.MagicMock()):
    from geopmdpy.session import Session
    from geopmdpy.session import RequestQueue
    from geopmdpy.session import ReadRequestQueue
    from geopmdpy.session import Agent

class DummyAgent(Agent):
    def __init__(self):
        super().__init__()
        self.parser_updated = False
        self.args_updated = False
        self.run_begin_called = False
        self.run_end_called = False
        self.loop_count = 0
        self._trace_header = ['extra_col']
        self._trace_out = ['extra_val']
        self._override = 'signal1 board 0\n'
    def update_parser(self, parser):
        self.parser_updated = True
        parser.add_argument('--dummy', action='store_true')
        return parser
    def update_args(self, args):
        self.args_updated = True
        self._dummy = getattr(args, 'dummy', False)
        return args
    def signal_config_override(self):
        return self._override
    def run_begin(self):
        self.run_begin_called = True
    def run_end(self):
        self.run_end_called = True
    def update_loop(self):
        self.loop_count += 1
    def header_names(self):
        return self._trace_header
    def trace_out(self):
        return self._trace_out

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
            self._session.check_read_args(7 * day, day + 1, None, None, None)

        err_msg = 'Specified a negative run time or period'
        with self.assertRaisesRegex(RuntimeError, err_msg):
            self._session.check_read_args(-1, -1, None, None, None)

        err_msg = 'Specified a negative run time or period'
        with self.assertRaisesRegex(RuntimeError, err_msg):
            self._session.check_read_args(1, -1, None, None, None)

        err_msg = 'Specified report samples is negative'
        with self.assertRaisesRegex(RuntimeError, err_msg):
            self._session.check_read_args(1, 0.01, -10, None, None)

        err_msg = 'Cannot use pid option when launching a command'
        with self.assertRaisesRegex(RuntimeError, err_msg):
            self._session.check_read_args(1, 0.01, 10, 1234, ['sleep', '2'])

        self._session.check_read_args(1, 1, None, None, None)
        self._session.check_read_args(1, .01, 10, None, None)

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
            scra.assert_called_once_with(runtime, period, None, None, None)
            srr.assert_called_once_with(rrq_return_value, runtime, period, None, out_stream, None, None, None, None, None)

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
            scra.assert_called_once_with(runtime, period, None, None, None)
            srr.assert_called_once_with(rrq_return_value, runtime, period, None, out_stream, None, None, None, None, None)
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

    def test_agent_update_parser_and_args(self):
        agent = DummyAgent()
        parser = mock.MagicMock()
        parser.add_argument = mock.MagicMock(return_value=None)
        updated_parser = agent.update_parser(parser)
        self.assertTrue(agent.parser_updated)
        args = mock.MagicMock()
        args.dummy = True
        updated_args = agent.update_args(args)
        self.assertTrue(agent.args_updated)
        self.assertTrue(agent._dummy)

    def test_agent_signal_config_override(self):
        agent = DummyAgent()
        self.assertEqual(agent.signal_config_override(), 'signal1 board 0\n')

    def test_agent_header_names_and_trace_out(self):
        agent = DummyAgent()
        session = Session(agent=agent)
        # Simulate requests
        requests = [('signal1', 0, 0)]
        headers = session.header_names(requests)
        self.assertIn('extra_col', headers)
        # Simulate trace output
        with mock.patch('geopmdpy.pio.push_signal', return_value=0), \
             mock.patch('geopmdpy.pio.read_batch'), \
             mock.patch('geopmdpy.pio.sample', return_value=1.0), \
             mock.patch('geopmdpy.session.Session.format_signals', return_value='1.0,'):
            out_stream = StringIO()
            session.run_read(mock.MagicMock(__iter__=lambda self: iter(requests), get_formats=lambda: [0]), 1, 1, None, out_stream)
            self.assertIn('extra_val', out_stream.getvalue())

    def test_agent_run_begin_and_end(self):
        agent = DummyAgent()
        session = Session(agent=agent)
        # Patch run_read to avoid actual execution
        with mock.patch.object(session, 'run_read'):
            session.run(1, 1, None, False, request_stream=StringIO('TIME board 0\n'), out_stream=StringIO())
        self.assertTrue(agent.run_begin_called)
        self.assertTrue(agent.run_end_called)

    def test_agent_update_loop_called(self):
        agent = DummyAgent()
        session = Session(agent=agent)
        requests = [('signal1', 0, 0)]
        with mock.patch('geopmdpy.pio.push_signal', return_value=0), \
             mock.patch('geopmdpy.pio.read_batch'), \
             mock.patch('geopmdpy.pio.sample', return_value=1.0), \
             mock.patch('geopmdpy.session.Session.format_signals', return_value='1.0,'):
            out_stream = StringIO()
            session.run_read(mock.MagicMock(__iter__=lambda self: iter(requests), get_formats=lambda: [0]), 2, 1, None, out_stream)
            self.assertGreater(agent.loop_count, 0)

    def test_run_launch_subprocess_success(self):
        """Test that Session.run() launches and monitors a subprocess that exits cleanly."""
        session = Session()
        # Use a simple command that exits with 0
        launch_cmd = ['python3', '-c', 'import time; time.sleep(0.1)']
        with mock.patch('geopmdpy.session.ReadRequestQueue', return_value=[('signal1', 0, 0)]) as srrq, \
             mock.patch('geopmdpy.session.Session.check_read_args'), \
             mock.patch('geopmdpy.session.Session.check_requests'), \
             mock.patch('geopmdpy.session.Session.run_read'), \
             mock.patch('geopmdpy.session._ReportStream'), \
             mock.patch('geopmdpy.session.stats.Collector', return_value=None), \
             mock.patch('geopmdpy.session.nullcontext', side_effect=lambda: contextmanager(lambda: (yield None))()):
            # Should not raise or print warnings for clean exit
            out_stream = StringIO()
            session.run(1, 0, None, False, request_stream=StringIO('signal1 board 0\n'), out_stream=out_stream, launch=launch_cmd)

    def test_run_launch_subprocess_nonzero_exit(self):
        """Test that Session.run() launches a subprocess that exits with nonzero code and prints a warning."""
        session = Session()
        launch_cmd = ['python3', '-c', 'import sys; sys.exit(42)']
        # Patch push_signal/sample/read_batch to avoid real pio errors
        mock_requests = mock.MagicMock()
        mock_requests.__iter__.return_value = [('signal1', 0, 0)]
        mock_requests.get_formats.return_value = [0]
        with mock.patch('geopmdpy.session.ReadRequestQueue', return_value=mock_requests), \
             mock.patch('geopmdpy.pio.push_signal', return_value=0), \
             mock.patch('geopmdpy.pio.read_batch'), \
             mock.patch('geopmdpy.pio.sample', return_value=1.0), \
             mock.patch('geopmdpy.session.Session.check_read_args'), \
             mock.patch('geopmdpy.session.Session.check_requests'), \
             mock.patch('geopmdpy.session._ReportStream'), \
             mock.patch('geopmdpy.session.stats.Collector', return_value=None), \
             mock.patch('geopmdpy.session.nullcontext', side_effect=lambda: contextmanager(lambda: (yield None))()):
            out_stream = StringIO()
            with mock.patch('sys.stderr', new_callable=StringIO) as fake_stderr:
                session.run(1, 0, None, False, request_stream=StringIO('signal1 board 0\n'), out_stream=out_stream, launch=launch_cmd)
                fake_stderr.flush()
                if 'terminated with non-zero return code' not in fake_stderr.getvalue():
                    print("Captured sys.stderr:", repr(fake_stderr.getvalue()))
                self.assertIn('terminated with non-zero return code', fake_stderr.getvalue())

    def test_run_launch_subprocess_sigterm(self):
        """Test that Session.run_read() launches and terminates the subprocess with SIGTERM."""
        session = Session()
        launch_cmd = ['python3', '-c', 'import time; time.sleep(10)']
        requests = [('signal1', 0, 0)]
        # Patch g_session_handler to a mock with set_subprocess and stop
        import geopmdpy.session
        geopmdpy.session.g_session_handler = mock.Mock()
        with mock.patch('geopmdpy.pio.push_signal', return_value=0), \
             mock.patch('geopmdpy.pio.read_batch'), \
             mock.patch('geopmdpy.pio.sample', return_value=1.0), \
             mock.patch('geopmdpy.session.loop.TimedLoop', return_value=[0, 1]), \
             mock.patch('subprocess.Popen') as mock_popen, \
             mock.patch('os.setsid', return_value=None):
            mock_proc = mock.Mock()
            mock_proc.poll.return_value = None
            mock_proc.pid = 12345
            mock_popen.return_value = mock_proc
            out_stream = StringIO()
            # run_read should launch the subprocess and call set_subprocess
            session.run_read(
                mock.MagicMock(__iter__=lambda self: iter(requests), get_formats=lambda: [0]),
                2, 1, None, out_stream, launch=launch_cmd
            )
            geopmdpy.session.g_session_handler.set_subprocess.assert_called_once_with(mock_proc)

if __name__ == '__main__':
    unittest.main()
