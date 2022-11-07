/*
 * Copyright (c) 2015 - 2022, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#define _GNU_SOURCE
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sched.h>
#include <assert.h>


void no_omp_cpu(int num_cpu, cpu_set_t *no_omp)
{
    int cpu_index, i;

    for (i = 0; i < num_cpu; ++i) {
        CPU_SET(i, no_omp);
    }
    #pragma omp parallel default(shared)
    {
        #pragma omp critical
        {
            cpu_index = sched_getcpu();
            assert(cpu_index < num_cpu);
            CPU_CLR(cpu_index, no_omp);
        } /* end pragma omp critical */
    } /* end pragma omp parallel */
}

int main(int argc, char **argv)
{
    int i, num_cpu = sysconf(_SC_NPROCESSORS_ONLN);
    cpu_set_t *no_omp = CPU_ALLOC(num_cpu);
    no_omp_cpu(num_cpu, no_omp);
    printf("Free CPU list: ");
    for (i = 0; i < num_cpu; ++i) {
        if (CPU_ISSET(i, no_omp)) {
            printf("%i ", i);
        }
    }
    printf("\n\n");
    CPU_FREE(no_omp);
    return 0;
}
