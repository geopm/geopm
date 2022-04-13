/*
 * Copyright (c) 2015 - 2022, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdlib.h>
#include <stdio.h>
#include <mpi.h>
#include <geopm_prof.h>
#include <geopm_hint.h>

#include "geopm_imbalancer.h"
#include "tutorial_region.h"


int main(int argc, char **argv)
{
    int size = 0;
    int rank = 0;

    int err = MPI_Init(&argc, &argv);
    if (!err) {
        err = MPI_Comm_size(MPI_COMM_WORLD, &size);
    }
    if (!err) {
        err = MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    }
    if (!err && !rank) {
        printf("MPI_COMM_WORLD size: %d\n", size);
    }

    uint64_t dgemm_rid;
    if (!err) {
        err = geopm_prof_region("tutorial_dgemm",
                                GEOPM_REGION_HINT_COMPUTE,
                                &dgemm_rid);
    }

    int num_iter = 500;
    double dgemm_big_o = 8.0;

    if (!rank) {
        printf("Beginning loop of %d iterations.\n", num_iter);
        fflush(stdout);
    }
    for (int i = 0; !err && i < num_iter; ++i) {
        err = geopm_prof_epoch();
        if (!err) {
            err = geopm_prof_enter(dgemm_rid);
        }
        if (!err) {
            err = geopm_imbalancer_enter();
        }
        if (!err) {
            err = tutorial_dgemm_static(dgemm_big_o, 0);
        }
        if (!err) {
            err = geopm_imbalancer_exit();
        }
        if (!err) {
            err = geopm_prof_exit(dgemm_rid);
        }
        if (!err) {
            err = MPI_Barrier(MPI_COMM_WORLD);
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
    if (!err) {
        err = tutorial_dgemm_static(0.0, 0);
    }

    int err_fin = MPI_Finalize();
    err = err ? err : err_fin;

    return err;
}
