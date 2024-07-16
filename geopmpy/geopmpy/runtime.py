#  Copyright (c) 2015 - 2024, Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#

# Example client of the geopm runtime service.

import sys
import grpc
import sqlite3
import random
import os
from argparse import ArgumentParser
from geopmdpy.loop import TimedLoop
from . import __version__
from . import geopm_runtime_pb2_grpc
from . import geopm_runtime_pb2
import requests

def run(geopmrtd_address, controller_address, duration, agent, profile, sample_period, report_period):
    if duration < 0:
        raise RuntimeError('Run duration must not be negative')
    if sample_period > report_period:
        raise RuntimeError('Report period must be longer than the sample period')
    if ':' not in geopmrtd_address:
        raise RuntimeError('Server address must include a port')
    try:
        int(geopmrtd_address.split(':')[1])
    except ValueError:
        raise RuntimeError('Server address port must be an integer')

    with grpc.insecure_channel(geopmrtd_address) as channel:
        rt_proxy = geopm_runtime_pb2_grpc.GEOPMRuntimeStub(channel)
        # TODO: get policy: (agent, sample_period, profile, params)
        controller_response = requests.get(f'{controller_address}/policy')
        controller_policy = controller_response.json()
        policy = geopm_runtime_pb2.Policy(agent=controller_policy['agent'],
                                          period=controller_policy['sample_period'],
                                          profile=controller_policy['profile'],
                                          params=controller_policy['params'])
        rt_proxy.SetPolicy(policy)
        try:
            request = geopm_runtime_pb2.ReportRequest()
            if duration == 0 or duration is None:
                num_period=None
            else:
                num_period = int(duration / report_period)
            timed_loop = iter(TimedLoop(report_period, num_period))
            next(timed_loop)
            for loop_idx in timed_loop:
                report_list = rt_proxy.GetReport(request)
                reports = [
                    {
                        'host_url': report.host.url,
                        'begin_sec': report.begin.sec + report.begin.nsec / 1e9,
                        'end_sec': report.end.sec + report.end.nsec / 1e9,
                        'stats': [
                            {'name': stats.name,
                             'count': stats.count,
                             'first': stats.first,
                             'last': stats.last,
                             'min': stats.min,
                             'max': stats.max,
                             'mean': stats.mean,
                             'std': stats.std
                             }
                            for stats in report.stats
                        ]
                    }
                    for report in report_list.list]
                # TODO: notify reports to network endpoint
                controller_response = requests.post(
                    f'{controller_address}/reports',
                    json=reports,
                )
        finally:
            policy = geopm_runtime_pb2.Policy(agent='',
                                              period=sample_period,
                                              profile='',
                                              params=dict())
            rt_proxy.SetPolicy(policy)

def main():
    '''GEOPM Runtime client command line tool

    Starts an agent on an endpoint, gathers reports for a user specified
    period of time and stores the report data in an sqlite3 database.
    '''
    version_str = f"""\
GEOPM version {__version__}
Copyright (c) 2015 - 2024, Intel Corporation. All rights reserved.
"""


    err = 0
    parser = ArgumentParser(description=main.__doc__)
    parser.add_argument('-v', '--version', action='store_true', default=False,
                        help='show version information and exit')
    parser.add_argument('-s', '--runtime-server', type=str, default='localhost:8080',
                        help='server running leading geopmrtd daemon')
    parser.add_argument('-t', '--time', type=float, default=0.0,
                        help='total run time of the session to be opened in seconds, if not specified run until signaled')
    parser.add_argument('-p', '--sample-period', type=float, default=0.01,
                        help='run remote sampling periodically with the specified period in seconds')
    parser.add_argument('-r', '--report-period', type=float, default=1,
                        help='collect reports from remote systems periodically with the specified period in seconds')
    parser.add_argument('-a', '--agent', type=str, default='monitor',
                        help='name of agent algorithm to run on remote system')
    parser.add_argument('-n', '--profile', type=str, default='',
                        help='name stored with records in database that can be used for selection')
    parser.add_argument('controller_address',
                        help='Server listening for geopmrt messages. E.g., localhost:5000.')
    args = parser.parse_args()
    if (args.version):
        print(verion_str)
        return 0
    try:
        run(geopmrtd_address=args.runtime_server,
            controller_address=args.controller_address,
            duration=args.time,
            agent=args.agent,
            profile=args.profile,
            sample_period=args.sample_period,
            report_period=args.report_period)
    except RuntimeError as ee:
        if 'GEOPM_DEBUG' in os.environ:
            # Do not handle exception if GEOPM_DEBUG is set
            raise
        sys.stderr.write(f'Error: {ee}\n\n')
        err = -1
    return err
