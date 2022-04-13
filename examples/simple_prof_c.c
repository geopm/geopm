/*
 * Copyright (c) 2015 - 2022, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <stdint.h>
#include <mpi.h>
#include <omp.h>

#include "geopm_prof.h"
#include "geopm_hint.h"


int main(int argc, char **argv)
{
    int err = 0;
    int index = 0;
    int rank = 0;
    int num_iter = 100000000;
    double sum = 0.0;
    uint64_t region_id = 0;

    err = MPI_Init(&argc, &argv);
    if (!err) {
        err = geopm_prof_region("loop_0", GEOPM_REGION_HINT_UNKNOWN, &region_id);
    }
    MPI_Barrier(MPI_COMM_WORLD);
    if (!err) {
        err = geopm_prof_enter(region_id);
    }
    if (!err) {
#pragma omp parallel default(shared) private(index)
{
        (void)geopm_tprof_init(num_iter);
#pragma omp for reduction(+:sum)
        for (index = 0; index < num_iter; ++index) {
            sum += (double)index;
            (void)geopm_tprof_post();
        }
}
        err = geopm_prof_exit(region_id);
    }
    if (!err) {
        err = MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    }
    if (!err && !rank) {
        printf("sum = %e\n\n", sum);
    }

    int tmp_err = MPI_Finalize();

    return err ? err : tmp_err;
}
