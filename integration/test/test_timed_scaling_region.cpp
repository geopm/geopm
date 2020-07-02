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

#include <string.h>
#include <limits.h>
#include <mpi.h>
#include <cmath>
#include <string>
#include <vector>
#include <memory>

#include "geopm.h"
#include "Profile.hpp"
#include "PlatformIO.hpp"
#include "PlatformTopo.hpp"
#include "Exception.hpp"
#include "ModelRegion.hpp"

using geopm::platform_io;

int main(int argc, char **argv)
{
    int comm_rank;
    int comm_size;
    MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &comm_size);
    MPI_Comm_rank(MPI_COMM_WORLD, &comm_rank);
    bool is_verbose = false;
    if (comm_rank == 0) {
        for (int arg_idx = 1; arg_idx < argc; ++arg_idx) {
            if (strncmp(argv[arg_idx], "--verbose", strlen("--verbose")) == 0 ||
                strncmp(argv[arg_idx], "-v", strlen("-v")) == 0) {
                is_verbose = true;
            }
        }
    }
    std::unique_ptr<geopm::ModelRegion> scaling_model(
        geopm::ModelRegion::model_region("timed_scaling", 1.0, is_verbose));

    double freq_min = platform_io().read_signal("CPUINFO::FREQ_MIN", GEOPM_DOMAIN_BOARD, 0);
    double freq_sticker = platform_io().read_signal("CPUINFO::FREQ_STICKER", GEOPM_DOMAIN_BOARD, 0);
    double freq_step = platform_io().read_signal("CPUINFO::FREQ_STEP", GEOPM_DOMAIN_BOARD, 0);
    int num_step = std::lround((freq_sticker - freq_min) / freq_step) + 1;
    geopm::Profile &prof = geopm::Profile::default_profile();

    for (int idx = 0; idx != num_step; ++idx) {
        std::string name = "timed_scaling_region_" + std::to_string(idx);
        uint64_t rid = prof.region(name, GEOPM_REGION_HINT_UNKNOWN);
        prof.enter(rid);
        scaling_model->run();
        prof.exit(rid);
    }
    MPI_Finalize();
    return 0;
}
