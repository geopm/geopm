/*
 * Copyright (c) 2015 - 2024 Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef GEOPM_HINT_H_INCLUDE
#define GEOPM_HINT_H_INCLUDE
#include <stdint.h>

#include "geopm_public.h"

#ifdef __cplusplus
extern "C" {
#endif

/****************************/
/* APPLICATION REGION HINTS */
/****************************/
enum geopm_region_hint_e {
    GEOPM_REGION_HINT_UNSET =   0ULL,       /* For clearing hints */
    GEOPM_REGION_HINT_UNKNOWN,  /* Region with unknown or varying characteristics */
    GEOPM_REGION_HINT_COMPUTE,  /* Region dominated by compute */
    GEOPM_REGION_HINT_MEMORY,   /* Region dominated by memory access */
    GEOPM_REGION_HINT_NETWORK,  /* Region dominated by network traffic */
    GEOPM_REGION_HINT_IO,       /* Region dominated by disk access */
    GEOPM_REGION_HINT_SERIAL,   /* Single threaded region */
    GEOPM_REGION_HINT_PARALLEL, /* Region is threaded */
    GEOPM_REGION_HINT_IGNORE,   /* Do not add region time to epoch */
    GEOPM_REGION_HINT_INACTIVE, /* Used to mark CPUs that are not running application */
    GEOPM_REGION_HINT_SPIN,     /* Region dominated by spin wait */
    GEOPM_NUM_REGION_HINT,
};

#ifdef __cplusplus
}

namespace geopm {
    /// @brief Verify a region's hint value is legal for use.
    void GEOPM_PUBLIC
        check_hint(uint64_t hint);
}
#endif

#endif
