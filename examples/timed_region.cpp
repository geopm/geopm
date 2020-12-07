/*
 * Copyright (c) 2015, 2016, 2017, 2018, 2019, 2020, Intel Corporation
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

#include <mpi.h>

#include "geopm.h"
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
