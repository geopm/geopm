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

#include <stdint.h>

#include "geopm_policy.h"

#ifdef __cplusplus
extern "C" {
#endif

/****************************/
/* APPLICATION REGION HINTS */
/****************************/
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

int geopm_prof_shutdown(void);

int geopm_tprof_init(uint32_t num_work_unit);

int geopm_tprof_init_loop(int num_thread,
                          int thread_idx,
                          size_t num_iter,
                          size_t chunk_size);

int geopm_tprof_post(void);


#ifdef __cplusplus
}
#endif
#endif
