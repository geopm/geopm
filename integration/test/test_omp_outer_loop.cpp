/*
 * Copyright (c) 2015 - 2023, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <string.h>
#include <mpi.h>
#include <unistd.h>

#include <vector>

#ifdef _OPENMP
#include <omp.h>
#else
static int omp_get_num_threads(void) {return 1;}
static int omp_get_thread_num(void) {return 0;}
#endif

int main(int argc, char **argv)
{
    int err = 0;
    int comm_rank = -1;
    int comm_size = -1;
    err = MPI_Init(&argc, &argv);
    sleep(5);
    if (!err) {
        err = MPI_Comm_size(MPI_COMM_WORLD, &comm_size);
    }
    if (!err) {
        err = MPI_Comm_rank(MPI_COMM_WORLD, &comm_rank);
    }

    int num_threads = 0;
# pragma omp parallel
{
    if (omp_get_thread_num() == 0) {
        num_threads = omp_get_num_threads();
    }
}

    int per_thread = 100;
    int repeat = num_threads * per_thread;
    std::vector<double> in_buffer(10000000, comm_rank);
    std::vector<double> out_buffer(10000000, 0.0);

#pragma omp parallel for
    for (int rep_idx = 0; rep_idx < repeat; ++rep_idx) {
        if (omp_get_thread_num() == 0) {
            if (!err) {
                MPI_Allreduce(in_buffer.data(), out_buffer.data(), in_buffer.size(),
                              MPI_DOUBLE, MPI_SUM, MPI_COMM_WORLD);
            }
        }
    }
    MPI_Finalize();
    return err;
}
