/*
 * Copyright (c) 2015, 2016, Intel Corporation
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
#include <stdio.h>
#include <stdint.h>
#include <pthread.h>

#include "geopm_policy.h"
#include "geopm_message.h"

#ifdef __cplusplus
extern "C" {
#endif

enum geopm_const_e {
    GEOPM_CONST_DEFAULT_CTL_NUM_LEVEL = 3,
    GEOPM_CONST_PROF_SAMPLE_PERIOD = 64,
    GEOPM_CONST_SHMEM_REGION_SIZE = 4096,
};

enum geopm_sample_reduce_e {
    GEOPM_SAMPLE_REDUCE_THREAD = 1,
    GEOPM_SAMPLE_REDUCE_PROC = 2,
    GEOPM_SAMPLE_REDUCE_NODE = 3,
};

/* Opaque structure which is a handle for a geopm::Controller object. */
struct geopm_ctl_c;

/* Opaque structure which is a handle for a geopm::Profile object. */
struct geopm_prof_c;

/************************/
/* OBJECT INSTANTIATION */
/************************/
int geopm_ctl_create(struct geopm_policy_c *policy,
                     const char *sample_key,
                     MPI_Comm comm,
                     struct geopm_ctl_c **ctl);

int geopm_ctl_destroy(struct geopm_ctl_c *ctl);

/********************/
/* POWER MANAGEMENT */
/********************/
int geopm_ctl_step(struct geopm_ctl_c *ctl);

int geopm_ctl_run(struct geopm_ctl_c *ctl);

int geopm_ctl_pthread(struct geopm_ctl_c *ctl,
                      const pthread_attr_t *attr,
                      pthread_t *thread);

int geopm_ctl_spawn(struct geopm_ctl_c *ctl);

/*************************/
/* APPLICATION PROFILING */
/*************************/
int geopm_prof_create(const char *name,
                      size_t table_size,
                      const char *shm_key,
                      MPI_Comm comm,
                      struct geopm_prof_c **prof);

int geopm_prof_destroy(struct geopm_prof_c *prof);

int geopm_prof_region(struct geopm_prof_c *prof,
                      const char *region_name,
                      long policy_hint,
                      uint64_t *region_id);

int geopm_prof_enter(struct geopm_prof_c *prof,
                     uint64_t region_id);

int geopm_prof_exit(struct geopm_prof_c *prof,
                    uint64_t region_id);

int geopm_prof_progress(struct geopm_prof_c *prof,
                        uint64_t region_id,
                        double fraction);

int geopm_prof_outer_sync(struct geopm_prof_c *prof);

int geopm_prof_sample(struct geopm_prof_c *prof,
                      uint64_t region_id);

int geopm_prof_disable(struct geopm_prof_c *prof,
                       const char *feature_name);

int geopm_prof_print(struct geopm_prof_c *prof,
                     const char *file_name,
                     int depth);



/***************/
/* HELPER APIS */
/***************/
int geopm_num_node(MPI_Comm comm, int *num_node);

int geopm_comm_split_ppn1(MPI_Comm comm, MPI_Comm *ppn1_comm);

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
