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

#ifndef GEOPM_POLICY_H_INCLUDE
#define GEOPM_POLICY_H_INCLUDE

#include <pthread.h>

#ifdef __cplusplus
extern "C" {
#endif

struct geopm_policy_message_s {
    long phase_id;
    int goal;
    int mode;
    int num_sample;
    double power_budget;
};

struct geopm_policy_shmem_s {
    int is_init;
    pthread_mutex_t lock;
    struct geopm_policy_message_s policy;
};

enum geopm_policy_goal_e {
    GEOPM_GOAL_PERFORMANCE = 1,
    GEOPM_GOAL_EFFICENCY = 2,
};

enum geopm_policy_mode_e {
    GEOPM_MODE_UNIFORM_FREQ = 1,
    GEOPM_MODE_DYNAMIC_POWER = 2,
    GEOPM_MODE_SHUTDOWN = 3,
};

extern const struct geopm_policy_message_s GEOPM_UNKNOWN_POLICY;

struct geopm_policy_controller_c;

int geopm_policy_controller_create(char *shm_key, struct geopm_policy_message_s initial_policy, struct geopm_policy_controller_c **policy_controller);
int geopm_policy_controller_destroy(struct geopm_policy_controller_c *policy_controller);
int geopm_policy_controller_set_policy(struct geopm_policy_controller_c *policy_controller, struct geopm_policy_message_s policy);

int is_policy_equal(const struct geopm_policy_message_s *a, const struct geopm_policy_message_s *b);

#ifdef __cplusplus
}
#endif
#endif
