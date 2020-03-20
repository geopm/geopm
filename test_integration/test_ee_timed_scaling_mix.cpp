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
#include <string>
#include <vector>
#include <memory>

#include "geopm.h"
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

    std::string test_name = "test_ee_timed_scaling_mix";
    std::unique_ptr<geopm::ModelRegion> ignore_model(geopm::ModelRegion::model_region("spin", 0.075, is_verbose));
    uint64_t ignore_region_id = 0;
    err = geopm_prof_region("ignore", GEOPM_REGION_HINT_IGNORE, &ignore_region_id);
    if (err) {
        throw geopm::Exception(test_name, err, __FILE__, __LINE__);
    }
    int repeat = 100;
    double scaling_factor = 1;
    double timed_factor = 1;
    int num_mix = 5;
    double mix_factor = 1.0 / (num_mix - 1);
    for (int mix_idx = 0; mix_idx != num_mix; ++mix_idx) {
        int timed_idx = num_mix - 1 - mix_idx;
        int scaling_idx = mix_idx;
        double timed_big_o = timed_factor * mix_factor * timed_idx;
        double scaling_big_o = scaling_factor * mix_factor * scaling_idx;
        std::unique_ptr<geopm::ModelRegion> timed_model(geopm::ModelRegion::model_region("timed_scaling-unmarked", timed_big_o, is_verbose));
        std::unique_ptr<geopm::ModelRegion> scaling_model(geopm::ModelRegion::model_region("scaling-unmarked", scaling_big_o, is_verbose));
        char region_name[NAME_MAX];
        region_name[NAME_MAX - 1] = '\0';
        snprintf(region_name, NAME_MAX - 1, "timed-%.2f-scaling-%.2f", timed_big_o, scaling_big_o);
        uint64_t region_id = 0;
        err = geopm_prof_region(region_name, GEOPM_REGION_HINT_UNKNOWN, &region_id);
        if (err) {
            throw geopm::Exception(test_name, err, __FILE__, __LINE__);
        }
        for (int rep_idx = 0; rep_idx != repeat; ++rep_idx) {
            err = geopm_prof_enter(region_id);
            if (err) {
                throw geopm::Exception(test_name, err, __FILE__, __LINE__);
            }
            timed_model->run();
            scaling_model->run();
            err = geopm_prof_exit(region_id);
            if (err) {
                throw geopm::Exception(test_name, err, __FILE__, __LINE__);
            }
            err = geopm_prof_enter(ignore_region_id);
            if (err) {
                throw geopm::Exception(test_name, err, __FILE__, __LINE__);
            }
            ignore_model->run();
            err = geopm_prof_exit(ignore_region_id);
            if (err) {
                throw geopm::Exception(test_name, err, __FILE__, __LINE__);
            }
            MPI_Barrier(MPI_COMM_WORLD);
        }
    }
    MPI_Finalize();
    return err;
}
