GEOPM TUTORIAL
==============
This directory contains a step by step tutorial on how to use the
geopm package.  Each step has an associated source and script file.
The script file will run the associated program and demonstrate a
geopm feature.  There is a script called "tutorial_env.sh" which is
sourced by all other tutorial scripts, and defines variables which
describe the install location of geopm.  The environment script may
have to be modified to describe the installed locations on your
system.  Each step in the tutorial is documented below in this README.
The tutorial is a work in progress.

Building the tutorials
----------------------
A simple Makefile which is not part of the geopm autotools build
system compiles the tutorial code.  There are two build scripts, one
which compiles with the GNU toolchain: "tutorial_build_gnu.sh", and
one which compiles with the Intel toolchain:
"tutorial_build_intel.sh".  The build scripts use the geopm install
location defined in "tutorial_env.sh".  If "mpicc" is not in the
user's PATH, the environment variable "MPICC" must be set to the path
of the to the user's MPI C compiler wrapper.


0. Profiling and Tracing an Unmodified Application
--------------------------------------------------
The first thing an HPC application user will want to do when
integrating their application with the geopm runtime is to analyze
performance of the application without modifying its source code.
This can be enabled by setting a few environment variables before
launching the application.  The tutorial_0.sh sets these as follows:

    LD_PRELOAD=$GEOPM_LIBDIR/libgeopm.so
    LD_DYNAMIC_WEAK=true
    GEOPM_PMPI_CTL=process
    GEOPM_REPORT=tutorial_0_report
    GEOPM_TRACE=tutorial_0_trace

The LD_PRELOAD environment variable enables the geopm library to
interpose on MPI using the PMPI interface.  Linking directly to
libgeopm has the same effect, but this is not done in the Makefile for
tutorial_0 or tutorial_1.  See the geopm.7 man page for a detailed
description of the other environment variables.

The tutorial_0.c application is an extremely simple MPI application.
It queries the size of the world communicator, prints it out from rank
0 and sleeps for 5 seconds.  Submit this script while specifying the
number of ranks using the MPI runtime execution appropriate for your
environment.  Some examples are:

    mpiexec -n 2 ./tutorial_0.sh

or

    srun -N 2 -n 6 ./tutorial_0.sh

Since this script sets the environment variable "GEOPM_PMPI_CTL" to
"process" you will notice that the MPI world communicator is reported
to have one fewer rank per compute node than the "-n" parameter passed
to the MPI runtime execution.  This is because the geopm controller is
using one rank per compute node to execute the runtime and has removed
this rank from the world communicator.  This is important to
understand when launching the controller in this way.

The geopm report will be created in the file named

    tutorial_0_report-`hostname`

where geopm writes a report file for each compute node and the name
is extended with the hostname of the compute node.  Similarly the trace
file for each compute node will be named

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
operation.  In this example we are again using geopm without including
any geopm APIs in the application and using LD_PRELOAD to interpose
geopm on MPI.

2. Adding geopm mark up to the application
------------------------------------------
Tutorial 2 takes the application used in tutorial 1 and modifies it
with the geopm profiling markup.  This enables the report and trace to
contain region specific information.

3. Adding work imbalance to the application
-------------------------------------------
Tutorial 3 modifies tutorial 2 by adding a load imbalance in the
compute intensive region of the application.  It scales the amount of
work done by each rank by one percent so that rank N is doing N
percent more work than rank 0.  In this example we also enable geopm
to do control in addition to simply profiling the application.  This
is enabled through the GEOPM_POLICY environment variable which refers
to a json formatted policy file.  This control is intended to
synchronize the run time of each rank in the face of this load
imbalance.

4. Adding artificial imbalance to the application
-------------------------------------------------
Tutorial 4 enables artificial injection of imbalance and includes a
loop that runs only the DGEMM region.  The imbalance is controlled by
a file who's path is given by the IMBALANCER_CONFIG environment
variable.  This file gives a list of hostnames and imbalance injection
fraction.  An example file might be:

    my-cluster-node3 0.25
    my-cluster-node11 0.15

which would inject 25% extra time on node with hostname
"my-cluster-node3" and 15% extra time on node "my-cluster-node11" for
each pass through the loop.  All nodes which have hostnames that are
not included in the configuration file will perform normally.  The
tutorial_4.sh script will create a configuration file called
"tutorial_3_imbalance.conf" if one does not exist, and one of the
nodes will have a 50% injection of imbalance.  The node is chosen
arbitrarily by a race if the configuration file is not present.

5. Using the progress interface
-------------------------------
A computational application may make use of the geopm_prof_progress()
or the geopm_tprof_progress() interfaces to report fractional progress
through a region to the controller.  These interfaces are documented
in the geopm_prof_c(3) man page.  In tutorial 5 we modify the stream
region to send progress updates though either the threaded or
unthreaded interface depending on if OpenMP is enabled at compile
time.  Note that the unmodified tutorial build scripts do enable
OpenMP, so the geopm_tprof* interfaces will be used by default.  The
progress values recorded can be seen in the trace output.

6. The model application
------------------------
Tutorial 6 is the first tutorial written in C++.  The regions defined
in the previous examples (with the exception of the sleep region) have
non-trivial amounts of time dedicated to start-up and shutdown
operations in each call to execute the region.  These include memory
allocation, value initialization and memory deallocation.  In tutorial
6 we move these start-up and shutdown operations into the beginning
and end of the application so that the execution of a region is
dedicated entirely to a compute intensive (dgemm), memory intensive
(stream) or network intensive (all2all) operation.  The ModelRegion
and ModelApplication will form the basis for the geopm integration
tests.

The tutorial_6 application is the first to accept command line
parameters.  The tutorial_6 --help output:

    ./tutorial_6 -h | --help
        Print this help message.

    ./tutorial_6 [--verbose] [config_file]

        --verbose: Print output from rank zero as every region executes.

        config_file: Path to json file containing loop count and sequence
                     of regions in each loop.

                     Example configuration json string:

                     {"loop-count": 100,
                      "region": ["sleep", "stream", "dgemm", "stream", "all2all"],
                      "big-o": [1.0, 1.0, 1.0, 1.0, 1.0]}

                     The "loop-count" value is an integer that sets the
                     number of loops executed.  Each time through the loop
                     the regions listed in the "region" array are
                     executed.  The "big-o" array gives double precision
                     values for each region.  Region names can be one of
                     the following options:

                     sleep: Executes clock_nanosleep() for big-o seconds.

                     stream: Executes stream "triadd" on a vector with
                     length proportional to big-o.

                     dgemm: Dense matrix vector multiply with floating
                     point operations proportional to big-o.

                     all2all: All processes send buffers to all other
                     processes.  The time of this operation is
                     proportional to big-o.

        If "-imbalance" is appended to any region name in the
        configuration file then the "IMBALANCER_CONFIG" environment
        variable will be used to delay processes on selected hosts for
        the region.  The "IMBALANCER_CONFIG" environment variable
        should be set to point to a text file with two entries on each
        line.  The first entry is the hostname to be delayed and the
        second column is the fractional delay added to the region.
        Example imbalancer config file:

            my-compute-node-3 0.05
            my-compute-node-15 0.15

        This would inforce a 5% delay on my-compute-node-3 and a 15%
        delay on my-compute-node-15.