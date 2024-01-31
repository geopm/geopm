/*
 * Copyright (c) 2015 - 2024, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdlib.h>
#include <stdio.h>
#include <mpi.h>
#include <geopm_prof.h>
#include <geopm_hint.h>

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

    uint64_t sleep_rid;
    uint64_t stream_rid;
    uint64_t dgemm_rid;
    uint64_t all2all_rid;

    if (!err) {
        err = geopm_prof_region("tutorial_sleep",
                                GEOPM_REGION_HINT_UNKNOWN,
                                &sleep_rid);
    }
    if (!err) {
        err = geopm_prof_region("tutorial_stream",
                                GEOPM_REGION_HINT_MEMORY,
                                &stream_rid);
    }
    if (!err) {
        err = geopm_prof_region("tutorial_dgemm",
                                GEOPM_REGION_HINT_COMPUTE,
                                &dgemm_rid);
    }
    if (!err) {
        err = geopm_prof_region("tutorial_all2all",
                                GEOPM_REGION_HINT_NETWORK,
                                &all2all_rid);
    }

    int num_iter = 10;
    double sleep_big_o = 1.0;
    double stream0_big_o = 1.0;
    double dgemm_big_o = 1.0;
    double all2all_big_o = 1.0;
    double stream1_big_o = 1.0;

    if (!rank) {
        printf("Beginning loop of %d iterations.\n", num_iter);
        fflush(stdout);
    }
    for (int i = 0; !err && i < num_iter; ++i) {
        err = geopm_prof_epoch();
        if (!err) {
            err = geopm_prof_enter(sleep_rid);
        }
        if (!err) {
            err = tutorial_sleep(sleep_big_o, 0);
        }
        if (!err) {
            err = geopm_prof_exit(sleep_rid);
        }
        if (!err) {
            err = geopm_prof_enter(stream_rid);
        }
        if (!err) {
            err = tutorial_stream(stream0_big_o, 0);
        }
        if (!err) {
            err = geopm_prof_exit(stream_rid);
        }
        if (!err) {
            err = geopm_prof_enter(dgemm_rid);
        }
        if (!err) {
            err = tutorial_dgemm(dgemm_big_o, 0);
        }
        if (!err) {
            err = geopm_prof_exit(dgemm_rid);
        }
        if (!err) {
            err = geopm_prof_enter(stream_rid);
        }
        if (!err) {
            err = tutorial_stream(stream1_big_o, 0);
        }
        if (!err) {
            err = geopm_prof_exit(stream_rid);
        }
        if (!err) {
            err = geopm_prof_enter(all2all_rid);
        }
        if (!err) {
            err = tutorial_all2all(all2all_big_o, 0);
        }
        if (!err) {
            err = geopm_prof_exit(all2all_rid);
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
