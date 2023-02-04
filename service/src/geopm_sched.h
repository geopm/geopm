/*
 * Copyright (c) 2015 - 2023, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef GEOPM_SCHED_H_INCLUDE
#define GEOPM_SCHED_H_INCLUDE

#include <stdio.h>

#ifdef __cplusplus
extern "C"
{
#endif

#ifndef __linux__
/*!
 * @brief cpuset definition for non-linux platforms.
 */
typedef struct cpu_set_t {
    long int x[512];
} cpu_set_t;

static inline void CPU_SET(int cpu, cpu_set_t *set)
{
    int array_num = -1;
    long comp_mask;

    array_num = cpu / 64;
    comp_mask = 1 << (cpu % 64);

    set->x[array_num] |= comp_mask;
}

static inline int  CPU_ISSET(int cpu, cpu_set_t *set)
{
    int array_num = -1;
    long comp_mask;

    array_num = cpu / 64;
    comp_mask = 1 << (cpu % 64);

    return set->x[array_num] &= comp_mask;
}
#else
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <sched.h>
#endif

int geopm_sched_num_cpu(void);

int geopm_sched_get_cpu(void);

int geopm_sched_proc_cpuset(int num_cpu, cpu_set_t *proc_cpuset);

int geopm_sched_proc_cpuset_pid(int pid, int num_cpu, cpu_set_t *cpuset);

int geopm_sched_woomp(int num_cpu, cpu_set_t *woomp);

#ifdef __cplusplus
}
#endif

#endif
