#!/usr/bin/env python3
#
#  Copyright (c) 2015 - 2025 Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#

from unittest import mock
import sys
import unittest
from defusedxml import ElementTree as ET
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
