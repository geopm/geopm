#  Copyright (c) 2015 - 2023, Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#

import geopmpy.reporter
import geopmdpy.pio
import time

geopmpy.reporter.init()
geopmdpy.pio.read_batch()
geopmpy.reporter.update()
time.sleep(1)
geopmdpy.pio.read_batch()
geopmpy.reporter.update()
report = geopmpy.reporter.generate("profile_hello", "agent_hello")
print(report)
