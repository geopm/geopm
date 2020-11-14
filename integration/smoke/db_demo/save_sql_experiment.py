#!/usr/bin/env python

import os
import sys
import mysql.connector
import getpass
import glob
from datetime import datetime

import geopmpy.io


def add_policy_db(cursor, agent, policy_vals):
    # Always create a new policy, even if identical
    # TODO: optimize this by checking for duplicates? assume
    # policy data will never change once inserted
    sql = "INSERT INTO Policies (agent) VALUES (%s)"
    cursor.execute(sql, (agent,))
    policy_id = cursor.lastrowid

    for kk, vv in policy_vals.items():
        # check if existing
        cursor.execute("SELECT * FROM PolicyParams WHERE name = %s and value = %s",
                       (kk, vv))
        existing = cursor.fetchall()
        if cursor.rowcount == 0:
            sql = "INSERT INTO PolicyParams (name, value) VALUES (%s, %s)"
            cursor.execute(sql, (kk, vv))
            param_id = cursor.lastrowid
            print('{} rows updated'.format(cursor.rowcount))
        elif cursor.rowcount == 1:
            print('Already in db: {}'.format(existing))
            param_id = existing[0][0]
        else:
            sys.stderr.write("Warning: duplicate entries in PolicyParams!\n")

        print("param id: {}".format(param_id))

        # Add to link table
        sql = "INSERT INTO PolicyParamLink (policy_id, param_id) VALUES (%s, %s)"
        cursor.execute(sql, (policy_id, param_id))
    return policy_id


def show_policy(cursor, policy_id):
    sys.stdout.write('policy {}:\n'.format(policy_id))
    sql = '''SELECT name, value FROM PolicyParamLink
             INNER JOIN Policies ON Policies.id = PolicyParamLink.policy_id
             INNER JOIN PolicyParams ON PolicyParams.id = PolicyParamLink.param_id
             WHERE Policies.id = %s'''
    cursor.execute(sql, (policy_id, ))
    result = cursor.fetchall()
    result = {kk: vv for kk, vv in result}
    return result


def add_experiment(cursor, output_dir, job_id, exp_type, date, notes):
    # raw data blob
    try:
        output = geopmpy.io.RawReportCollection("*report", dir_name=output_dir)
    except:
        sys.stderr.write('<geopm> Error: No report data found in {}; run a monitor experiment before using this analysis\n'.format(output_dir))
        sys.exit(1)
    h5_name = output.hdf5_name
    with open(h5_name, 'rb') as ifile:
        blob = ifile.read()
    print(h5_name, output.get_df())

    # add experiment containing all reports
    sql = '''INSERT INTO Experiments (output_dir, jobid, type, date, notes, raw_data)
             VALUES (%s, %s, %s, %s, %s, %s)'''
    cursor.execute(sql, (output_dir, job_id, exp_type, date, notes, blob))
    exp_id = cursor.lastrowid

    # add reports
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
        total_runtime = report.total_runtime() or -1
        power_vals = []
        for host in report.host_names():
            power_vals.append(report.raw_totals(host)['power (watts)'])
        average_power = sum(power_vals) / len(power_vals)

        # insert policy and get id
        policy_id = add_policy_db(cursor, agent, policy_vals)

        sql = '''INSERT INTO Reports (experiment_id, agent, profile_name,
                                      geopm_version, start_time, policy_id,
                                      total_runtime, average_power, figure_of_merit)
                 VALUES (%s, %s, %s, %s, %s, %s, %s, %s, %s)'''
        cursor.execute(sql, (exp_id, agent, profile, version, start_time, policy_id,
                             total_runtime, average_power, fom))

    # sql = '''SELECT * FROM Reports'''
    # cursor.execute(sql)
    # print(cursor.fetchall())
    # sql = '''SELECT output_dir, jobid, type, date, notes FROM Experiments'''
    # cursor.execute(sql)
    # print(cursor.fetchall())


if __name__ == '__main__':

    if len(sys.argv) < 2:
        sys.stderr.write('Usage: {} <output dir>\n')
        sys.exit(1)

    config = {
        'user': 'diana',
        'database': 'test',
    }
    password = getpass.getpass("Password:")
    conn = mysql.connector.connect(password=password, **config)
    conn.autocommit = False  # require commit() at end of transaction

    # TODO: add try except with rollback()
    cursor = conn.cursor()
    # TODO: Brad: this is set globally through phpmyadmin
    # cursor.execute('set max_allowed_packet=67108864')  # for HDF5 blob

    # add_policy_db(cur, "bond", {"POL1": 123, "POL2": 345})
    # add_policy_db(cur, "powers", {"POL2": 345, "POL3": 567})
    # conn.commit()

    # print(show_policy(cur, 3))
    # print(show_policy(cur, 4))
    # print(show_policy(cur, 5))
    # print(show_policy(cur, 6))

    output_dir = os.path.realpath(sys.argv[1])

    add_experiment(cursor, output_dir, 1234, 'test exp', datetime.today(), 'some notes')
    conn.commit()
