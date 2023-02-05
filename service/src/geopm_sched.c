/*
 * Copyright (c) 2015 - 2023, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
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

int geopm_sched_num_cpu(void)
{
    return sysconf(_SC_NPROCESSORS_CONF);
}

int geopm_sched_get_cpu(void)
{
    return sched_getcpu();
}

static pthread_once_t g_proc_cpuset_once = PTHREAD_ONCE_INIT;
static cpu_set_t *g_proc_cpuset = NULL;
static size_t g_proc_cpuset_size = 0;

/* If /proc/self/status is usable and correct then parse this file to
   determine the process affinity. */

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
            while ((comma_ptr = strchr(comma_ptr, ','))) {
                ++comma_ptr;
                ++num_comma;
            }
            if (num_comma > num_read - 1) {
                num_comma -= num_read - 1;
                for (int i = 0; !err && i < num_comma; ++i) {
                    line_ptr = strchr(line_ptr, ',');
                    if (!line_ptr) {
                        err = GEOPM_ERROR_LOGIC;
                    }
                    else {
                        ++line_ptr;
                    }
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
    const int num_cpu = geopm_sched_num_cpu();
    g_proc_cpuset = CPU_ALLOC(num_cpu);
    if (g_proc_cpuset == NULL) {
        err = ENOMEM;
    }
    else {
        g_proc_cpuset_size = CPU_ALLOC_SIZE(num_cpu);
        (void)geopm_proc_cpuset(num_cpu, g_proc_cpuset);
    }
}

int geopm_sched_proc_cpuset(int num_cpu, cpu_set_t *cpuset)
{
    return geopm_sched_proc_cpuset_pid(getpid(), num_cpu, cpuset);
}

int geopm_sched_proc_cpuset_pid(int pid, int num_cpu, cpu_set_t *cpuset)
{
    const size_t cpuset_size = CPU_ALLOC_SIZE(num_cpu);
    const int num_read = num_cpu / 32 + (num_cpu % 32 ? 1 : 0);
    int err = 0;
    uint32_t *proc_cpuset = NULL;
    FILE *fid = NULL;

    char status_path[NAME_MAX];
    int nprint = snprintf(status_path, NAME_MAX, "/proc/%d/status", pid);
    if (nprint == NAME_MAX) {
        err = EINVALID;
    }
    if (!err) {
        proc_cpuset = calloc(num_read, sizeof(*proc_cpuset));
        if (proc_cpuset == NULL) {
            err = ENOMEM;
        }
    }
    if (!err) {
        fid = fopen(status_path, "r");
        if (!fid) {
            err = errno ? errno : GEOPM_ERROR_RUNTIME;
        }
    }
    if (!err) {
        err = geopm_sched_proc_cpuset_helper(num_cpu, proc_cpuset, fid);
    }
    if (fid) {
        fclose(fid);
    }
    if (!err) {
        /* cpu_set_t is managed in units of unsigned long, and may have extra
         * bits at the end with undefined values. If that happens,
         * cpuset_size may be greater than the size of proc_cpuset,
         * resulting in reading past the end of proc_cpuset. Avoid this by
         * only copying the number of bytes needed to contain the mask. Zero
         * the destination first, since it may not be fully overwritten.
         *
         * See the CPU_SET(3) man page for more details about cpu_set_t.
         */
        CPU_ZERO_S(cpuset_size, cpuset);
        memcpy(cpuset, proc_cpuset, num_read * sizeof(*proc_cpuset));
    }
    else if (cpuset) {
        for (int i = 0; i < num_cpu; ++i) {
            CPU_SET_S(i, cpuset_size, cpuset);
        }
    }
    if (proc_cpuset) {
        free(proc_cpuset);
    }
}

int geopm_sched_proc_cpuset(int num_cpu, cpu_set_t *proc_cpuset)
{
    int err = pthread_once(&g_proc_cpuset_once, geopm_proc_cpuset_once);
    int sched_num_cpu = geopm_sched_num_cpu();
    size_t cpuset_size = CPU_ALLOC_SIZE(num_cpu);
    if (!err && cpuset_size < g_proc_cpuset_size) {
        err = GEOPM_ERROR_INVALID;
    }
    if (!err) {
        /* Copy up to the smaller of the sizes to avoid buffer overruns. Zero
         * the destination set first, since it may not be fully overwritten
         */
        CPU_ZERO_S(cpuset_size, proc_cpuset);
        memcpy(proc_cpuset, g_proc_cpuset, g_proc_cpuset_size);
        for (int i = sched_num_cpu; i < num_cpu; ++i) {
            CPU_CLR_S(i, cpuset_size, proc_cpuset);
        }
    }
    return err;
}

int geopm_sched_woomp(int num_cpu, cpu_set_t *woomp)
{
    /*! @brief Function that returns a cpuset that has bits set for
               all CPUs enabled for the process which are not used by
               OpenMP.  Rather than returning an empty mask, if all
               CPUs allocated for the process are used by OpenMP, then
               the woomp mask will have all bits set. */

    int err = pthread_once(&g_proc_cpuset_once, geopm_proc_cpuset_once);
    int sched_num_cpu = geopm_sched_num_cpu();
    size_t req_alloc_size = CPU_ALLOC_SIZE(num_cpu);

    if (!err && !g_proc_cpuset) {
        err = ENOMEM;
    }
    if (!err && req_alloc_size < g_proc_cpuset_size) {
        err = EINVAL;
    }
    if (!err) {
        /* Copy the process CPU mask into the output. */
        CPU_ZERO_S(req_alloc_size, woomp);
        memcpy(woomp, g_proc_cpuset, g_proc_cpuset_size);
        /* Start an OpenMP parallel region and have each thread clear
           its bit from the mask. */
#ifdef _OPENMP
#pragma omp parallel default(shared)
{
#pragma omp critical
{
        int cpu_index = sched_getcpu();
        if (cpu_index != -1 && cpu_index < num_cpu) {
            /* Clear the bit for this OpenMP thread's CPU. */
            CPU_CLR_S(cpu_index, g_proc_cpuset_size, woomp);
        }
        else {
            err = errno ? errno : GEOPM_ERROR_LOGIC;
        }
} /* end pragma omp critical */
} /* end pragma omp parallel */
#endif /* _OPENMP */
    }
    if (!err) {
        for (int i = sched_num_cpu; i < num_cpu; ++i) {
            CPU_CLR_S(i, req_alloc_size, woomp);
        }
    }
    if (err || CPU_COUNT_S(g_proc_cpuset_size, woomp) == 0) {
        /* If all CPUs are used by the OpenMP gang, then leave the
           mask open and allow the Linux scheduler to choose. */
        for (int i = 0; i < num_cpu; ++i) {
            CPU_SET_S(i, g_proc_cpuset_size, woomp);
        }
    }
    return err;
}
