/*
 * Copyright (c) 2015 - 2022, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef GEOPM_REPORTER_H_INCLUDE
#define GEOPM_REPORTER_H_INCLUDE

#include <cstdlib>

#ifdef __cplusplus
extern "C" {
#endif


int geopm_reporter_init(void);
int geopm_reporter_update(void);
int geopm_reporter_generate(const char *profile_name,
                            const char *agent_name,
                            size_t result_max,
                            char *result);

#ifdef __cplusplus
}
#endif
#endif
