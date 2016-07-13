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

0. Profiling and Tracing an Unmodified Application
--------------------------------------------------
The first thing a user will want to do when integrating with the geopm
runtime is to analyze performance of the application being integrated
without modifying the application.  This can be enabled by setting a
few environment variables before launching the application.  See
tutorial-0.sh and tutorial-0.c for more information.

LD_DYNAMIC_WEAK and LD_PRELOAD
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Enable linking of the geopm PMPI wrappers.

GEOPM_PMPI_CTL
~~~~~~~~~~~~~~
Enable the controller to be run through the PMPI wrappers.

GEOPM_REPORT
~~~~~~~~~~~~
Enable report generation and set the base name for the generated
report files.

GEOPM_TRACE
~~~~~~~~~~~
Enable trace generation and set the base name for the generated trace
files.

