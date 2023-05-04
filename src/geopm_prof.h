/*
 * Copyright (c) 2015 - 2023, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef GEOPM_PROF_H_INCLUDE
#define GEOPM_PROF_H_INCLUDE

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

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

int geopm_prof_overhead(double overhead_sec);

#ifdef __cplusplus
}
#endif
#endif
