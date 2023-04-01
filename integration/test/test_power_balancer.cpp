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

#include "geopm_prof.h"
#include "geopm/Exception.hpp"
#include "ModelRegion.hpp"
#include "geopm/PlatformTopo.hpp"
#include "geopm_sched.h"

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

    int cpu_idx = geopm_sched_get_cpu();
    int package_idx = geopm::platform_topo().domain_idx(GEOPM_DOMAIN_PACKAGE, cpu_idx);
    double big_o_base = 5.0;
    double big_o = big_o_base * (1.0 + package_idx * 1.00);

    // Create a model region
    std::unique_ptr<geopm::ModelRegion> model(
        geopm::ModelRegion::model_region("dgemm", big_o, is_verbose));

    // Loop over 1000 iterations of executing the renamed region
    int num_step = 1000;
    for (int idx = 0; idx != num_step; ++idx) {
        geopm_prof_epoch();
        model->run();
        MPI_Barrier(MPI_COMM_WORLD);
    }
    // Shutdown MPI
    MPI_Finalize();
    return 0;
}
