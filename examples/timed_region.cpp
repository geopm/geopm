/*
 * Copyright (c) 2015, 2016, 2017, 2018, Intel Corporation
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
    struct geopm_time_s start, curr;
    double timeout = 0;
    int rank;

    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    geopm_prof_region("loop_one", GEOPM_REGION_HINT_UNKNOWN, &region_id[0]);
    geopm_prof_enter(region_id[0]);
    geopm_time(&start);
    while (timeout < 1.0) {
        geopm_time(&curr);
        timeout = geopm_time_diff(&start, &curr);
        geopm_prof_progress(region_id[2], timeout/1.0);
    }
    geopm_prof_exit(region_id[0]);

    geopm_prof_region("loop_two", GEOPM_REGION_HINT_UNKNOWN, &region_id[1]);
    geopm_prof_enter(region_id[1]);
    geopm_time(&start);
    while (timeout < 2.0) {
        geopm_time(&curr);
        timeout = geopm_time_diff(&start, &curr);
        geopm_prof_progress(region_id[2], timeout/2.0);
    }
    geopm_prof_exit(region_id[1]);

    geopm_prof_region("loop_three", GEOPM_REGION_HINT_UNKNOWN, &region_id[2]);
    geopm_prof_enter(region_id[2]);
    geopm_time(&start);
    while (timeout < 3.0) {
        geopm_time(&curr);
        timeout = geopm_time_diff(&start, &curr);
        geopm_prof_progress(region_id[2], timeout/3.0);
    }
    geopm_prof_exit(region_id[2]);

    MPI_Finalize();

    return 0;
}
