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

from unittest import mock
import sys
import unittest
import xml.etree.ElementTree as ET
from geopmdpy import dbus_xml
with mock.patch('cffi.FFI.dlopen', return_value=mock.MagicMock()):
    from geopmdpy import service

class TestDBusXML(unittest.TestCase):
    def setUp(self):
        self._test_name = 'TestDBusXML'

    def tearDown(self):
        pass

    def test_xml_parse_no_doc(self):
        xml = dbus_xml.geopm_dbus_xml()
        try:
            ET.fromstring(xml)
        except (Exception) as ex:
            sys.stderr.write('Error: Failed to parse GEOPM DBus XML:\n\n')
            sys.stderr.write('{}\n\n'.format(xml))
            raise ex

    def test_xml_parse_with_doc(self):
        xml = dbus_xml.geopm_dbus_xml(service.TopoService, service.PlatformService)
        try:
            ET.fromstring(xml)
        except (Exception) as ex:
            sys.stderr.write('Error: Failed to parse GEOPM DBus XML:\n\n')
            sys.stderr.write('{}\n\n'.format(xml))
            raise ex

if __name__ == '__main__':
    unittest.main()
