/*
 * Copyright (c) 2015 - 2024, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <mpi.h>

int main(int argc, char **argv)
{
    struct timespec interval = {5, 0};
    int size, rank;
    MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    if (!rank) {
        printf("MPI_COMM_WORLD size: %d\n", size);
        printf("Sleeping for five seconds\n");
        fflush(stdout);
    }
    clock_nanosleep(CLOCK_REALTIME, 0, &interval, NULL);
    MPI_Finalize();
    return 0;
}
