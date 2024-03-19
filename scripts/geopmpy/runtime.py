#  Copyright (c) 2015 - 2024, Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#

# Example client of the geopm runtime service.
#
# Starts an agent on an endpoint, gathers reports for a user specified
# period of time and stores the report data in an sqlite3 database.
#

import sys
import grpc
import sqlite3
from geopmdpy.loop import TimedLoop
from . import geopm_runtime_pb2_grpc
from . import geopm_runtime_pb2

def run(server_address, db_path, duration, agent):
    fast_period = 0.01
    slow_period = 1
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
        params_id = 0
        policy_id = 0
        profile = ''
        db_cur.execute("INSERT INTO policy VALUES(?,?,?,?,?)",
                       (policy_id, agent, fast_period, profile, params_id))
        policy = geopm_runtime_pb2.Policy(agent=agent,
                                          period=fast_period,
                                          profile='',
                                          params=dict())
        rt_proxy.SetPolicy(policy)
        request = geopm_runtime_pb2.ReportRequest()
        num_period = int(duration / slow_period)
        for report_idx in TimedLoop(slow_period, num_period):
            report_list = rt_proxy.GetReport(request)
            report_command = "INSERT INTO report VALUES(?,?,?,?,?,?,?)"
            report_values = []
            stats_command = "INSERT INTO stats VALUES(?,?,?,?,?,?,?,?,?)"
            stats_values = []
            for report in report_list.list:
                report_values.append((report_idx, report.host.url, report.begin.sec, report.begin.nsec, report.end.sec, report.end.nsec, policy_id))
                for stats in report.stats:
                    stats_values.append((report_idx, stats.name, stats.count, stats.first, stats.last, stats.min, stats.max, stats.mean, stats.std))
            db_cur.executemany(stats_command, stats_values)
            db_cur.executemany(report_command, report_values)
            db_con.commit()

def main():
    usage = f'{sys.argv[0]} SERVER_ADDRESS DB_PATH DURATION AGENT'
    if len(sys.argv) != 5:
        print(usage)
    else:
        server_address = sys.argv[1]
        db_path = sys.argv[2]
        duration = float(sys.argv[3])
        agent = sys.argv[4]
        run(server_address, db_path, duration, agent)
    return 0
