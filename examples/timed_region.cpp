/*
 * Copyright (c) 2015 - 2022, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <mpi.h>

#include "geopm_prof.h"
#include "geopm_hint.h"
#include "geopm_time.h"

int main(int argc, char**argv)
{
    uint64_t region_id[3];
    int rank;
    int iterations, total_iterations;

    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    geopm_prof_region("loop_one", GEOPM_REGION_HINT_UNKNOWN, &region_id[0]);
    geopm_prof_enter(region_id[0]);
    total_iterations = 1000;
    geopm_tprof_init(total_iterations);
    iterations = 0;
    while (iterations < total_iterations) {
        geopm_tprof_post();
        ++iterations;
    }
    geopm_prof_exit(region_id[0]);

    geopm_prof_region("loop_two", GEOPM_REGION_HINT_UNKNOWN, &region_id[1]);
    geopm_prof_enter(region_id[1]);
    total_iterations = 2000;
    geopm_tprof_init(total_iterations);
    iterations = 0;
    while (iterations < total_iterations) {
        geopm_tprof_post();
    }
    geopm_prof_exit(region_id[1]);

    geopm_prof_region("loop_three", GEOPM_REGION_HINT_UNKNOWN, &region_id[2]);
    geopm_prof_enter(region_id[2]);
    total_iterations = 1000;
    geopm_tprof_init(total_iterations);
    iterations = 0;
    while (iterations < total_iterations) {
        geopm_tprof_post();
    }
    geopm_prof_exit(region_id[2]);

    MPI_Finalize();

    return 0;
}
