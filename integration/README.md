# GEOPM INTEGRATION SCRIPTS

Welcome to GEOPM integration.  This directory contains scripts for
running GEOPM for performance and integration testing.  The
integration directory is organized into subdirectories, and the
contents of each are described below.

## `apps`

The apps directory is where information about applications is
located.  Each application supported has its own sub-directory
within the apps directory.

Each application directory contains scripts that encapsulate the
required setup, run configuration, and command line arguments for
executing the application.  There may be more than one script for
an application to support different workloads or scenarios.

For some applications, there are dedicated sub-directories
containing more information on how to download and build the
application, and sometimes GEOPM-related patches to be applied on
top of publicly-available source code.

## `experiment`

The scripts in this directory are intended to be used as a
starting point for various types of experiments using GEOPM.  An
experiment consists of a run step, containing application launches
with GEOPM, and an analysis step, in which the reports and traces
produced by the run step are processed, interpreted, and
summarized.  Analysis scripts may also produce visualizations of
measurements and results.

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

## `test`

The test directory contains the integration tests of the GEOPM
runtime.  These tests may use functions from the experiment
scripts to assist in launch and analysis, but they also perform
assertions on the results.

