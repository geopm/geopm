#  Copyright (c) 2015 - 2024, Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#

# Example client of the geopm runtime service.

import sys
import grpc
import sqlite3
import random
from argparse import ArgumentParser
from geopmdpy.loop import TimedLoop
from . import __version__
from . import geopm_runtime_pb2_grpc
from . import geopm_runtime_pb2

def run(server_address, db_path, duration, agent, profile, sample_period, report_period):
    # TODO check for valid input!
    run_id = random.randint(0, 0x7FFFFFFF) << 32 # Randomize top 31 bits
    params_id = run_id
    policy_id = run_id
    report_id = run_id
    db_con = sqlite3.connect(db_path)
    db_cur = db_con.cursor()
    try:
        db_cur.execute("CREATE TABLE report(report_id, host, begin_sec, begin_nsec, end_sec, end_nsec, policy_id)")
        db_cur.execute("CREATE TABLE stats(report_id, name, count, first, last, min, max, mean, std)")
        db_cur.execute("CREATE TABLE policy(policy_id, agent, period, profile, params_id)")
        db_con.commit()
    except sqlite3.OperationalError as ex:
        if str(ex) == 'table report already exists':
            pass
        else:
            raise
    with grpc.insecure_channel(server_address) as channel:
        rt_proxy = geopm_runtime_pb2_grpc.GEOPMRuntimeStub(channel)
        db_cur.execute("INSERT INTO policy VALUES(?,?,?,?,?)",
                       (policy_id, agent, sample_period, profile, params_id))
        policy = geopm_runtime_pb2.Policy(agent=agent,
                                          period=sample_period,
                                          profile=profile,
                                          params=dict())
        rt_proxy.SetPolicy(policy)
        try:
            request = geopm_runtime_pb2.ReportRequest()
            if duration == 0 or duration is None:
                num_period=None
            else:
                num_period = int(duration / report_period)
            report_command = "INSERT INTO report VALUES(?,?,?,?,?,?,?)"
            stats_command = "INSERT INTO stats VALUES(?,?,?,?,?,?,?,?,?)"
            for loop_idx in TimedLoop(report_period, num_period):
                if loop_idx == 0:
                    continue
                report_list = rt_proxy.GetReport(request)
                report_values = []
                stats_values = []
                for report in report_list.list:
                    report_values.append((report_id, report.host.url, report.begin.sec, report.begin.nsec, report.end.sec, report.end.nsec, policy_id))
                    for stats in report.stats:
                        stats_values.append((report_id, stats.name, stats.count, stats.first, stats.last, stats.min, stats.max, stats.mean, stats.std))
                    report_id += 1
                db_cur.executemany(stats_command, stats_values)
                db_cur.executemany(report_command, report_values)
                db_con.commit()
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
    parser.add_argument('-d', '--database', type=str, default='geopmrtd.sqlite3',
                        help='path to sqlite3 database file where records will be stored')
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
    args = parser.parse_args()
    if (args.version):
        print(verion_str)
        return 0
    try:
        run(server_address=args.runtime_server,
            db_path=args.database,
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
