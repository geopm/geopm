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

/// @brief Create a yaml report
///
/// Create a report that shows all statistics gathered by calls to
/// geopm_stats_collector_update().  To determine the size of the report string,
/// call with *max_report_size == 0.  If *max_report_size provided by the user is
/// not sufficient, EINVAL is returned and the value of *max_report_size is set
/// to the required size and report_yaml is unmodified.
///
/// @param [in] collector Handle created with a call to geopm_stats_collector_create()
///
/// @param [in,out] max_report_size Set to the length of the report_yaml string
///        provided by the user.  If too small, will be updated with required value.
///
/// @param [out] report_yaml Generated report string allocated by the user
///
/// @returns 0 upon success, or error code upon failure
int GEOPM_PUBLIC
    geopm_stats_collector_report_yaml(const struct geopm_stats_collector_s *collector,
                                      size_t *max_report_size, char *report_yaml);
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
