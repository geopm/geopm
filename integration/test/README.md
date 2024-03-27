GEOPM Integration Tests
=======================

This directory contains the integration tests for the GEOPM package.

Please refer to the [README](../README.md) in the parent directory for
information on how to define environment variables needed by these
scripts in your `.geopmrc` file and how to use the scripts in the
[config](../config) directory.

The integration tests are built using the python unittest infrastructure.
Each of the python test scripts begins with the `test_` prefix, and may be
run as a batch or individually.  When running test scripts individually, it
is important to add the GEOPM source code directory to your python path:
```
export PYTHONPATH=${GEOPM_SOURCE}:${PYTHONPATH}
```
Alternatively, the python unittest discovery mechanism may be used.  When
using this method, the unittest module is executed using the discover
subcommand.  For example, the script below will execute all of the GEOPM HPC
runtime integration tests:
```
#!/bin/bash

source ${GEOPM_SOURCE}/config/run_env.sh

# Run all of the GEOPM HPC Runtime integration tests
python3 -m unittest discover \
        --top-level-directory ${GEOPM_SOURCE} \
        --start-directory ${GEOPM_SOURCE}/integration/test

# Execute tests in test_monitor.py and log to file
PYTHONPATH=${GEOPM_SOURCE}:${PYTHONPATH} \
python3 ${GEOPM_SOURCE}/integration/test/test_monitor.py \
    --verbose >& test-monitor.log

# Use discover to execute tests in test_monitor.py log to file
python3 -m unittest discover \
        --top-level-directory ${GEOPM_SOURCE} \
        --start-directory ${GEOPM_SOURCE}/integration/test \
        --pattern test_monitor.py \
        --verbose >& test-monitor.log
```

TEST ENVIRONMENT VARIABLES
--------------------------

The environment variables below may be set by the user to modify test
execution.

### `GEOPM_LAUNCHER`
Used to force the integration tests to target a specific launcher
implementation as supported by the launcher factory.  See
**geopmlaunch(1)** for more details.

#### `GEOPM_NUM_NODE`
Set the number of compute nodes used by each integration test.  Some tests
are required to run on one node only, and these tests are not affected by
this environment variable.  If this variable is not set, the tests default to
`SLURM_NNODES` (the number of nodes allocated within the slurm environment),
and if this is not set, the tests default to four.

### `GEOPM_EXEC_WRAPPER`
This variable may be used to specify an arbitrary application to use as an
execution wrapper. This wrapper will be invoked by the launcher, and the
arguments provided by this invocation will include the workload executable
and its command line arguments, which the wrapper is expected to invoke in
turn, possibly in a modified or restricted environment. Some examples that
use `GEOPM_EXEC_WRAPPER` are given below.

```
#!/bin/bash

source ${GEOPM_SOURCE}/config/run_env.sh

GEOPM_EXEC_WRAPPER="numactl -m 1" \
python3 -m unittest discover \
        --top-level-directory ${GEOPM_SOURCE} \
        --start-directory ${GEOPM_SOURCE}/integration/test \
        --pattern test_monitor.py

GEOPM_EXEC_WRAPPER="valgrind --tool=memcheck --vgdb=yes --vgdb-error=0" \
python3 -m unittest discover \
        --top-level-directory ${GEOPM_SOURCE} \
        --start-directory ${GEOPM_SOURCE}/integration/test \
        --pattern test_monitor.py

GEOPM_EXEC_WRAPPER="$PATH_TO_DDT_CLIENT_BIN --ddtsessionfile <SESSION_FILE>" \
python3 -m unittest discover \
        --top-level-directory ${GEOPM_SOURCE} \
        --start-directory ${GEOPM_SOURCE}/integration/test \
        --pattern test_monitor.py
```
