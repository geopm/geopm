/*
 * Copyright (c) 2015, 2016, 2017, 2018, Intel Corporation
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
#define _GNU_SOURCE             /* See feature_test_macros(7) */
#endif
#include <mpi.h>
#include <stdio.h>
#include <sched.h>
#include <sys/types.h>
#include <unistd.h>
#include <sstream>
#include <iostream>
#ifdef _OPENMP
#include <omp.h>
#endif

int main(int argc, char** argv)
{
    // Initialize the MPI environment
    MPI_Init(&argc, &argv);

    // Get the number of processes
    int world_size;
    MPI_Comm_size(MPI_COMM_WORLD, &world_size);

    // Get the rank of the process
    int world_rank;
    MPI_Comm_rank(MPI_COMM_WORLD, &world_rank);

    cpu_set_t my_set;

    CPU_ZERO(&my_set);
    (void)sched_getaffinity(0, sizeof(my_set), &my_set);

    std::ostringstream cpu_list;
    bool is_found = false;
    for (int cpu = 0; cpu < CPU_SETSIZE; ++cpu) {
        if (CPU_ISSET(cpu, &my_set)) {
            if (is_found) {
                cpu_list << ",";
            }
            is_found = true;
            cpu_list << cpu;
        }
    }

    std::ostringstream thread_list;
#ifdef _OPENMP
#pragma omp parallel for ordered
    for (int i = 0; i < omp_get_num_threads(); ++i) {
        int cpu = sched_getcpu();
#pragma omp ordered
{
        if (i) {
            thread_list << ",";
        }
        thread_list << cpu;
}
    }
#endif

    for (int rank = 0; rank < world_size; ++rank) {
        if (rank == world_rank) {
            // Print off affinity message
            std::cout << "Rank: " << world_rank
                      << " cgroup CPUs: [" << cpu_list.str() << "]"
                      << " omp CPUs: [" << thread_list.str() << "]"
                      << std::endl << std::flush;
        }
        MPI_Barrier(MPI_COMM_WORLD);
    }
    // Finalize the MPI environment.
    MPI_Finalize();

    return 0;
}
