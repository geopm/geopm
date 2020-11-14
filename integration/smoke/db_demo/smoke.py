#!/usr/bin/env python

# Database for holding historical data about smoke tests.

import sys
import datetime
import sqlite3
import argparse
import getpass
import mysql.connector


class SQLiteDBConn:
    def __init__(self, filename='smoke_tests.db'):
        # TODO: if file does not exist, prompt for filename
        # use default location.  option to set a path instead?  then they would need to use it every time.
        self.conn = sqlite3.connect(filename)
        self.cursor = self.conn.cursor()
        self.bind_str = '?'
        self.setup_db()

    def setup_db(self):
        cursor = self.conn.cursor()
        cursor.execute('''CREATE TABLE IF NOT EXISTS
                          RunResults (id integer PRIMARY KEY,
                                      jobid integer,
                                      date text,
                                      app text,
                                      experiment_type text,
                                      result text)''')
        self.conn.commit()


class MariaDBConn:
    def __init__(self):
        config = {
            'user': 'diana',
            'database': 'test',
        }
        password = getpass.getpass("Password:")
        self.conn = mysql.connector.connect(password=password, **config)
        self.bind_str = '%s'


class SmokeTestDB:
    def __init__(self, conn):
        self.conn = conn.conn
        self.cursor = self.conn.cursor()
        self.bind_str = conn.bind_str

    def add_run_test_result(self, jobid, app_name, exp_type, result):
        timestamp = datetime.datetime.now()
        sql = 'INSERT INTO RunResults (jobid, date, app, experiment_type, result) VALUES ({b},{b},{b},{b},{b})'.format(b=self.bind_str)
        self.cursor.execute(sql, (jobid, timestamp, app_name, exp_type, result))
        self.conn.commit()


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
