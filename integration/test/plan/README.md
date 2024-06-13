# GEOPM Integration Test Plan

This directory contains the test plan for validating specific
features within GEOPM.

## Prerequisites

The tests described in this plan require GEOPM's systemd Service and Runtime to
be made available to the user.  A typical configuration involves installing
latest tagged release of the service globally, and leveraging a user build for
providing the GEOPM Runtime.  For information on installing the service via
system packages, see the [Install Guide](https://geopm.github.io/install.html).

### HPC

In an HPC system context, the GEOPM Service should be integrated into the
compute image and configured appropriately.  The GEOPM Runtime can be made
available to users via the spack recipe here:
[geopm-runtime](https://github.com/spack/spack/tree/develop/var/spack/repos/builtin/packages/geopm-runtime).
If spack has been used to build a geopm-runtime module, the user can simply
issue `module load geopm-runtime` to setup the necessary environment.  Ensure
the expected version is in use after loading the module:
```bash
[bgeltz@login_node1 ~]$ geopmctl --version
3.1.0


Copyright (c) 2015 - 2024 Intel Corporation. All rights reserved.
```

### Development Snapshot

Certain test scenarios may require a user-build from the main development
branch (dev) of GEOPM on GitHub.  The following steps can be used to build all
lyaers from source when not utilizing spack:

> [!NOTE]
> When setting up your `~/.geopmrc` file, use the following example
> as a basis for systems configured for PALS based job launch when
> setting GEOPM_SYSTEM_ENV:
> [australis_env.sh](../../config/australis_env.sh)

1. Clone the repo: ``git clone https://github.com/geopm/geopm.git``
2. The default branch is `dev`, but if necessary: ``git checkout dev``
3. Pull the latest updates: ``git rebase origin/dev`` or ``git pull``
4. Ensure you have completed the creation of your `~/.geopmrc` file
described in the [integration README](../../README.md#prerequisites).
5. You can manually build and install the required layers at this point, or use
``build.sh`` to automate the process:
 ```bash
source ~/.geopmrc
cd ${GEOPM_INSTALL}
GEOPM_SERVICE_CONFIG_OPTIONS="--disable-libcap --disable-io-uring" ${GEOPM_SOURCE}/integration/config/build.sh
```
6. After the script has completed, the installed artifacts are available in the
``GEOPM_INSTALL`` directory specified in ``~/.geopmrc``.  ``geopmdpy`` and
``geopmpy`` will be installed in ``~/.local`` with other pip installed
packages.
> [!NOTE]
> When going back to using system provided modules, unsure you have uninstalled any user-installed GEOPM python packages:
> ```bash
> python3 -m pip uninstall geopmpy geopmdpy
> ```
7. The runtime environment for using this build can be setup by issuing the
following in your terminal or job script: ``source
${GEOPM_INSTALL}/integration/config/run_env.sh`` or by manually modifying
``PATH`` and ``LD_LIBRARY_PATH`` environment variables.

Non-MPI App Profiling
---------------------

The integration test scripts `test_multi_app.[sh|py]`  in
`./non_mpi_profiling` have been designed to demonstrate GEOPM's
ability to profile applications that do not invoke MPI.

### test_multi_app.sh
This script depicts the necessary environment setup needed to use the GEOPM
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

Using `setsid` will limit the scope of the write lock management of the service
to the duration of that command.  Otherwise, due to how the service tracks the
session, the write lock will be associated with the TTY for the session, and
this my cause unexpected issues if other sessions need to use the write lock
capabilities.  Additionally, using `setsid` will ensure that all control values
are restored upon termination of the command rather than upon the termination
of the TTY.

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
`test_multi_app_report.yaml-node1` (if using `GEOPM_CTL_LOCAL`) or
`test_multi_app_report.yaml` (if *not* using `GEOPM_CTL_LOCAL`), the
time series data - `test_multi_app_trace.csv-node1` (if regular
tracing is enabled), and the time series data of per-process messages
between the application and the Controller -
`test_multi_app_trace_profile.csv-node1` (if profile tracing is
enabled).  Some of these files will be used to verify the run
completed as expected in the next section.

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
     `MPI_Init` and `MPI_Finalize` but will not otherwise use MPI
     (v3.1 vs. v3.2 differ on this; see below).
   - If the Runtime was built *without* MPI support, the
     `MPI_Init_thread`  region would not be present in the output
     files.  `geopmbench` can be invoked with the `GEOPMBENCH_NO_MPI`
     environment variable to avoid MPI calls. For more information
     about the environment configuration of `geopmbench`, see
     [here](https://geopm.github.io/geopmbench.1.html#environment).
     - `geopmbench ` @ v3.1 will always call `MPI_Init` and `MPI_Finalize`
       regardless of setting `GEOPMBENCH_NO_MPI`.  The only way to prevent this
       is to disable MPI at configure time.
     - `geopmbench` @ v3.2 and beyond will respect either `GEOPMBENCH_NO_MPI`
       or disabling MPI at configure time to properly prevent all MPI calls
       *including* Init and Finalize.
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
     were tracked properly.  `geopmbench` is affinitized to package
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

#### Example Runs

Single node runs can be accomplished by invoking test_multi_app.py
directly:
```
export PATH=${HOME}/build/stress-ng/bin:${PATH}
export PYTHONPATH=${GEOPM_SOURCE}:${PYTHONPATH} # GEOPM_SOURCE set in ~/.geopmrc or manually
GEOPM_NUM_NODE=1 mpiexec -n 1 -ppn 1 ~/geopm/integration/test/test_multi_app.py
```

Multi-node runs when libgeopm is built with MPI support, and when
`GEOPM_CTL_LOCAL` is unset, requires 2 steps.  The first is to generate
a single report with the data from all hosts.  For 4 nodes, the command
is as follows:
```
export PATH=${HOME}/build/stress-ng/bin:${PATH}
mpiexec -n 4 -ppn 1 ~/geopm/integration/test/test_multi_app.sh
```

The second is to invoke test_multi_app.py with ``--skip-launch`` since the data
is already present:
```
export PYTHONPATH=${GEOPM_SOURCE}:${PYTHONPATH}
GEOPM_NUM_NODE=4 ~/geopm/integration/test/test_multi_app.py -fv --skip-launch
```

### test_multi_app_python.sh

This script demonstrates how to profile a Python code in place of ``stress-ng``
in the previous example.  It's important to note a few differences between the
2 scripts:

1. Utilizing `LD_PRELOAD` selectively per-command rather than exported globally
allows the user to fine-tune which invocation of `python` is profiled.
Utilizing `LD_PRELOAD` this way is required for v3.1 or in the case where the
Python code to be profiled cannot be modified.
2. Starting with v3.2, `LD_PRELOAD` is not required.  Instead the user can
`import geopmpy.gffi` in the Python code to be profiled.  This call will
`dlopen` `libgeopm.so` in the context of the Python code.  Combined with the
use of the `GEOPM_PROGRAM_FILTER`, this will allow the user to profile their
code.
