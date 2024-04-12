GEOPM Integration Test Plan
===========================

This directory contains the test plan for validating specific
features within GEOPM.

Prerequisites
-------------

Ensure you have completed the creation of your `~/.geopmrc` file
described in the [integration README](../../README.md#prerequisites).

> [!NOTE]
> When setting up your `~/.geopmrc` file, use the following example
> as a basis for systems configured for PALS based job launch when
> setting GEOPM_SYSTEM_ENV:
> [australis_env.sh](../../config/australis_env.sh)

Non-MPI App Profiling
---------------------

The integration test scripts `test_multi_app.[sh|py]` have been
designed to demonstrate GEOPM's ability to profile applications that
do not invoke MPI.

### test_multi_app.sh
This file depicts the necessary environment needed to use the GEOPM
Runtime.  First, the script creates a JSON configuration that will be
used with `geopmbench`.  Next, the script sources the `run_env.sh` to
setup the necessary environment for accessing the python modules for
the Runtime.  The next block sets the `GEOPM_PROFILE`,
`GEOPM_PROGRAM_FILTER`, and `LD_PRELOAD` variables appropriately for this
app configuration.

The next block sets up the configuration for the GEOPM Controller.
The configuration enables summary reports, time-series traces, and
profile traces to be emitted at the conclusion of the run.
`GEOPM_REPORT_SIGNALS` adds additional signals to the report files with
the given configuration.  `GEOPM_NUM_PROC` informs the Controller how
many user processes it should expect to profile, and `GEOPM_CTL_LOCAL`
tells the Controller not to use any MPI communication for transmitting
data (assuming the Runtime was compiled with MPI support, which is the
default).

Prepending the invocation of `geopmctl` with `setsid` is vital to ensure
that the save/restore mechanism of the Service is invoked.  This is
required because of the session tracking design of the Service.  If
you issue `geopmctl` without the `setsid`, any control changes that are
made during the run will persist afterward until the current tty
ends.

Once the Controller is up and waiting for processes to connect, all
that remains is to launch the desired app.  In this case, `numactl` is
used to run `geopmbench` on the first package, and run `stress-ng` on the
second package.  This script assumes the system has dual-socket
CPUs.

The `wait` call at the end ensures that the script does not return until
all apps are complete.

For more information on the environment variables used to configure
GEOPM see: [man 7 geopm](https://geopm.github.io/geopm.7.html).

When the script is complete several GEOPM output files will be
present in the current working directory, suffixed with the compute
node that they were executed on: the summarized report file -
`test_multi_app_report.yaml-node1`, the time series data -
`test_multi_app_trace.csv-node1`, and the time series data of
per-process messages between the application and the Controller -
`test_multi_app_trace_profile.csv-node1`.  These files will be used to
verify the run completed as expected in the next section.

### test_multi_app.py
This file contains the test harness necessary to launch
`test_multi_app.sh` and also the test assertions that verify the data
gathered during the run.

By default, this test is setup to run on a single compute node.  After
`test_multi_app.sh` completes, the following assertions are tested:

1. `test_meta_data` - The test ran on the expected number of nodes.
2. `test_expected_regions_exist` - The summarized report file contains
   data for the expected set of application regions.
   - Notice that `MPI_Init_thread` is still listed as an expected
     region even though the application does not use MPI.  This is
     because the GEOPM Runtime is still built with MPI support by
     default, and will track calls to `MPI_Init_thread` by any
     application process that calls it.  In this case, `geopmbench`
     *is* being invoked in it's default configuration which enables
     MPI support.  However, the JSON configuration for `geopmbench`
     specifies no MPI regions.  This means `geopmbench` will call
     `MPI_Init` and `MPI_Finalize` but will not otherwise use MPI.
   - If the Runtime was built *without* MPI support, the
     `MPI_Init_thread`  region would not be present in the output
     files.  `geopmbench` can be invoked with the GEOPMBENCH_NO_MPI
     environment variable to avoid any MPI calls. For more information
     about the environment configuration of `geopmbench`, see
     [here](https://geopm.github.io/geopmbench.1.html#environment).
3. `test_regions_valid` - The summarized report file contains valid
   data for the expected regions.
   - Region data is first validated by examining the count of how many
     times a particular region was called.  Notice that `model-init`
     and `MPI_Init_thread` have an expected count of 0.5 which may be
     confusing.  This is due to the way the reports are summarized.
     Since there are only 2 processes connecting to the Controller to
     be profiled, and only one of those applications (`geopmbench`)
     calls `model-init` and `MPI_Init_thread`, when the count is
     summarized over the 2 processes, 1 call / 2 processes = 0.5 calls
     on average.
   - `geopmbench` contains region markup for the `dgemm` and `stream`
     regions.  This leads to distinct sections in the report denoted
     by those names.  Notice that the count of these regions is 1, not
     0.5.  This is because the `geopmbench` configuration in
     `test_multi_app.sh` specifies that the `stream` and `dgemm`
     regions will have a loop count of 2.  While the process that
     executes `geopmbench` does call each region twice, when that data
     is summarized in the report for the 2 processes, 2 loops / 2
     processes = 1 call.
   - `stress-ng` contains no region markup.  All of the data
     associated with this application will be accumulated in the
     `Unmarked Totals` section.
   - The assertions for `TIME@package` verify that both applications
     werer tracked properly.  `geopmbench` is affinitized to package
     0, and should have non-zero runtime for `TIME@package-0` and zero
     runtime for `TIME@package-1`.
   - This test does not make any assertions about the non-marked
     application regions (e.g. Unmarked Totals or Application Totals).
4. `test_non_mpi_app_tracked` - The "Unmarked Totals" section of the
   report contains valid data.
   - The data from `stress-ng` will be accumulated in this section as
     the application has no region markup.
   - `stress-ng` is affinitized to package 1 and should have non-zero
     runtime for `TIME@package-1`.
5. `test_runtime` - The summation of the runtime of the regions in the
   report and the "Unmarked Totals" runtime is nearly equal to the
   runtime reported by the "Application Totals".
   - This is a consistency check to ensure valid data between the
     individual regions and the "Application Totals".
