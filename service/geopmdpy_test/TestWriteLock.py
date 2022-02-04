#!/usr/bin/env python3
#
#  Copyright (c) 2015 - 2021, Intel Corporation
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
import tempfile

from geopmdpy.varrun import WriteLock

class TestWriteLock(unittest.TestCase):
    def setUp(self):
        """Create temporary directory

        """
        self._test_name = 'TestWriteLock'
        self._TEMP_DIR = tempfile.TemporaryDirectory(self._test_name)

    def tearDown(self):
        """Clean up temporary directory

        """
        self._TEMP_DIR.cleanup()

    def test_default_creation(self):
        """Default creation of an WriteLock object

        """
        sess_path = f'{self._TEMP_DIR.name}'
        orig_pid = 1234
        other_pid = 4321
        with WriteLock(sess_path) as write_lock:
            out_pid = write_lock.try_lock(orig_pid)
            self.assertEqual(orig_pid, out_pid)
        with WriteLock(sess_path) as other_lock:
            out_pid = other_lock.try_lock(other_pid)
            self.assertEqual(orig_pid, out_pid)
            other_lock.unlock(orig_pid)
            out_pid = other_lock.try_lock(other_pid)
            self.assertEqual(other_pid, out_pid)
            other_lock.unlock(other_pid)
            self.assertIsNone(other_lock.try_lock())

if __name__ == '__main__':
    unittest.main()
