GEOPM Integration Scripts
=========================
This directory contains scripts for running GEOPM for performance and
integration testing.  The integration directory is organized into
subdirectories, described briefly below and in more detail in each
directory's README file.

Subdirectories:

* The [`apps`](apps) directory is where information about
  applications is located.  Each supported application has its own
  sub-directory within the apps directory which contains scripts that setup,
  configure and execute it.
* The `bash` scripts in the [`config`](config) directory set environment
  variables used to enable GEOPM.  These `bash` scripts may be sourced
  by the user in order to build GEOPM, or to run against a locally
  installed version of GEOPM.
* The scripts in the [`experiment`](experiment) directory are used as a
  starting point for various types of experiments using GEOPM.  An experiment
  consists of a run step, in which the application is launched with GEOPM, and
  an analysis step, in which the reports and traces produced by the run step
  are processed, interpreted and summarized.  Analysis scripts may also produce
  visualizations of measurements and results.
* The [`runtime_examples`](runtime_examples) directory contains example code
  for use with the GEOPM runtime.
* The [`service`](service) directory contains examples and tests for the GEOPM
  service.
* The [`smoke`](smoke) directory contains smoke tests to validate a new GEOPM
  deployment.
* The [`test`](test) directory contains the integration tests of the GEOPM
  runtime.  These tests share components with the experiment scripts to assist
  in launch and analysis; unlike the experiments, they also perform correctness
  assertions on the results.
* The [`test_skipped`](test_skipped) directory contains integration tests that
  are not appropriate for running in automation. These include tests that
  require manual verification, tests that are out of date with the current
  implementation and tests for deprecated features. Some of these files should
  be deleted after associated issues have been filed, and others should be
  modified so that they may be run in automation with effective assertions.

Prerequisites
-------------
Prior to running these tests for the first time, users must setup their
environment.  Edit your `~/.bashrc` (or other shell initialization file) to
include the following:

* `.bashrc` *(optional)*: If you will be sourcing `run_env.sh` before your
  test, modifying your `.bashrc` is not necessary.  To always source your
  `~/.geopmrc` at login, edit your `~/.bashrc` to include the following:
  ```
  source ${HOME}/.geopmrc
  ```
* `.geopmrc`: Additionally, `~/.geopmrc` should have the following contents (at a minimum):
  ```
  #!/bin/bash
  
  # Path to the GEOPM source code
  export GEOPM_SOURCE=${HOME}/geopm
  
  # Path to the installation artifacts for GEOPM
  export GEOPM_INSTALL=${HOME}/build/geopm
  
  # Path to location for job output directories
  export GEOPM_WORKDIR=${HOME}/output
  
  # Path to location of application archives
  export GEOPM_APPS_SOURCES=${HOME}/geopm_apps
  
  # Path to system override for environment setup (OPTIONAL)
  export GEOPM_SYSTEM_ENV=${GEOPM_SOURCE}/integration/config/<UNIQUE MACHINE NAME>_env.sh
  ```
