/*
 * Copyright (c) 2015 - 2024 Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef GEOPM_STATS_COLLECTOR_H_INCLUDE
#define GEOPM_STATS_COLLECTOR_H_INCLUDE
#ifdef __cplusplus
extern "C" {
#endif

#include "geopm_public.h"
#include <stddef.h>

struct geopm_request_s;

int GEOPM_PUBLIC
    geopm_stats_collector(int num_requests, struct geopm_request_s *requests);
int GEOPM_PUBLIC
    geopm_stats_collector_update(void);
int GEOPM_PUBLIC
    geopm_stats_collector_report(size_t max_report_size, char *report);

#ifdef __cplusplus
}
#endif
#endif
