/*
 * Copyright (c) 2015, Intel Corporation
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

#ifndef GEOPM_POLICY_MESSAGE_H_INCLUDE
#define GEOPM_POLICY_MESSAGE_H_INCLUDE

#ifdef __cplusplus
extern "C" {
#endif

enum geopm_policy_flags_e {
    GEOPM_FLAGS_LITTLE_CPU_FREQ_100MHZ_1 = 1ULL << 0,
    GEOPM_FLAGS_LITTLE_CPU_FREQ_100MHZ_2 = 1ULL << 1,
    GEOPM_FLAGS_LITTLE_CPU_FREQ_100MHZ_4 = 1ULL << 2,
    GEOPM_FLAGS_LITTLE_CPU_FREQ_100MHZ_8 = 1ULL << 3,
    GEOPM_FLAGS_LITTLE_CPU_FREQ_100MHZ_16 = 1ULL << 4,
    GEOPM_FLAGS_LITTLE_CPU_FREQ_100MHZ_32 = 1ULL << 5,
    GEOPM_FLAGS_LITTLE_CPU_FREQ_100MHZ_64 = 1ULL << 6,
    GEOPM_FLAGS_LITTLE_CPU_FREQ_100MHZ_128 = 1ULL << 7,
    GEOPM_FLAGS_BIG_CPU_NUM_1 = 1ULL << 8,
    GEOPM_FLAGS_BIG_CPU_NUM_2 = 1ULL << 9,
    GEOPM_FLAGS_BIG_CPU_NUM_4 = 1ULL << 10,
    GEOPM_FLAGS_BIG_CPU_NUM_8 = 1ULL << 11,
    GEOPM_FLAGS_BIG_CPU_NUM_16 = 1ULL << 12,
    GEOPM_FLAGS_BIG_CPU_NUM_32 = 1ULL << 13,
    GEOPM_FLAGS_BIG_CPU_NUM_64 = 1ULL << 14,
    GEOPM_FLAGS_BIG_CPU_NUM_128 = 1ULL << 15,
    GEOPM_FLAGS_BIG_CPU_TOPOLOGY_COMPACT = 1ULL << 16,
    GEOPM_FLAGS_BIG_CPU_TOPOLOGY_SCATTER = 1ULL << 17,
    GEOPM_FLAGS_TDP_PERCENT_1 = 1ULL << 18,
    GEOPM_FLAGS_TDP_PERCENT_2 = 1ULL << 19,
    GEOMP_FLAGS_TDP_PERCENT_4 = 1ULL << 20,
    GEOMP_FLAGS_TDP_PERCENT_8 = 1ULL << 21,
    GEOMP_FLAGS_TDP_PERCENT_16 = 1ULL << 22,
    GEOMP_FLAGS_TDP_PERCENT_32 = 1ULL << 23,
    GEOMP_FLAGS_TDP_PERCENT_64 = 1ULL << 24,
    GEOPM_FLAGS_GOAL_CPU_EFFICIENCY = 1ULL << 25,
    GEOPM_FLAGS_GOAL_NETWORK_EFFICIENCY = 1ULL << 26,
    GEOPM_FLAGS_GOAL_MEMORY_EFFICIENCY = 1ULL << 28,
};

enum geopm_policy_mode_e {
    GEOPM_MODE_TDP_BALANCE_STATIC = 1,
    GEOPM_MODE_FREQ_UNIFORM_STATIC = 2,
    GEOPM_MODE_FREQ_HYBRID_STATIC = 3,
    GEOPM_MODE_PERF_BALANCE_DYNAMIC = 4,
    GEOPM_MODE_FREQ_UNIFORM_DYNAMIC = 5,
    GEOPM_MODE_FREQ_HYBRID_DYNAMIC = 6,
};

struct geopm_policy_message_s {
    int mode;
    unsigned long flags;
    int num_sample;
    double power_budget;
};

extern const struct geopm_policy_message_s GEOPM_UNKNOWN_POLICY;

int geopm_is_policy_equal(const struct geopm_policy_message_s *a, const struct geopm_policy_message_s *b);

#ifdef __cplusplus
}
#endif
#endif
