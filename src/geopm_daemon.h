/*
 * Copyright (c) 2015 - 2023, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef GEOPM_DAEMON_H_INCLUDE
#define GEOPM_DAEMON_H_INCLUDE

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

struct geopm_daemon_c;

int geopm_daemon_create(const char *endpoint_name,
                        const char *policystore_path,
                        struct geopm_daemon_c **daemon);
int geopm_daemon_destroy(struct geopm_daemon_c *daemon);
int geopm_daemon_update_endpoint_from_policystore(struct geopm_daemon_c *daemon,
                                                  double timeout);
int geopm_daemon_stop_wait_loop(struct geopm_daemon_c *daemon);
int geopm_daemon_reset_wait_loop(struct geopm_daemon_c *daemon);

#ifdef __cplusplus
}
#endif

#endif
