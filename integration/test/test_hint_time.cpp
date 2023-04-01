/*
 * Copyright (c) 2015 - 2023, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "config.h"

#include <mpi.h>
#include <cmath>
#include <string>
#include <vector>
#include <memory>
#include <unistd.h>

#include "geopm_hint.h"
#include "Profile.hpp"
#include "geopm/Exception.hpp"
#include "ModelRegion.hpp"

int main(int argc, char **argv)
{
    // Start MPI
    int comm_rank;
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &comm_rank);
    sleep(5);
    // Parse command line option for verbosity
    bool is_verbose = false;
    if (comm_rank == 0) {
        for (int arg_idx = 1; arg_idx < argc; ++arg_idx) {
            std::string arg(argv[arg_idx]);
            if (arg == "--verbose" || arg == "-v") {
                is_verbose = true;
            }
        }
    }
    // Create an unmarked spin model region to get exact timings
    // without calling the geopm profile API.
    std::unique_ptr<geopm::ModelRegion> spin(
        geopm::ModelRegion::model_region("spin-unmarked", 1.0, is_verbose));

    geopm::Profile &prof = geopm::Profile::default_profile();
    uint64_t nw_rid = prof.region("network", GEOPM_REGION_HINT_NETWORK);
    uint64_t nw_mem_rid = prof.region("network-memory", GEOPM_REGION_HINT_NETWORK);
    uint64_t mem_rid = prof.region("memory", GEOPM_REGION_HINT_MEMORY);

    prof.enter(nw_rid);
    spin->run();
    prof.exit(nw_rid);
    prof.enter(nw_mem_rid);
    spin->run();
    prof.epoch();
    prof.enter(mem_rid);
    spin->run();
    prof.exit(mem_rid);
    spin->run();
    prof.exit(nw_mem_rid);

    // Shutdown MPI
    MPI_Finalize();
    return 0;
}
