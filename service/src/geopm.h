/*
 * Copyright (c) 2015 - 2021, Intel Corporation
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

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/***************************/
/* APPLICATION REGION HASH */
/***************************/
enum geopm_region_hash_e {
    GEOPM_REGION_HASH_INVALID  = 0x0ULL,
    GEOPM_REGION_HASH_UNMARKED = 0x725e8066ULL, /* Note the value is the geopm_crc32_str() of the stringified enum */
    GEOPM_U64_SENTINEL_REGION_HASH = UINT64_MAX, /* Force enum type to uint64_t */
};

/****************************/
/* APPLICATION REGION HINTS */
/****************************/
enum geopm_region_hint_e {
    GEOPM_REGION_HINT_UNSET =     0ULL,       /* For clearing hints */
    GEOPM_REGION_HINT_UNKNOWN =   1ULL << 32, /* Region with unknown or varying characteristics */
    GEOPM_REGION_HINT_COMPUTE =   1ULL << 33, /* Region dominated by compute */
    GEOPM_REGION_HINT_MEMORY =    1ULL << 34, /* Region dominated by memory access */
    GEOPM_REGION_HINT_NETWORK =   1ULL << 35, /* Region dominated by network traffic */
    GEOPM_REGION_HINT_IO =        1ULL << 36, /* Region dominated by disk access */
    GEOPM_REGION_HINT_SERIAL =    1ULL << 37, /* Single threaded region */
    GEOPM_REGION_HINT_PARALLEL =  1ULL << 38, /* Region is threaded */
    GEOPM_REGION_HINT_IGNORE =    1ULL << 39, /* Do not add region time to epoch */
    GEOPM_REGION_HINT_INACTIVE =  1ULL << 40, /* Used to marke CPUs that are not running application */
    GEOPM_MASK_REGION_HINT =      GEOPM_REGION_HINT_UNKNOWN |
                                  GEOPM_REGION_HINT_COMPUTE |
                                  GEOPM_REGION_HINT_MEMORY |
                                  GEOPM_REGION_HINT_NETWORK |
                                  GEOPM_REGION_HINT_IO |
                                  GEOPM_REGION_HINT_SERIAL |
                                  GEOPM_REGION_HINT_PARALLEL |
                                  GEOPM_REGION_HINT_IGNORE |
                                  GEOPM_REGION_HINT_INACTIVE,
    GEOPM_U64_SENTINEL_REGION_HINT = UINT64_MAX, /* Force enum type to uint64_t */
};

/*!
 * @brief Used to pass information about regions entered and exited
 * from the application to the tracer.
 */
struct geopm_region_info_s
{
    uint64_t hash;
    uint64_t hint;
    double progress;
    double runtime;
};

/*************************/
/* APPLICATION PROFILING */
/*************************/
int geopm_prof_region(const char *region_name,
                      uint64_t hint,
                      uint64_t *region_id);

int geopm_prof_enter(uint64_t region_id);

int geopm_prof_exit(uint64_t region_id);

int geopm_prof_epoch(void);

int geopm_prof_shutdown(void);

int geopm_tprof_init(uint32_t num_work_unit);

int geopm_tprof_post(void);

#ifdef __cplusplus
}
#endif
#endif
