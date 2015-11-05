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

#include <pthread.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define GEOPM_MAX_NUM_CPU 768

enum geopm_policy_flags_e {
    GEOPM_FLAGS_SMALL_CPU_FREQ_100MHZ_1 = 1ULL << 0,
    GEOPM_FLAGS_SMALL_CPU_FREQ_100MHZ_2 = 1ULL << 1,
    GEOPM_FLAGS_SMALL_CPU_FREQ_100MHZ_4 = 1ULL << 2,
    GEOPM_FLAGS_SMALL_CPU_FREQ_100MHZ_8 = 1ULL << 3,
    GEOPM_FLAGS_SMALL_CPU_FREQ_100MHZ_16 = 1ULL << 4,
    GEOPM_FLAGS_SMALL_CPU_FREQ_100MHZ_32 = 1ULL << 5,
    GEOPM_FLAGS_SMALL_CPU_FREQ_100MHZ_64 = 1ULL << 6,
    GEOPM_FLAGS_SMALL_CPU_FREQ_100MHZ_128 = 1ULL << 7,
    GEOPM_FLAGS_BIG_CPU_NUM_1 = 1ULL << 8,
    GEOPM_FLAGS_BIG_CPU_NUM_2 = 1ULL << 9,
    GEOPM_FLAGS_BIG_CPU_NUM_4 = 1ULL << 10,
    GEOPM_FLAGS_BIG_CPU_NUM_8 = 1ULL << 11,
    GEOPM_FLAGS_BIG_CPU_NUM_16 = 1ULL << 12,
    GEOPM_FLAGS_BIG_CPU_NUM_32 = 1ULL << 13,
    GEOPM_FLAGS_BIG_CPU_NUM_64 = 1ULL << 14,
    GEOPM_FLAGS_BIG_CPU_NUM_128 = 1ULL << 15,
    GEOPM_FLAGS_SMALL_CPU_TOPOLOGY_COMPACT = 1ULL << 16,
    GEOPM_FLAGS_SMALL_CPU_TOPOLOGY_SCATTER = 1ULL << 17,
    GEOPM_FLAGS_TDP_PERCENT_1 = 1ULL << 18,
    GEOPM_FLAGS_TDP_PERCENT_2 = 1ULL << 19,
    GEOMP_FLAGS_TDP_PERCENT_4 = 1ULL << 20,
    GEOMP_FLAGS_TDP_PERCENT_8 = 1ULL << 21,
    GEOMP_FLAGS_TDP_PERCENT_16 = 1ULL << 22,
    GEOMP_FLAGS_TDP_PERCENT_32 = 1ULL << 23,
    GEOMP_FLAGS_TDP_PERCENT_64 = 1ULL << 24,
    GEOPM_FLAGS_GOAL_CPU_EFFICIENCY = 1ULL << 25,
    GEOPM_FLAGS_GOAL_NETWORK_EFFICIENCY = 1ULL << 26,
    GEOPM_FLAGS_GOAL_MEMORY_EFFICIENCY = 1ULL << 27,
};

enum geopm_policy_mode_e {
    GEOPM_MODE_TDP_BALANCE_STATIC = 1,
    GEOPM_MODE_FREQ_UNIFORM_STATIC = 2,
    GEOPM_MODE_FREQ_HYBRID_STATIC = 3,
    GEOPM_MODE_PERF_BALANCE_DYNAMIC = 4,
    GEOPM_MODE_FREQ_UNIFORM_DYNAMIC = 5,
    GEOPM_MODE_FREQ_HYBRID_DYNAMIC = 6,
    GEOPM_MODE_SHUTDOWN = 255,
};

enum geopm_policy_hint_e {
    GEOPM_POLICY_HINT_UNKNOWN = 0,
    GEOPM_POLICY_HINT_COMPUTE = 1,
    GEOPM_POLICY_HINT_MEMORY = 2,
    GEOPM_POLICY_HINT_NETWORK = 3,
};

enum geopm_status_e {
    GEOPM_STATUS_UNDEFINED = 0,
    GEOPM_STATUS_INITIALIZED = 1,
    GEOPM_STATUS_ACTIVE = 2,
    GEOPM_STATUS_REPORT = 3,
    GEOPM_STATUS_SHUTDOWN = 4,
};

struct geopm_policy_message_s {
    int phase_id;
    int mode;
    unsigned long flags;
    int num_sample;
    double power_budget;
};

struct geopm_policy_shmem_s {
    int is_init;
    pthread_mutex_t lock;
    struct geopm_policy_message_s policy;
};

struct geopm_sample_message_s {
    int rank;
    uint64_t phase_id;
    double runtime;
    double progress;
    double energy;
    double frequency;
};

struct geopm_ctl_message_s {
    volitile uint32_t ctl_status;
    volitile uint32_t app_status;
    int cpu_rank[GEOPM_MAX_NUM_CPU];
};

struct geopm_sample_shmem_s {
    int is_init;
    pthread_mutex_t lock;
    struct geopm_sample_message_s sample;
}; // FIXME: Controller still uses this, but Profile uses LockingHashTable

extern const struct geopm_policy_message_s GEOPM_UNKNOWN_POLICY;

int geopm_is_policy_equal(const struct geopm_policy_message_s *a, const struct geopm_policy_message_s *b);

#ifdef __cplusplus
}
#endif
#endif
