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

enum geopm_report_sizes_e {
    GEOPM_NUM_SAMPLE_STATS = 4,
    GEOPM_NUM_METRIC_STATS = 7,
};

struct geopm_metric_stats_s {
    char name[NAME_MAX];
    double stats[GEOPM_NUM_METRIC_STATS];
};

struct geopm_report_s {
    char host[NAME_MAX];
    char sample_time_first[NAME_MAX];
    double sample_stats[GEOPM_NUM_SAMPLE_STATS];
    size_t num_metric;
    struct geopm_metric_stats_s *metric_stats;
};

/// @brief Create a stats collector handle
///
/// Provide a list of PlatformIO signal requests and construct a stats collector
/// object.  The request list determine which statistics will be included in the
/// generated report.
///
/// @param [in] num_requests Number of requests in array pointed to by the
///        request pointer
///
/// @param [in] requests Array of PlatformIO signal requests that configures the
///        report contents
///
/// @param [out] collector Handle to the constructed StatsCollector object, must
///        be de-allocated with geopm_stats_collector_free()
///
/// @returns 0 upon success, or error code upon failure
int GEOPM_PUBLIC
    geopm_stats_collector_create(size_t num_requests, const struct geopm_request_s *requests,
                                 struct geopm_stats_collector_s **collector);

/// @brief Update a stat collector with new values
///
/// User is expected to call PlatformIO::read_batch() prior to calling this
/// interface.  The sampled values will be used to update the report statistics.
///
/// @param [in] collector Handle created with a call to geopm_stats_collector_create()
///
/// @returns 0 upon success, or error code upon failure
int GEOPM_PUBLIC
    geopm_stats_collector_update(struct geopm_stats_collector_s *collector);

int GEOPM_PUBLIC
    geopm_stats_collector_update_count(const struct geopm_stats_collector_s *collector,
                                       size_t *update_count);

/// @brief Create a yaml report
///
/// Create a report that shows all statistics gathered by calls to
/// geopm_stats_collector_update().  To determine the size of the report string,
/// call with *max_report_size == 0 and report_yaml == NULL.  In this case
/// max_report_size will be updated with the required string length and zero is
/// returned.  Otherwise, if *max_report_size provided by the user is not
/// sufficient, EINVAL is returned and the value of *max_report_size is set to
/// the required size and report_yaml is unmodified.
///
/// @param [in] collector Handle created with a call to geopm_stats_collector_create()
///
/// @param [in,out] max_report_size Set to the length of the report_yaml string
///        provided by the user, set to zero to query value.  If too small, will
///        be updated with required value.
///
/// @param [out] report_yaml Generated report string allocated by the user, set
///              to NULL to query max_report_size without error.
///
/// @returns 0 upon success, or error code upon failure
int GEOPM_PUBLIC
    geopm_stats_collector_report_yaml(const struct geopm_stats_collector_s *collector,
                                      size_t *max_report_size, char *report_yaml);

int GEOPM_PUBLIC
    geopm_stats_collector_report(const struct geopm_stats_collector_s *collector,
                                 size_t num_requests, struct geopm_report_s *report);

/// @brief Reset statistics
///
/// Called by user to zero all statistics gathered.  This may be called after a
/// call to geopm_stats_collector_report_yaml() and before the next call to
/// geopm_stats_collector_update() so that the next report that is generated is
/// independent of the last.
///
/// @param [in] collector Handle created with a call to geopm_stats_collector_create()
///
/// @returns 0 upon success, or error code upon failure
int GEOPM_PUBLIC
    geopm_stats_collector_reset(struct geopm_stats_collector_s *collector);

/// @brief Release resources associated with collector handle
///
/// @param [in] collector Handle created with a call to geopm_stats_collector_create()
///
/// @returns 0 upon success, or error code upon failure
int GEOPM_PUBLIC
    geopm_stats_collector_free(struct geopm_stats_collector_s *collector);

#ifdef __cplusplus
}
#endif
#endif
