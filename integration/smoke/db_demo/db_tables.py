#!/usr/bin/env python

import datetime
import sys
import os
import glob
import pandas

import geopmpy.io


class SmokeTestDB:
    def __init__(self, db_conn):
        self.conn = db_conn.conn()
        self.cursor = self.conn.cursor()
        self.bind_str = self.conn.bind_str

    def add_run_test_result(self, jobid, app_name, exp_type, result):
        timestamp = datetime.datetime.now()
        sql = 'INSERT INTO RunResults (jobid, date, app, experiment_type, result) VALUES ({b},{b},{b},{b},{b})'.format(b=self.bind_str)
        self.cursor.execute(sql, (jobid, timestamp, app_name, exp_type, result))

    def setup_run_results(self):
        sql = '''CREATE TABLE IF NOT EXISTS
                     RunResults (id integer PRIMARY KEY,
                                 jobid integer,
                                 date text,
                                 app text,
                                 experiment_type text,
                                 result text)'''
        self.cursor.execute(sql)
        self.conn.commit()

    def show_run_results(self):
        sql = '''SELECT * FROM RunResults'''
        self.conn.execute(sql)


# Note: better to edit these in phpMyAdmin, then update these
# statements to match.  CREATE TABLE here is for the sqlite DB
class ExperimentsDB:
    def __init__(self, db_conn):
        self.conn = db_conn.conn()
        self.cursor = self.conn.cursor()

    def add_policy_db(self, agent, policy_vals):
        # Always create a new policy, even if identical
        # TODO: optimize this by checking for duplicates? assume
        # policy data will never change once inserted
        sql = "INSERT INTO Policies (agent) VALUES (?)"
        self.cursor.execute(sql, (agent,))
        policy_id = self.cursor.lastrowid

        for kk, vv in policy_vals.items():
            # check if existing
            self.cursor.execute("SELECT * FROM PolicyParams WHERE name = ? and value = ?",
                                (kk, vv))
            existing = self.cursor.fetchall()
            if self.cursor.rowcount == 0:
                sql = "INSERT INTO PolicyParams (name, value) VALUES (?, ?)"
                self.cursor.execute(sql, (kk, vv))
                param_id = self.cursor.lastrowid
                print('{} rows updated'.format(self.cursor.rowcount))
            elif self.cursor.rowcount == 1:
                print('Already in db: {}'.format(existing))
                param_id = existing[0][0]
            else:
                sys.stderr.write("Warning: duplicate entries in PolicyParams!\n")

            print("param id: {}".format(param_id))

            # Add to link table
            sql = "INSERT INTO PolicyParamLink (policy_id, param_id) VALUES (?, ?)"
            self.cursor.execute(sql, (policy_id, param_id))
        return policy_id

    def show_policy(self, policy_id):
        sys.stdout.write('policy {}:\n'.format(policy_id))
        sql = '''SELECT name, value FROM PolicyParamLink
                 INNER JOIN Policies ON Policies.id = PolicyParamLink.policy_id
                 INNER JOIN PolicyParams ON PolicyParams.id = PolicyParamLink.param_id
                 WHERE Policies.id = ?'''
        self.cursor.execute(sql, (policy_id, ))
        result = self.cursor.fetchall()
        result = {kk: vv for kk, vv in result}
        return result

    def add_experiment(self, output_dir, job_id, exp_type, date, notes, save_blob):

        blob = None
        if save_blob:
            # raw data blob
            try:
                output = geopmpy.io.RawReportCollection("*report", dir_name=output_dir)
            except Exception as ex:
                raise RuntimeError('<geopm> Error: No report data found in {}; error: {}\n'.format(output_dir, ex))
            h5_name = output.hdf5_name
            with open(h5_name, 'rb') as ifile:
                blob = ifile.read()
                print(h5_name, output.get_df())
        # add experiment containing all reports
        sql = '''INSERT INTO Experiments (output_dir, jobid, type, date, notes, raw_data)
                 VALUES (?, ?, ?, ?, ?, ?)'''
        self.cursor.execute(sql, (output_dir, job_id, exp_type, date, notes, blob))
        exp_id = self.cursor.lastrowid

        # add individual reports
        report_paths = glob.glob(os.path.join(output_dir, '*report'))
        for path in report_paths:
            report = geopmpy.io.RawReport(path)
            metadata = report.meta_data()
            print(metadata)
            version = metadata['GEOPM Version']
            start_time = metadata['Start Time']
            agent = metadata['Agent']
            profile = metadata['Profile']
            policy_vals = metadata['Policy']
            fom = report.figure_of_merit() or -1
            try:
                total_runtime = report.total_runtime()
            except:
                total_runtime = None
            power_vals = []
            for host in report.host_names():
                power_vals.append(report.raw_totals(host)['power (W)'])
            average_power = sum(power_vals) / len(power_vals)

            # insert policy and get id
            policy_id = self.add_policy_db(agent, policy_vals)

            sql = '''INSERT INTO Reports (experiment_id, agent, profile_name,
                                          geopm_version, start_time, policy_id,
                                          total_runtime, average_power, figure_of_merit)
                     VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?)'''
            self.cursor.execute(sql, (exp_id, agent, profile, version, start_time, policy_id,
                                      total_runtime, average_power, fom))

        # sql = '''SELECT * FROM Reports'''
        # self.cursor.execute(sql)
        # print(self.cursor.fetchall())
        self.conn.commit()

    def show_experiments(self):
        #sql = '''SELECT id, output_dir, jobid, type, date, notes FROM Experiments'''
        sql = '''SELECT * from Experiments'''
        df = pandas.read_sql_query(sql, self.conn)
        sys.stdout.write('{}\n'.format(df))
        sql = '''SELECT * FROM Reports'''
        df = pandas.read_sql_query(sql, self.conn)
        sys.stdout.write('{}\n'.format(df))

    def setup_tables(self):
        sql = '''
CREATE TABLE IF NOT EXISTS `ExperimentResults` (
  `id` INTEGER PRIMARY KEY AUTOINCREMENT,
  `experiment_id` INTEGER,
  `policy_id` INTEGER,
  `label` text,
  `value` double,
  FOREIGN KEY (policy_id) REFERENCES Policies (id),
  FOREIGN KEY (experiment_id) REFERENCES Experiments (id)
)
        '''
        self.cursor.execute(sql)

        sql = '''
CREATE TABLE IF NOT EXISTS `Experiments` (
  `id` INTEGER PRIMARY KEY AUTOINCREMENT,
  `output_dir` text,
  `jobid` INTEGER,
  `type` text,
  `date` date,
  `notes` text,
  `raw_data` longblob
)
        '''
        self.cursor.execute(sql)

        sql = '''
CREATE TABLE IF NOT EXISTS `Policies` (
  `id` INTEGER PRIMARY KEY AUTOINCREMENT,
  `agent` text
)
        '''
        self.cursor.execute(sql)

        sql = '''
CREATE TABLE IF NOT EXISTS `PolicyParamLink` (
  `id` INTEGER PRIMARY KEY AUTOINCREMENT,
  `policy_id` INTEGER,
  `param_id` INTEGER,
  FOREIGN KEY (`policy_id`) REFERENCES `Policies` (`id`),
  FOREIGN KEY (`param_id`) REFERENCES `PolicyParams` (`id`)
)
        '''
        self.cursor.execute(sql)

        sql = '''
CREATE TABLE IF NOT EXISTS `PolicyParams` (
  `id` INTEGER PRIMARY KEY AUTOINCREMENT,
  `name` text,
  `value` double
)
        '''
        self.cursor.execute(sql)

        sql = '''
CREATE TABLE IF NOT EXISTS `Reports` (
  `id` INTEGER PRIMARY KEY AUTOINCREMENT,
  `experiment_id` INTEGER,
  `agent` text,
  `profile_name` text,
  `geopm_version` text,
  `start_time` date,
  `policy_id` INTEGER,
  `total_runtime` double,
  `average_power` double,
  `figure_of_merit` double,
  FOREIGN KEY (`policy_id`) REFERENCES `Policies` (`id`),
  FOREIGN KEY (`experiment_id`) REFERENCES `Experiments` (`id`)
)
        '''
        self.cursor.execute(sql)
        self.conn.commit()
