GEOPM TUTORIAL
==============
This directory contains a step by step tutorial on how to use the
GEOPM package.  Each step has an associated source and script file.
The script file will run the associated program and demonstrate a
GEOPM feature.  There is a script called "tutorial_env.sh" which is
sourced by all other tutorial scripts, and defines variables which
describe the install location of GEOPM.  The environment script may
have to be modified to describe the installed locations on your
system.  Each step in the tutorial is documented below in this README.

Building the tutorials
----------------------
A simple Makefile which is not part of the GEOPM autotools build
system compiles the tutorial code.  There are two build scripts, one
that compiles with the GNU toolchain: "tutorial_build_gnu.sh", and one
that compiles with the Intel toolchain: "tutorial_build_intel.sh".
The build scripts use the GEOPM install location defined in
"tutorial_env.sh".  If "mpicc" is not in the user's PATH, the
environment variable "MPICC" must be set to the path of the user's MPI
C compiler wrapper.


0. Profiling and Tracing an Unmodified Application
--------------------------------------------------
The first thing an HPC application user will want to do when
integrating their application with the GEOPM runtime is to analyze
performance of the application without modifying its source code.
This can be enabled by using the GEOPM launcher script or by setting a
few environment variables before launching the application.  The
tutorial_0.sh shows three different methods for launching the GEOPM
runtime.  The first method uses the geopmlaunch wrapper script for the
SLURM srun job launcher:

    geopmlaunch srun \
                -N 2 -n 8 \
                --geopm-preload --geopm-ctl=process \
                --geopm-report=tutorial_0_report --geopm-trace=tutorial_0_trace \
                -- ./tutorial_0

The second method uses the geopmlaunch wrapper script for the ALPS
aprun job launcher:

    geopmlaunch aprun \
                -N 4 -n 8 \
                --geopm-preload --geopm-ctl=process \
                --geopm-report=tutorial_0_report --geopm-trace=tutorial_0_trace \
                -- ./tutorial_0

If your system does not support srun or aprun launch, the third option
is to set a few environment variables for GEOPM as follows:

    LD_PRELOAD=$GEOPM_LIBDIR/libgeopm.so
    LD_DYNAMIC_WEAK=true
    GEOPM_PMPI_CTL=process
    GEOPM_REPORT=tutorial_0_report
    GEOPM_TRACE=tutorial_0_trace

The environment variable MPIEXEC must also be set to a command and
options that will launch a job on your system using two compute nodes
and ten MPI ranks (e.g. MPIEXEC='srun -N 2 -n 10').

The LD_PRELOAD environment variable enables the GEOPM library to
interpose on MPI using the PMPI interface.  Linking directly to
libgeopm has the same effect, but this is not done in the Makefile for
tutorial_0 or tutorial_1.  See the geopm.7 man page for a detailed
description of the other environment variables.

The tutorial_0.c application is an extremely simple MPI application.
It queries the size of the world communicator, prints it out from rank
0 and sleeps for 5 seconds.

Since this script uses the command line option to the launcher
"--geopm-ctl=process" or sets the environment variable
"GEOPM_PMPI_CTL" to "process" you will notice that the MPI world
communicator is reported to have one fewer rank per compute node than
the was requested when the job was launched.  This is because the
GEOPM controller is using one rank per compute node to execute the
runtime and has removed this rank from the world communicator.  This
is important to understand when launching the controller in this way.

The summary report will be created in the file named

    tutorial_0_report

and one trace file will be output for each compute node and the name
of each trace file will be extended with the host name of the node it
describes:

    tutorial_0_trace-`hostname`

The report file will contain information about time and energy spent
in MPI regions and outside of MPI regions as well as the average CPU
frequency.

1. A slightly more realistic application
----------------------------------------
Tutorial 1 shows a slightly more realistic application.  This
application implements a loop that does a number of different types of
operations.  In addition to sleeping, the loop does a memory intensive
operation, then a compute intensive operation, then again does a
memory intensive operation followed by a communication intensive
operation.  In this example we are again using GEOPM without including
any GEOPM APIs in the application and using LD_PRELOAD to interpose
GEOPM on MPI.

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
MPI_Barrier has also been added to the loop.  The work imbalance is
done by assigning the first half of the MPI ranks 10% more work than
the second half.  In this example we also enable GEOPM to do control
in addition to simply profiling the application.  This is enabled
through the GEOPM_POLICY environment variable which refers to a json
formatted policy file.  This control is intended to synchronize the
run time of each rank to overcome this load imbalance.  The tutorial 3
script executes the application with two different policies.  The
first run enforces a uniform power budget of 150 Watts to each compute
node using the governing agent alone, and the second run enforces an
average power budget of 150 Watts across all compute nodes while
diverting power to the nodes which have more work to do using the
balancing agent.


4. Adding artificial imbalance to the application
-------------------------------------------------
Tutorial 4 enables artificial injection of imbalance.  This differs
from from tutorial by 3 having the application sleep for a period of
time proportional to the amount of work done rather than simply
increasing the amount of work done.  This type of modeling is useful
when the amount of work within cannot be easily scaled.  The imbalance
is controlled by a file who's path is given by the IMBALANCER_CONFIG
environment variable.  This file gives a list of hostnames and
imbalance injection fraction.  An example file might be:

    my-cluster-node3 0.25
    my-cluster-node11 0.15

which would inject 25% extra time on node with hostname
"my-cluster-node3" and 15% extra time on node "my-cluster-node11" for
each pass through the loop.  All nodes which have hostnames that are
not included in the configuration file will perform normally.  The
tutorial_4.sh script will create a configuration file called
"tutorial_3_imbalance.conf" if one does not exist, and one of the
nodes will have a 10% injection of imbalance.  The node is chosen
arbitrarily by a race if the configuration file is not present.

5. Using the progress interface
-------------------------------
A computational application may make use of the geopm_prof_progress()
or the geopm_tprof_post() interfaces to report fractional progress
through a region to the controller.  These interfaces are documented
in the geopm_prof_c(3) man page.  In tutorial 5 we modify the stream
region to send progress updates though either the threaded or
unthreaded interface depending on if OpenMP is enabled at compile
time.  Note that the unmodified tutorial build scripts do enable
OpenMP, so the geopm_tprof\* interfaces will be used by default.  The
progress values recorded can be seen in the trace output.

6. The model application
------------------------
Tutoral 6 uses the geopmbench tool and configures it with the json
input file.  The geopmbench application is documented in the
geopmbench(1) man page and can be use to to a wide range of
experiments with GEOPM.  Note that geopmbench is used in most
of the GEOPM integration tests.

7. Agent and IOGroup extension
------------------------------
See agent and iogroup sub-directories and their enclosed README.md
files for information about how to extend the GEOPM runtime through
the development of plugins.

8. YouTube Videos
-----------------
A video demonstration of these tutorials is available online here:

https://www.youtube.com/playlist?list=PLwm-z8c2AbIBU-T7HnMi_Pux7iO3gQQnz

These videos do not reflect changes that have happened to GEOPM since
September 2016 when they were recorded.  In particular, the videos do
not use the geopmpy.launcher launch wrapper which was introduced prior
to the v0.3.0 alpha release.  The tutorial scripts have been updated
to use the launcher, but the videos have not.  These videos also use
the Decider/Platform/PlatformImp code path which are deprecated and
will be removed in the 1.0 release in favor of the
Agent/PlatformIO/IOGroup class relationship.
