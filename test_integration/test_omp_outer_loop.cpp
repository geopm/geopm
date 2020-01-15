/*
 * Copyright (c) 2015, 2016, 2017, 2018, 2019, Intel Corporation
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
#include <mpi.h>
#include <omp.h>
#include <memory>
#include <vector>

#include "geopm.h"
#include "ModelRegion.hpp"

int main(int argc, char **argv)
{
    int err = 0;
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

    std::vector<std::unique_ptr<geopm::ModelRegionBase> > spin_models;
    std::vector<std::unique_ptr<geopm::ModelRegionBase> > all2all_models;
    int num_threads = 0;
# pragma omp parallel
{
    if (omp_get_thread_num() == 0) {
        num_threads = omp_get_num_threads();
    }
}
    for (int x = 0; x < num_threads; ++x) {
        spin_models.push_back(std::unique_ptr<geopm::ModelRegionBase>(geopm::model_region_factory("spin", 1.0, is_verbose)));
        all2all_models.push_back(std::unique_ptr<geopm::ModelRegionBase>(geopm::model_region_factory("all2all", 1.0, is_verbose)));
    }

    int repeat = 50;
#pragma omp parallel for
    for (int rep_idx = 0; rep_idx < repeat; ++rep_idx) {
        int thread_id = omp_get_thread_num();
        spin_models[thread_id]->run();
        all2all_models[thread_id]->run();
    }
    MPI_Finalize();
    return err;
}
