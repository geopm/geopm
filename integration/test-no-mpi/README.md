
# GEOPM Runtime integration tests without MPI

This directory provides a few examples that use the GEOPM Runtime without MPI.
Disabling MPI for the GEOPM Runtime is achieved by passing ``--disable-mpi``
to the configure script for the build of the base directory in the GEOPM Git
repository.

## Test setup

In this configuration, the[geopmlaunch(1)](https://geopm.github.io/geopmlaunch.1.html)
command is not used for simplicity, and instead the ``geopmctl`` command is
configured directly using the ``GEOPM_*`` [GEOPM Runtime environment
variables](https://geopm.github.io/geopm.7.html#geopm-environment-variables)

There is a Makefile that uses the build environment documented in the
[geopm/integration
README](https://github.com/geopm/geopm/tree/dev/integration#readme)

The tests require that the GEOPM Service supporting the profile APIs is
running.  Follow the instructions for the [service integration test
setup](https://github.com/geopm/geopm/tree/dev/service/integration#test-setup)
to achieve this.


## Sleep test

Runs the GEOPM Runtime with the ``sleep(1)`` command producing a report, a
trace and an empty profile trace.  Run with ``test_sleep.sh`` command.


## Regions test

Compile the ``test_sleep_region`` binary with the ``make`` command, then
execute the binary with the GEOPM runtime using the ``test_sleep_region.sh``
script.  This first case just runs the [Monitor
Agent](https://geopm.github.io/geopm_agent_monitor.7.html).  The second test
case ``test_sleep_region_map.sh`` runs the [Frequency Map
Agent](https://geopm.github.io/geopm_agent_frequency_map.7.html) setting a
different frequency for each region.


### Notes

This is a work in progress.

TODO: Add some tests that use the REPORT_SIGNALS and TRACE_SIGNALS features.

TODO: Add some assertions to your "tests".

TODO: Add command line arguments to ``geopmctl`` that align with the
``geopmlaunch`` command line interface.  Could probably reuse the argparse
object if the ``geopmctl`` command were ported to a python wrapper to CFFI.
