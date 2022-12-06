#!/usr/bin/env python3

#  Copyright (c) 2015 - 2022, Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#

import unittest

from TestAccess import *
from TestAccessLists import *
from TestActiveSessions import *
from TestController import *
from TestDBusXML import *
from TestError import *
from TestMSRDataFiles import *
from TestPIO import *
from TestPlatformService import *
from TestRequestQueue import *
from TestSecureFiles import *
from TestSession import *
from TestTimedLoop import *
from TestTopo import *
from TestWriteLock import *


if __name__ == '__main__':
    unittest.main()
