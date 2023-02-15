#!/usr/bin/env python3
#
#  Copyright (c) 2015 - 2023, Intel Corporation
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
        self._geopm_proxy = mock.MagicMock()

    def tearDown(self):
        self._geopm_proxy.reset_mock(return_value=True, side_effect=True)

    def test_request_queue_invalid(self):
        err_msg = 'RequestQueue class is an abstract base class for ReadRequestQueue'
        with self.assertRaisesRegex(RuntimeError, err_msg):
            RequestQueue()

    def test_read_request_queue_invalid(self):
        err_msg = 'Empty request stream.'
        with self.assertRaisesRegex(RuntimeError, err_msg):
            rrq = ReadRequestQueue(StringIO(''), None)

        data = 'not_enough_data'
        err_msg = 'Read request must be three words: "{}"'.format(data)
        with self.assertRaisesRegex(RuntimeError, err_msg):
            rrq = ReadRequestQueue(StringIO(data), None)

        data = 'Bad data provided'
        err_msg = 'Unable to convert values into a read request: "{}"'.format(data)
        with mock.patch('geopmdpy.topo.domain_type', return_value = 42), \
             self.assertRaisesRegex(RuntimeError, err_msg):
            rrq = ReadRequestQueue(StringIO(data), None)

        data = 'Bad data 123'
        err_msg = 'Unable to convert values into a read request: "{}"'.format(data)
        with mock.patch('geopmdpy.topo.domain_type', side_effect = ValueError('bad')), \
             self.assertRaisesRegex(RuntimeError, err_msg):
            rrq = ReadRequestQueue(StringIO(data), None)

        dbus_err_msg = 'io.github.geopm was not provided'
        self._geopm_proxy.PlatformGetSignalInfo.side_effect = DBusError(dbus_err_msg)
        data = 'Bogus data 123'
        err_msg = """The geopm systemd service is not enabled.
    Install geopm service and run 'systemctl start geopm'"""
        with mock.patch('geopmdpy.topo.domain_type', return_value = 42), \
             self.assertRaisesRegex(RuntimeError, err_msg):
            rrq = ReadRequestQueue(StringIO(data), self._geopm_proxy)

        dbus_err_msg = "Other DBusError"
        self._geopm_proxy.PlatformGetSignalInfo.side_effect = DBusError(dbus_err_msg)
        with mock.patch('geopmdpy.topo.domain_type', return_value = 42), \
             self.assertRaisesRegex(DBusError, dbus_err_msg):
            rrq = ReadRequestQueue(StringIO(data), self._geopm_proxy)

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

        pgsi_return_value = [1, 2, 3, 4, 5]
        self._geopm_proxy.PlatformGetSignalInfo.return_value = [pgsi_return_value] * len(requests)
        formats = [pgsi_return_value[4]] * len(requests)

        with mock.patch('geopmdpy.topo.domain_type', return_value = 42) as pdt:
            rrq = ReadRequestQueue(request_stream, self._geopm_proxy)

            self.assertEqual(requests, rrq._requests)
            self.assertEqual(formats, rrq.get_formats())
            self._geopm_proxy.PlatformGetSignalInfo.assert_has_calls([mock.call(names)])

            parsed_reqs = []
            for req in rrq:
                parsed_reqs += [req]
            self.assertEqual(requests, parsed_reqs)


if __name__ == '__main__':
    unittest.main()
