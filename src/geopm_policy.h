/*
 * Copyright (c) 2015, 2016, 2017, 2018, Intel Corporation
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in
 *       the documentation and/or other materials provided with the
 *       distribution.
 *
 *     * Neither the name of Intel Corporation nor the names of its
 *       contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY LOG OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef GEOPM_POLICY_H_INCLUDE
#define GEOPM_POLICY_H_INCLUDE

#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

/* opaque structure that is a handle for a geopm::Policy object. */
struct geopm_policy_c;

/// @brief Enum encompassing geopm power management modes.
enum geopm_policy_mode_e {
    GEOPM_POLICY_MODE_TDP_BALANCE_STATIC = 1,
    GEOPM_POLICY_MODE_FREQ_UNIFORM_STATIC = 2,
    GEOPM_POLICY_MODE_FREQ_HYBRID_STATIC = 3,
    GEOPM_POLICY_MODE_PERF_BALANCE_DYNAMIC = 4,
    GEOPM_POLICY_MODE_FREQ_UNIFORM_DYNAMIC = 5,
    GEOPM_POLICY_MODE_FREQ_HYBRID_DYNAMIC = 6,
    GEOPM_POLICY_MODE_STATIC = 253,
    GEOPM_POLICY_MODE_DYNAMIC = 254,
    GEOPM_POLICY_MODE_SHUTDOWN = 255,
};

enum geopm_policy_affinity_e {
    GEOPM_POLICY_AFFINITY_INVALID,
    GEOPM_POLICY_AFFINITY_COMPACT,
    GEOPM_POLICY_AFFINITY_SCATTER,
};

enum geopm_policy_goal_e {
    GEOPM_POLICY_GOAL_CPU_EFFICIENCY ,
    GEOPM_POLICY_GOAL_NETWORK_EFFICIENCY,
    GEOPM_POLICY_GOAL_MEMORY_EFFICIENCY,
};

int geopm_policy_create(const char *in_config,
                        const char *out_config,
                        struct geopm_policy_c **policy);

int geopm_policy_destroy(struct geopm_policy_c *policy);

int geopm_policy_power(struct geopm_policy_c *policy,
                       int power_budget);

int geopm_policy_mode(struct geopm_policy_c *policy,
                      int mode);

int geopm_policy_cpu_freq(struct geopm_policy_c *policy,
                          int cpu_mhz);

int geopm_policy_full_perf(struct geopm_policy_c *policy,
                           int num_cpu_full_perf);

int geopm_policy_tdp_percent(struct geopm_policy_c *policy,
                             double tdp_percent);

int geopm_policy_affinity(struct geopm_policy_c *policy,
                          int affinity);

int geopm_policy_goal(struct geopm_policy_c *policy,
                      int goal);

int geopm_policy_tree_decider(struct geopm_policy_c *policy, const char *description);

int geopm_policy_leaf_decider(struct geopm_policy_c *policy, const char *description);

int geopm_policy_platform(struct geopm_policy_c *policy, const char *description);

int geopm_policy_write(const struct geopm_policy_c *policy);

int geopm_policy_enforce_static(const struct geopm_policy_c *policy);

int geopm_platform_msr_save(const char *path);

int geopm_platform_msr_restore(const char *path);

int geopm_platform_msr_whitelist(FILE *file_desc);

#ifdef __cplusplus
}
#endif
#endif
