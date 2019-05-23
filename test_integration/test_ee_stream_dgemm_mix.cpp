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
#include <limits.h>
#include <mpi.h>
#include <string>
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
    geopm::ModelRegionBase *sleep_model = geopm::model_region_factory("sleep", 0.05, is_verbose);
    int repeat = 500;
    double dgemm_factor = 6.0;
    double stream_factor = 0.5;
    int num_mix = 11;
    double mix_factor = 1.0 / (num_mix - 1);
    for (int mix_idx = 0;  !err && mix_idx != num_mix; ++mix_idx) {
        int stream_idx = num_mix - 1 - mix_idx;
        int dgemm_idx = mix_idx;
        double stream_big_o = stream_factor * mix_factor * stream_idx;
        double dgemm_big_o = dgemm_factor * mix_factor * dgemm_idx;
        geopm::ModelRegionBase *stream_model = geopm::model_region_factory("stream-unmarked", stream_big_o, is_verbose);
        geopm::ModelRegionBase *dgemm_model = geopm::model_region_factory("dgemm-unmarked", dgemm_big_o, is_verbose);
        char region_name[NAME_MAX];
        region_name[NAME_MAX - 1] = '\0';
        snprintf(region_name, NAME_MAX - 1, "stream-%.2f-dgemm-%.2f", stream_big_o, dgemm_big_o);
        uint64_t region_id;
        err = geopm_prof_region(region_name, GEOPM_REGION_HINT_UNKNOWN, &region_id);
        if (err) goto exit;
        for (int rep_idx = 0; rep_idx != repeat; ++rep_idx) {
            err = geopm_prof_enter(region_id);
            if (err) goto exit;
            stream_model->run();
            dgemm_model->run();
            err = geopm_prof_exit(region_id);
            if (err) goto exit;
            sleep_model->run();
        }
exit:
        delete dgemm_model;
        delete stream_model;
    }
    delete sleep_model;
    MPI_Finalize();
    return err;
}
