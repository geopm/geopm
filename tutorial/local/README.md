GEOPM Runtime Local Tutorial
============================

This is a simple tutorial on how to use the GEOPM Runtime locally on a
compute-node without any inter-node communication.  This simplifies
the requirements for the GEOPM Runtime and is good place to start
learning how the GEOPM Runtime works.

Environment
-----------

To simplify use of a local install of GEOPM a few variables should be
set in your environment.  This will allow you to use the scripts in
the ``geopm/integration/config`` directory without modification.

```
# Set required variables for run_env.sh
export GEOPM_SOURCE=<PATH_TO_GEOPM_SOURCE>
export GEOPM_INSTALL=${HOME}/build/geopm
export GEOPM_WORKDIR=${PWD}
```

Build and Install
-----------------

First, follow the instructions on how to build and install the GEOPM
Service <https://geopm.github.io/install.html> system wide.

Additionally, create a user-local build and install the GEOPM Runtime
while disabling the MPI features.

```
cd ${GEOPM_SOURCE}/service
./autogen.sh
./configure --prefix=${GEOPM_INSTALL}
make
make install
cd ..
./autogen.sh
./configure --prefix=${GEOPM_INSTALL} --disable-mpi
make
make install

```

Simple Example
--------------

Execute the ``sleep(1)`` command and create a report, trace, and event
log with the GEOPM monitor agent.

```
# Set paths to include local install
source ${GEOPM_SOURCE}/integration/config/run_env.sh

# Run the GEOPM controller in the background
GEOPM_PROFILE=sleep-example \
GEOPM_REPORT=sleep-example-report.yaml \
GEOPM_TRACE=sleep-example-trace.csv \
GEOPM_TRACE_PROFILE=sleep-example-event-log.csv \
geopmctl &

# Preload GEOPM into the sleep(1) command
GEOPM_PROFILE=sleep-example \
LD_PRELOAD=libgeopm.so.1.0.0 \
    sleep 10

```

Note that the ``GEOPM_PROFILE`` environment variable matches for both
the environment of the GEOPM controller and the environment of the
application being tracked.  Using ``LD_PRELOAD`` enables any
executable to register for profiling with GEOPM.

Inspect the contents of the report and trace files.
