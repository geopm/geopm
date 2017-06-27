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
#include "geopm_ctl.h"
#include "geopm_env.h"
#include "Profile.hpp"
#include "SharedMemory.hpp"
#include "geopm_mpi_comm_split.h"

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
        bool m_is_root_process;
        int m_report_size;
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
    , m_is_root_process(false)
    , m_report_size(0)
    , m_check_val_default({3.0, 6.0, 9.0})
    , m_check_val_single({6.0, 0.0, 9.0})
    , m_check_val_multi({1.0, 2.0, 3.0})
{
    MPI_Comm ppn1_comm;
    int rank;
    geopm_comm_split_ppn1(MPI_COMM_WORLD, "prof", &ppn1_comm);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &m_report_size);
    if (!rank) {
        m_is_root_process = true;
        MPI_Comm_free(&ppn1_comm);
    }
    else {
        m_is_root_process = false;
    }
}

MPIProfileTest::~MPIProfileTest()
{
    MPI_Barrier(MPI_COMM_WORLD);
    if (m_is_root_process) {
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

void MPIProfileTest::parse_log(const std::vector<double> &check_val)
{
    ASSERT_EQ(3ULL, check_val.size());
    int err = geopm_prof_shutdown();
    ASSERT_EQ(0, err);
    sleep(1); // Wait for controller to finish writing the report

    if (m_is_root_process) {
        std::string line;
        std::vector<double> curr_value(m_report_size, -1.0);
        std::vector<double> value(m_report_size, 0.0);
        std::vector<double> epoch_value(m_report_size, 0.0);
        std::vector<double> startup_value(m_report_size, 0.0);
        std::vector<double> total_runtime_value(m_report_size, 0.0);

        std::ifstream log(m_log_file_node, std::ios_base::in);

        ASSERT_TRUE(log.is_open());

        int hostnum = -1;
        while(std::getline(log, line)) {
            if (hostnum >= 0) {
                curr_value[hostnum] = -1.0;
            }
            if (line.find("Region loop_one") == 0 && hostnum >= 0) {
                curr_value[hostnum] = check_val[0];
            }
            else if (line.find("Region loop_two") == 0 && hostnum >= 0) {
                curr_value[hostnum] = check_val[1];
            }
            else if (line.find("Region loop_three") == 0 && hostnum >= 0) {
                curr_value[hostnum] = check_val[2];
            }
            else if (line.find("Region epoch") == 0 && hostnum >= 0) {
                std::getline(log, line);
                ASSERT_NE(0, sscanf(line.c_str(), "\truntime (sec): %lf", &epoch_value[hostnum]));
            }
            else if (line.find("Region geopm_mpi_test-startup:") == 0 && hostnum >= 0) {
                std::getline(log, line);
                ASSERT_NE(0, sscanf(line.c_str(), "\truntime (sec): %lf", &startup_value[hostnum]));
            }
            else if (line.find("Application Totals:") == 0 && hostnum >= 0) {
                std::getline(log, line);
                ASSERT_NE(0, sscanf(line.c_str(), "\truntime (sec): %lf", &total_runtime_value[hostnum]));
            }
            if (hostnum >= 0 && curr_value[hostnum] != -1.0) {
                std::getline(log, line);
                ASSERT_NE(0, sscanf(line.c_str(), "\truntime (sec): %lf", &value[hostnum]));
                ASSERT_NEAR(value[hostnum], curr_value[hostnum], m_epsilon);
            }
            if (line.find("Host:") == 0) {
                ++hostnum;
            }
        }

        for(hostnum = 0; hostnum < m_report_size; ++hostnum) {
            if (epoch_value[hostnum] != 0.0) {
                double epoch_target = std::accumulate(check_val.begin(), check_val.end(), 0.0);
                ASSERT_NEAR(epoch_target, epoch_value[hostnum], m_epsilon);
                double total_runtime_target = std::accumulate(check_val.begin(), check_val.end(), startup_value[hostnum]);
                ASSERT_LT(total_runtime_target, total_runtime_value[hostnum]);
                /// @todo: The assert below may fail because of unaccounted for time reported (~1 second)
                /// ASSERT_NEAR(total_runtime_target, total_runtime_value, m_epsilon);
            }
        }

        log.close();
    }
}

TEST_F(MPIProfileTest, runtime)
{
    uint64_t region_id[3];
    struct geopm_time_s start, curr;
    double timeout = 0.0;
    int rank;

    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    ASSERT_EQ(0, geopm_prof_region("loop_one", GEOPM_REGION_HINT_UNKNOWN, &region_id[0]));
    ASSERT_EQ(0, geopm_prof_enter(region_id[0]));
    ASSERT_EQ(0, geopm_time(&start));
    while (timeout < 1.0) {
        ASSERT_EQ(0, geopm_time(&curr));
        timeout = geopm_time_diff(&start, &curr);
    }
    ASSERT_EQ(0, geopm_prof_exit(region_id[0]));

    timeout = 0.0;
    ASSERT_EQ(0, geopm_prof_region("loop_two", GEOPM_REGION_HINT_UNKNOWN, &region_id[1]));
    ASSERT_EQ(0, geopm_prof_enter(region_id[1]));
    ASSERT_EQ(0, geopm_time(&start));
    while (timeout < 2.0) {
        ASSERT_EQ(0, geopm_time(&curr));
        timeout = geopm_time_diff(&start, &curr);
    }
    ASSERT_EQ(0, geopm_prof_exit(region_id[1]));

    timeout = 0.0;
    ASSERT_EQ(0, geopm_prof_region("loop_three", GEOPM_REGION_HINT_UNKNOWN, &region_id[2]));
    ASSERT_EQ(0, geopm_prof_enter(region_id[2]));
    ASSERT_EQ(0, geopm_time(&start));
    while (timeout < 3.0) {
        ASSERT_EQ(0, geopm_time(&curr));
        timeout = geopm_time_diff(&start, &curr);
    }
    ASSERT_EQ(0, geopm_prof_exit(region_id[2]));

    parse_log(m_check_val_multi);
}

TEST_F(MPIProfileTest, progress)
{
    uint64_t region_id[3];
    struct geopm_time_s start, curr;
    double timeout = 0.0;
    int rank;

    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    ASSERT_EQ(0, geopm_prof_region("loop_one", GEOPM_REGION_HINT_UNKNOWN, &region_id[0]));
    ASSERT_EQ(0, geopm_prof_enter(region_id[0]));
    ASSERT_EQ(0, geopm_time(&start));
    while (timeout < 1.0) {
        ASSERT_EQ(0, geopm_time(&curr));
        timeout = geopm_time_diff(&start, &curr);
        geopm_prof_progress(region_id[0], timeout/1.0);
    }
    ASSERT_EQ(0, geopm_prof_exit(region_id[0]));

    timeout = 0.0;
    ASSERT_EQ(0, geopm_prof_region("loop_two", GEOPM_REGION_HINT_UNKNOWN, &region_id[1]));
    ASSERT_EQ(0, geopm_prof_enter(region_id[1]));
    ASSERT_EQ(0, geopm_time(&start));
    while (timeout < 2.0) {
        ASSERT_EQ(0, geopm_time(&curr));
        timeout = geopm_time_diff(&start, &curr);
        geopm_prof_progress(region_id[1], timeout/2.0);
    }
    ASSERT_EQ(0, geopm_prof_exit(region_id[1]));

    timeout = 0.0;
    ASSERT_EQ(0, geopm_prof_region("loop_three", GEOPM_REGION_HINT_UNKNOWN, &region_id[2]));
    ASSERT_EQ(0, geopm_prof_enter(region_id[2]));
    ASSERT_EQ(0, geopm_time(&start));
    while (timeout < 3.0) {
        ASSERT_EQ(0, geopm_time(&curr));
        timeout = geopm_time_diff(&start, &curr);
        geopm_prof_progress(region_id[2], timeout/3.0);
    }
    ASSERT_EQ(0, geopm_prof_exit(region_id[2]));

    parse_log(m_check_val_multi);
}

TEST_F(MPIProfileTest, multiple_entries)
{
    uint64_t region_id[2];
    struct geopm_time_s start, curr;
    double timeout = 0.0;
    int rank;

    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    ASSERT_EQ(0, geopm_prof_region("loop_one", GEOPM_REGION_HINT_UNKNOWN, &region_id[0]));
    ASSERT_EQ(0, geopm_prof_enter(region_id[0]));
    ASSERT_EQ(0, geopm_time(&start));
    while (timeout < 1.0) {
        ASSERT_EQ(0, geopm_time(&curr));
        timeout = geopm_time_diff(&start, &curr);
        geopm_prof_progress(region_id[0], timeout/1.0);
    }
    ASSERT_EQ(0, geopm_prof_exit(region_id[0]));

    timeout = 0.0;
    ASSERT_EQ(0, geopm_prof_region("loop_three", GEOPM_REGION_HINT_UNKNOWN, &region_id[1]));
    ASSERT_EQ(0, geopm_prof_enter(region_id[1]));
    ASSERT_EQ(0, geopm_time(&start));
    while (timeout < 3.0) {
        ASSERT_EQ(0, geopm_time(&curr));
        timeout = geopm_time_diff(&start, &curr);
        geopm_prof_progress(region_id[1], timeout/3.0);
    }
    ASSERT_EQ(0, geopm_prof_exit(region_id[1]));

    timeout = 0.0;
    ASSERT_EQ(0, geopm_prof_region("loop_one", GEOPM_REGION_HINT_UNKNOWN, &region_id[0]));
    ASSERT_EQ(0, geopm_prof_enter(region_id[0]));
    ASSERT_EQ(0, geopm_time(&start));
    while (timeout < 2.0) {
        ASSERT_EQ(0, geopm_time(&curr));
        timeout = geopm_time_diff(&start, &curr);
        geopm_prof_progress(region_id[0], timeout/2.0);
    }
    ASSERT_EQ(0, geopm_prof_exit(region_id[0]));

    timeout = 0.0;
    ASSERT_EQ(0, geopm_prof_region("loop_three", GEOPM_REGION_HINT_UNKNOWN, &region_id[1]));
    ASSERT_EQ(0, geopm_prof_enter(region_id[1]));
    ASSERT_EQ(0, geopm_time(&start));
    while (timeout < 3.0) {
        ASSERT_EQ(0, geopm_time(&curr));
        timeout = geopm_time_diff(&start, &curr);
        geopm_prof_progress(region_id[1], timeout/3.0);
    }
    ASSERT_EQ(0, geopm_prof_exit(region_id[1]));

    timeout = 0.0;
    ASSERT_EQ(0, geopm_prof_region("loop_one", GEOPM_REGION_HINT_UNKNOWN, &region_id[0]));
    ASSERT_EQ(0, geopm_prof_enter(region_id[0]));
    ASSERT_EQ(0, geopm_time(&start));
    while (timeout < 3.0) {
        ASSERT_EQ(0, geopm_time(&curr));
        timeout = geopm_time_diff(&start, &curr);
        geopm_prof_progress(region_id[0], timeout/3.0);
    }
    ASSERT_EQ(0, geopm_prof_exit(region_id[0]));

    timeout = 0.0;
    ASSERT_EQ(0, geopm_prof_region("loop_three", GEOPM_REGION_HINT_UNKNOWN, &region_id[1]));
    ASSERT_EQ(0, geopm_prof_enter(region_id[1]));
    ASSERT_EQ(0, geopm_time(&start));
    while (timeout < 3.0) {
        ASSERT_EQ(0, geopm_time(&curr));
        timeout = geopm_time_diff(&start, &curr);
        geopm_prof_progress(region_id[1], timeout/3.0);
    }
    ASSERT_EQ(0, geopm_prof_exit(region_id[1]));

    parse_log(m_check_val_single);
}

TEST_F(MPIProfileTest, nested_region)
{
    uint64_t region_id[3];
    struct geopm_time_s start, curr;
    double timeout = 0.0;
    int rank;

    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    ASSERT_EQ(0, geopm_prof_region("loop_one", GEOPM_REGION_HINT_UNKNOWN, &region_id[0]));
    ASSERT_EQ(0, geopm_prof_enter(region_id[0]));
    ASSERT_EQ(0, geopm_prof_region("loop_two", GEOPM_REGION_HINT_UNKNOWN, &region_id[1]));
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
    ASSERT_EQ(0, geopm_prof_region("loop_three", GEOPM_REGION_HINT_UNKNOWN, &region_id[2]));
    ASSERT_EQ(0, geopm_prof_enter(region_id[2]));
    ASSERT_EQ(0, geopm_prof_region("loop_two", GEOPM_REGION_HINT_UNKNOWN, &region_id[1]));
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
    ASSERT_EQ(0, geopm_prof_region("loop_one", GEOPM_REGION_HINT_UNKNOWN, &region_id[0]));
    ASSERT_EQ(0, geopm_prof_enter(region_id[0]));
    ASSERT_EQ(0, geopm_prof_region("loop_two", GEOPM_REGION_HINT_UNKNOWN, &region_id[1]));
    ASSERT_EQ(0, geopm_prof_enter(region_id[1]));
    ASSERT_EQ(0, geopm_time(&start));
    while (timeout < 3.0) {
        ASSERT_EQ(0, geopm_time(&curr));
        timeout = geopm_time_diff(&start, &curr);
        geopm_prof_progress(region_id[1], timeout/1.0);
    }
    ASSERT_EQ(0, geopm_prof_exit(region_id[1]));
    ASSERT_EQ(0, geopm_prof_exit(region_id[0]));

    parse_log(m_check_val_single);
}

TEST_F(MPIProfileTest, epoch)
{
    uint64_t region_id[4];
    int rank;

    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    for (int i = 0; i < 3; i++) {
        ASSERT_EQ(0, geopm_prof_epoch());

        ASSERT_EQ(0, geopm_prof_region("loop_one", GEOPM_REGION_HINT_UNKNOWN, &region_id[0]));
        ASSERT_EQ(0, geopm_prof_enter(region_id[0]));
        sleep_exact(1.0);
        ASSERT_EQ(0, geopm_prof_exit(region_id[0]));

        ASSERT_EQ(0, geopm_prof_region("loop_two", GEOPM_REGION_HINT_UNKNOWN, &region_id[1]));
        ASSERT_EQ(0, geopm_prof_enter(region_id[1]));
        sleep_exact(2.0);
        ASSERT_EQ(0, geopm_prof_exit(region_id[1]));

        ASSERT_EQ(0, geopm_prof_region("loop_three", GEOPM_REGION_HINT_UNKNOWN, &region_id[2]));
        ASSERT_EQ(0, geopm_prof_enter(region_id[2]));
        sleep_exact(3.0);
        ASSERT_EQ(0, geopm_prof_exit(region_id[2]));

        MPI_Barrier(MPI_COMM_WORLD);
    }

    parse_log(m_check_val_default);
}

TEST_F(MPIProfileTest, noctl)
{
    uint64_t region_id[3];
    struct geopm_time_s start, curr;
    double timeout = 0.0;
    int rank;

    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    ASSERT_EQ(0, geopm_prof_region("loop_one", GEOPM_REGION_HINT_UNKNOWN, &region_id[0]));
    ASSERT_EQ(0, geopm_prof_enter(region_id[0]));
    ASSERT_EQ(0, geopm_time(&start));
    while (timeout < 1.0) {
        ASSERT_EQ(0, geopm_time(&curr));
        timeout = geopm_time_diff(&start, &curr);
    }
    ASSERT_EQ(0, geopm_prof_exit(region_id[0]));

    timeout = 0.0;
    ASSERT_EQ(0, geopm_prof_region("loop_two", GEOPM_REGION_HINT_UNKNOWN, &region_id[1]));
    ASSERT_EQ(0, geopm_prof_enter(region_id[1]));
    ASSERT_EQ(0, geopm_time(&start));
    while (timeout < 2.0) {
        ASSERT_EQ(0, geopm_time(&curr));
        timeout = geopm_time_diff(&start, &curr);
    }
    ASSERT_EQ(0, geopm_prof_exit(region_id[1]));

    timeout = 0.0;
    ASSERT_EQ(0, geopm_prof_region("loop_three", GEOPM_REGION_HINT_UNKNOWN, &region_id[2]));
    ASSERT_EQ(0, geopm_prof_enter(region_id[2]));
    ASSERT_EQ(0, geopm_time(&start));
    while (timeout < 3.0) {
        ASSERT_EQ(0, geopm_time(&curr));
        timeout = geopm_time_diff(&start, &curr);
    }
    ASSERT_EQ(0, geopm_prof_exit(region_id[2]));

}
