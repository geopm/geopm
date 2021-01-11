#!/usr/bin/env python

import os
import sys
import datetime
import argparse

import db_connection
import db_tables


if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    # general options
    parser.add_argument('--db-type', dest='db_type', action='store',
                        default='sqlite3', help='type of DB; one of "sqlite3", "mariadb".  Default is "sqlite3"')

    subcommand = sys.argv[1]
    subargs = sys.argv[2:]

    if subcommand == 'save':
        # save experiment options
        parser.add_argument('--output-dir', dest='output_dir',
                            action='store', default='.',
                            help='location for reports and other output files')
        parser.add_argument('--save-blob', dest='save_blob', action='store_true',
                            default=False, help='whether to save the raw data HDF5 cache; default=False')
        parser.add_argument('--exp-type', type=str, dest='exp_type', required=True,
                            help='experiment type')
        parser.add_argument('--jobid', type=int, required=True,
                            help='job id')
    elif subcommand == 'smoke':
        # for smoke test results
        parser.add_argument('--app', type=str, required=True,
                            help='application name')
        # TODO: these are repeated... would be nice to parse the
        # folder name, but some apps and experiment types contain '_'
        parser.add_argument('--exp-type', type=str, dest='exp_type', required=True,
                            help='experiment type')
        parser.add_argument('--jobid', type=int, required=True,
                            help='job id')
        parser.add_argument('--result', type=str, required=True,
                            help='result of experiment')
    elif subcommand == 'show':
        pass
    else:
        sys.stdout.write("Usage: {} save | smoke | show [--help]".format(sys.argv[0]))
        sys.exit(1)

    args = parser.parse_args(subargs)

    # set up connection
    if args.db_type == 'sqlite3':
        filename = 'smoke_tests.db'
        db_conn = db_connection.SQLiteDBConn(filename)
    elif args.db_type == 'mariadb':
        db_conn = db_connection.MariaDBConn()

    db = db_tables.ExperimentsDB(db_conn)
    db.setup_tables()
    #db_tables.SmokeTestDB.setup_run_results(db_conn)

    if subcommand == 'save':
        db.add_experiment(output_dir=args.output_dir,
                          job_id=args.jobid,
                          exp_type=args.exp_type,
                          date=datetime.datetime.now(),  # TODO: where to get from?
                          notes="a cool experiment",
                          save_blob=args.save_blob)
    elif subcommand == 'show':
        db.show_experiments()
