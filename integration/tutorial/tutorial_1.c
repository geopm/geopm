/*
 * Copyright (c) 2015 - 2024, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdlib.h>
#include <stdio.h>
#include <mpi.h>

#include "tutorial_region.h"


int main(int argc, char **argv)
{
    int size = 0;
    int rank = 0;
    int num_iter = 10;
    double sleep_big_o = 1.0;
    double stream0_big_o = 1.0;
    double dgemm_big_o = 1.0;
    double all2all_big_o = 1.0;
    double stream1_big_o = 1.0;

    int err = MPI_Init(&argc, &argv);
    if (!err) {
        err = MPI_Comm_size(MPI_COMM_WORLD, &size);
    }
    if (!err) {
        err = MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    }
    if (!err && !rank ) {
        printf("MPI_COMM_WORLD size: %d\n", size);
    }
    if (!rank) {
        printf("Beginning loop of %d iterations.\n", num_iter);
        fflush(stdout);
    }
    for (int i = 0; !err && i < num_iter; ++i) {
        err = tutorial_sleep(sleep_big_o, 0);
        if (!err) {
            err = tutorial_stream(stream0_big_o, 0);
        }
        if (!err) {
            err = tutorial_dgemm(dgemm_big_o, 0);
        }
        if (!err) {
            err = tutorial_stream(stream1_big_o, 0);
        }
        if (!err) {
            err = tutorial_all2all(all2all_big_o, 0);
        }
        if (!err && !rank) {
            printf("Iteration=%.3d\r", i);
            fflush(stdout);
        }
    }
    if (!err && !rank) {
        printf("Completed loop.                    \n");
        fflush(stdout);
    }

    int err_fin = MPI_Finalize();
    err = err ? err : err_fin;

    return err;
}
