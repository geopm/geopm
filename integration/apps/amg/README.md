# AMG: Algebraic Multi-Grid

### Summary

From the authors <https://proxyapps.exascaleproject.org/app/amg>:

- "AMG is a parallel algebraic multigrid solver for linear systems
  arising from problems on unstructured grids."

- "The default problem is a Laplace type problem on an unstructured
  domain with various jumps and an anisotropy in one part."


### Versions available:

- The version of posted to the CORAL-2 benchmark website is curently
  one patch behind the github repository:
  https://asc.llnl.gov/sites/asc/files/2020-09/amg-master-5.zip

- The github repository has a correction for how "FOM_2" is
  calculated:
  https://github.com/LLNL/AMG/commit/3ada8a128e311543e84d9d66344ece77924127a8

- This value is not printed in the configuration set up in amg.py (we
  specify `-problem 1` on the CLI), rather "FOM_1" is printed at the
  end of the log.


### Parallelism

This benchmark is a hybrid MPI/OpenMP application. Unclear which
process / thread balance works best, but the app config sets the
number of MPI ranks per node to 16.  Performance and scaling
characteristics depend significantly on AVX compiler flags provided.
The `build.sh` script specifies -xAVX2.

The command line arguments to the benchmark specify the type of
problem to be solved, the size of the problem and distribution of the
work across the ranks in a highly customizable way.  In the AmgAppConf
"problem 1" is executed and the sizing of the problem was chosen so it
would run for as long as possible without very large memory
requirements given the number of nodes requested.
