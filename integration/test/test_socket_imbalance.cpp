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

#include "config.h"

#include <mpi.h>
#include <cmath>
#include <string>
#include <vector>
#include <memory>

#include "geopm.h"
#include "Exception.hpp"
#include "ModelRegion.hpp"
#include "PlatformTopo.hpp"
#include "geopm_mpi_comm_split.h"
#include "geopm_sched.h"

int main(int argc, char **argv)
{
    // Start MPI
    int comm_rank;
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &comm_rank);
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
    double big_o = big_o_base * (1.0 + package_idx * 0.25);

    MPI_Comm shared_comm;
    int err = geopm_comm_split_shared(MPI_COMM_WORLD, NULL, &shared_comm);
    if (err) {
        throw geopm::Exception("geopm_comm_split_shared()", err, __FILE__, __LINE__);
    }
    int shared_rank;
    err = MPI_Comm_rank(shared_comm, &shared_rank);
    if (err) {
        throw geopm::Exception("MPI_Comm_rank()", err, __FILE__, __LINE__);
    }
    
    // Create a model region
    std::unique_ptr<geopm::ModelRegion> model(
        geopm::ModelRegion::model_region("dgemm", big_o, is_verbose));

    // Loop over 1000 iterations of executing the renamed region
    int num_step = 1000;
    for (int idx = 0; idx != num_step; ++idx) {
        model->run();
        MPI_Barrier(MPI_COMM_WORLD);
    }
    // Shutdown MPI
    MPI_Finalize();
    return 0;
}
