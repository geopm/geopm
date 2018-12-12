GEOPM Integration Tests
=======================

This directory contains the integration tests for the GEOPM package.
These tests are implemented by wrapping the ModelApplication class
introduced in tutorial 6 with a Python unittest framework.  The
geopm_test_integration.py wrapper generates input configuration files,
executes the geopm_test_integration model application one or more
times and checks that the GEOPM report and trace files demonstrate
features of the GEOPM runtime.

ENVIRONMENT VARIABLES
---------------------

### GEOPM_KEEP_FILES
Normally, the test output is cleaned up (i.e. deleted) when a test is
successful and is only preserved if the test were to fail.  If this
variable is set then no files will be removed and all of the test's
output will be preserved.

### GEOPM_RUN_LONG_TESTS
This variable specifies whether or not to run the integration tests
that take a considerable amount of time.  The rough rule is that all
tests running longer than 5 minutes will require this to be set.
The current list of tests requiring this variable to be set are:
1. test_region_runtimes
2. test_scaling
3. test_power_consumption
4. test_sample_rate
5. test_plugin_simple_freq_offline
6. test_plugin_simple_freq_online

### GEOPM_EXEC_WRAPPER
Configure string for GEOPM test launcher to run arbitrary
application to wrap target job execution.

    `$ export GEOPM_EXEC_WRAPPER="numactl -m 1" \
              ./geopm_test_integration.py -v TestIntegration.test_count`

    `$ export GEOPM_EXEC_WRAPPER="valgrind --tool=memcheck --vgdb=yes \
              --vgdb-error=0" \
              ./geopm_test_integration.py -v TestIntegration.test_count`

    `$ export GEOPM_EXEC_WRAPPER="$PATH_TO_DDT_CLIENT_BIN --ddtsessionfile \
              $PATH_TO_DDT_CLIENT_SESSION_FILE" \
              ./geopm_test_integration.py -v TestIntegration.test_count`

EXAMPLES
--------
Since the log files that are emitted from the individual tests are
opened and appended to on subsequent runs, it may become necessary to
periodically clean up the files to avoid conflating experimental data.
```git clean``` can be used in this instance to clean up all of the
temporary files:

    $ git clean -fd .
    $ export GEOPM_KEEP_FILES=true
    $ export GEOPM_RUN_LONG_TESTS=true
    $ ./geopm_test_integration.py -v TestIntegration.test_power_consumption

Or as a one-liner:

    $ git clean -fd . ; GEOPM_KEEP_FILES=true GEOPM_RUN_LONG_TESTS=true ./geopm_test_integration.py -v TestIntegration.test_power_consumption

