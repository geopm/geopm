/*
 * Copyright (c) 2015, Intel Corporation
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in
 *       the documentation and/or other materials provided with the
 *       distribution.
 *
 *     * Neither the name of Intel Corporation nor the names of its
 *       contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY LOG OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef GEOPM_H_INCLUDE
#define GEOPM_H_INCLUDE

#include <mpi.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Opaque structure which is a handle for a geopm::Controller object. */
struct geopm_ctl_c;

struct geopm_prof_c;

/* Returns a human readable string representing the geopm version of the
   linked geopm library. */
const char *geopm_version(void);

/************************/
/* OBJECT INSTANTIATION */
/************************/

/* Create a geopm_ctl_c object.  An error will occur and an error code
   returned if both policy_config and policy_key are specified or both are
   unspecified.

    num_level: Number of levels of the hierarchy tree, if zero optimal value
               will be guessed.

    fan_out: A vector of tree fan out values for each level of the tree.  The
             fan out of each node is a function only of its depth in the tree.
             The tree is balanced.  If pointer is NULL then MPI_Dims_create()
             will be used.  Product of values in fan_out must equal the
             num_node calculated by the function geopm_num_node().

    policy_config: The path to a configuration file that sets the policy
                   attributes of the job. If pointer is NULL or string is
                   empty use policy_key instead.

    policy_key: POSIX shared memory key referring to a shared memory region on
                the root node of the communicator configured by the resource
                manager to dynamically control policy at job runtime.  If
                pointer is NULL or string is empty, use policy_config instead.

    prof: A geopm_prof_c object which

    comm: MPI communicator that includes all compute nodes under control.  If
          controller will be used only for sampling and not for control then
          pass MPI_COMM_NULL.

    ctl: The geopm controller object created by the call.  Use
         geopm_ctl_destroy() to release resources. */
int geopm_ctl_create(int num_level,
                     const int *fan_out,
                     const char *policy_config,
                     const char *policy_key,
                     struct geopm_prof_c *prof,
                     MPI_Comm comm,
                     struct geopm_ctl_c **ctl);


/* Destroy all resources associated with the "ctl" object. */
int geopm_ctl_destroy(struct geopm_ctl_c *ctl);

/********************/
/* POWER MANAGEMENT */
/********************/

/* Step the control algorithm sending samples up and then policies down the
   tree hierarchy once.  If the calling process detects a change in policy the
   proccess will enforce the policy by writing MSR values. */
int geopm_ctl_step(struct geopm_ctl_c *ctl);

/* Run control loop continuously until kill signal is received. */
int geopm_ctl_run(struct geopm_ctl_c *ctl);

/* Create POSIX thread running control loop continuously until thread is
   killed. */
int geopm_ctl_pthread(struct geopm_ctl_c *ctl,
                      const pthread_attr_t *attr,
                      pthread_t *thread);

/* Use MPI_Comm_spawn() to spawn a new communicator with one process per
   compute node.  The geopm application will be spawned on this communicator,
   and future use of the geopm_ctl_prof_* APIs by the calling process will
   send profile information to the control process on the same node via POSIX
   shared memory. */
int geopm_ctl_spawn(struct geopm_ctl_c *ctl);

/******************************/
/* INTERPROCESS SHARED MEMORY */
/******************************/

/* Future use of the geopm_ctl_run(), geopm_ctl_step(), geopm_ctl_pthread(),
   or geopm_ctl_spawn() APIs will read samples for control from the shared
   memory region referenced by the key.  Note that if geopm is run in this
   mode (using interprocess shared memory), then the processes calling the
   geopm_ctl_prof_* APIs should be a different process than the one calling
   the geopm_ctl_run(), geopm_ctl_step(), or geopm_ctl_pthread() APIs.  A call
   to geopm_ctl_connect() must be made prior to calling geopm_ctl_spawn(). */
int geopm_ctl_connect(struct geopm_ctl_c *ctl,
                      const char *sample_key);

/*************************/
/* APPLICATION PROFILING */
/*************************/

/* Create a profile object, prof.  Requires a name which is displayed when the
   profile is printed.  The sample_reduce parameter determines the level of
   reduction that occurs when a sample is generated with the
   geopm_prof_sample() API.  If sample_reduce is 0, no reduction occurs when
   sampling samples include thread specific information, if sample_reduce is 1
   then thread specific data is aggregated to the MPI rank level, if
   sample_reduce is 2 then data is aggregated to the compute node granularity
   over all MPI ranks on each compute node. */
int geopm_prof_create(const char *name,
                      int sample_reduce,
                      struct geopm_prof_c **prof);

/* Destroy all resources associated with the prof object. */
int geopm_prof_destroy(struct geopm_prof_c *prof);

/* Future use of the geopm_ctl_prof_* APIs by the calling process will send
   profile information to the control process on the same node via POSIX
   shared memory.  */
int geopm_prof_connect(struct geopm_prof_c *prof,
                       const char *sample_key);

/* Register an application region with the profile object.  The region name
   and hint are input parameters, and the region id is returned.  If the
   region name has been previously registered, a call to this function will
   return the region id, but the state in the controller associated with the
   region is unmodified. */
int geopm_prof_register(struct geopm_prof_c *prof,
                        const char *region_name,
                        long policy_hint,
                        int *region_id);

/* Called by compute application to mark the beginning of a profiled compute
   region.  If this call is made after entering a different region, but before
   exiting that region, the call is simply ignored (i.e. nested regions are
   not heeded).*/
int geopm_prof_enter(struct geopm_prof_c *prof,
                     int region_id);

/* Called by compute application to mark the end of a compute region.  If the
   region_id does not match the region_id of the last call to
   geopm_prof_prof_enter() which was not nested, then this call is ignored. */
int geopm_prof_exit(struct geopm_prof_c *prof,
                    int region_id);

/* Called by compute application in single threaded context to signal the
   fractional progress through the work required to complete the region. If
   the region_id does not match the region_id of the last call to
   geopm_prof_prof_enter() which was not nested, then this call is ignored. */
int geopm_prof_progress(struct geopm_prof_c *prof,
                        int region_id,
                        double fraction);

/* Called just prior to the highest level global synchronization point in an
   application.  This may occur in the application's outermost loop in an
   iterative algorithm just prior to the last synchronizing MPI call.  There
   should be just one place in an application code where this call occurs, and
   it should be called repeatedly inside of a loop. */
int geopm_prof_outer_sync(struct geopm_prof_c *prof);

/* Derive a sample based on the profiling information collected.  This may
   aggregate data as specified by the sample_reduce parameter passed when the
   profile object was created. */
int geopm_prof_sample(struct geopm_prof_c *prof);

/* Enable a profiling feature. */
int geopm_prof_enable(struct geopm_prof_c *prof,
                      const char *feature_name);

/* Disable a profiling feature. */
int geopm_prof_disable(struct geopm_prof_c *prof,
                       const char *feature_name);

/* Write a report file to the path "report" summarizing profile information at
   the granularity of the specified "depth" in the control hierarchy.  All
   profile information above the specified depth in the is contained in the
   report.  A depth of zero gives only statistics aggregated over the entire
   job. */
int geopm_prof_fprint(FILE *fid,
                      struct geopm_prof_c *prof,
                      int depth);
/* Print a report to standard output.  This call is otherwise the same as
   geopm_prof_fprint() */
int geopm_prof_print(struct geopm_prof_c *prof,
                     int depth);

/***************/
/* HELPER APIS */
/***************/

/* Number of compute nodes associated with the communicator comm.  A compute
   node is defined to be a shared memory coherency domain. The product of the
   elements of "fan_out" passed to geopm_ctl_create() must equal
   "num_node". */
int geopm_num_node(MPI_Comm comm,
                   int *num_node);

/* If the OpenMP threads are statically affinitized this returns a CPU_SET(3)
   which can be used with pthread_attr_setaffinity_np(3) to bind the pthread
   created by geopm_ctl_pthread() to CPUs that do not have an OpenMP thread
   affinity.  The mask generated when OpenMP threads are not statically
   affinitized is unreliable.  The "no_omp" mask is zeroed and an error code
   is returned when all online CPUs have an OpenMP thread affinity. */
int geopm_no_omp_cpu(int num_cpu, cpu_set_t *no_omp)

/* Will calculate the "norm" vector that can be used with
   geopm_ctl_prof_progress_threaded_min() in the case where a loop is OpenMP
   parallel using the static scheduling algorithm with the specified
   "chunk_size". */
int geopm_omp_sched_static_norm(int num_iter,
                                int chunk_size,
                                int num_thread,
                                double *norm,
                                double *progress);

/* Called by compute application in multi-threaded context calculate
   fractional progress through the work required to complete the region.  The
   fractional progress is calculated to be the minimum of fractional progress
   (min_i(progress[stride*i]*norm[i])) of all of the threads that are given
   work. */
double geopm_progress_threaded_min(int num_thread,
                                   size_t stride,
                                   const uint32_t *progress,
                                   const double *norm);

/* Called by compute application in multi-threaded context to calculate
   fractional progress through the work required to complete the region.  The
   fractional progress is calculated to be the sum of the values in the
   "progress" vector multiplied by "norm". */
int geopm_progress_threaded_sum(int num_thread,
                                size_t stride,
                                const uint32_t *progress,
                                double norm,
                                double *progress);

#ifdef __cplusplus
}
#endif
#endif
