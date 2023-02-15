/*
 * Copyright (c) 2015 - 2023, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
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
