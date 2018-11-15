/*
 * Copyright (c) 2015, 2016, 2017, 2018, Intel Corporation
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

#ifndef GEOPM_REGION_ID_H_INCLUDE
#define GEOPM_REGION_ID_H_INCLUDE

#include <stdint.h>
#include "geopm.h"

#ifdef __cplusplus
extern "C" {
#endif

/*********************************/
/* APPLICATION PROFILING INSIGHT */
/*********************************/
int geopm_region_id_is_mpi(uint64_t region_id);

int geopm_region_id_is_unmarked(uint64_t region_id);

static inline int geopm_region_id_hint_is_equal(uint64_t hint_type, uint64_t region_id)
{
    return (region_id & hint_type) ? 1 : 0;
}

static inline uint64_t geopm_region_id_set_hint(uint64_t hint_type, uint64_t region_id)
{
    return (region_id | hint_type);
}

static inline uint64_t geopm_region_id_unset_hint(uint64_t hint_type, uint64_t region_id)
{
    return (region_id & (~hint_type));
}

static inline uint64_t geopm_region_id_hint(uint64_t region_id)
{
    return (region_id & GEOPM_MASK_REGION_HINT);
}

uint64_t geopm_region_id_hash(uint64_t region_id);

#ifdef __cplusplus
}
#endif
#endif
