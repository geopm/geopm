# GEOPM INTEGRATION SCRIPTS

Welcome to GEOPM integration.  This directory contains scripts for
running GEOPM for performance and integration testing.  The
integration directory is organized into subdirectories, and the
contents of each are described below.

Prior to running for the first time, users must setup their environment
properly.  Please edit your ~/.bashrc to include the following:

```
source ${HOME}/.geopmrc
```

Additionally, ~/.geopmrc should have the following contents (at a minimum):
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

## `apps`

The [apps](apps) directory is where information about
applications is located.  Each application supported has its own
sub-directory within the apps directory.

## `experiment`

The scripts in the [experiment](experiment) directory are intended to
be used as a starting point for various types of experiments using
GEOPM.  An experiment consists of a run step, containing application
launches with GEOPM, and an analysis step, in which the reports and
traces produced by the run step are processed, interpreted, and
summarized.  Analysis scripts may also produce visualizations of
measurements and results.

## `test`

The [test](test) directory contains the integration tests of
the GEOPM runtime.  These tests may use functions from the experiment
scripts to assist in launch and analysis, but they also perform
assertions on the results.
