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
#include <string.h>

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

/* If /proc/self/status is usable and correct then parse this file to
   determine the process affinity. */
#ifdef GEOPM_PROCFS

int geopm_sched_proc_cpuset_helper(int num_cpu, uint32_t *proc_cpuset, FILE *fid)
{
    const char *key = "Cpus_allowed:";
    const size_t key_len = strlen(key);
    const int num_read = num_cpu / 32 + (num_cpu % 32 ? 1 : 0);
    int err = 0;
    char *line = NULL;
    size_t line_len = 0;

    int read_idx = 0;
    while ((getline(&line, &line_len, fid)) != -1) {
        if (strncmp(line, key, key_len) == 0) {
            char *line_ptr = line + key_len;
            /* On some systems we have seen the mask padded with zeros
	       beyond the number of online CPUs.  Deal with this by
	       skipping extra leading 32 bit masks */
            int num_comma = 0;
            char *comma_ptr = line_ptr;
            while (comma_ptr = strchr(comma_ptr, ',')) {
               ++comma_ptr;
               ++num_comma;
            }
            if (num_comma > num_read - 1) {
                num_comma -= num_read - 1;
                for (int i = 0; i < num_comma; ++i) {
                    line_ptr = strchr(line_ptr, ',') + 1;
                }
            }
            for (read_idx = num_read - 1; !err && read_idx >= 0; --read_idx) {
                int num_match = sscanf(line_ptr, "%x", proc_cpuset + read_idx);
                if (num_match != 1) {
                    err = GEOPM_ERROR_RUNTIME;
                }
                else {
                    line_ptr = strchr(line_ptr, ',');
                    if (read_idx != 0 && line_ptr == NULL) {
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
    if (read_idx != -1) {
        err = GEOPM_ERROR_RUNTIME;
    }
    return err;
}

static void geopm_proc_cpuset_once(void)
{
    const char *status_path = "/proc/self/status";
    const int num_cpu = geopm_sched_num_cpu();
    const int num_read = num_cpu / 32 + (num_cpu % 32 ? 1 : 0);

    int err = 0;
    uint32_t *proc_cpuset = NULL;
    FILE *fid = NULL;

    g_proc_cpuset = CPU_ALLOC(num_cpu);
    if (g_proc_cpuset == NULL) {
        err = ENOMEM;
    }
    proc_cpuset = calloc(num_read, sizeof(uint32_t));
    if (proc_cpuset == NULL) {
        err = ENOMEM;
    }

    if (!err) {
        fid = fopen(status_path, "r");
        if (!fid) {
            err = errno ? errno : GEOPM_ERROR_RUNTIME;
        }
    }
    if (!err) {
        err = geopm_sched_proc_cpuset_helper(num_cpu, proc_cpuset, fid);
        fclose(fid);
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

/* If /proc/self/status is not available spawn a pthread requesting an
   open affinity mask and then have the thread query the affinity mask
   enforced by the OS using sched_getaffinity(). */
#else /* GEOPM_PROCFS */

static void *geopm_proc_cpuset_pthread(void *arg)
{
    void *result = NULL;
    int err = sched_getaffinity(0, CPU_ALLOC_SIZE(geopm_sched_num_cpu()), g_proc_cpuset);
    if (err) {
        result = (void *)(size_t)(errno ? errno : GEOPM_ERROR_RUNTIME);
    }
    return result;
}

static void geopm_proc_cpuset_once(void)
{
    int err = 0;
    int num_cpu = geopm_sched_num_cpu();
    pthread_t tid;
    pthread_attr_t attr;

    g_proc_cpuset = CPU_ALLOC(num_cpu);
    if (g_proc_cpuset == NULL) {
        err = ENOMEM;
    }
    if (!err) {
        for (int i = 0; i < num_cpu; ++i) {
            CPU_SET(i, g_proc_cpuset);
        }
    }
    if (!err) {
        err = pthread_attr_init(&attr);
    }
    if (!err) {
        err = pthread_attr_setaffinity_np(&attr, CPU_ALLOC_SIZE(num_cpu), g_proc_cpuset);
    }
    if (!err) {
        err = pthread_create(&tid, &attr, geopm_proc_cpuset_pthread, NULL);
    }
    if (!err) {
        void *result = NULL;
        err = pthread_join(tid, &result);
        if (!err && result) {
            err = (int)(size_t)result;
        }
    }
    if (err && err != ENOMEM)
    {
        for (int i = 0; i < num_cpu; ++i) {
            CPU_SET(i, g_proc_cpuset);
        }
    }
    if (!err) {
        err = pthread_attr_destroy(&attr);
    }
}

#endif /* GEOPM_PROCFS */

int geopm_sched_proc_cpuset(int num_cpu, cpu_set_t *proc_cpuset)
{
    int err = pthread_once(&g_proc_cpuset_once, geopm_proc_cpuset_once);
    int sched_num_cpu = geopm_sched_num_cpu();
    if (!err && sched_num_cpu > num_cpu) {
        err = GEOPM_ERROR_INVALID;
    }
    if (!err) {
        memcpy(proc_cpuset, g_proc_cpuset, CPU_ALLOC_SIZE(sched_num_cpu));
        for (int i = sched_num_cpu; i < num_cpu; ++i) {
            CPU_CLR(i, proc_cpuset);
        }
    }
    return err;
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
#endif /* _OPENMP */
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

#else /* __APPLE__ */

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

// On Mac OS just fill in all bits for the cpuset for both the process
// mask and woomp to get the tests passing.
static void geopm_cpuset_fill(int num_cpu, cpu_set_t *proc_cpuset)
{
    for (int i = 0; i < num_cpu; ++i) {
        CPU_SET(i, woomp);
    }
}
int geopm_sched_proc_cpuset(int num_cpu, cpu_set_t *proc_cpuset)
{
    geopm_cpuset_fill(num_cpu, proc_cpuset);
    return 0;
}

int geopm_sched_woomp(int num_cpu, cpu_set_t *woomp)
{
    geopm_cpuset_fill(num_cpu, woomp);
    return 0;
}

#endif /* __APPLE__ */
