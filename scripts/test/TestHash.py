#!/usr/bin/env python
#
#  Copyright (c) 2015, 2016, 2017, 2018, 2019, Intel Corporation
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
import mock
import geopm_context
from geopmpy.hash import Hash

class TestHash(unittest.TestCase):
    def test_string_key(self):
        """Test that a hash can be created from a string key.
        """
        test_hash = Hash.from_string_key("Here, have a string!")
        self.assertEqual(0x30b63125, int(test_hash))

    def test_int_key(self):
        """Test that a hash can be created from an int key.
        """
        test_hash = Hash.from_int_key(0xdeadbeef, 0xbadfee)
        self.assertEqual(0xa347ade3, int(test_hash))

    def test_representations(self):
        """Test that a hash correctly converts between representations.
        """
        test_hash = Hash(0x30b63125)
        self.assertEqual("0x0000000030b63125", str(test_hash))
        self.assertEqual(test_hash, eval(repr(test_hash)))
        self.assertEqual(test_hash, Hash(int(test_hash)))
        self.assertEqual(test_hash, Hash.from_signal(float(test_hash)))
        self.assertNotEqual(test_hash, Hash(int(test_hash) + 1))
        self.assertEqual(0xffffffff, int(Hash(0xffffffff)))

    def test_invalid(self):
        """Test that a hash rejects an invalid value.
        """
        with self.assertRaises(ValueError):
            Hash(0x100000000)

if __name__ == '__main__':
    unittest.main()
