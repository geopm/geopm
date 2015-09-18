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
#include <pthread.h>

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
int geopm_ctl_create(const char *policy_config,
                     MPI_Comm comm,
                     struct geopm_prof_c *prof,
                     struct geopm_ctl_c **ctl);


/* Destroy all resources associated with the "ctl" object. */
int geopm_ctl_destroy(struct geopm_ctl_c *ctl);

/********************/
/* POWER MANAGEMENT */
/********************/
int geopm_ctl_pthread(struct geopm_ctl_c *ctl,
                      const pthread_attr_t *attr,
                      pthread_t *thread);

int geopm_ctl_spawn(struct geopm_ctl_c *ctl);

/*************************/
/* APPLICATION PROFILING */
/*************************/
int geopm_prof_create(const char *name,
                      int sample_reduce,
                      const char *sample_key,
                      struct geopm_prof_c **prof);

int geopm_prof_destroy(struct geopm_prof_c *prof);

int geopm_prof_register(struct geopm_prof_c *prof,
                        const char *region_name,
                        long policy_hint,
                        int *region_id);

int geopm_prof_enter(struct geopm_prof_c *prof,
                     int region_id);

int geopm_prof_exit(struct geopm_prof_c *prof,
                    int region_id);

int geopm_prof_progress(struct geopm_prof_c *prof,
                        int region_id,
                        double fraction);

int geopm_prof_outer_sync(struct geopm_prof_c *prof);

int geopm_prof_sample(struct geopm_prof_c *prof);

int geopm_prof_enable(struct geopm_prof_c *prof,
                      const char *feature_name);

int geopm_prof_disable(struct geopm_prof_c *prof,
                       const char *feature_name);

int geopm_prof_print(struct geopm_prof_c *prof,
                     int depth);

int geopm_prof_fprint(struct geopm_prof_c *prof,
                      int depth,
                      FILE *fid);


/***************/
/* HELPER APIS */
/***************/
int geopm_no_omp_cpu(int num_cpu, cpu_set_t *no_omp);

int geopm_omp_sched_static_norm(int num_iter,
                                int chunk_size,
                                int num_thread,
                                double *norm);

double geopm_progress_threaded_min(int num_thread,
                                   size_t stride,
                                   const uint32_t *progress,
                                   const double *norm);

double geopm_progress_threaded_sum(int num_thread,
                                   size_t stride,
                                   const uint32_t *progress,
                                   double norm);

#ifdef __cplusplus
}
#endif
#endif
