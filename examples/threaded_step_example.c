/*
 * Copyright (c) 2015, 2016, 2017, Intel Corporation
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

#ifndef _POSIX_C_SOURCE
#define _POSIX_C_SOURCE 200809L
#endif
#include <stdlib.h>
#include <string.h>
#include <mpi.h>
#include <omp.h>
#include <errno.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "geopm.h"
#include "geopm_policy.h"

#ifndef NAME_MAX
#define NAME_MAX 512
#endif

static inline double do_something(int input)
{
    int i;
    double result = (double)input;
    for (i = 0; i < 1000; i++) {
        result += i*result;
    }
    return result;
}

static int run_something(void)
{
    int err = 0;
    struct geopm_ctl_c *ctl;
    struct geopm_policy_c *policy;
    double x = 0;
    int num_thread, thread_idx, i, num_iter = 1000000, iter_per_step = 100, chunk_size = 128;
    int step_counter = 0;
    uint64_t region_id;

    #pragma omp parallel
    {
        num_thread = omp_get_num_threads();
    }
    // In this example we will create the policy, but in general it
    // should be created prior to application runtime.
    err = geopm_policy_create("", "profile_policy", &policy);
    if (!err) {
        err = geopm_policy_mode(policy, GEOPM_POLICY_MODE_PERF_BALANCE_DYNAMIC);
    }
    if (!err) {
        err = geopm_policy_power(policy, 2000);
    }
    if (!err) {
        err = geopm_policy_write(policy);
    }
    if (!err) {
        err = geopm_policy_destroy(policy);
    }
    // Now that we have a policy on disk, use it as a normal
    // application would
    if (!err) {
        err = geopm_policy_create("profile_policy", "", &policy);
    }
    if (!err) {
        err = geopm_ctl_create(policy, MPI_COMM_WORLD, &ctl);
    }
    if (!err) {
        err = geopm_tprof_init(num_thread, num_iter, chunk_size, NULL); 
    }
    if (!err) {
        err = geopm_ctl_step(ctl);
    }
    if (!err) {
        err = geopm_prof_region("main-loop", GEOPM_REGION_HINT_UNKNOWN, &region_id);
    }
    if (!err) {
        #pragma omp parallel default(shared) private(i)
        {
            thread_idx = omp_get_thread_num();
            #pragma omp for schedule(static, chunk_size)
            for (i = 0; i < num_iter; ++i) {
                x += do_something(i);
                geopm_tprof_increment();
                if (!thread_idx) {
                    step_counter++;
                    if (step_counter == iter_per_step) {
                        geopm_ctl_step(ctl);
                        step_counter = 0;
                    }
                }
            }
        }
    }
    return err;
}

int main(int argc, char **argv)
{
    MPI_Init(&argc, &argv);
    return run_something();
}


