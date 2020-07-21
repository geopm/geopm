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
#include "Profile.hpp"
#include "Exception.hpp"
#include "ModelRegion.hpp"

int main(int argc, char **argv)
{
    // Start MPI
    MPI_Init(&argc, &argv);
    bool is_verbose = false;
    geopm::Profile &prof = geopm::Profile::default_profile();
    size_t num_duration = 7;
    double duration = 0.2048;
    double repeat = 200; // Each trial takes 41 seconds at sticker frequency
    for (size_t duration_idx = 0; duration_idx != num_duration; ++duration_idx) {
        // Create scaling and scaling_timed model regions
        std::unique_ptr<geopm::ModelRegion> model_scaling(
            geopm::ModelRegion::model_region("scaling", duration, is_verbose));
        std::unique_ptr<geopm::ModelRegion> model_timed(
            geopm::ModelRegion::model_region("timed_scaling", duration, is_verbose));

        // Rename model regions
        std::string scaling_name = "scaling_" + std::to_string(duration_idx);
        std::string timed_name = "timed_" + std::to_string(duration_idx);
        std::string barrier_scaling_name = "barrier_scaling_" + std::to_string(duration_idx);
        std::string barrier_timed_name = "barrier_timed_" + std::to_string(duration_idx);

        uint64_t scaling_rid = prof.region(scaling_name, GEOPM_REGION_HINT_UNKNOWN);
        uint64_t timed_rid = prof.region(timed_name, GEOPM_REGION_HINT_UNKNOWN);
        uint64_t barrier_scaling_rid = prof.region(barrier_scaling_name, GEOPM_REGION_HINT_UNKNOWN);
        uint64_t barrier_timed_rid = prof.region(barrier_timed_name, GEOPM_REGION_HINT_UNKNOWN);
        // Execute the regions back to back repeatedly
        for (int idx = 0; idx != repeat; ++idx) {
            prof.enter(scaling_rid);
            model_scaling->run();
            prof.exit(scaling_rid);
            prof.enter(barrier_scaling_rid);
            MPI_Barrier(MPI_COMM_WORLD);
            prof.exit(barrier_scaling_rid);
            prof.enter(timed_rid);
            model_timed->run();
            prof.exit(timed_rid);
            prof.enter(barrier_timed_rid);
            MPI_Barrier(MPI_COMM_WORLD);
            prof.exit(barrier_timed_rid);
        }
        repeat *= 2;
        duration /= 2.0;
    }

    // Shutdown MPI
    MPI_Finalize();
    return 0;
}
