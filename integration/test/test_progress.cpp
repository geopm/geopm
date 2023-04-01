/*
 * Copyright (c) 2015 - 2023, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <mpi.h>
#include <unistd.h>
#include <vector>
#include "geopm_prof.h"
#include "ModelRegion.hpp"


__attribute__((noinline))
void setup(std::vector<double> &aa_vec,
           std::vector<double> &bb_vec,
           std::vector<double> &cc_vec)
{
#pragma omp parallel for
    for (size_t idx = 0; idx < aa_vec.size(); ++idx) {
        aa_vec[idx] = 0.0;
        aa_vec[idx] = 1.0;
        aa_vec[idx] = 2.0;
    }
}

__attribute__((noinline))
void triad_with_post(std::vector<double> &aa_vec,
                     const std::vector<double> &bb_vec,
                     const std::vector<double> &cc_vec)
{
    double scalar = 3.0;
#pragma omp parallel for
    for (size_t idx = 0; idx < aa_vec.size(); ++idx) {
        geopm_tprof_post();
        aa_vec[idx] = bb_vec[idx] + scalar * cc_vec[idx];
    }
}

__attribute__((noinline))
void triad_no_post(std::vector<double> &aa_vec,
                   const std::vector<double> &bb_vec,
                   const std::vector<double> &cc_vec)
{
    double scalar = 3.0;
#pragma omp parallel for
    for (size_t idx = 0; idx < aa_vec.size(); ++idx) {
        aa_vec[idx] = bb_vec[idx] + scalar * cc_vec[idx];
    }
}

__attribute__((noinline))
void warmup(std::vector<double> &aa_vec,
            const std::vector<double> &bb_vec,
            const std::vector<double> &cc_vec)
{
    double scalar = 3.0;
#pragma omp parallel for
    for (size_t idx = 0; idx < aa_vec.size(); ++idx) {
        aa_vec[idx] = bb_vec[idx] + scalar * cc_vec[idx];
    }
}

__attribute__((noinline))
void loop_dgemm_with_post(double big_o, int count)
{
#pragma omp parallel
{
    std::shared_ptr<geopm::ModelRegion> dgemm_model(
        geopm::ModelRegion::model_region("dgemm-unmarked", big_o, false));
#pragma omp for
    for (int idx = 0; idx < count; ++idx) {
        geopm_tprof_post();
        dgemm_model->run();
    }
}
}

__attribute__((noinline))
void loop_dgemm_no_post(double big_o, int count)
{
#pragma omp parallel
{
    std::shared_ptr<geopm::ModelRegion> dgemm_model(
        geopm::ModelRegion::model_region("dgemm-unmarked", big_o, false));
#pragma omp for
    for (int idx = 0; idx < count; ++idx) {
        dgemm_model->run();
    }
}
}

__attribute__((noinline))
void loop_dgemm_warmup(double big_o, int count)
{
#pragma omp parallel
{
    std::shared_ptr<geopm::ModelRegion> dgemm_model(
        geopm::ModelRegion::model_region("dgemm-unmarked", big_o, false));
#pragma omp for
    for (int idx = 0; idx < count; ++idx) {
        dgemm_model->run();
    }
}
}

int main(int argc, char **argv)
{
    MPI_Init(&argc, &argv);
    sleep(5);
    int vec_size = 134217728; // 1 GiB

    std::vector<double> aa_vec(vec_size);
    std::vector<double> bb_vec(vec_size);
    std::vector<double> cc_vec(vec_size);

    MPI_Barrier(MPI_COMM_WORLD);
    setup(aa_vec, bb_vec, cc_vec);

    MPI_Barrier(MPI_COMM_WORLD);
    warmup(aa_vec, bb_vec, cc_vec);

    MPI_Barrier(MPI_COMM_WORLD);
    triad_with_post(aa_vec, bb_vec, cc_vec);

    MPI_Barrier(MPI_COMM_WORLD);
    triad_no_post(aa_vec, bb_vec, cc_vec);

    MPI_Barrier(MPI_COMM_WORLD);
    loop_dgemm_warmup(0.01, 100);

    MPI_Barrier(MPI_COMM_WORLD);
    loop_dgemm_with_post(0.01, 10000);

    MPI_Barrier(MPI_COMM_WORLD);
    loop_dgemm_no_post(0.01, 10000);

    MPI_Finalize();
    return 0;
}
