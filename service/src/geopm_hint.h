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
    GEOPM_REGION_HINT_UNSET =     0ULL,       /* For clearing hints */
    GEOPM_REGION_HINT_UNKNOWN =   1ULL << 32, /* Region with unknown or varying characteristics */
    GEOPM_REGION_HINT_COMPUTE =   2ULL << 32, /* Region dominated by compute */
    GEOPM_REGION_HINT_MEMORY =    3ULL << 32, /* Region dominated by memory access */
    GEOPM_REGION_HINT_NETWORK =   4ULL << 32, /* Region dominated by network traffic */
    GEOPM_REGION_HINT_IO =        5ULL << 32, /* Region dominated by disk access */
    GEOPM_REGION_HINT_SERIAL =    6ULL << 32, /* Single threaded region */
    GEOPM_REGION_HINT_PARALLEL =  7ULL << 32, /* Region is threaded */
    GEOPM_REGION_HINT_IGNORE =    8ULL << 32, /* Do not add region time to epoch */
    GEOPM_REGION_HINT_INACTIVE =  9ULL << 32, /* Used to marke CPUs that are not running application */
    GEOPM_SENTINEL_REGION_HINT =  10ULL,
    GEOPM_MASK_REGION_HINT =      GEOPM_REGION_HINT_UNKNOWN |
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
