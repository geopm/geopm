#!/usr/bin/env python
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

import sys
import os
import unittest

from test_omp_outer_loop import *
from test_enforce_policy import *
from test_profile_policy import *
from test_plugin_static_policy import *
from test_tutorial_base import *
from test_frequency_hint_usage import *
from test_profile_overflow import *
from test_trace import *
from test_monitor import *
from test_geopmio import *
from test_ompt import *
from test_launch_application import *
from test_launch_pthread import *
from test_geopmagent import *
from test_environment import *
from test_frequency_map import *
from test_hint_time import *
from test_progress import *

if 'GEOPM_RUN_LONG_TESTS' in os.environ:
    from test_ee_timed_scaling_mix import *
    from test_power_balancer import *
    from test_power_governor import *
    from test_scaling_region import *
    from test_timed_scaling_region import *
else:
    skipped_modules = ['test_ee_timed_scaling_mix',
                       'test_power_balancer',
                       'test_power_governor',
                       'test_scaling_region',
                       'test_timed_scaling_region',
                       ]
    for sm in skipped_modules:
        sys.stderr.write("* ({}.*) ... skipped 'Requires GEOPM_RUN_LONG_TESTS environment variable'\n".format(sm))

if __name__ == '__main__':
    unittest.main()
