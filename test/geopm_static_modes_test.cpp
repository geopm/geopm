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
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <iostream>
#include <string>
#include <cmath>

#include <cpuid.h>
#include <omp.h>
#include <pthread.h>
#include <unistd.h>

#include "geopm_sched.h"

#include "XeonPlatformImp.hpp"

#define ASSERT(x) if (!(x)) { fprintf(stderr, "Error: Test failure in %s:%d\n", __FILE__, __LINE__); exit(-1); }

static const int ivb_id = 0x63E;
static const int snb_id = 0x62D;
static const int hsx_id = 0x63F;
static volatile int exit_signal_g = 0;
int cpuid();

struct work_s {
    int input;
    double result;
} work_s;


static inline void* do_something(void *work_struct)
{
    struct work_s *work = (struct work_s*)work_struct;
    int i = 0;

    work->result = (double)work->input;
    while (!exit_signal_g) {
        work->result += i*work->result;
    }
    work->result = std::isinf(work->result) ? 100.0 : work->result;
    work->result = std::max(work->result, 100.0);

    return NULL;
}

int main(int argc, char **argv)
{
    int err = 0;
    int id = 0;
    int cpus = 0;
    int freq = 0;
    double sum = 0.0;
    int count = 0;
    int num_omp_cpus = 0;
    pthread_attr_t attr;
    geopm::PlatformImp *plat = NULL;
    cpu_set_t no_omp = {0};
    cpu_set_t cpu_mask;
    const int MAX_FREQ = 16;

    id = cpuid();
    if (id == snb_id || id == ivb_id)
        plat = new geopm::IVTPlatformImp();
    else if (id == hsx_id)
        plat = new geopm::HSXPlatformImp();
    else
        plat = NULL;

    ASSERT(plat != NULL);

    plat->initialize();

    cpus = plat->num_hw_cpu();
    ASSERT(cpus);
    ASSERT(geopm_no_omp_cpu(cpus, &no_omp) == 0);

    //spawn worker threads on all cpus
    pthread_t *threads = (pthread_t*)malloc(sizeof(pthread_t) * cpus);
    ASSERT(threads);
    struct work_s *work = (struct work_s*)malloc(sizeof(struct work_s) * cpus);
    ASSERT(work);
    pthread_attr_init(&attr);

    for (int i = 0; i < cpus; i++) {
        CPU_ZERO(&cpu_mask);
        CPU_SET(i, &cpu_mask);
        pthread_attr_setaffinity_np(&attr, sizeof(cpu_set_t), &cpu_mask);
        err = pthread_create(&threads[i], &attr, do_something, (void *)(&work[i]));
        ASSERT(err == 0);
    }

    sleep(1);

    #pragma omp parallel
    {
        num_omp_cpus = omp_get_num_threads();
    }

    std::cout << "OMP_THREAD_NUM = " << num_omp_cpus << std::endl;

    #pragma omp parallel
    {
        freq = plat->msr_read(geopm::GEOPM_DOMAIN_CPU, sched_getcpu(), "IA32_PERF_STATUS");
        freq = freq >> 8;
        // make sure we are not over the set frequency limit
        if (freq <= MAX_FREQ) {
            #pragma omp critical
            {
                count++;
            }
        }
    }

    exit_signal_g = 1;

    for (int i = 0; i < cpus; i++) {
        ASSERT(pthread_join(threads[i], NULL) == 0);
    }

    for (int i = 0; i < cpus; i++) {
        sum += work[i].result;
    }

    std::cout << "sum = " << sum << std::endl;
    //assert that everyone was under the frequency limit
    ASSERT(count == num_omp_cpus);

    free(threads);
    free(work);
    return err;
}

int cpuid()
{
    uint32_t key = 1; //processor features
    uint32_t proc_info = 0;
    uint32_t model;
    uint32_t family;
    uint32_t ext_model;
    uint32_t ext_family;
    uint32_t ebx, ecx, edx;
    const uint32_t model_mask = 0xF0;
    const uint32_t family_mask = 0xF00;
    const uint32_t extended_model_mask = 0xF0000;
    const uint32_t extended_family_mask = 0xFF00000;

    __get_cpuid(key, &proc_info, &ebx, &ecx, &edx);

    model = (proc_info & model_mask) >> 4;
    family = (proc_info & family_mask) >> 8;
    ext_model = (proc_info & extended_model_mask) >> 16;
    ext_family = (proc_info & extended_family_mask)>> 20;

    if (family == 6) {
        model+=(ext_model << 4);
    }
    else if (family == 15) {
        model+=(ext_model << 4);
        family+=ext_family;
    }

    return ((family << 8) + model);
}
