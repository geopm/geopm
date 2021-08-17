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

from __future__ import absolute_import

import unittest
from geopmdpy import error


class TestError(unittest.TestCase):
    def test_error_message(self):
        err_msg = error.message(error.ERROR_RUNTIME)
        self.assertTrue('runtime' in err_msg.lower())
        err_msg = error.message(error.ERROR_INVALID)
        self.assertTrue('invalid' in err_msg.lower())
        err_msg = error.message(error.ERROR_FILE_PARSE)
        self.assertTrue('parse' in err_msg.lower())
        err_msg = error.message(error.ERROR_LEVEL_RANGE)
        self.assertTrue('level' in err_msg.lower())
        err_msg = error.message(error.ERROR_NOT_IMPLEMENTED)
        self.assertTrue('implemented' in err_msg.lower())
        err_msg = error.message(error.ERROR_PLATFORM_UNSUPPORTED)
        self.assertTrue('not supported' in err_msg.lower())
        err_msg = error.message(error.ERROR_MSR_OPEN)
        self.assertTrue('open' in err_msg.lower())
        self.assertTrue('msr' in err_msg.lower())
        err_msg = error.message(error.ERROR_MSR_READ)
        self.assertTrue('read' in err_msg.lower())
        self.assertTrue('msr' in err_msg.lower())
        err_msg = error.message(error.ERROR_MSR_WRITE)
        self.assertTrue('write' in err_msg.lower())
        self.assertTrue('msr' in err_msg.lower())
        err_msg = error.message(error.ERROR_AGENT_UNSUPPORTED)
        self.assertTrue('agent' in err_msg.lower())
        self.assertTrue('not supported' in err_msg.lower())
        err_msg = error.message(error.ERROR_AFFINITY)
        self.assertTrue('affinitized' in err_msg.lower())
        err_msg = error.message(error.ERROR_NO_AGENT)
        self.assertTrue('agent' in err_msg.lower())

if __name__ == '__main__':
    unittest.main()
