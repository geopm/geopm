GEOPM TUTORIAL
==============
This directory contains a step by step tutorial on how to use the
GEOPM Runtime.  Each step has an associated source and script file.
The script file will run the associated program and demonstrate a
GEOPM feature.  Each step in the tutorial is documented below in this
README.

Building the tutorials
----------------------
A simple Makefile which is not part of the GEOPM autotools build
system compiles the tutorial code.  There are two build scripts, one
that compiles with the GNU toolchain: `tutorial_build_gnu.sh`, and one
that compiles with the Intel toolchain: `tutorial_build_intel.sh`.
The build scripts use the GEOPM install location defined in
`tutorial_env.sh`.  The required compilers will be enforced through the
usage of that script, and will rely on the configuration specified in
~/.geopmrc.  For more information see the [integration
README](../integration/README.md#prerequisites).

0. Profiling and Tracing an Unmodified Application
--------------------------------------------------
The first thing an HPC application user will want to do when
integrating their application with the GEOPM runtime is to analyze
performance of the application without modifying its source code.
This can be enabled by using the GEOPM launcher script or by setting a
few environment variables before launching the application.  The
`tutorial_0.sh` shows the various  methods for launching the GEOPM
runtime.  The first geopmlaunch method using the wrapper script for the
SLURM srun job launcher:

    geopmlaunch srun \
                -N 4 -n 16 \
                --geopm-preload \
                --geopm-ctl=application \
                --geopm-report=tutorial_0_report \
                --geopm-trace=tutorial_0_trace \
                --geopm-program-filter=tutorial_0 \
                -- ./tutorial_0

The second geopmlaunch method uses the wrapper script for the ALPS
aprun job launcher:

    geopmlaunch aprun \
                -N 4 -n 16 \
                --geopm-preload \
                --geopm-ctl=process \
                --geopm-report=tutorial_0_report \
                --geopm-trace=tutorial_0_trace \
                --geopm-program-filter=tutorial_0 \
                -- ./tutorial_0

If your system does not support srun or aprun launch, the third
non-geopmlaunch option is to set a few environment variables for launching
the GEOPM Controller process (`geopmctl`) and then launching both
the Controller and the app with `$MPIEXEC_*`:

    export LD_PRELOAD=$GEOPM_LIB/libgeopm.so
    export LD_DYNAMIC_WEAK=true
    export GEOPM_PROGRAM_FILTER=tutorial_0

    GEOPM_REPORT=tutorial_0_report \
    GEOPM_TRACE=tutorial_0_trace \
    GEOPM_NUM_PROC=${RANKS_PER_NODE} \
    $MPIEXEC_CTL geopmctl &

    $MPIEXEC_APP ./tutorial_0

The environment variables `MPIEXEC_CTL` and `MPIEXEC_APP` must also be
set to a command and options that will launch a job on your system.

`MPIEXEC_CTL` must be set to launch 1 process on each compute node.
This will launch the GEOPM Controller on each node which is required
for profiling.

`MPIEXEC_APP` must be set to the desired parameters for your
application with the appropriate number of nodes and ranks.

The `LD_PRELOAD` environment variable enables the GEOPM Runtime
library (i.e. `libgeopm`) to connect with the application at
startup.  When the GEOPM Runtime is built with MPI support (via
`--enable-mpi` at configure time) `LD_PRELOAD` also allows the Runtime
to interpose on MPI using the PMPI interface for MPI enabled
applications.  Linking directly to `libgeopm` has the same effects, but
this is not done in the `Makefile` for `tutorial_0` or `tutorial_1`.
See the geopm.7 man page for a detailed description of the other
environment variables.

The `tutorial_0.c` application is an extremely simple MPI application.
It queries the size of the world communicator, prints it out from rank
0 and sleeps for 5 seconds.

If the runtime was built with MPI support, the summary report will be
created in a file named:

    tutorial_0_report

or if the Runtime was built *without* MPI support, a file for each
compute node:

    tutorial_0_report-`hostname`

and one trace file will be output for each compute node and the name
of each trace file will be extended with the host name of the node it
describes:

    tutorial_0_trace-`hostname`

The report file will contain information about time and energy spent
in MPI regions (if MPI support is enabled) and outside of MPI regions
as well as the average CPU frequency, energy, power, and other
telemetry.

1. A slightly more realistic application
----------------------------------------
Tutorial 1 shows a slightly more realistic application.  This
application implements a loop that does a number of different types of
operations.  In addition to sleeping, the loop does a memory intensive
operation, then a compute intensive operation, then again does a
memory intensive operation followed by a communication intensive
operation.  In this example we are again using GEOPM without including
any GEOPM APIs in the application and using `LD_PRELOAD` to connect
the app to `gepomctl` and interpose the GEOPM Runtime on MPI (if enabled).

2. Adding GEOPM mark up to the application
------------------------------------------
Tutorial 2 takes the application used in tutorial 1 and modifies it
with the GEOPM profiling markup.  This enables the report and trace to
contain region specific information.

3. Adding work imbalance to the application
-------------------------------------------
Tutorial 3 modifies tutorial 2 removing all but the compute intensive
region from the application and then adding work imbalance across the
MPI ranks.  This tutorial also uses a modified implementation of the
DGEMM region which does set up and shutdown once per application run
rather than once per main loop iteration.  In this way the main
application loop is focused entirely on the DGEMM operation.  Note an
`MPI_Barrier` has also been added to the loop.  The work imbalance is
done by assigning the first half of the MPI ranks 10% more work than
the second half.  In this example we also enable GEOPM to do control
in addition to simply profiling the application.  This is enabled
through the `GEOPM_POLICY` environment variable which refers to a json
formatted policy file.  This control is intended to synchronize the
run time of each rank to overcome this load imbalance.  The tutorial 3
script executes the application with two different policies.  The
first run enforces a uniform power budget of 150 Watts to each compute
node using the governing agent alone, and the second run enforces an
average power budget of 150 Watts across all compute nodes while
diverting power to the nodes which have more work to do using the
balancing agent.

Note that the data in the summary report(s) or traces will only show
the expected benefits when the Runtime is built with MPI support.
This is due to the fact that the balancing agent requires MPI to
implement its communication algorithm for optimally re-distributing
power to the nodes in a job.

4. Adding artificial imbalance to the application
-------------------------------------------------
Tutorial 4 enables artificial injection of imbalance.  This differs
from from tutorial by 3 having the application sleep for a period of
time proportional to the amount of work done rather than simply
increasing the amount of work done.  This type of modeling is useful
when the amount of work within cannot be easily scaled.  The imbalance
is controlled by a file who's path is given by the `IMBALANCER_CONFIG`
environment variable.  This file gives a list of hostnames and
imbalance injection fraction.  An example file might be:

    my-cluster-node3 0.25
    my-cluster-node11 0.15

which would inject 25% extra time on node with hostname
"my-cluster-node3" and 15% extra time on node "my-cluster-node11" for
each pass through the loop.  All nodes which have hostnames that are
not included in the configuration file will perform normally.  The
`tutorial_4.sh` script will create a configuration file called
`tutorial_3_imbalance.conf` if one does not exist, and one of the
nodes will have a 10% injection of imbalance.  The node is chosen
arbitrarily by a race if the configuration file is not present.

Note that the data in the summary report(s) or traces will only show
the expected benefits when the Runtime is built with MPI support.
This is due to the fact that the balancing agent requires MPI to
implement its communication algorithm for optimally re-distributing
power to the nodes in a job.

5. Using the progress interface
-------------------------------
A computational application may make use of the `geopm_tprof_init()`
and `geopm_tprof_post()` interfaces to report fractional progress
through a region to the controller.  These interfaces are documented
in the `geopm_prof(3)` man page.  In tutorial 5 we modify the stream
region to send progress updates though either the threaded or
unthreaded interface depending on if OpenMP is enabled at compile
time.  Note that the unmodified tutorial build scripts do enable
OpenMP, so the `geopm_tprof*` interfaces will be used by default.  The
progress values recorded can be seen in the trace output.

6. The model application
------------------------
Tutorial 6 uses the `geopmbench` tool and configures it with the json
input file.  The `geopmbench` application is documented in the
`geopmbench(1)` man page and can be use to to a wide range of
experiments with GEOPM.  Note that geopmbench is used in most
of the GEOPM integration tests.

7. Agent and IOGroup extension
------------------------------
See agent and iogroup sub-directories and their enclosed README.md
files for information about how to extend the GEOPM Runtime through
the development of plugins.
