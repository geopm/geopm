geopm_prof(3) -- application profiling interfaces
===================================================

Synopsis
--------

#include `<geopm_prof.h> <https://github.com/geopm/geopm/blob/dev/src/geopm_prof.h>`_

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
control algorithm for identifying regions of code, progress within
regions, and iterations through loops that contain inter-node
synchronization points in the application.  Regions of code define
periods in the application during which control parameters are tuned
with the expectation that control parameters for a region can be
optimized independently of other regions.  In this way a region is
associated with a set of control parameters which can be optimized,
and future time intervals associated with the same region will benefit
from the application of control parameters which were determined from
tuning within previous occurrences of the region.  There are two
competing motivations for defining a region within the application.
The first is to identify a section of code that has distinct compute,
memory or network characteristics.  The second is to avoid defining
these regions such that they are nested within each other, as nested
regions are ignored, and only the outer most region is used for tuning
when nesting occurs.  Identifying progress within a region can be used
to alleviate load imbalance in the application under the assumption
that the region is bulk synchronous.  Under the assumption that the
application employs an iterative algorithm which synchronizes
periodically the user can alleviate load imbalance on larger time
scales than the regions provide.  This is done by marking iterations
through an outer loop in the application, the *epoch*.

**WARNING:** All of the functions described herein require that MPI has
been initialized (via ``MPI_Init()`` or ``MPI_Init_thread()``) and is properly
functioning before they are invoked.  These functions make use of various
MPI calls in their implementations and will return errors if MPI is not
initialized.


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
  <https://github.com/geopm/geopm/blob/dev/service/src/geopm_hint.h>`_ which
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

Example
-------

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
