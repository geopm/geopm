#!/usr/bin/env python

import sys
import datetime
import argparse

import db_connection
import db_tables

''' Main experiment DB interface script.

Prepares and passes arguments to the functions in db_tables.py, which
contains the knowledge of the schema.  The goals of this script are:
- Make it easy to store and read data from known types of GEOPM
  experiments
- Provide commands to log for smoke test results



TODO list, known issues, other thoughts..

- Trying to make most options required for now.  Annoying but a better
  default can be determined later.  Some columns are set to allow
  NULL.

- Currently requires the user to pass the experiment type and
  application name, even though it should be possible to determine
  this from the data itself (and they can make a mistake).  would be
  nice to parse the folder name, but some apps and experiment types
  contain '_'.

- Smoke tests are not linked to experiments in the schema.  Easy to
  do; is it something we want?  smoke test data is not necessarily
  useful long term; also relately small and easy to parse from raw
  report files if needed for debugging.  Long term, could have some
  other types of tests that are more sophisticated than return code
  failure, e.g. regression tests with thresholds for performance
  metrics based on past runs.

- Not checking for duplicate policies, only duplicate policy vals.
  Maybe could use some kind of hash of the set of policy val ids.
  How did PolicyStore handle this?

- No attempt to detect or avoid duplicate experiments or reports.

- We have jobids from mcfly and quartz; jobid as an int doesn't
  uniquely identify a job.  Also, multiple experiments per job is
  allowed.

- If no cache exists, parsing reports will still be bottleneck, as it
  is currently for other analysis scripts.

- No way to easily get policy out of RawReportCollection; for now this
  has a workaround using geopmpy.agent.

- RawReportCollection doesn't have a nice interface for linking
  region, epoch, and app total data.  Need to match on agent, policy
  (another reason finding matching policy ids would be useful),
  profile, iteration, etc. and know that those are the index columns.

- mildly annoying that db connect happens before trying to parse commandline

- use full paths for Experiments.output_dir for traceability

'''


def save_experiment_command(db, subargs):
    '''Save data for an experiment contained in a folder.  Finds and saves
       all reports and their policies.

    '''
    parser = argparse.ArgumentParser()

    parser.add_argument('--output-dir', dest='output_dir',
                        action='store', required=True,
                        help='location for reports and other output files')
    parser.add_argument('--save-blob', dest='save_blob', action='store_true',
                        default=False, help='whether to save the raw data HDF5 cache; default=False')
    parser.add_argument('--exp-type', type=str, dest='exp_type', required=True,
                        help='experiment type')
    parser.add_argument('--jobid', type=int, default=None,
                        help='job id')
    args = parser.parse_args(subargs)
    db.add_experiment(output_dir=args.output_dir,
                      job_id=args.jobid,
                      exp_type=args.exp_type,
                      date=datetime.datetime.now(),  # TODO: where to get from?
                      notes="a cool experiment",
                      save_blob=args.save_blob)


def show_experiments_command(db, subargs):
    # TODO: optional filter by app, type
    # TODO: print most recent first
    parser = argparse.ArgumentParser()
    parser.add_argument('--exp-type', type=str, dest='exp_type', default=None,
                        help='experiment type for filter')
    args = parser.parse_args(subargs)
    db.show_all_experiments(exp_type=args.exp_type)


def show_average_power_command(db, subargs):
    parser = argparse.ArgumentParser()
    parser.add_argument('--num-report', dest='num_report',
                        action='store', default=10,
                        help='maximum number of reports to display')
    args = parser.parse_args(subargs)
    db.show_report_by_power(args.num_report)


def show_policy_command(db, subargs):
    parser = argparse.ArgumentParser()
    parser.add_argument('--policy-id', dest='policy_id',
                        action='store', required=True, type=int,
                        help='id of policy')
    args = parser.parse_args(subargs)
    db.show_policy(args.policy_id)


def save_smoke_run_test_command(db, args):
    ''' Log a run script smoke test as pass, fail, or skipped.'''
    parser = argparse.ArgumentParser()

    # TODO: to make implementation simpler but a little less user
    # friendly, have this take an existing experiment ID (optional).
    # alternative is to pass through to save_command and have an
    # option about where to save the experiment data or not.
    # For now the schema is disconnected from experiments anyway.
    parser.add_argument('--app', type=str, required=True,
                        help='application name')
    parser.add_argument('--exp-type', type=str, dest='exp_type', required=True,
                        help='experiment type')
    parser.add_argument('--jobid', type=int, required=True,
                        help='job id')
    parser.add_argument('--result', type=str, required=True,
                        help='result of experiment')


def save_smoke_gen_test_command(args):
    pass


if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    # general options
    parser.add_argument('--db-type', dest='db_type', action='store',
                        default='sqlite3', help='type of DB; one of "sqlite3", "mariadb".  Default is "sqlite3"')

    subcommands = {
        "save": save_experiment_command,
        "show": show_experiments_command,
        "show_power": show_average_power_command,
        "show_policy": show_policy_command,
        #"save_smoke_run_test": save_smoke_run_test_command,
        #"save_smoke_gen_test": save_smoke_gen_test_command,
    }
    usage = "Usage: {} {} [--db-type DB] [--help]".format(sys.argv[0],
                                           ' | '.join(subcommands.keys()))

    if len(sys.argv) < 2:
        sys.stdout.write(usage + '\n')
        sys.exit(1)

    subcomm = sys.argv[1]
    args = sys.argv[2:]

    if subcomm not in subcommands:
        sys.stdout.write(usage + '\n')
        sys.exit(1)

    args, subcomm_args = parser.parse_known_args(args)
    # set up connection
    if args.db_type == 'sqlite3':
        filename = 'smoke_tests.db'
        db_conn = db_connection.SQLiteDBConn(filename)
    elif args.db_type == 'mariadb':
        db_conn = db_connection.MariaDBConn()
    else:
        sys.stderr.write('Invalid DB type: {}\n'.format(args.db_type))
        sys.exit(1)

    db = db_tables.ExperimentsDB(db_conn)

    if args.db_type == 'sqlite3':
        # setup for sqlite only
        db.setup_tables()

    subcommands[subcomm](db, subcomm_args)
