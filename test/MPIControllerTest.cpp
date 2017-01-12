/*
 * Copyright (c) 2015, 2016, 2017, Intel Corporation
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

#include <mpi.h>
#include <unistd.h>

#include "gtest/gtest.h"
#include "Controller.hpp"
#include "geopm.h"

#ifndef NAME_MAX
#define NAME_MAX 256
#endif

class MPIControllerTest: public :: testing :: Test {};

TEST_F(MPIControllerTest, hello)
{
    FILE *fid;
    int rank;
    std::vector<int> factor(2);
    std::string control;
    std::string report;
    factor[0] = 4;
    factor[1] = 4;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    if (!rank) {
        control = "/tmp/MPIControllerTest.hello.control";
        report =  "/tmp/MPIControllerTest.hello.report";
        fid = fopen(control.c_str(), "w");
        EXPECT_TRUE(fid != NULL);
        fprintf(fid, "goal:performance\n");
        fprintf(fid, "mode:dynamic_power\n");
        fprintf(fid, "power_budget:1.0\n");
        fclose(fid);
    }
    geopm::Controller hello_ctl(factor, control, report, MPI_COMM_WORLD);
    hello_ctl.run();
    if (!rank) {
        unlink(control.c_str());
    }
}

TEST_F(MPIControllerTest, geopm_ctl_run)
{
    FILE *fid;
    int rank;
    int num_factor = 2;
    const int factor[2] = {4,4};
    char control[NAME_MAX] = {0};
    char report[NAME_MAX] = {0};

    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    if (!rank) {
        strncpy(control, "/tmp/MPIControllerTest.geopm_ctl_run.control", NAME_MAX - 1);
        strncpy(report, "/tmp/MPIControllerTest.geopm_ctl_run.report", NAME_MAX - 1);
        fid = fopen(control, "w");
        EXPECT_TRUE(fid != NULL);
        fprintf(fid, "goal:efficency\n");
        fprintf(fid, "mode:dynamic_power\n");
        fprintf(fid, "power_budget:1.0\n");
        fclose(fid);
    }

    EXPECT_EQ(0, geopm_ctl_run(num_factor, factor, control, report, MPI_COMM_WORLD));

    if (!rank) {
        unlink(control);
    }
}
