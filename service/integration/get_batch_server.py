#
#  Copyright (c) 2015 - 2022, Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#

import json
import sys

session_pid=sys.argv[1]
with open(f'/run/geopm-service/session-{session_pid}.json') as fid:
     print(json.loads(fid.read())['batch_server'])

