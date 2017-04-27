GEOPM EXAMPLES
==============
The examples directory contains externally developed benchmarks and a
few simple auxiliary programs that show some of GEOPM's features.  The
geopm/tutorial directory provides a more organized demonstration of
the GEOPM feature set and is a better place for first time users to
begin learning about GEOPM.

BENCHMARKS
==========
There are several directories dedicated to standard benchmarks used to
measure the effectiveness of the GEOPM runtime.  These are benchmarks
developed by the HPC community to estimate how well real science
applications will perform by performing algorithms used in different
scientific domains.  In these directories we show how to modify the
benchmarks with the GEOPM instrumentation.

amg2013
-------
The AMG benchmark solves linear systems derived from multi-grid
sampling and unstructured meshes.  We provide a patch that introduces
GEOPM instrumentation along with instructions for downloading the
benchmark and applying the patch.

comd
----
The CoMD benchmark is a proxy for the computations done in molecular
dynamic simulations.  We provide a patch that introduces GEOPM
instrumentation along with instructions for downloading the benchmark
and applying the patch.

fft
---
The NAS-FFT benchmark is a Fortran implementation of a distributed
Fast Fourier Transform algorithm.  The fft directory contains an
optimized version of the NAS-FFT benchmark that includes modifications
for GEOPM instrumentation.

hacc
----
The HACC benchmark calculates the N-body gravitational problem as is
done in cosmological simulations of structure formation.  We have run
HACC benchmark with the GEOPM runtime, but are not able to publish the
modified version that was used.  We provide a note and contact
information in the hacc directory.

minife
------
The miniFE benchmark is a proxy application for finite element
analysis using a conjugate-gradient algorithm on a sparse linear
system without preconditioning.  We provide a patch that introduces
GEOPM instrumentation and a patch that optimizes the benchmark for
Xeon Phi.  Along with the patches we provide instructions for
downloading the benchmark and applying the patches.

nekbone
-------
The NEKBONE proxy application is a benchmark approximating large eddy
simulation calculations and direct numerical simulation of turbulence.
We provide a patch that introduces GEOPM instrumentation and a patch
that optimizes the benchmark for Xeon Phi.  Along with the patches we
provide instructions for downloading the benchmark and applying the
patches.

qbox
----
The QBox benchmark is a molecular dynamics simulation proxy
application using Density Functional Theory.  The computation has two
aspects: dense linear algebra and a three dimensional Fast Fourier
Transform.  We provide a patch that introduces GEOPM instrumentation
and a patch that modifies the run scripts.  Along with the patches we
provide instructions for downloading the benchmark and applying the
patches.

AUXILIARY EXECUTABLES
=====================
The executables described here serve as helpers to the build and test
infrastructure.  They may be useful in other contexts as well.

geopm_print_error
-----------------
Prints all GEOPM defined error codes and a brief explanation of what
they mean.  This is used to generate documentation for these error
codes based on the string that is printed by the library.

geopm_platform_supported
------------------------
Checks if the current hardware platform supports the GEOPM runtime
requirements.  In particular it verifies the crc32 instruction output,
checks that the CPUID is one that is supported by a GEOPM platform
implementation, and verifies that the msr-safe driver is installed.

geopmhash
---------
Will hash any string given on the command line using the GEOPM crc32
based hashing function.  This hash function is used to create a region
ID from a region name.

print_affinity
--------------
Print the MPI process and OpenMP thread affinity to standard output.
This is useful for testing assignment of CPU affinities for MPI/OpenMP
hybrid applications.


EARLY EXAMPLE CODE
==================
There are a number of source files compiled by the
examples/Makefile.mk which were written early in the development of
GEOPM as example use cases.  These still provide correct example code,
but the primary goal of writing these examples has been superseded
by the code in the geopm/tutorial directory.

simple_prof_c
-------------
Very basic use of the GEOPM profile mark up in a C program.

simple_prof_f
-------------
Very basic use of the GEOPM profile mark up in a Fortran program.

timed_region
------------
Simple example that uses the GEOPM progress interface to report
progress through timed regions.

threaded_step_example
---------------------
Example using the GEOPM thread profiling interface while running the
controller by stepping the algorithm in the primary compute
application.  Both of these features: threaded profiling and running
the controller without a separate thread are not features that are
frequently used, nor are these features covered in the tutorials.

synthetic_benchmark
-------------------
Synthetic benchmark that shows how GEOPM can correct work imbalance
across MPI ranks.

