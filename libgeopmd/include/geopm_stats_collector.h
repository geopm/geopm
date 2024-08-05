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
struct geopm_stats_collector_s;

int GEOPM_PUBLIC
    geopm_stats_collector_create(size_t num_requests, const struct geopm_request_s *requests,
                                 struct geopm_stats_collector_s **collector);

int GEOPM_PUBLIC
    geopm_stats_collector_update(struct geopm_stats_collector_s *collector);

// If *max_report_size is zero, update it with the required size for the report
// and do not modify report
int GEOPM_PUBLIC
    geopm_stats_collector_report_yaml(const struct geopm_stats_collector_s *collector,
                                      size_t *max_report_size, char *report_yaml);

int GEOPM_PUBLIC
    geopm_stats_collector_reset(struct geopm_stats_collector_s *collector);

int GEOPM_PUBLIC
    geopm_stats_collector_free(struct geopm_stats_collector_s *collector);

#ifdef __cplusplus
}
#endif
#endif
