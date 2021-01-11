#!/usr/bin/env python

# Database for holding historical data about smoke tests.

import sys
import datetime
import argparse

import db_conn
import db_tables


if __name__ == '__main__':

    parser = argparse.ArgumentParser()
    parser.add_argument('--app', type=str, required=True,
                        help='application name')
    parser.add_argument('--exp-type', type=str, dest='exp_type', required=True,
                        help='experiment type')
    parser.add_argument('--result', type=str, required=True,
                        help='result of experiment')
    parser.add_argument('--jobid', type=int, required=True,
                        help='job id')
    args = parser.parse_args()

    app_name = args.app
    exp_dir = args.exp_type
    exp_type = args.exp_type
    if exp_type in ['barrier_frequency_sweep', 'power_balancer_energy']:
        exp_dir = 'energy_efficient'
    result = args.result
    jobid = args.jobid

    sys.stdout.write('Result: {} {} {} {}\n'.format(app_name, exp_dir, exp_type, result))

    db_conn = SQLiteDBConn()
    #db_conn = MariaDBConn()
    print('created conn')

    db = SmokeTestDB(db_conn)
    print('created db')

    db.add_run_test_result(jobid=jobid, app_name=app_name,
                           exp_type=exp_type, result=result)

    print('done')
