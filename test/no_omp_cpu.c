/*
 * Copyright (c) 2015, 2016, 2017, 2018, 2019, Intel Corporation
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
    } /* end pragam omp parallel */
}

int main(int argc, char **argv)
{
    int i, num_cpu = sysconf(_SC_NPROCESSORS_ONLN);
    cpu_set_t *no_omp = CPU_ALLOC(num_cpu);
    no_omp_cpu(num_cpu, no_omp);
    printf("Free cpu list: ");
    for (i = 0; i < num_cpu; ++i) {
        if (CPU_ISSET(i, no_omp)) {
            printf("%i ", i);
        }
    }
    printf("\n\n");
    CPU_FREE(no_omp);
    return 0;
}
