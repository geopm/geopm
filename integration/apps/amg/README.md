# AMG: Algebraic Multi-Grid

### Last update

October 8, 2020
Christopher Cantalupo <christopher.m.cantalupo@intel.com>

### Summary

From the authors <https://proxyapps.exascaleproject.org/app/amg>:

- "AMG is a parallel algebraic multigrid solver for linear systems
  arising from problems on unstructured grids."

- "The default problem is a Laplace type problem on an unstructured
  domain with various jumps and an anisotropy in one part."

### GEOPM Opportunity

This application may expose work imbalance across ranks, and appears
to be capable of drawing near TDP on Intel (R) Xeon (R) platforms.

### Obtaining the Source Code

Build script is written to download the latest from
<https://github.com/LLNL/AMG.git>.  This version is currently one
commit (3ada8a128e) ahead of the download posted to the CORAL-2
benchmark webside.

### Parallelism

This benchmark is a hybrid MPI/OpenMP application. It is unclear which
process / thread balance works best, but the AmgAppConf sets the
number of MPI ranks per node to 16.  Performance and scaling
characteristics depend significantly on AVX compiler flags provided.
The `build.sh` script specifies -xAVX2.

The command line arguments to the benchmark specify the type of
problem to be solved, the size of the problem and distribution of the
work across the ranks in a highly customizable way.  In the AmgAppConf
"problem 1" is executed and the sizing of the problem was chosen so it
will run for as long as possible without very large memory
requirements given the number of nodes requested.

### Modifications

#### Adding geopm profiling calls

The existing AMG benchmark uses the hypre infrastructure for reporting
timings: hypre_InitializeTiming() and hypre_InitializeTiming().  These
functions were modified to a call the geopm_prof_enter() /
geopm_prof_exit() calls.  This enables each timing reported by the
application to be recorded in the GEOPM report with the same naming
conventions.  Additionally a call to geopm_prof_epoch() was inserted
into the outer loop of the PCG solver.

#### Increasing the PCG iteration count

The PCG termination criteria were modified to execute many more
iterations of the solve phase (500 as opposed to 23).  This
modification increases the runtime of the benchmark to enable GEOPM
control algorithms more time to converge.

The AMG benchmark represents a part of a calculation that is typically
a building block for a larger application.  The entire PCG calculation
would typically be executed many times during an application
execution, each time with different input vectors to solve for.  For
example, this may happen for each time step of a physics simulation,
or once per realization of a random sample in a Monte Carlo technique.

Rather than increasing the iteration count of the solve phase, a more
realistic use case would be running the PCG solver repeatedly for
different input vectors.  A change like this would involve
more extensive benchmark modifications without achieving a
significantly different mix of instructions and memory accesses
executed over time.

Besides increasing the total runtime of the benchmark, this change to
the benchmark source code does have a small impact on the figure of
merit reported by the benchmark.  In one experiment increasing the
iteration count causes an increase of 4% to the figure of merit
reported.
