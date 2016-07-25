GEOPM TUTORIAL
==============
This directory contains a step by step tutorial on how to use the
geopm software.  There is a simple Makefile which is not part of the
geopm build system that compiles the tutorial code.  Each step has an
associated source and script file.  The script file will run the
associated program and demonstrate a geopm feature.  There is a script
called "tutorial-env.sh" which is sourced by all other tutorial
scripts, and defines some variables which describe the install
location for MPI and geopm.  The environment script may have to be
modified to describe the install locations on your system.  Each step
in the tutorial is documented below in this README.  The tutorial is a
work in progress.

Building the tutorials
----------------------
A build script called tutorial_build.sh can be executed to build the
tutorial suite.  The tutorial_build.sh script and the geopm_env.sh
script may have to be modified to suit your build environment.


0. Profiling and Tracing an Unmodified Application
--------------------------------------------------
The first thing a user will want to do when integrating with the geopm
runtime is to analyze performance of the application being integrated
without modifying the application.  This can be enabled by setting a
few environment variables before launching the application.  See the
geopm.7 man page for a more detailed description of these environment
variables.

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
to the MPI runtime execution.  This is because the GEOPM controller is
using one rank per compute node to execute and has removed this rank
from the world communicator.  This is important to understand when
launching the controller in this way.

The geopm report will be created in the file named

    tutorial_0_report-`hostname`

where geopm writes a report file for each compute node and the name
is extended with the hostname of the compute node.  Similarly the trace
file for each compute node will be named

    tutorial_0_trace-`hostname`

The report file will contain information about time and energy spent
in MPI regions and outside of MPI regions.

1. A slightly more realistic application
----------------------------------------
In tutorial 1 show a slightly more realistic application which
contains a loop that does a number of different types of operations.
In addition to sleeping, the loop does a memory intensive operation,
then a compute intensive operation, then again does a memory
intensive operation followed by a communication intensive operation.
In this example we are again using geopm without modifying the
application with the geopm APIs.

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
