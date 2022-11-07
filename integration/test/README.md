GEOPM Integration Tests
=======================

This directory contains the integration tests for the GEOPM package.

Please refer to the [README](../README.md) one directory up for
information about how to define environment variables relevant to these
scripts in your `.geopmrc` file and how to use the scripts in the
[config](../config) directory.

The integration tests are built using the python unittest
infrastructure.  Each of the python test scripts begins with
the `test_` prefix, and these may be run individually.  When
running test scripts individually, it is important to add
the GEOPM source code directory to your python path:

```
export PYTHONPATH=${GEOPM_SOURCE}:${PYTHONPATH}
```

Alternatively, the Python unittest discovery mechanism may be used.
When using this method, the unittest module is executed using the
discover subcommand.  An example that will execute all of the GEOPM
HPC runtime integration tests is below:

```
#!/bin/bash

source ${GEOPM_SOURCE}/config/run_env.sh

# Run all of the GEOPM HPC Runtime integration tests
python3 -m unittest discover \
        --top-level-directory ${GEOPM_SOURCE} \
        --start-directory ${GEOPM_SOURCE}/integration/test

# Execute tests in test_monitor.py and log to file
PYTHONPATH=${GEOPM_SOURCE}:${PYTHONPATH} \
python3 ${GEOPM_SOURCE}/test/integration/test_monitor.py \
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

### GEOPM_LAUNCHER
Used to force the integration tests to target a specific Launcher
implementation as supported by the Launcher factory.  See
**geopmlaunch(1)** for more details.

### GEOPM_EXEC_WRAPPER
Configure string for GEOPM test launcher to run arbitrary
application to wrap target job execution.  Some examples
that use `GEOPM_EXEC_WRAPPER` are given below.

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
