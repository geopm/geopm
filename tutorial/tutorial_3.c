/*
 * Copyright (c) 2015 - 2021, Intel Corporation
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

#include <stdlib.h>
#include <stdio.h>
#include <mpi.h>
#include <geopm.h>

#include "tutorial_region.h"


int main(int argc, char **argv)
{
    int size = 0;
    int rank = 0;

    int err = MPI_Init(&argc, &argv);
    if (!err) {
        err = MPI_Comm_size(MPI_COMM_WORLD, &size);
    }
    if (!err) {
        err = MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    }
    if (!err && !rank) {
        printf("MPI_COMM_WORLD size: %d\n", size);
    }

    uint64_t dgemm_rid;
    if (!err) {
        err = geopm_prof_region("tutorial_dgemm",
                                GEOPM_REGION_HINT_COMPUTE,
                                &dgemm_rid);
    }

    int num_iter = 500;
    double dgemm_big_o = 8.0;
    if (rank < size / 2) {
        dgemm_big_o *= 1.1;
    }

    if (!rank) {
        printf("Beginning loop of %d iterations.\n", num_iter);
        fflush(stdout);
    }
    for (int i = 0; !err && i < num_iter; ++i) {
        err = geopm_prof_epoch();
        if (!err) {
            err = geopm_prof_enter(dgemm_rid);
        }
        if (!err) {
            err = tutorial_dgemm_static(dgemm_big_o, 0);
        }
        if (!err) {
            err = geopm_prof_exit(dgemm_rid);
        }
        if (!err) {
            err = MPI_Barrier(MPI_COMM_WORLD);
        }
        if (!err && !rank) {
            printf("Iteration=%.3d\r", i);
            fflush(stdout);
        }
    }
    if (!err && !rank) {
        printf("Completed loop.                    \n");
        fflush(stdout);
    }
    if (!err) {
        err = tutorial_dgemm_static(0.0, 0);
    }

    int err_fin = MPI_Finalize();
    err = err ? err : err_fin;

    return err;
}
