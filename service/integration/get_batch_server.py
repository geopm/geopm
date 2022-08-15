#!/usr/bin/env python3
#
#  Copyright (c) 2015 - 2022, Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#

import json
import sys
import psutil
import os

if len(sys.argv) < 2:
     print(f'Usage: {sys.argv[0]} SESSION_PID')
     exit(-1)
if os.getuid() != 0:
     raise RuntimeError(f'Script must be run as root user')
session_pid=int(sys.argv[1])
if session_pid < 0 or not psutil.pid_exists(session_pid):
     raise RuntimeError(f'{session_pid} is not a valid PID')
with open(f'/run/geopm-service/session-{session_pid}.json') as fid:
     server_pid = json.loads(fid.read()).get('batch_server')
if server_pid is not None:
     print(server_pid)
