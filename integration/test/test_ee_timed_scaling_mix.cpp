/*
 * Copyright (c) 2015 - 2022, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <string.h>
#include <limits.h>
#include <mpi.h>
#include <string>
#include <vector>
#include <memory>

#include "geopm_hint.h"
#include "Profile.hpp"
#include "geopm/Exception.hpp"
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
    geopm::Profile &prof = geopm::Profile::default_profile();
    uint64_t ignore_region_id = prof.region("ignore", GEOPM_REGION_HINT_IGNORE);
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
        region_id = prof.region(region_name, GEOPM_REGION_HINT_UNKNOWN);
        for (int rep_idx = 0; rep_idx != repeat; ++rep_idx) {
            prof.enter(region_id);
            timed_model->run();
            scaling_model->run();
            prof.exit(region_id);
            prof.enter(ignore_region_id);
            ignore_model->run();
            prof.exit(ignore_region_id);
            MPI_Barrier(MPI_COMM_WORLD);
        }
    }
    MPI_Finalize();
    return err;
}
