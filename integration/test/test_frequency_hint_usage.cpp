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

    // Create a model region
    std::unique_ptr<geopm::ModelRegion> model(
        geopm::ModelRegion::model_region("reduce", 1.0, is_verbose));

    // Rename model region to the test name
    geopm::Profile &prof = geopm::Profile::default_profile();
    std::string region_name = "compute_region";
    uint64_t rid = prof.region(region_name, GEOPM_REGION_HINT_COMPUTE);

    // Loop over 100 iterations of executing the renamed region
    int num_step = 100;
    for (int idx = 0; idx != num_step; ++idx) {
        prof.enter(rid);
        model->run();
        prof.exit(rid);
    }

    // Shutdown MPI
    MPI_Finalize();
    return 0;
}
