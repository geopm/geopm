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

#ifdef __APPLE__
#define _DARWIN_C_SOURCE
#include <sys/types.h>
#include <sys/sysctl.h>
#endif

#include <stdint.h>
#include <unistd.h>
#include <errno.h>

#include "geopm_sched.h"
#include "geopm_error.h"
#include "config.h"

#ifdef _OPENMP
#include <omp.h>
#endif

#ifndef __APPLE__

int geopm_sched_num_cpu(void)
{
    return sysconf(_SC_NPROCESSORS_ONLN);
}

int geopm_sched_get_cpu(void)
{
    return sched_getcpu();
}

int geopm_sched_woomp(int num_cpu, cpu_set_t *woomp)
{
    int err = sched_getaffinity(0, CPU_ALLOC_SIZE(num_cpu), woomp);
    if (err && errno) {
        err = errno;
    }
    if (!err) {
#ifdef _OPENMP
#pragma omp parallel default(shared)
{
#pragma omp critical
{
        int cpu_index = sched_getcpu();
        if (cpu_index < num_cpu)
        {
            CPU_CLR(cpu_index, woomp);
        }
        else {
            err = GEOPM_ERROR_LOGIC;
        }
} /* end pragma omp critical */
} /* end pragma omp parallel */
#endif
    }
    if (CPU_COUNT(woomp) == 0) {
        /* If all CPUs are used by the OpenMP gang, then leave the
	   mask open and allow the Linux scheduler to choose. */
        for (int i = 0; i < num_cpu; ++i) {
            CPU_SET(i, woomp);
        }
    }
    return err;
}

#else

int geopm_sched_num_cpu(void)
{
    uint32_t result = 1;
    size_t len = sizeof(result);
    sysctl((int[2]) {CTL_HW, HW_NCPU}, 2, &result, &len, NULL, 0);
    return result;
}


int geopm_sched_get_cpu(void)
{
    int result = -1;
    uint32_t cpu_info[4];
    __cpuid(cpu_info, 1);
    // Check APIC
    if (cpu_info[3] & (1 << 9)) {
        result = (int)(cpu_info[1] >> 24);
    }
    return result;
}

int geopm_sched_woomp(int num_cpu, cpu_set_t *woomp)
{
    for (int i = 0; i < num_cpu; ++i) {
        CPU_SET(i, woomp);
    }
    return 0;
}

#endif
