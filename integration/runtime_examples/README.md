GEOPM Examples
==============
The examples directory contains externally developed benchmarks and a
few simple auxiliary programs that show some of GEOPM's features.

The [../tutorial](../tutorial) directory provides a more organized
demonstration of the GEOPM feature set and is a better place for first time
users to begin learning about GEOPM.


Benchmarks
==========
There are several directories dedicated to benchmarks used to
measure the effectiveness of the GEOPM runtime. In these directories we show
how to modify the benchmarks with the GEOPM instrumentation.

The [../experiment](../experiment) directory contains applications with more
structured build-and-run automation which may be more appropriate for automated
tests and experiments.

comd
----
The CoMD benchmark is a proxy for the computations done in molecular
dynamic simulations.  We provide a patch that introduces GEOPM
instrumentation along with instructions for downloading the benchmark
and applying the patch.

hacc
----
The HACC benchmark calculates the N-body gravitational problem as is
done in cosmological simulations of structure formation.  We have run
HACC benchmark with the GEOPM runtime, but are not able to publish the
modified version that was used.  We provide a note and contact
information in the hacc directory.

qbox
----
The QBox benchmark is a molecular dynamics simulation proxy
application using Density Functional Theory.  The computation has two
aspects: dense linear algebra and a three dimensional Fast Fourier
Transform.  We provide a patch that introduces GEOPM instrumentation
and a patch that modifies the run scripts.  Along with the patches we
provide instructions for downloading the benchmark and applying the
patches.

Auxiliary Executables
=====================
The executables described here serve as helpers to the build and test
infrastructure.  They may be useful in other contexts as well.

`geopm_print_error`
-------------------
Prints all GEOPM defined error codes and a brief explanation of what
they mean.  This is used to generate documentation for these error
codes based on the string that is printed by the library.

`geopmhash`
-----------
Will hash any string given on the command line using the GEOPM
hashing function.  This hash function is used to create a region
ID from a region name.

`print_affinity`
----------------
Print the MPI process and OpenMP thread affinity to standard output.
This is useful for testing assignment of CPU affinities for MPI/OpenMP
hybrid applications.


Early Example Code
==================
There are a number of source files compiled by the
examples/Makefile.mk which were written early in the development of
GEOPM as example use cases.  These still provide correct example code,
but the primary goal of writing these examples has been superseded
by the code in the geopm/tutorial directory.

`simple_prof_c`
---------------
Very basic use of the GEOPM profile mark up in a C program.

`simple_prof_f`
---------------
Very basic use of the GEOPM profile mark up in a Fortran program.

`timed_region`
--------------
Simple example that uses the GEOPM progress interface to report
progress through timed regions.

`synthetic_benchmark`
---------------------
Synthetic benchmark that shows how GEOPM can correct work imbalance
across MPI ranks.
