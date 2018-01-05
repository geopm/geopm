/*
 * Copyright (c) 2015, 2016, 2017, 2018, Intel Corporation
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
#include "mpi.h"
#include "gtest/gtest.h"
#include "geopm.h"

#ifndef NAME_MAX
#define NAME_MAX 1024
#endif

int main(int argc, char **argv)
{
    int err = 0;
    int rank = 0;
    int comm_size = 0;

    err = MPI_Init(&argc, &argv);
    if (err) {
        std::cerr << "Error: <geopm_mpi_test>, MPI_Init() failed: " << err << std::endl;
        (void)MPI_Abort(MPI_COMM_WORLD, err);
        return err;
    }

    uint64_t startup_rid = 0;
    geopm_prof_region("geopm_mpi_test-startup", GEOPM_REGION_HINT_UNKNOWN, &startup_rid);
    MPI_Barrier(MPI_COMM_WORLD);
    geopm_prof_enter(startup_rid);

    testing::InitGoogleTest(&argc, argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &comm_size);
    char per_rank_log_name[NAME_MAX];
    char per_rank_err_name[NAME_MAX];
    snprintf(per_rank_log_name, NAME_MAX, ".geopm_mpi_test.%.3d.log", rank);
    snprintf(per_rank_err_name, NAME_MAX, ".geopm_mpi_test.%.3d.err", rank);
    int stdout_fileno_dup;
    int stderr_fileno_dup;

    stdout_fileno_dup = dup(STDOUT_FILENO);
    stderr_fileno_dup = dup(STDERR_FILENO);

    if (!freopen(per_rank_log_name, "w", stdout)) {
        perror(per_rank_log_name);
        exit(EXIT_FAILURE);
    }
    if (!freopen(per_rank_err_name, "w", stderr)) {
        perror(per_rank_err_name);
        exit(EXIT_FAILURE);
    }

    MPI_Barrier(MPI_COMM_WORLD);
    geopm_prof_exit(startup_rid);

    try {
        err = RUN_ALL_TESTS();
    }
    catch (std::exception ex) {
        err = err ? err : 1;
        std::cerr << "Error: <geopm_mpi_test> [" << rank << "] " << ex.what() << std::endl;
    }

    fflush(stdout);
    dup2(stdout_fileno_dup, STDOUT_FILENO);
    dup2(stderr_fileno_dup, STDERR_FILENO);

    MPI_Barrier(MPI_COMM_WORLD);

    if (!rank) {
        FILE *fid_in = NULL;
        int nread;
        char buffer[NAME_MAX];
        for (int i = 0; i < comm_size; ++i) {
            snprintf(per_rank_log_name, NAME_MAX, ".geopm_mpi_test.%.3d.log", i);
            fid_in = fopen(per_rank_log_name, "r");
            fprintf(stdout, "**********       Log: <geopm_mpi_test> [%.3d]      **********\n", i);
            nread = -1;
            while (fid_in && nread) {
                nread = fread(buffer, 1, NAME_MAX, fid_in);
                fwrite(buffer, 1, nread, stdout);
            }
            fprintf(stdout,     "************************************************************\n");
            fclose(fid_in);
            unlink(per_rank_log_name);

            snprintf(per_rank_err_name, NAME_MAX, ".geopm_mpi_test.%.3d.err", i);
            fid_in = fopen(per_rank_err_name, "r");
            nread = -1;

            bool is_first_print = true;
            bool is_err_empty = true;
            while (fid_in && nread) {
                nread = fread(buffer, 1, NAME_MAX, fid_in);
                if (nread && is_first_print) {
                    fprintf(stdout, "**********      Error: <geopm_mpi_test> [%.3d]     **********\n", i);
                    is_err_empty = false;
                }
                fwrite(buffer, 1, nread, stdout);
                is_first_print = false;
            }
            if (!is_err_empty) {
                fprintf(stdout,     "************************************************************\n");
            }
            fclose(fid_in);
            unlink(per_rank_err_name);
        }
        fflush(stdout);
    }

    int all_err;
    MPI_Allreduce(&err, &all_err, 1, MPI_INT, MPI_LOR, MPI_COMM_WORLD);
    if (all_err) {
        all_err = -255;
    }

    // If death testing, bypass controller since it's dead.
    char* is_death_test = getenv("GEOPM_DEATH_TESTING");
    if (!(is_death_test != NULL &&
          strncmp(is_death_test, "1", NAME_MAX) == 0)) {
        MPI_Finalize();
    }

    if (!err) {
        err = all_err;
    }
    if (!rank) {
        _exit(err);
    }
    return err;
}
