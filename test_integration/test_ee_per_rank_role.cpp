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

#include <cmath>
#include <string.h>
#include <limits.h>
#include <mpi.h>
#include <string>
#include <vector>
#include <memory>

#include "geopm.h"
#include "Helper.hpp"
#include "Exception.hpp"
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

    int repeat = 10;//300;
    double dgemm_big_o = NAN;
    double stream_big_o = NAN;
    // @todo hostname based detection
    dgemm_big_o = 17.0;
    stream_big_o = 1.45;
    std::unique_ptr<geopm::ModelRegionBase> region = nullptr;
    switch (comm_rank) {
        case 0:
            {
            std::unique_ptr<geopm::ModelRegionBase> dgemm_model(geopm::model_region_factory("dgemm"/*-unmarked"*/, dgemm_big_o, is_verbose));
            region = std::move(dgemm_model);
            }
            break;
        case 1:
            {
            std::unique_ptr<geopm::ModelRegionBase> stream_model(geopm::model_region_factory("stream"/*-unmarked"*/, stream_big_o, is_verbose));
            region = std::move(stream_model);
            }
            break;
        case 2:
            {
            std::unique_ptr<geopm::ModelRegionBase> spin_model(geopm::model_region_factory("spin"/*-unmarked"*/, 0.80, is_verbose));
            region = std::move(spin_model);
            }
            break;
        case 3:
            {
            std::unique_ptr<geopm::ModelRegionBase> sleep_model(geopm::model_region_factory("sleep"/*-unmarked"*/, 0.80, is_verbose));
            region = std::move(sleep_model);
            }
            break;
    }
    for (int rep_idx = 0; rep_idx != repeat; ++rep_idx) {
        if (nullptr != region) {
            region->run();
        }
        MPI_Barrier(MPI_COMM_WORLD);
    }
    MPI_Finalize();
    return err;
}
