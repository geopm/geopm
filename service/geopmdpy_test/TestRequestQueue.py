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
from unittest import mock
from io import StringIO
from dasbus.error import DBusError

# Patch dlopen to allow the tests to run when there is no build
with mock.patch('cffi.FFI.dlopen', return_value=mock.MagicMock()):
    from geopmdpy.session import RequestQueue
    from geopmdpy.session import ReadRequestQueue
    from geopmdpy.session import WriteRequestQueue

class TestRequestQueue(unittest.TestCase):
    def setUp(self):
        self._test_name = 'TestRequestQueue'
        self._geopm_proxy = mock.MagicMock()

    def tearDown(self):
        self._geopm_proxy.reset_mock(return_value=True, side_effect=True)

    def test_request_queue_invalid(self):
        err_msg = 'RequestQueue class is an abstract base class for ReadRequestQueue and WriteRequestQueue'
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

    def test_write_request_queue_invalid(self):
        err_msg = 'Empty request stream.'
        with self.assertRaisesRegex(RuntimeError, err_msg):
            wrq = WriteRequestQueue(StringIO(''))

        data = 'not_enough_data'
        err_msg = 'Write request must be four words: "{}"'.format(data)
        with self.assertRaisesRegex(RuntimeError, err_msg):
            wrq = WriteRequestQueue(StringIO(data))

        data = 'Bad data provided here'
        err_msg = 'Unable to convert values into a write request: "{}"'.format(data)
        with mock.patch('geopmdpy.topo.domain_type', return_value = 42), \
             self.assertRaisesRegex(RuntimeError, err_msg):
            wrq = WriteRequestQueue(StringIO(data))

        data = 'Bad data 123 456'
        err_msg = 'Unable to convert values into a write request: "{}"'.format(data)
        with mock.patch('geopmdpy.topo.domain_type', side_effect = ValueError('bad')), \
             self.assertRaisesRegex(RuntimeError, err_msg):
            wrq = WriteRequestQueue(StringIO(data))

    def test_write_request_queue(self):
        power_req = ('power', 42, 0, 5.55)
        energy_req = ('energy', 42, 1, 6.66)
        frequency_req = ('frequency', 42, 2, 7.77)
        requests = [power_req, energy_req, frequency_req]
        names = [power_req[0], energy_req[0], frequency_req[0]]

        request_stream = ''
        for rr in requests:
            request_stream += ' '.join(map(str, rr)) + '\n'
        request_stream = StringIO(request_stream)

        with mock.patch('geopmdpy.topo.domain_type', return_value = 42) as pdt:
            wrq = WriteRequestQueue(request_stream)

            self.assertEqual(requests, wrq._requests)
            parsed_reqs = []
            for req in wrq:
                parsed_reqs += [req]
            self.assertEqual(requests, parsed_reqs)


if __name__ == '__main__':
    unittest.main()
