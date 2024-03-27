# Experiments

This directory provides the integration infrastructure with
experiments.  All experiments are broken into two parts: running
parallel jobs, and analyzing the job output.

## Common Python Supporting Files

There is some functionality that is shared between different
expereiments and these features are captured in python modules
at the base of the experiment directory.

#### `common_args.py`

There are many command line arguments that are used by run and
analysis scripts that are common.  The `common_args` module provides
functions for adding command line arguments to an `argparse` command
line parser.  When run and gen scripts use these command line
interfaces it provides a more uniform user experience when calling the
scripts, and some common code for adding command line options.

#### `launch_util.py`

The `launch_util` module provides the high level launch functions that
enable run scripts to launch a sequence of job configurations.

#### `machine.py`

Used to generate a json file containing descriptive information about
the compute nodes where jobs are running.  Each experiment creates one
of these json files at run time.  The machine object may be used to
configure an application and it is referred to by the "gen" analysis
scripts to understand the system where the jobs were run without
requiring compute node access.

#### `util.py`

The `util` module provides low level functions that assist in querying
system configurations.  This module is not commonly used by run or gen
scripts, but is used by the other supporting files in this directory.

## Experiment Directories

Scripts are grouped into sub-folders based on the type of
experiment, such as a sweep across power limits or processor
frequency settings.  In each experiment directory there should be
a python script with a name that matches the experiment directory
name.  This script contains common code used by the other scripts
in the directory.  These other scripts are broken into two
categories: run scripts which are prefixed with "run_" and
analysis scripts which are prefixed with "gen_".

Each run script is specific to an application, and an application
may have several run scripts that describe different application
configurations.  This is because applications have specific
runtime requirements and are scaled to different node and rank
counts.  The base experiment script describes output requirements
for the run scripts: whether traces required for the analysis and
which extra signals are added to the reports and traces.  In this
way the output data from all of the run scripts will meet the
requirements of all of the analysis scripts.  Each analysis script
is designed to generate particular summary tables or plots based
on output reports and traces located in a user provided directory.


Application Specific Run Scripts
--------------------------------

Application run scripts are named `run_<experiment type>_<app>.py`
according to the experiment and application to be run.

The general workflow is as follows:

1. Run the sweep for a specific application with
   run_<exp>_<app>.py.  A directory to hold all output files is
   recommended and can be specified with `--output-dir`.  The number
   of nodes can be specified with `--nodes`; otherwise 1 node will be
   used.

2. Generate the desired analysis targeting the resulting output with
   `--output-dir`; by default all reports/traces in the current
   working directory are used.  Some scripts support more verbose
   output with `--show-details`.

Example:

1. Run DGEMM on 4 nodes:

  ./run_power_sweep_dgemm.py --nodes=4 --output-dir=test42

2. Calculate the achieved frequency on each node and generate histogram plots:

  ./gen_plot_node_efficiency.py --output-dir=test42
