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

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <pthread.h>
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

static pthread_once_t g_proc_cpuset_once = PTHREAD_ONCE_INIT;
static cpu_set_t *g_proc_cpuset = NULL;

static void geopm_proc_cpuset_once(void)
{
    const char *key = "Cpus_allowed:";
    const size_t key_len = strlen(key);
    const int num_cpu = geopm_sched_num_cpu();
    int err = 0;
    FILE *fid = NULL;
    char *line = NULL;
    size_t line_len = 0;

    g_proc_cpuset = CPU_ALLOC(num_cpu);
    if (g_proc_cpuset == NULL) {
        err = ENOMEM;
    }
    int num_read = num_cpu / 32 + (num_cpu % 32 ? 1 : 0);
    uint32_t *proc_cpuset = calloc(num_read, sizeof(uint32_t));
    if (proc_cpuset == NULL) {
        err = ENOMEM;
    }

    if (!err) {
        fid = fopen("/proc/self/status", "r");
        if (!fid) {
            err = errno ? errno : GEOPM_ERROR_RUNTIME;
        }
    }
    if (!err) {
        int read_idx = 0;
        while ((getline(&line, &line_len, fid)) != -1) {
            if (strncmp(line, key, key_len) == 0) {
                char *line_ptr = line + key_len;
                for (read_idx = 0; !err && read_idx < num_read; ++read_idx) {
                    int num_match = sscanf(line_ptr, "%x", proc_cpuset + read_idx);
                    if (num_match != 1) {
                        err = GEOPM_ERROR_RUNTIME;
                    }
                    else {
                        line_ptr = strchr(line_ptr, ',');
                        if (read_idx != num_read - 1 && line_ptr == NULL) {
                            err = GEOPM_ERROR_RUNTIME;
                        }
                        else {
                            ++line_ptr;
                        }
                    }
                }
            }
        }
        if (line) {
            free(line);
        }
        fclose(fid);
        if (read_idx != num_read) {
            err = GEOPM_ERROR_RUNTIME;
        }
    }
    if (!err) {
        memcpy(g_proc_cpuset, proc_cpuset, CPU_ALLOC_SIZE(num_cpu));
    }
    else if (g_proc_cpuset) {
        for (int i = 0; i < num_cpu; ++i) {
            CPU_SET(i, g_proc_cpuset);
        }
    }
    if (proc_cpuset) {
        free(proc_cpuset);
    }
}

int geopm_sched_woomp(int num_cpu, cpu_set_t *woomp)
{
    int err = pthread_once(&g_proc_cpuset_once, geopm_proc_cpuset_once);
    if (!g_proc_cpuset) {
        err = ENOMEM;
    }
    if (!err) {
        memcpy(woomp, g_proc_cpuset, CPU_ALLOC_SIZE(num_cpu));
    }
    if (!err) {
#ifdef _OPENMP
#pragma omp parallel default(shared)
{
#pragma omp critical
{
        int cpu_index = sched_getcpu();
        if (cpu_index != -1 && cpu_index < num_cpu)
        {
            CPU_CLR(cpu_index, woomp);
        }
        else {
            err = errno ? errno : GEOPM_ERROR_LOGIC;
        }
} /* end pragma omp critical */
} /* end pragma omp parallel */
#endif
    }
    if (err || CPU_COUNT(woomp) == 0) {
        /* If all CPUs are used by the OpenMP gang, then leave the
           mask open and allow the Linux scheduler to choose. */
        for (int i = 0; i < num_cpu; ++i) {
            CPU_SET(i, woomp);
        }
    }
    return err;
}

#else

void __cpuid(uint32_t*, int);

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
