/*
 * Copyright (c) 2015 - 2024, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "config.h"

#include <mpi.h>
#include <unistd.h>
#include <cmath>
#include <string>
#include <vector>
#include <memory>

#include "geopm/Profile.hpp"
#include "geopm/Exception.hpp"
#include "geopm/ModelRegion.hpp"

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

    std::unique_ptr<geopm::ModelRegion> all2all(
        geopm::ModelRegion::model_region("all2all", 1.0, is_verbose));
    std::unique_ptr<geopm::ModelRegion> spin(
        geopm::ModelRegion::model_region("spin", 1.0, is_verbose));
    std::unique_ptr<geopm::ModelRegion> ignore(
        geopm::ModelRegion::model_region("ignore", 0.5, is_verbose));

    int num_step = 10;
    for (int idx = 0; idx != num_step; ++idx) {
        spin->run();
        ignore->run();
        spin->run();
        all2all->run();
    }
    sleep(1);

    // Shutdown MPI
    MPI_Finalize();
    return 0;
}
