nasft
-----
The NAS-FT benchmark is a Fortran implementation of a distributed
Fast Fourier Transform algorithm.  The nasft directory contains an
optimized version of the NAS-FT benchmark that includes modifications
for GEOPM instrumentation.

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

