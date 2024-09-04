geopm_prof(3) -- application profiling interfaces
===================================================

Synopsis
--------

#include `<geopm_prof.h> <https://github.com/geopm/geopm/blob/dev/libgeopm/include/geopm_prof.h>`_

``Link with -lgeopm``

.. code-block:: c

   int geopm_prof_region(const char *region_name,
                         uint64_t hint,
                         uint64_t *region_id);

   int geopm_prof_enter(uint64_t region_id);

   int geopm_prof_exit(uint64_t region_id);

   int geopm_prof_epoch(void);

   int geopm_tprof_init(uint32_t num_work_unit);

   int geopm_tprof_post(void);

Description
-----------

The functions described here enable application feedback to the GEOPM
control algorithm for:

* Identifying regions of code
* Determining progress within regions
* Counting iterations through loops that contain inter-node synchronization
  points in the application

A region is defined here as a demarcated section of code, typically
with distinct compute, memory, or network characteristics. Defining
regions of code can be useful for: understanding the behavior of a section
of code, tuning control parameters to optimize the behavior of a region
independently of the rest of the code, to alleviate load imbalance in the
application (under the assumption that the region is bulk synchronous), etc.
Often, a set of regions will be marked up within an *epoch* (i.e. outer loop), and
will execute a fixed number of times per *epoch*. This can be leveraged to learn and optimize
control parameters, e.g. employing an iterative algorithm which synchronizes
periodically to alleviate load imbalance at a larger timescale. It is important
to avoid defining regions so that they are nested, as nested regions are ignored,
and only the outermost region is used for tuning when nesting occurs.

**Note:** The functions described herein do not require MPI.

``geopm_prof_region()``
  Registers an application region.  The *region_name* and *hint* are
  input parameters, and the *region_id* is output.  The *region_id*
  can be used with ``geopm_prof_enter()`` and ``geopm_prof_exit()`` to
  reference the region.  If the *region_name* has been previously
  registered, a call to this function will set the *region_id* but
  the state associated with the region is unmodified.  The
  *region_name* is used to determine the output *region_id* and is
  also displayed in the profiling report to identify the region.

  The *hint* is one of the values given by the enum ``geopm_region_hint_e``
  defined in `geopm_hint.h
  <https://github.com/geopm/geopm/blob/dev/libgeopmd/include/geopm_hint.h>`_ which
  determines the initial control settings.  The following hints are supported:

  ``GEOPM_REGION_HINT_UNKNOWN``
  Default value, provides no hint to the runtime.

  ``GEOPM_REGION_HINT_COMPUTE``
  Compute limited region.

  ``GEOPM_REGION_HINT_MEMORY``
  Memory bandwidth bound region.

  ``GEOPM_REGION_HINT_NETWORK``
  Inter-node network dominated region, default for unnested MPI
  calls.  User defined regions that have this hint will have the MPI
  time spent within this region attributed to the region as whole
  for the periods of time when all ranks are within an MPI function.

  ``GEOPM_REGION_HINT_IO``
  Disk input/output dominated region.

  ``GEOPM_REGION_HINT_SERIAL``
  Calculation that is not executed by a multi-threaded process (may
  be multi-process).

  ``GEOPM_REGION_HINT_PARALLEL``
  Calculation that is executed by a multi-threaded process in a
  hybrid thread/process parallelism (e.g. MPI + OpenMP).

  ``GEOPM_REGION_HINT_IGNORE``
  Region that control algorithms should ignore, and/or apply default
  policies.  This hint should be applied to application start up,
  shutdown, and events that do not happen on every trip through the
  outer loop.

  ``GEOPM_REGION_HINT_SPIN``
  Spin wait dominated region.

``geopm_prof_enter()``
  is called by the compute application to mark the beginning of the
  profiled compute region associated with the *region_id*. If this
  call is made after entering a different region, but before exiting
  that region, the call is ignored and an error code is returned
  (i.e. nested regions are ignored).

``geopm_prof_exit()``
  is called by the compute application to mark the end of a compute
  region.  If this region is nested then the call is ignored and an
  error code is returned.

``geopm_prof_epoch()``
  is called once for each pass through a computational loop
  containing inter-node synchronization events.  This call acts as a
  beacon signal emitted by each MPI rank as it begins a loop
  iteration.  The divergence in the elapsed time between calls by
  different MPI ranks is interpreted as an imbalance to be corrected
  by the runtime.  This function may be called at different places
  in an application, but it should not be used to mark a loop that
  is nested inside of another loop which is also marked.  All calls
  to ``geopm_prof_epoch()`` made inside of a user defined region with
  the ``GEOPM_REGION_HINT_IGNORE`` hint bit set will be ignored.

``geopm_tprof_init()``
  resets the thread progress and updates the total work for a
  threaded region.  Along with ``geopm_tprof_post()``, it provides a
  way for threads to report progress within a region.  This should
  be called by all threads with *num_work_unit*, the total number of
  work units to be completed by all threads after entering a thread
  parallel region.  The total work units corresponds to the number
  of times that the ``geopm_tprof_post()`` interface will be called by
  any thread to report progress within the region.

``geopm_tprof_post()``
  is called after a thread has completed each work unit to report
  progress.  This method signals the completion of one work unit out
  of the total passed to ``geopm_tprof_init()``.

OMP Integration
----------------

GEOPM is able to track OMP offload calls from OMP enabled applications
via the OMPT interface.  OMPT integration in GEOPM provides automation
for region identification, entry, and exit. With OMPT support, a GEOPM 
report can provide per-region metrics (such as region runtime, CPU/GPU
frequency, power/energy consumption, etc, without the need to mark up the
application. However, for each region where region progress is desired, the
application must be explicitly marked up with ``geopm_tprof_post()``.

Examples of progress markup with/without OMP are provided here.
`geopm/integration/test/test_progress.cpp <https://github.com/geopm/geopm/blob/dev/integration/test/test_progress.cpp>`_

Note that there can be a non-negligible overhead for using OMPT callbacks on
regions that are quick to execute, particularly when using
``geopm_tprof_post()``

Examples
--------

Obtaining region progress, identification, entry, and exit while using OpenMP:

.. code-block:: c

   #pragma omp parallel for
           for (idx = 0; idx < num_iter; ++idx) {
               example_function();
               geopm_tprof_post();
           }
   }

Obtaining region progress, identification, entry, and exit without OpenMP:

.. code-block:: c

    uint64_t region_id
    geopm_prof_region("region_name", GEOPM_REGION_HINT_COMPUTE, &region_id);
    geopm_tprof_init(num_iter);

    geopm_prof_enter(region_id);
    for (idx = 0; idx < num_iter; ++idx) {
        example_function();
        geopm_tprof_post();
    }
    geopm_prof_exit(region_id);


Full example:

.. code-block:: c

   #include <stdlib.h>
   #include <stdio.h>
   #include <string.h>
   #include <errno.h>
   #include <stdint.h>
   #include <mpi.h>
   #include <omp.h>

   #include "geopm_prof.h"
   #include "geopm_hint.h"


   int main(int argc, char **argv)
   {
       int chunk_size = 0;
       int err = 0;
       int index = 0;
       int rank = 0;
       int num_iter = 100000000;
       double sum = 0.0;
       int num_thread = 0;
       int thread_idx = 0 ;
       uint64_t region_id = 0;

       err = MPI_Init(&argc, &argv);
       if (!err) {
   #pragma omp parallel
   {
           num_thread = omp_get_num_threads();
   }
           chunk_size = num_iter / num_thread;
           if (num_iter % num_thread) {
               ++chunk_size;
           }
       }
       if (!err) {
           err = geopm_prof_region("loop_0", GEOPM_REGION_HINT_UNKNOWN, &region_id);
       }
       MPI_Barrier(MPI_COMM_WORLD);
       if (!err) {
           err = geopm_prof_enter(region_id);
       }
       if (!err) {
   #pragma omp parallel default(shared) private(thread_idx, index)
   {
           thread_idx = omp_get_thread_num();
           geopm_tprof_init(chunk_size);
   #pragma omp for reduction(+:sum) schedule(static, chunk_size)
           for (index = 0; index < num_iter; ++index) {
               sum += (double)index;
               geopm_tprof_post();
           }
   }
           err = geopm_prof_exit(region_id);
       }
       if (!err) {
           err = MPI_Comm_rank(MPI_COMM_WORLD, &rank);
       }
       if (!err && !rank) {
           printf("sum = %e\n\n", sum);
       }

       int tmp_err = MPI_Finalize();

       return err ? err : tmp_err;
   }

Errors
------

All functions described on this man page return an error code.  See
:doc:`geopm_error(3) <geopm_error.3>` for a full description of the error numbers and how
to convert them to strings.

See Also
--------

:doc:`geopm(7) <geopm.7>`\ ,
:doc:`geopm_error(3) <geopm_error.3>`
