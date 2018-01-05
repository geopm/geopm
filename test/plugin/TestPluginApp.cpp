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

#include <stdlib.h>
#include <mpi.h>
#include <stdint.h>

#include "geopm.h"
#include "geopm_time.h"
#include "SharedMemory.hpp"
#include "ProfileTable.hpp"

int main(int argc, char **argv)
{
    const char *policy_name = "geopm_test_plugin_policy";
    const char *report_name = "TestPluginApp-prof.txt";

    const double imbalance = 0.10;
    /// @todo: Lost find method.
    /// const int rank_per_node = 8;
    const size_t clock_req_base = 100000000000;
    int comm_size, comm_rank;
    /// @todo: Lost find method.
    /// int local_rank;
    uint64_t region_id;
    size_t clock_req, num_clock;
    double progress, time_delta, clock_freq;
    struct geopm_time_s last_time;
    struct geopm_time_s curr_time;

    setenv("GEOPM_POLICY", policy_name, 1);
    setenv("GEOPM_REPORT", report_name, 1);

    MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &comm_size);
    MPI_Comm_rank(MPI_COMM_WORLD, &comm_rank);
    /// @todo: Lost find method.
    /// local_rank = comm_rank % rank_per_node;

    geopm::SharedMemoryUser shmem("/geopm_test_platform_shmem_freq", 5.0);
    geopm::ProfileTable table(shmem.size(), shmem.pointer());

    geopm_prof_region("main_loop", GEOPM_REGION_HINT_UNKNOWN, &region_id);

    // imbalance is proportional to rank and ranges from 0 to 10%
    clock_req = clock_req_base * (1.0 + (comm_rank * imbalance) / comm_size);

    geopm_prof_enter(region_id);
    geopm_time(&last_time);
    num_clock = 0;
    while (num_clock < clock_req) {
        geopm_time(&curr_time);
        time_delta = geopm_time_diff(&last_time, &curr_time);
        /// @todo: Lost find method.
        std::vector<std::pair<uint64_t, struct geopm_prof_message_s> >::iterator it;
        size_t len;
        table.dump(it, len);
        clock_freq = it->second.progress;
        num_clock += time_delta * clock_freq;
        progress = (double)num_clock / (double)clock_req;
        progress = progress <= 1.0 ? progress : 1.0;
        geopm_prof_progress(region_id, progress);
        last_time = curr_time;
    }
    geopm_prof_exit(region_id);
    MPI_Finalize();
    return 0;
}
