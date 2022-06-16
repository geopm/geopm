/*
 * Copyright (c) 2015 - 2022, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef GEOPM_HINT_H_INCLUDE
#define GEOPM_HINT_H_INCLUDE
#include <stdint.h>

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
    GEOPM_REGION_HINT_INACTIVE, /* Used to marke CPUs that are not running application */
    GEOPM_SENTINEL_REGION_HINT,
    GEOPM_MASK_REGION_HINT =    GEOPM_REGION_HINT_UNKNOWN |
                                GEOPM_REGION_HINT_COMPUTE |
                                GEOPM_REGION_HINT_MEMORY |
                                GEOPM_REGION_HINT_NETWORK |
                                GEOPM_REGION_HINT_IO |
                                GEOPM_REGION_HINT_SERIAL |
                                GEOPM_REGION_HINT_PARALLEL |
                                GEOPM_REGION_HINT_IGNORE |
                                GEOPM_REGION_HINT_INACTIVE,
};

#ifdef __cplusplus
}
#endif
#endif
