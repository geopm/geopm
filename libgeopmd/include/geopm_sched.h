/*
 * Copyright (c) 2015 - 2024, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef GEOPM_SCHED_H_INCLUDE
#define GEOPM_SCHED_H_INCLUDE

#include <stdio.h>
#include "geopm_public.h"

#ifdef __cplusplus
extern "C"
{
#endif

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <sched.h>

int GEOPM_PUBLIC
    geopm_sched_num_cpu(void);

int GEOPM_PUBLIC
    geopm_sched_get_cpu(void);

int GEOPM_PUBLIC
    geopm_sched_proc_cpuset(int num_cpu, cpu_set_t *proc_cpuset);

int GEOPM_PUBLIC
    geopm_sched_proc_cpuset_pid(int pid, int num_cpu, cpu_set_t *cpuset);

int GEOPM_PUBLIC
    geopm_sched_woomp(int num_cpu, cpu_set_t *woomp);

#ifdef __cplusplus
}
#endif

#endif
