#!/usr/bin/env python

import datetime
import sys
import pandas
import math

import geopmpy.io


class SmokeTestDB:
    def __init__(self, db_conn):
        self.conn = db_conn.conn()
        self.cursor = self.conn.cursor()
        self.bind_str = db_conn.bind_str()

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


class ExperimentsDB:
    def __init__(self, db_conn):
        self.conn = db_conn.conn()
        self.cursor = self.conn.cursor()
        self.bind_str = db_conn.bind_str()

    def add_policy_db(self, agent, policy_vals):
        # Always create a new policy, even if identical
        # TODO: optimize this by checking for duplicates? assume
        # policy data will never change once inserted
        sql = "INSERT INTO Policies (agent) VALUES ({b})".format(b=self.bind_str)
        self.cursor.execute(sql, (agent,))
        policy_id = self.cursor.lastrowid

        for kk, vv in policy_vals.items():
            # check if existing
            sql = "SELECT * FROM PolicyParams WHERE name = {b} and value = {b}".format(b=self.bind_str)
            self.cursor.execute(sql, (kk, vv))
            # TODO: could be a lot of rows; use fetchone() or
            # fetchmany() if it becomes a problem.  Note:
            # cursor.rowcount does not work as expected here.
            existing = self.cursor.fetchall()
            if len(existing) == 0:
                sql = "INSERT INTO PolicyParams (name, value) VALUES ({b}, {b})".format(b=self.bind_str)
                self.cursor.execute(sql, (kk, vv))
                param_id = self.cursor.lastrowid
                print('{} rows updated'.format(self.cursor.rowcount))
            elif len(existing) == 1:
                print('Already in db: {}'.format(existing))
                param_id = existing[0][0]
            else:
                sys.stderr.write("Warning: duplicate entries in PolicyParams!\n")
                param_id = existing[0][0]

            print("rowcount: {}, param id: {}".format(self.cursor.rowcount, param_id))

            # Add to link table
            sql = '''INSERT INTO PolicyParamLink (policy_id, param_id)
                     VALUES ({b}, {b})'''.format(b=self.bind_str)
            self.cursor.execute(sql, (policy_id, param_id))
        return policy_id

    def show_policy(self, policy_id):
        sys.stdout.write('policy {}:\n'.format(policy_id))
        sql = '''SELECT name, value FROM PolicyParamLink
                 INNER JOIN Policies ON Policies.id = PolicyParamLink.policy_id
                 INNER JOIN PolicyParams ON PolicyParams.id = PolicyParamLink.param_id
                 WHERE Policies.id = {b}'''.format(b=self.bind_str)
        self.cursor.execute(sql, (policy_id, ))
        result = self.cursor.fetchall()
        result = {kk: vv for kk, vv in result}
        return result

    def _insert_policy_id(self, row):
        # insert policy and set policy_id column
        # Note that this function depends on the columns having already been renamed
        agent = row['agent']
        policy_names = geopmpy.agent.policy_names(agent)
        # Note: roundabout way of getting the policy.  Much easier
        # to call RawReport.meta_data['Policy'], but RawReport has
        # no caching
        policy_vals = {name: row[name] for name in policy_names if not math.isnan(row[name])}
        policy_id = self.add_policy_db(agent, policy_vals)
        row['policy_id'] = policy_id
        return policy_id

    def add_experiment(self, output_dir, job_id, exp_type, date, notes, save_blob):

        blob = None
        try:
            output = geopmpy.io.RawReportCollection("*report", dir_name=output_dir)
        except Exception as ex:
            raise RuntimeError('<geopm> Error: No report data found in {}; error: {}\n'.format(output_dir, ex))
        if save_blob:
            h5_name = output.hdf5_name
            with open(h5_name, 'rb') as ifile:
                blob = ifile.read()
                print(h5_name, output.get_df())
        # add experiment containing all reports
        sql = '''INSERT INTO Experiments (output_dir, jobid, type, date, notes, raw_data)
                 VALUES ({b}, {b}, {b}, {b}, {b}, {b})'''.format(b=self.bind_str)
        self.cursor.execute(sql, (output_dir, job_id, exp_type, date, notes, blob))
        exp_id = self.cursor.lastrowid

        # add individual reports
        # TODO: currently only saving app totals
        all_reports = output.get_app_df()

        # TODO: group using profile, which happens to contain the
        # relevant part of the policy for this data.  Other data will
        # require actually looking at the policy columns.
        index_cols = ['Profile', 'Agent', 'Start Time', 'GEOPM Version']

        #all_reports = all_reports.set_index(['Profile', 'Agent'], drop=False)
        # merge multiple nodes data
        all_reports = all_reports.groupby(index_cols).mean().reset_index()

        renamer = {
            'Agent': 'agent',
            'Profile': 'profile_name',
            'GEOPM Version': 'geopm_version',
            'Start Time': 'start_time',
            'power (watts)': 'average_power',  # old report cache
            'power (W)': 'average_power',
            'FOM': 'figure_of_merit'
        }
        all_reports = all_reports.rename(columns=renamer)
        all_reports['experiment_id'] = exp_id
        all_reports['policy_id'] = all_reports.apply(self._insert_policy_id, axis=1)
        # TODO: add this to rename once the feature is present in experiments
        all_reports['total_runtime'] = None

        table_cols = ['experiment_id', 'profile_name', 'agent', 'geopm_version',
                      'start_time', 'total_runtime', 'figure_of_merit',
                      'average_power', 'policy_id']

        all_reports = all_reports[table_cols]

        # Insert multiple rows into Reports
        # This apparently requires sqlalchemy for mysql to work
        #all_reports.to_sql('Reports', self.conn, if_exists='append', index=False)
        # workaround: add each report separately
        for _, row in all_reports.iterrows():
            exp_id = row['experiment_id']
            agent = row['agent']
            profile = row['profile_name']
            version = row['geopm_version']
            start_time = row['start_time']
            policy_id = row['policy_id']
            total_runtime = row['total_runtime']
            average_power = row['average_power']
            fom = row['figure_of_merit']
            sql = '''INSERT INTO Reports (experiment_id, agent, profile_name,
                                          geopm_version, start_time, policy_id,
                                          total_runtime, average_power, figure_of_merit)
                     VALUES ({b}, {b}, {b}, {b}, {b}, {b}, {b}, {b}, {b})'''.format(b=self.bind_str)
            self.cursor.execute(sql, (exp_id, agent, profile, version, start_time, policy_id,
                                      total_runtime, average_power, fom))

        self.conn.commit()

    def show_all_experiments(self):
        '''Display a summary table of all the experiments stored.

            TODO: There are some tradeoffs with handling some stats
            like count, etc. in pandas vs DB but pandas is probably
            more readable.  Below is a weird hybrid.  Consider just
            returning everything as long as it fits in memory.  Could
            also consider defining some guidelines such as
            filter/group on DB side, "describe" stats in pandas (for
            example).  This is also somewhat moot if using the giant
            HDF5 blob stored in the DB (to get some report details
            that are not reflected in the schema); then no choice but
            to copy whole thing into memory and do filtering in pandas
            as in current analysis scripts.

        '''

        # Note: exclude data blob from columns shown
        columns = ['Experiments.'+x for x in ['id', 'output_dir', 'jobid', 'type', 'date', 'notes']]
        sql = '''SELECT {cols}, count(Reports.id) AS num_report FROM Experiments
                 INNER JOIN Reports ON Experiments.id
                 WHERE Reports.experiment_id = Experiments.id GROUP BY Experiments.id
              '''.format(cols=', '.join(columns))
        df = pandas.read_sql_query(sql, self.conn)
        sys.stdout.write('{}\n'.format(df))

    def setup_tables(self):
        # Note: better to edit these in phpMyAdmin, then update these
        # statements to match.  CREATE TABLE here is for the sqlite DB

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
