/*
 * Copyright (c) 2015, 2016, Intel Corporation
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
#include <limits.h>
#include <unistd.h>
#include <fcntl.h>
#include <math.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fstream>
#include <numeric>

#include "gtest/gtest.h"
#include "geopm.h"
#include "geopm_env.h"
#include "Profile.hpp"
#include "SharedMemory.hpp"

class MPIProfileTest: public :: testing :: Test
{
    public:
        MPIProfileTest();
        virtual ~MPIProfileTest();
        void parse_log(const std::vector<double> &check_val);
        void sleep_exact(double duration);
    protected:
        size_t m_table_size;
        double m_epsilon;
        bool m_use_std_sleep;
        std::string m_log_file;
        std::string m_log_file_node;
        bool m_is_node_root;
        std::vector<double> m_check_val_default;
        std::vector<double> m_check_val_single;
        std::vector<double> m_check_val_multi;
};

MPIProfileTest::MPIProfileTest()
    : m_table_size(4096)
    , m_epsilon(0.5)
    , m_use_std_sleep(false)
    , m_log_file(geopm_env_report())
    , m_log_file_node(m_log_file)
    , m_is_node_root(false)
    , m_check_val_default({3.0, 6.0, 9.0})
    , m_check_val_single({6.0, 0.0, 9.0})
    , m_check_val_multi({1.0, 2.0, 3.0})
{
    char hostname[NAME_MAX];
    MPI_Comm ppn1_comm;
    gethostname(hostname, NAME_MAX);
    m_log_file_node.append("-");
    m_log_file_node.append(hostname);

    geopm_comm_split_ppn1(MPI_COMM_WORLD, "prof_test", &ppn1_comm);
    m_is_node_root = ppn1_comm != MPI_COMM_NULL;
}

MPIProfileTest::~MPIProfileTest()
{
    MPI_Barrier(MPI_COMM_WORLD);
    if (m_is_node_root) {
        remove(m_log_file_node.c_str());
    }

}

void MPIProfileTest::sleep_exact(double duration)
{
    if (m_use_std_sleep) {
        sleep(duration);
    }
    else {
        struct geopm_time_s start;
        geopm_time(&start);

        struct geopm_time_s curr;
        double timeout = 0.0;
        while (timeout < duration) {
            geopm_time(&curr);
            timeout = geopm_time_diff(&start, &curr);
        }
    }
}

TEST_F(MPIProfileTest, runtime)
{
    uint64_t region_id[3];
    struct geopm_time_s start, curr;
    double timeout = 0.0;
    int rank;

    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    ASSERT_EQ(0, geopm_prof_region("loop_one", GEOPM_POLICY_HINT_UNKNOWN, &region_id[0]));
    ASSERT_EQ(0, geopm_prof_enter(region_id[0]));
    ASSERT_EQ(0, geopm_time(&start));
    while (timeout < 1.0) {
        ASSERT_EQ(0, geopm_time(&curr));
        timeout = geopm_time_diff(&start, &curr);
    }
    ASSERT_EQ(0, geopm_prof_exit(region_id[0]));

    timeout = 0.0;
    ASSERT_EQ(0, geopm_prof_region("loop_two", GEOPM_POLICY_HINT_UNKNOWN, &region_id[1]));
    ASSERT_EQ(0, geopm_prof_enter(region_id[1]));
    ASSERT_EQ(0, geopm_time(&start));
    while (timeout < 2.0) {
        ASSERT_EQ(0, geopm_time(&curr));
        timeout = geopm_time_diff(&start, &curr);
    }
    ASSERT_EQ(0, geopm_prof_exit(region_id[1]));

    timeout = 0.0;
    ASSERT_EQ(0, geopm_prof_region("loop_three", GEOPM_POLICY_HINT_UNKNOWN, &region_id[2]));
    ASSERT_EQ(0, geopm_prof_enter(region_id[2]));
    ASSERT_EQ(0, geopm_time(&start));
    while (timeout < 3.0) {
        ASSERT_EQ(0, geopm_time(&curr));
        timeout = geopm_time_diff(&start, &curr);
    }
    ASSERT_EQ(0, geopm_prof_exit(region_id[2]));

}

TEST_F(MPIProfileTest, progress)
{
    uint64_t region_id[3];
    struct geopm_time_s start, curr;
    double timeout = 0.0;
    int rank;

    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    ASSERT_EQ(0, geopm_prof_region("loop_one", GEOPM_POLICY_HINT_UNKNOWN, &region_id[0]));
    ASSERT_EQ(0, geopm_prof_enter(region_id[0]));
    ASSERT_EQ(0, geopm_time(&start));
    while (timeout < 1.0) {
        ASSERT_EQ(0, geopm_time(&curr));
        timeout = geopm_time_diff(&start, &curr);
        geopm_prof_progress(region_id[0], timeout/1.0);
    }
    ASSERT_EQ(0, geopm_prof_exit(region_id[0]));

    timeout = 0.0;
    ASSERT_EQ(0, geopm_prof_region("loop_two", GEOPM_POLICY_HINT_UNKNOWN, &region_id[1]));
    ASSERT_EQ(0, geopm_prof_enter(region_id[1]));
    ASSERT_EQ(0, geopm_time(&start));
    while (timeout < 2.0) {
        ASSERT_EQ(0, geopm_time(&curr));
        timeout = geopm_time_diff(&start, &curr);
        geopm_prof_progress(region_id[1], timeout/2.0);
    }
    ASSERT_EQ(0, geopm_prof_exit(region_id[1]));

    timeout = 0.0;
    ASSERT_EQ(0, geopm_prof_region("loop_three", GEOPM_POLICY_HINT_UNKNOWN, &region_id[2]));
    ASSERT_EQ(0, geopm_prof_enter(region_id[2]));
    ASSERT_EQ(0, geopm_time(&start));
    while (timeout < 3.0) {
        ASSERT_EQ(0, geopm_time(&curr));
        timeout = geopm_time_diff(&start, &curr);
        geopm_prof_progress(region_id[2], timeout/3.0);
    }
    ASSERT_EQ(0, geopm_prof_exit(region_id[2]));
}

TEST_F(MPIProfileTest, multiple_entries)
{
    uint64_t region_id[2];
    struct geopm_time_s start, curr;
    double timeout = 0.0;
    int rank;

    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    ASSERT_EQ(0, geopm_prof_region("loop_one", GEOPM_POLICY_HINT_UNKNOWN, &region_id[0]));
    ASSERT_EQ(0, geopm_prof_enter(region_id[0]));
    ASSERT_EQ(0, geopm_time(&start));
    while (timeout < 1.0) {
        ASSERT_EQ(0, geopm_time(&curr));
        timeout = geopm_time_diff(&start, &curr);
        geopm_prof_progress(region_id[0], timeout/1.0);
    }
    ASSERT_EQ(0, geopm_prof_exit(region_id[0]));

    timeout = 0.0;
    ASSERT_EQ(0, geopm_prof_region("loop_three", GEOPM_POLICY_HINT_UNKNOWN, &region_id[1]));
    ASSERT_EQ(0, geopm_prof_enter(region_id[1]));
    ASSERT_EQ(0, geopm_time(&start));
    while (timeout < 3.0) {
        ASSERT_EQ(0, geopm_time(&curr));
        timeout = geopm_time_diff(&start, &curr);
        geopm_prof_progress(region_id[1], timeout/3.0);
    }
    ASSERT_EQ(0, geopm_prof_exit(region_id[1]));

    timeout = 0.0;
    ASSERT_EQ(0, geopm_prof_region("loop_one", GEOPM_POLICY_HINT_UNKNOWN, &region_id[0]));
    ASSERT_EQ(0, geopm_prof_enter(region_id[0]));
    ASSERT_EQ(0, geopm_time(&start));
    while (timeout < 2.0) {
        ASSERT_EQ(0, geopm_time(&curr));
        timeout = geopm_time_diff(&start, &curr);
        geopm_prof_progress(region_id[0], timeout/2.0);
    }
    ASSERT_EQ(0, geopm_prof_exit(region_id[0]));

    timeout = 0.0;
    ASSERT_EQ(0, geopm_prof_region("loop_three", GEOPM_POLICY_HINT_UNKNOWN, &region_id[1]));
    ASSERT_EQ(0, geopm_prof_enter(region_id[1]));
    ASSERT_EQ(0, geopm_time(&start));
    while (timeout < 3.0) {
        ASSERT_EQ(0, geopm_time(&curr));
        timeout = geopm_time_diff(&start, &curr);
        geopm_prof_progress(region_id[1], timeout/3.0);
    }
    ASSERT_EQ(0, geopm_prof_exit(region_id[1]));

    timeout = 0.0;
    ASSERT_EQ(0, geopm_prof_region("loop_one", GEOPM_POLICY_HINT_UNKNOWN, &region_id[0]));
    ASSERT_EQ(0, geopm_prof_enter(region_id[0]));
    ASSERT_EQ(0, geopm_time(&start));
    while (timeout < 3.0) {
        ASSERT_EQ(0, geopm_time(&curr));
        timeout = geopm_time_diff(&start, &curr);
        geopm_prof_progress(region_id[0], timeout/3.0);
    }
    ASSERT_EQ(0, geopm_prof_exit(region_id[0]));

    timeout = 0.0;
    ASSERT_EQ(0, geopm_prof_region("loop_three", GEOPM_POLICY_HINT_UNKNOWN, &region_id[1]));
    ASSERT_EQ(0, geopm_prof_enter(region_id[1]));
    ASSERT_EQ(0, geopm_time(&start));
    while (timeout < 3.0) {
        ASSERT_EQ(0, geopm_time(&curr));
        timeout = geopm_time_diff(&start, &curr);
        geopm_prof_progress(region_id[1], timeout/3.0);
    }
    ASSERT_EQ(0, geopm_prof_exit(region_id[1]));
}

TEST_F(MPIProfileTest, nested_region)
{
    uint64_t region_id[3];
    struct geopm_time_s start, curr;
    double timeout = 0.0;
    int rank;

    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    ASSERT_EQ(0, geopm_prof_region("loop_one", GEOPM_POLICY_HINT_UNKNOWN, &region_id[0]));
    ASSERT_EQ(0, geopm_prof_enter(region_id[0]));
    ASSERT_EQ(0, geopm_prof_region("loop_two", GEOPM_POLICY_HINT_UNKNOWN, &region_id[1]));
    ASSERT_EQ(0, geopm_prof_enter(region_id[1]));
    ASSERT_EQ(0, geopm_time(&start));
    while (timeout < 3.0) {
        ASSERT_EQ(0, geopm_time(&curr));
        timeout = geopm_time_diff(&start, &curr);
        geopm_prof_progress(region_id[1], timeout/1.0);
    }
    ASSERT_EQ(0, geopm_prof_exit(region_id[1]));
    ASSERT_EQ(0, geopm_prof_exit(region_id[0]));

    timeout = 0.0;
    ASSERT_EQ(0, geopm_prof_region("loop_three", GEOPM_POLICY_HINT_UNKNOWN, &region_id[2]));
    ASSERT_EQ(0, geopm_prof_enter(region_id[2]));
    ASSERT_EQ(0, geopm_prof_region("loop_two", GEOPM_POLICY_HINT_UNKNOWN, &region_id[1]));
    ASSERT_EQ(0, geopm_prof_enter(region_id[1]));
    ASSERT_EQ(0, geopm_time(&start));
    while (timeout < 9.0) {
        ASSERT_EQ(0, geopm_time(&curr));
        timeout = geopm_time_diff(&start, &curr);
        geopm_prof_progress(region_id[1], timeout/1.0);
    }
    ASSERT_EQ(0, geopm_prof_exit(region_id[1]));
    ASSERT_EQ(0, geopm_prof_exit(region_id[2]));

    timeout = 0.0;
    ASSERT_EQ(0, geopm_prof_region("loop_one", GEOPM_POLICY_HINT_UNKNOWN, &region_id[0]));
    ASSERT_EQ(0, geopm_prof_enter(region_id[0]));
    ASSERT_EQ(0, geopm_prof_region("loop_two", GEOPM_POLICY_HINT_UNKNOWN, &region_id[1]));
    ASSERT_EQ(0, geopm_prof_enter(region_id[1]));
    ASSERT_EQ(0, geopm_time(&start));
    while (timeout < 3.0) {
        ASSERT_EQ(0, geopm_time(&curr));
        timeout = geopm_time_diff(&start, &curr);
        geopm_prof_progress(region_id[1], timeout/1.0);
    }
    ASSERT_EQ(0, geopm_prof_exit(region_id[1]));
    ASSERT_EQ(0, geopm_prof_exit(region_id[0]));
}

TEST_F(MPIProfileTest, outer_sync)
{
    uint64_t region_id[4];
    int rank;

    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    for (int i = 0; i < 3; i++) {
        ASSERT_EQ(0, geopm_prof_outer_sync());

        ASSERT_EQ(0, geopm_prof_region("loop_one", GEOPM_POLICY_HINT_UNKNOWN, &region_id[0]));
        ASSERT_EQ(0, geopm_prof_enter(region_id[0]));
        sleep_exact(1.0);
        ASSERT_EQ(0, geopm_prof_exit(region_id[0]));

        ASSERT_EQ(0, geopm_prof_region("loop_two", GEOPM_POLICY_HINT_UNKNOWN, &region_id[1]));
        ASSERT_EQ(0, geopm_prof_enter(region_id[1]));
        sleep_exact(2.0);
        ASSERT_EQ(0, geopm_prof_exit(region_id[1]));

        ASSERT_EQ(0, geopm_prof_region("loop_three", GEOPM_POLICY_HINT_UNKNOWN, &region_id[2]));
        ASSERT_EQ(0, geopm_prof_enter(region_id[2]));
        sleep_exact(3.0);
        ASSERT_EQ(0, geopm_prof_exit(region_id[2]));

        MPI_Barrier(MPI_COMM_WORLD);
    }
}

TEST_F(MPIProfileTest, noctl)
{
    uint64_t region_id[3];
    struct geopm_time_s start, curr;
    double timeout = 0.0;
    int rank;

    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    ASSERT_EQ(0, geopm_prof_region("loop_one", GEOPM_POLICY_HINT_UNKNOWN, &region_id[0]));
    ASSERT_EQ(0, geopm_prof_enter(region_id[0]));
    ASSERT_EQ(0, geopm_time(&start));
    while (timeout < 1.0) {
        ASSERT_EQ(0, geopm_time(&curr));
        timeout = geopm_time_diff(&start, &curr);
    }
    ASSERT_EQ(0, geopm_prof_exit(region_id[0]));

    timeout = 0.0;
    ASSERT_EQ(0, geopm_prof_region("loop_two", GEOPM_POLICY_HINT_UNKNOWN, &region_id[1]));
    ASSERT_EQ(0, geopm_prof_enter(region_id[1]));
    ASSERT_EQ(0, geopm_time(&start));
    while (timeout < 2.0) {
        ASSERT_EQ(0, geopm_time(&curr));
        timeout = geopm_time_diff(&start, &curr);
    }
    ASSERT_EQ(0, geopm_prof_exit(region_id[1]));

    timeout = 0.0;
    ASSERT_EQ(0, geopm_prof_region("loop_three", GEOPM_POLICY_HINT_UNKNOWN, &region_id[2]));
    ASSERT_EQ(0, geopm_prof_enter(region_id[2]));
    ASSERT_EQ(0, geopm_time(&start));
    while (timeout < 3.0) {
        ASSERT_EQ(0, geopm_time(&curr));
        timeout = geopm_time_diff(&start, &curr);
    }
    ASSERT_EQ(0, geopm_prof_exit(region_id[2]));

}
