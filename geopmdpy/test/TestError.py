#!/usr/bin/env python3
#
#  Copyright (c) 2015 - 2025 Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#


import unittest
from geopmdpy import error
from geopmdpy import gffi


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
