/*
 * Copyright (c) 2015 - 2023, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <string.h>
#include <limits.h>
#include <mpi.h>
#include <unistd.h>
#include <cmath>
#include <string>
#include <vector>
#include <memory>

#include "geopm_hint.h"
#include "Profile.hpp"
#include "geopm/PlatformIO.hpp"
#include "geopm/PlatformTopo.hpp"
#include "geopm/Exception.hpp"
#include "ModelRegion.hpp"

using geopm::platform_io;

int main(int argc, char **argv)
{
    int comm_rank;
    int comm_size;
    MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &comm_size);
    MPI_Comm_rank(MPI_COMM_WORLD, &comm_rank);
    sleep(5);
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
        geopm::ModelRegion::model_region("scaling", 0.005, is_verbose));

    double freq_min = platform_io().read_signal("CPUINFO::FREQ_MIN", GEOPM_DOMAIN_BOARD, 0);
    double freq_sticker = platform_io().read_signal("CPUINFO::FREQ_STICKER", GEOPM_DOMAIN_BOARD, 0);
    double freq_step = platform_io().read_signal("CPUINFO::FREQ_STEP", GEOPM_DOMAIN_BOARD, 0);
    int num_step = std::lround((freq_sticker - freq_min) / freq_step) + 1;
    geopm::Profile &prof = geopm::Profile::default_profile();

    int repeat = 1000;
    for (int idx = 0; idx != num_step; ++idx) {
        std::string scaling_name = "scaling_region_" + std::to_string(idx);
        uint64_t scaling_rid = prof.region(scaling_name, GEOPM_REGION_HINT_UNKNOWN);
        prof.enter(scaling_rid);
        for (int rep_idx = 0; rep_idx != repeat; ++rep_idx) {
            scaling_model->run();
        }
        prof.exit(scaling_rid);
    }
    MPI_Finalize();
    return 0;
}
