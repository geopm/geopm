/*
 * Copyright (c) 2015, 2016, 2017, 2018, 2019, Intel Corporation
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

#ifndef GEOPM_INTERNAL_H_INCLUDE
#define GEOPM_INTERNAL_H_INCLUDE
#include <stdint.h>

#include "geopm.h"
#include "geopm_time.h"

#ifdef __cplusplus
extern "C" {
#endif

/// @brief Enum for internally defined region hashes.
/// Note the value is the geopm_crc32_str() of the stringified enum
enum geopm_region_hash_epoch_e {
    GEOPM_REGION_HASH_EPOCH = 0x66c91423ULL,
};

/// Region id enums go from bit 63 and work their way down.
/// Hint enums in geopm.h go from bit 32 and work their way up.
/// There is a possibility of a conflict sometime in the future if they overlap.
enum geopm_region_id_e {
    GEOPM_REGION_ID_EPOCH =        1ULL << 63, // Signaling the start of an epoch, no associated Region
    GEOPM_REGION_ID_MPI =          1ULL << 62, // Execution of MPI calls
    GEOPM_NUM_REGION_ID_PRIVATE =  3,          // Number table entries reserved for GEOPM defined regions (ignoring UNMARKED)
};

enum geopm_ctl_e {
    GEOPM_CTL_NONE,
    GEOPM_CTL_PROCESS,
    GEOPM_CTL_PTHREAD,
};

/// @brief Used to pass information about regions entered and exited
/// from the application to the tracer.
struct geopm_region_info_s
{
    uint64_t region_hash;
    uint64_t region_hint;
    double progress;
    double runtime;
};

/// @brief Structure used to hold single profiling
/// messages obtained from the application.
struct geopm_prof_message_s {
    /// @brief Rank identifier.
    int rank;
    /// @brief 64-bit unique application region identifier.
    uint64_t region_id;
    /// @brief Time stamp of when the sample was taken.
    struct geopm_time_s timestamp;
    /// @brief Progress of the rank within the current region.
    double progress;
};

/// Helper that creates the DefaultProfile signleton (if not already created)
/// and catches all exceptions.
int geopm_prof_init(void);

/*********************************/
/* APPLICATION PROFILING INSIGHT */
/*********************************/
static inline uint64_t geopm_region_id_hash(uint64_t region_id)
{
    uint64_t ret = ((region_id << 32) >> 32);

    if (GEOPM_REGION_HASH_UNMARKED == region_id ||
        GEOPM_REGION_HASH_INVALID == ret) {
        ret = GEOPM_REGION_HASH_UNMARKED;
    }
    return ret;
}

static inline int geopm_region_id_is_epoch(uint64_t region_id)
{
    return (region_id & GEOPM_REGION_ID_EPOCH) ? 1 : 0;
}

static inline int geopm_region_id_is_mpi(uint64_t region_id)
{
    return (region_id & GEOPM_REGION_ID_MPI) ? 1 : 0;
}

static inline int geopm_region_id_is_nested(uint64_t region_id)
{
    return (geopm_region_id_is_mpi(region_id) && geopm_region_id_hash(region_id));
}

static inline uint64_t geopm_region_id_parent(uint64_t region_id)
{
    return (geopm_region_id_is_nested(region_id) ? geopm_region_id_hash(region_id) : GEOPM_REGION_HASH_INVALID);
}

static inline uint64_t geopm_region_id_set_mpi(uint64_t region_id)
{
    return (region_id | GEOPM_REGION_ID_MPI);
}

static inline uint64_t geopm_region_id_unset_mpi(uint64_t region_id)
{
    return (region_id & (~GEOPM_REGION_ID_MPI));
}

static inline uint64_t geopm_region_id_set_hint(uint64_t hint_type, uint64_t region_id)
{
    return (region_id | hint_type);
}

static inline uint64_t geopm_region_id_unset_hint(uint64_t hint_type, uint64_t region_id)
{
    return (region_id & (~hint_type));
}

static inline int geopm_region_id_hint_is_equal(uint64_t hint_type, uint64_t region_id)
{
    return (region_id & hint_type) ? 1 : 0;
}

static inline uint64_t geopm_region_id_hint(uint64_t region_id)
{
    uint64_t ret;
    if (GEOPM_REGION_HASH_UNMARKED == region_id) {
        ret = GEOPM_REGION_HINT_UNKNOWN;
    }
    else if (geopm_region_id_is_mpi(region_id)) {
        ret = GEOPM_REGION_HINT_NETWORK;
    }
    else {
        ret = region_id & GEOPM_MASK_REGION_HINT;
        if (!ret) {
            ret = GEOPM_REGION_HINT_UNKNOWN;
        }
    }
    return ret;
}

#ifdef __cplusplus
}
#endif
#endif
