#  Copyright (c) 2015 - 2024, Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#

import sys
import grpc
from geopmdpy.loop import TimedLoop
from . import geopm_runtime_pb2_grpc
from . import geopm_runtime_pb2

def run(server_address, duration, agent):
    fast_period = 0.01
    slow_period = 1
    with grpc.insecure_channel(server_address) as channel:
        rt_proxy = geopm_runtime_pb2_grpc.GEOPMRuntimeStub(channel)
        policy = geopm_runtime_pb2.Policy(agent=agent,
                                          period=fast_period,
                                          profile='',
                                          params=dict())
        rt_proxy.SetPolicy(policy)
        request = geopm_runtime_pb2.ReportRequest()
        num_period = int(duration / slow_period)
        for report_idx in TimedLoop(slow_period, num_period):
            report = rt_proxy.GetReport(request)
            print(f'Report {report_idx}:\n{report}\n')


def main():
    usage = f'{sys.argv[0]} SERVER_ADDRESS DURATION AGENT'
    if len(sys.argv) != 4:
        print(usage)
    else:
        server_address = sys.argv[1]
        duration = float(sys.argv[2])
        agent = sys.argv[3]
        run(server_address, duration, agent)
    return 0
