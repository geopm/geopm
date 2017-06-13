/*
 * Copyright (c) 2015, 2016, 2017, Intel Corporation
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

#ifdef __cplusplus
extern "C" {
#endif

/* Opaque structure which is a handle for a geopm::Controller object. */
struct geopm_ctl_c;

/************************/
/* OBJECT INSTANTIATION */
/************************/
int geopm_ctl_create(struct geopm_policy_c *policy,
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
int geopm_prof_init(void);

int geopm_prof_region(const char *region_name,
                      uint64_t hint,
                      uint64_t *region_id);

int geopm_prof_enter(uint64_t region_id);

int geopm_prof_exit(uint64_t region_id);

int geopm_prof_progress(uint64_t region_id,
                        double fraction);

int geopm_prof_epoch(void);

int geopm_prof_disable(const char *feature_name);

int geopm_prof_shutdown(void);

int geopm_tprof_init(int num_thread,
                     size_t num_iter,
                     size_t chunk_size,
                     uint32_t *num_work_unit);

int geopm_tprof_increment(void);

/* Application Region Hints */
enum geopm_region_hint_e {
    GEOPM_REGION_HINT_UNKNOWN =   1ULL << 32, // Region with unknown or varying characteristics
    GEOPM_REGION_HINT_COMPUTE =   1ULL << 33, // Region dominated by compute
    GEOPM_REGION_HINT_MEMORY =    1ULL << 34, // Region dominated by memory access
    GEOPM_REGION_HINT_NETWORK =   1ULL << 35, // Region dominated by network traffic
    GEOPM_REGION_HINT_IO =        1ULL << 36, // Region dominated by disk access
    GEOPM_REGION_HINT_SERIAL =    1ULL << 37, // Single threaded region
    GEOPM_REGION_HINT_PARALLEL =  1ULL << 38, // Region is threaded
    GEOPM_REGION_HINT_IGNORE =    1ULL << 39, // Do not add region time to epoch
};

/*****************/
/* MPI COMM APIS */
/*****************/
int geopm_comm_split(MPI_Comm comm, const char *tag, MPI_Comm *split_comm, int *is_ctl_comm);

int geopm_comm_split_ppn1(MPI_Comm comm, const char *tag, MPI_Comm *ppn1_comm);

int geopm_comm_split_shared(MPI_Comm comm, const char *tag, MPI_Comm *split_comm);

#ifdef __cplusplus
}
#endif
#endif
