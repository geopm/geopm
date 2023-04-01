/*
 * Copyright (c) 2015 - 2023, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "config.h"

#include <mpi.h>
#include <unistd.h>
#include <cmath>
#include <string>
#include <vector>
#include <memory>


int main(int argc, char **argv)
{
    int comm_rank;
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &comm_rank);
    sleep(5);
    // Note: do not change this value without updating the corresponding python test
    int num_step = 10000000;
    for (int idx = 0; idx != num_step; ++idx) {
        MPI_Barrier(MPI_COMM_WORLD);
    }
    MPI_Finalize();
    return 0;
}
