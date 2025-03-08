#!/usr/bin/env python3
#
#  Copyright (c) 2015 - 2025 Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#

import unittest
from unittest import mock
from io import StringIO
from dasbus.error import DBusError

# Patch dlopen to allow the tests to run when there is no build
with mock.patch('cffi.FFI.dlopen', return_value=mock.MagicMock()):
    from geopmdpy.session import RequestQueue
    from geopmdpy.session import ReadRequestQueue

class TestRequestQueue(unittest.TestCase):
    def setUp(self):
        self._test_name = 'TestRequestQueue'

    def test_request_queue_invalid(self):
        err_msg = 'RequestQueue class is an abstract base class for ReadRequestQueue'
        with self.assertRaisesRegex(RuntimeError, err_msg):
            RequestQueue()

    def test_read_request_queue_invalid(self):
        err_msg = 'Empty request stream.'
        with self.assertRaisesRegex(RuntimeError, err_msg):
            rrq = ReadRequestQueue(StringIO(''))

        data = 'not_enough_data'
        err_msg = 'Read request must be three words: "{}"'.format(data)
        with self.assertRaisesRegex(RuntimeError, err_msg):
            rrq = ReadRequestQueue(StringIO(data))

        data = 'Bad data provided'
        err_msg = 'Unable to convert values into a read request: "{}"'.format(data)
        with mock.patch('geopmdpy.topo.domain_type', return_value = 42), \
             self.assertRaisesRegex(RuntimeError, err_msg):
            rrq = ReadRequestQueue(StringIO(data))

        data = 'Bad data 123'
        err_msg = 'Unable to convert values into a read request: "{}"'.format(data)
        with mock.patch('geopmdpy.topo.domain_type', side_effect = ValueError('bad')), \
             self.assertRaisesRegex(RuntimeError, err_msg):
            rrq = ReadRequestQueue(StringIO(data))

    def test_read_request_queue(self):
        power_req = ('power', 42, 0)
        energy_req = ('energy', 42, 1)
        frequency_req = ('frequency', 42, 2)
        requests = [power_req, energy_req, frequency_req]
        names = [power_req[0], energy_req[0], frequency_req[0]]

        request_stream = ''
        for rr in requests:
            request_stream += ' '.join(map(str, rr)) + '\n'
        request_stream = StringIO(request_stream)

        psi_return_value = [1, 2, 3]

        formats = [psi_return_value[1]] * len(requests)

        with mock.patch('geopmdpy.topo.domain_type', return_value = 42) as pdt, \
             mock.patch('geopmdpy.pio.signal_info', return_value = psi_return_value) as piosi:
            rrq = ReadRequestQueue(request_stream)

            self.assertEqual(requests, rrq._requests)
            self.assertEqual(formats, rrq.get_formats())
            piosi.assert_has_calls([mock.call(nn) for nn in names])

            parsed_reqs = []
            for req in rrq:
                parsed_reqs += [req]
            self.assertEqual(requests, parsed_reqs)


    def test_read_request_queue_glob(self):
        request_stream = StringIO("TIME cpu *")
        num_cpu = 8
        expected_requests = [("TIME", 3, cpu_idx) for cpu_idx in range(num_cpu)]
        psi_return_value = [1, 2, 3]

        with mock.patch('geopmdpy.topo.domain_type', return_value = 3) as domain_type_mock, \
             mock.patch('geopmdpy.topo.num_domain', return_value = num_cpu) as num_domain_mock, \
             mock.patch('geopmdpy.pio.signal_info', return_value = psi_return_value):
            actual_requests = list(ReadRequestQueue(request_stream))
            self.assertEqual(expected_requests, actual_requests)

if __name__ == '__main__':
    unittest.main()
