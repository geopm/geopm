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

#include "gtest/gtest.h"
#include "geopm.h"
#include "geopm_policy.h"
#include "Profile.hpp"
#include "SharedMemory.hpp"

class MPIProfileTest: public :: testing :: Test
{
    public:
        MPIProfileTest();
        virtual ~MPIProfileTest();
        int parse_log(bool single);
        int parse_log_loop();
        void sleep_exact(double duration);
    protected:
        size_t m_table_size;
        const char *m_shm_key;
        char *m_env_orig;
        double m_epsilon;
        bool m_use_std_sleep;
        std::string m_log_file;
        std::string m_log_file_node;
        std::string m_policy_file;
};

MPIProfileTest::MPIProfileTest()
    : m_table_size(GEOPM_CONST_SHMEM_REGION_SIZE)
    , m_shm_key("/geopm_profile_test")
    , m_env_orig(getenv("GEOPM_ERROR_AFFINITY_IGNORE"))
    , m_epsilon(1E-2)
    , m_use_std_sleep(false)
    , m_log_file("MPIProfileTest_log")
    , m_log_file_node(m_log_file)
    , m_policy_file("MPIProfileTest_policy")
{
    char hostname[NAME_MAX];
    gethostname(hostname, NAME_MAX);
    m_log_file_node.append("_");
    m_log_file_node.append(hostname);

    geopm_policy_c *policy = NULL;
    shm_unlink(m_shm_key);
    for (int i = 0; i < 16; i++) {
        std::string cleanup(std::string(m_shm_key) + "_" + std::to_string(i));
        shm_unlink(cleanup.c_str());
    }
    setenv("GEOPM_ERROR_AFFINITY_IGNORE", "true", 1);
    EXPECT_EQ(0, geopm_policy_create("", m_policy_file.c_str(), &policy));
    EXPECT_EQ(0, geopm_policy_mode(policy, GEOPM_MODE_PERF_BALANCE_DYNAMIC));
    EXPECT_EQ(0, geopm_policy_power(policy, 2000));
    EXPECT_EQ(0, geopm_policy_write(policy));
    EXPECT_EQ(0, geopm_policy_destroy(policy));
}

MPIProfileTest::~MPIProfileTest()
{
    if (m_env_orig) {
        setenv("GEOPM_ERROR_AFFINITY_IGNORE", m_env_orig, 1);
    }
    else {
        unsetenv("GEOPM_ERROR_AFFINITY_IGNORE");
    }
    remove(m_policy_file.c_str());
    remove(m_log_file_node.c_str());
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

int MPIProfileTest::parse_log(bool single)
{
    int err = 0;
    std::string line;
    double checkval = 0.0;
    double value = 0.0;
    std::ifstream log(m_log_file_node, std::ios_base::in);

    while(err == 0 && std::getline(log, line)) {
        checkval = -1.0;
        if (line.find("Region loop_one:") == 0) {
            if (single) {
                checkval = 6.0;
            }
            else {
                checkval = 1.0;
            }
        }
        else if (line.find("Region loop_two:") == 0) {
            if (single) {
                checkval = 0.0;
            }
            else {
                checkval = 2.0;
            }
        }
        else if (line.find("Region loop_three:") == 0) {
            checkval = 3.0;
        }
        if (checkval != -1.0) {
            err = !std::getline(log, line);
            if (!err) {
                err = !sscanf(line.c_str(), "        runtime: %lf", &value);
            }
            if (!err) {
                err = fabs(checkval - value) > m_epsilon;
            }
        }
    }
    log.close();
    return err;
}

int MPIProfileTest::parse_log_loop(void)
{
    int err = 0;
    std::string line;
    double checkval = 0.0;
    double value = 0.0;
    double mpi_value = 0.0;
    std::ifstream log(m_log_file_node, std::ios_base::in);

    while(err == 0 && std::getline(log, line)) {
        checkval = -1.0;
        if (line.find("Region loop_one:") == 0) {
            checkval = 3.0;
        }
        else if (line.find("Region loop_two:") == 0) {
            checkval = 6.0;
        }
        else if (line.find("Region loop_three:") == 0) {
            checkval = 9.0;
        }
        else if (line.find("Region mpi-sync:") == 0) {
            err = !std::getline(log, line);
            if (!err) {
                sscanf(line.c_str(), "        runtime: %lf", &mpi_value);
            }
        }
        else if (line.find("Region outer-sync:") == 0) {
            checkval = 18.0 + mpi_value;
        }
        if (checkval != -1.0) {
            err = !std::getline(log, line);
            if (!err) {
                err = !sscanf(line.c_str(), "        runtime: %lf", &value);
            }
            if (!err) {
                err = fabs(checkval - value) > m_epsilon;
            }
        }
    }
    log.close();
    return err;
}

TEST_F(MPIProfileTest, runtime)
{
    struct geopm_policy_c *policy = NULL;
    struct geopm_prof_c *prof;
    struct geopm_ctl_c *ctl;
    pthread_t ctl_thread;
    uint64_t region_id[3];
    struct geopm_time_s start, curr;
    double timeout = 0;
    int rank;
    MPI_Comm ppn1_comm;
    int num_node = 0;
    int mpi_thread_level = 0;

    (void) geopm_num_node(MPI_COMM_WORLD, &num_node);
    ASSERT_LT(1, num_node);

    (void) MPI_Query_thread(&mpi_thread_level);
    ASSERT_LE(MPI_THREAD_MULTIPLE, mpi_thread_level);

    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    ASSERT_EQ(0, geopm_comm_split_ppn1(MPI_COMM_WORLD, &ppn1_comm));

    if (!rank) {
        ASSERT_EQ(0, geopm_policy_create(m_policy_file.c_str(), "", &policy));
    }
    ASSERT_EQ(0, geopm_ctl_create(policy, m_shm_key, MPI_COMM_WORLD, &ctl));
    ASSERT_EQ(0, geopm_ctl_pthread(ctl, NULL, &ctl_thread));

    MPI_Barrier(MPI_COMM_WORLD);

    ASSERT_EQ(0, geopm_prof_create("runtime_test", m_table_size, m_shm_key, MPI_COMM_WORLD, &prof));

    ASSERT_EQ(0, geopm_prof_region(prof, "loop_one", GEOPM_POLICY_HINT_UNKNOWN, &region_id[0]));
    ASSERT_EQ(0, geopm_prof_enter(prof, region_id[0]));
    ASSERT_EQ(0, geopm_time(&start));
    while (timeout < 1.0) {
        ASSERT_EQ(0, geopm_time(&curr));
        timeout = geopm_time_diff(&start, &curr);
    }
    ASSERT_EQ(0, geopm_prof_exit(prof, region_id[0]));

    ASSERT_EQ(0, geopm_prof_region(prof, "loop_two", GEOPM_POLICY_HINT_UNKNOWN, &region_id[1]));
    ASSERT_EQ(0, geopm_prof_enter(prof, region_id[1]));
    ASSERT_EQ(0, geopm_time(&start));
    while (timeout < 2.0) {
        ASSERT_EQ(0, geopm_time(&curr));
        timeout = geopm_time_diff(&start, &curr);
    }
    ASSERT_EQ(0, geopm_prof_exit(prof, region_id[1]));

    ASSERT_EQ(0, geopm_prof_region(prof, "loop_three", GEOPM_POLICY_HINT_UNKNOWN, &region_id[2]));
    ASSERT_EQ(0, geopm_prof_enter(prof, region_id[2]));
    ASSERT_EQ(0, geopm_time(&start));
    while (timeout < 3.0) {
        ASSERT_EQ(0, geopm_time(&curr));
        timeout = geopm_time_diff(&start, &curr);
    }
    ASSERT_EQ(0, geopm_prof_exit(prof, region_id[2]));

    ASSERT_EQ(0, geopm_prof_print(prof, m_log_file.c_str(), 0));

    if (ppn1_comm != MPI_COMM_NULL) {
        ASSERT_EQ(0, parse_log(false));
    }

    ASSERT_EQ(0, geopm_prof_destroy(prof));
    ASSERT_EQ(0, geopm_ctl_destroy(ctl));
    if (!rank) {
        ASSERT_EQ(0, geopm_policy_destroy(policy));
    }
}

TEST_F(MPIProfileTest, progress)
{
    struct geopm_policy_c *policy = NULL;
    struct geopm_prof_c *prof;
    struct geopm_ctl_c *ctl;
    pthread_t ctl_thread;
    uint64_t region_id[3];
    struct geopm_time_s start, curr;
    double timeout = 0;
    int rank;
    MPI_Comm ppn1_comm;
    int num_node = 0;

    (void) geopm_num_node(MPI_COMM_WORLD, &num_node);
    ASSERT_TRUE(num_node > 1);

    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    ASSERT_EQ(0, geopm_comm_split_ppn1(MPI_COMM_WORLD, &ppn1_comm));

    if (!rank) {
        ASSERT_EQ(0, geopm_policy_create(m_policy_file.c_str(), "", &policy));
    }
    ASSERT_EQ(0, geopm_ctl_create(policy, m_shm_key, MPI_COMM_WORLD, &ctl));
    ASSERT_EQ(0, geopm_ctl_pthread(ctl, NULL, &ctl_thread));

    MPI_Barrier(MPI_COMM_WORLD);

    ASSERT_EQ(0, geopm_prof_create("progress_test", m_table_size, m_shm_key, MPI_COMM_WORLD, &prof));

    ASSERT_EQ(0, geopm_prof_region(prof, "loop_one", GEOPM_POLICY_HINT_UNKNOWN, &region_id[0]));
    ASSERT_EQ(0, geopm_prof_enter(prof, region_id[0]));
    ASSERT_EQ(0, geopm_time(&start));
    while (timeout < 1.0) {
        ASSERT_EQ(0, geopm_time(&curr));
        timeout = geopm_time_diff(&start, &curr);
        geopm_prof_progress(prof, region_id[2], timeout/1.0);
    }
    ASSERT_EQ(0, geopm_prof_exit(prof, region_id[0]));

    ASSERT_EQ(0, geopm_prof_region(prof, "loop_two", GEOPM_POLICY_HINT_UNKNOWN, &region_id[1]));
    ASSERT_EQ(0, geopm_prof_enter(prof, region_id[1]));
    ASSERT_EQ(0, geopm_time(&start));
    while (timeout < 2.0) {
        ASSERT_EQ(0, geopm_time(&curr));
        timeout = geopm_time_diff(&start, &curr);
        geopm_prof_progress(prof, region_id[2], timeout/2.0);
    }
    ASSERT_EQ(0, geopm_prof_exit(prof, region_id[1]));

    ASSERT_EQ(0, geopm_prof_region(prof, "loop_three", GEOPM_POLICY_HINT_UNKNOWN, &region_id[2]));
    ASSERT_EQ(0, geopm_prof_enter(prof, region_id[2]));
    ASSERT_EQ(0, geopm_time(&start));
    while (timeout < 3.0) {
        ASSERT_EQ(0, geopm_time(&curr));
        timeout = geopm_time_diff(&start, &curr);
        geopm_prof_progress(prof, region_id[2], timeout/3.0);
    }
    ASSERT_EQ(0, geopm_prof_exit(prof, region_id[2]));

    ASSERT_EQ(0, geopm_prof_print(prof, m_log_file.c_str(), 0));

    if (ppn1_comm != MPI_COMM_NULL) {
        ASSERT_EQ(0, parse_log(false));
    }

    ASSERT_EQ(0, geopm_prof_destroy(prof));
    ASSERT_EQ(0, geopm_ctl_destroy(ctl));
    if (!rank) {
        ASSERT_EQ(0, geopm_policy_destroy(policy));
    }
}

TEST_F(MPIProfileTest, multiple_entries)
{
    struct geopm_policy_c *policy = NULL;
    struct geopm_prof_c *prof;
    struct geopm_ctl_c *ctl;
    pthread_t ctl_thread;
    uint64_t region_id;
    struct geopm_time_s start, curr;
    double timeout = 0;
    int rank;
    MPI_Comm ppn1_comm;
    int num_node = 0;

    (void) geopm_num_node(MPI_COMM_WORLD, &num_node);
    ASSERT_TRUE(num_node > 1);

    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    ASSERT_EQ(0, geopm_comm_split_ppn1(MPI_COMM_WORLD, &ppn1_comm));

    if (!rank) {
        ASSERT_EQ(0, geopm_policy_create(m_policy_file.c_str(), "", &policy));
    }
    ASSERT_EQ(0, geopm_ctl_create(policy, m_shm_key, MPI_COMM_WORLD, &ctl));
    ASSERT_EQ(0, geopm_ctl_pthread(ctl, NULL, &ctl_thread));

    MPI_Barrier(MPI_COMM_WORLD);

    ASSERT_EQ(0, geopm_prof_create("progress_test", m_table_size, m_shm_key, MPI_COMM_WORLD, &prof));

    ASSERT_EQ(0, geopm_prof_region(prof, "loop_one", GEOPM_POLICY_HINT_UNKNOWN, &region_id));
    ASSERT_EQ(0, geopm_prof_enter(prof, region_id));
    ASSERT_EQ(0, geopm_time(&start));
    while (timeout < 1.0) {
        ASSERT_EQ(0, geopm_time(&curr));
        timeout = geopm_time_diff(&start, &curr);
        geopm_prof_progress(prof, region_id, timeout/1.0);
    }
    ASSERT_EQ(0, geopm_prof_exit(prof, region_id));

    ASSERT_EQ(0, geopm_prof_region(prof, "loop_one", GEOPM_POLICY_HINT_UNKNOWN, &region_id));
    ASSERT_EQ(0, geopm_prof_enter(prof, region_id));
    ASSERT_EQ(0, geopm_time(&start));
    while (timeout < 2.0) {
        ASSERT_EQ(0, geopm_time(&curr));
        timeout = geopm_time_diff(&start, &curr);
        geopm_prof_progress(prof, region_id, timeout/2.0);
    }
    ASSERT_EQ(0, geopm_prof_exit(prof, region_id));

    ASSERT_EQ(0, geopm_prof_region(prof, "loop_one", GEOPM_POLICY_HINT_UNKNOWN, &region_id));
    ASSERT_EQ(0, geopm_prof_enter(prof, region_id));
    ASSERT_EQ(0, geopm_time(&start));
    while (timeout < 3.0) {
        ASSERT_EQ(0, geopm_time(&curr));
        timeout = geopm_time_diff(&start, &curr);
        geopm_prof_progress(prof, region_id, timeout/3.0);
    }
    ASSERT_EQ(0, geopm_prof_exit(prof, region_id));

    ASSERT_EQ(0, geopm_prof_print(prof, m_log_file.c_str(), 0));

    if (ppn1_comm != MPI_COMM_NULL) {
        ASSERT_EQ(0, parse_log(true));
    }

    ASSERT_EQ(0, geopm_prof_destroy(prof));
    ASSERT_EQ(0, geopm_ctl_destroy(ctl));
    if (!rank) {
        ASSERT_EQ(0, geopm_policy_destroy(policy));
    }
}

TEST_F(MPIProfileTest, nested_region)
{
    struct geopm_policy_c *policy = NULL;
    struct geopm_prof_c *prof;
    struct geopm_ctl_c *ctl;
    pthread_t ctl_thread;
    uint64_t region_id[2];
    struct geopm_time_s start, curr;
    double timeout = 0;
    int rank;
    MPI_Comm ppn1_comm;
    int num_node = 0;

    (void) geopm_num_node(MPI_COMM_WORLD, &num_node);
    ASSERT_TRUE(num_node > 1);

    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    ASSERT_EQ(0, geopm_comm_split_ppn1(MPI_COMM_WORLD, &ppn1_comm));

    if (!rank) {
        ASSERT_EQ(0, geopm_policy_create(m_policy_file.c_str(), "", &policy));
    }
    ASSERT_EQ(0, geopm_ctl_create(policy, m_shm_key, MPI_COMM_WORLD, &ctl));
    ASSERT_EQ(0, geopm_ctl_pthread(ctl, NULL, &ctl_thread));

    MPI_Barrier(MPI_COMM_WORLD);

    ASSERT_EQ(0, geopm_prof_create("progress_test", m_table_size, m_shm_key, MPI_COMM_WORLD, &prof));

    ASSERT_EQ(0, geopm_prof_region(prof, "loop_one", GEOPM_POLICY_HINT_UNKNOWN, &region_id[0]));
    ASSERT_EQ(0, geopm_prof_enter(prof, region_id[0]));
    ASSERT_EQ(0, geopm_prof_region(prof, "loop_two", GEOPM_POLICY_HINT_UNKNOWN, &region_id[1]));
    ASSERT_EQ(0, geopm_prof_enter(prof, region_id[1]));
    ASSERT_EQ(0, geopm_time(&start));
    while (timeout < 1.0) {
        ASSERT_EQ(0, geopm_time(&curr));
        timeout = geopm_time_diff(&start, &curr);
        geopm_prof_progress(prof, region_id[1], timeout/1.0);
    }
    ASSERT_EQ(0, geopm_prof_exit(prof, region_id[1]));
    ASSERT_EQ(0, geopm_prof_exit(prof, region_id[0]));

    ASSERT_EQ(0, geopm_prof_region(prof, "loop_one", GEOPM_POLICY_HINT_UNKNOWN, &region_id[0]));
    ASSERT_EQ(0, geopm_prof_enter(prof, region_id[0]));
    ASSERT_EQ(0, geopm_prof_region(prof, "loop_two", GEOPM_POLICY_HINT_UNKNOWN, &region_id[1]));
    ASSERT_EQ(0, geopm_prof_enter(prof, region_id[1]));
    ASSERT_EQ(0, geopm_time(&start));
    while (timeout < 2.0) {
        ASSERT_EQ(0, geopm_time(&curr));
        timeout = geopm_time_diff(&start, &curr);
        geopm_prof_progress(prof, region_id[1], timeout/1.0);
    }
    ASSERT_EQ(0, geopm_prof_exit(prof, region_id[1]));
    ASSERT_EQ(0, geopm_prof_exit(prof, region_id[0]));

    ASSERT_EQ(0, geopm_prof_region(prof, "loop_one", GEOPM_POLICY_HINT_UNKNOWN, &region_id[0]));
    ASSERT_EQ(0, geopm_prof_enter(prof, region_id[0]));
    ASSERT_EQ(0, geopm_prof_region(prof, "loop_two", GEOPM_POLICY_HINT_UNKNOWN, &region_id[1]));
    ASSERT_EQ(0, geopm_prof_enter(prof, region_id[1]));
    ASSERT_EQ(0, geopm_time(&start));
    while (timeout < 3.0) {
        ASSERT_EQ(0, geopm_time(&curr));
        timeout = geopm_time_diff(&start, &curr);
        geopm_prof_progress(prof, region_id[1], timeout/1.0);
    }
    ASSERT_EQ(0, geopm_prof_exit(prof, region_id[1]));
    ASSERT_EQ(0, geopm_prof_exit(prof, region_id[0]));

    ASSERT_EQ(0, geopm_prof_print(prof, m_log_file.c_str(), 0));

    if (ppn1_comm != MPI_COMM_NULL) {
        ASSERT_EQ(0, parse_log(true));
    }

    ASSERT_EQ(0, geopm_prof_destroy(prof));
    ASSERT_EQ(0, geopm_ctl_destroy(ctl));
    if (!rank) {
        ASSERT_EQ(0, geopm_policy_destroy(policy));
    }
}

TEST_F(MPIProfileTest, noctl)
{
    m_epsilon = 0.5;
    m_use_std_sleep = true;

    int rank, shm_rank;
    MPI_Comm split_comm;
    MPI_Comm shm_comm;

    (void) MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    (void) MPI_Comm_split_type(MPI_COMM_WORLD, MPI_COMM_TYPE_SHARED, rank, MPI_INFO_NULL, &shm_comm);
    (void) MPI_Comm_rank(shm_comm, &shm_rank);
    (void) MPI_Comm_split(MPI_COMM_WORLD, !shm_rank, rank, &split_comm);

    if (!shm_rank) { // This rank plays the role of controller
        geopm::ProfileSampler sampler(m_shm_key, m_table_size);
        sampler.initialize();
        std::vector<std::pair<uint64_t, struct geopm_prof_message_s> > content(sampler.capacity());
        size_t content_length = 0;

        while (!sampler.do_shutdown()) {
            sampler.sample(content, content_length);
        }
        ASSERT_EQ(0, parse_log(false));
    }
    else { // All other ranks play the role of application
        struct geopm_prof_c *prof;
        uint64_t region_id[3];

        ASSERT_EQ(0, geopm_prof_create("runtime_test", m_table_size, m_shm_key, split_comm, &prof));

        ASSERT_EQ(0, geopm_prof_region(prof, "loop_one", GEOPM_POLICY_HINT_UNKNOWN, &region_id[0]));
        ASSERT_EQ(0, geopm_prof_region(prof, "loop_two", GEOPM_POLICY_HINT_UNKNOWN, &region_id[1]));
        ASSERT_EQ(0, geopm_prof_region(prof, "loop_three", GEOPM_POLICY_HINT_UNKNOWN, &region_id[2]));

        ASSERT_EQ(0, geopm_prof_enter(prof, region_id[0]));
        sleep_exact(1.0);
        ASSERT_EQ(0, geopm_prof_exit(prof, region_id[0]));

        ASSERT_EQ(0, geopm_prof_enter(prof, region_id[1]));
        sleep_exact(2.0);
        ASSERT_EQ(0, geopm_prof_exit(prof, region_id[1]));

        ASSERT_EQ(0, geopm_prof_enter(prof, region_id[2]));
        sleep_exact(3.0);
        ASSERT_EQ(0, geopm_prof_exit(prof, region_id[2]));

        ASSERT_EQ(0, geopm_prof_print(prof, m_log_file.c_str(), 0));
        ASSERT_EQ(0, geopm_prof_destroy(prof));
    }
}

TEST_F(MPIProfileTest, outer_sync)
{
    struct geopm_policy_c *policy = NULL;
    struct geopm_prof_c *prof;
    struct geopm_ctl_c *ctl;
    pthread_t ctl_thread;
    uint64_t region_id[3];
    int rank;
    MPI_Comm ppn1_comm;
    int num_node = 0;
    int mpi_thread_level = 0;

    (void) geopm_num_node(MPI_COMM_WORLD, &num_node);
    ASSERT_LT(1, num_node);

    (void) MPI_Query_thread(&mpi_thread_level);
    ASSERT_LE(MPI_THREAD_MULTIPLE, mpi_thread_level);

    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    ASSERT_EQ(0, geopm_comm_split_ppn1(MPI_COMM_WORLD, &ppn1_comm));

    if (!rank) {
        ASSERT_EQ(0, geopm_policy_create(m_policy_file.c_str(), "", &policy));
    }
    ASSERT_EQ(0, geopm_ctl_create(policy, m_shm_key, MPI_COMM_WORLD, &ctl));
    ASSERT_EQ(0, geopm_ctl_pthread(ctl, NULL, &ctl_thread));

    MPI_Barrier(MPI_COMM_WORLD);

    ASSERT_EQ(0, geopm_prof_create("runtime_test", m_table_size, m_shm_key, MPI_COMM_WORLD, &prof));

    for (int i = 0; i < 3; i++) {
        ASSERT_EQ(0, geopm_prof_outer_sync(prof));

        ASSERT_EQ(0, geopm_prof_region(prof, "loop_one", GEOPM_POLICY_HINT_UNKNOWN, &region_id[0]));
        ASSERT_EQ(0, geopm_prof_enter(prof, region_id[0]));
        sleep_exact(1.0);
        ASSERT_EQ(0, geopm_prof_exit(prof, region_id[0]));

        ASSERT_EQ(0, geopm_prof_region(prof, "loop_two", GEOPM_POLICY_HINT_UNKNOWN, &region_id[1]));
        ASSERT_EQ(0, geopm_prof_enter(prof, region_id[1]));
        sleep_exact(2.0);
        ASSERT_EQ(0, geopm_prof_exit(prof, region_id[1]));

        ASSERT_EQ(0, geopm_prof_region(prof, "loop_three", GEOPM_POLICY_HINT_UNKNOWN, &region_id[2]));
        ASSERT_EQ(0, geopm_prof_enter(prof, region_id[2]));
        sleep_exact(3.0);
        ASSERT_EQ(0, geopm_prof_exit(prof, region_id[2]));

        MPI_Barrier(MPI_COMM_WORLD);
    }
        ASSERT_EQ(0, geopm_prof_outer_sync(prof));

    ASSERT_EQ(0, geopm_prof_print(prof, m_log_file.c_str(), 0));

    if (ppn1_comm != MPI_COMM_NULL) {
        ASSERT_EQ(0, parse_log_loop());
    }

    ASSERT_EQ(0, geopm_prof_destroy(prof));
    ASSERT_EQ(0, geopm_ctl_destroy(ctl));
    if (!rank) {
        ASSERT_EQ(0, geopm_policy_destroy(policy));
    }
}
