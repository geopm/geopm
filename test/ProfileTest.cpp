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

#include "Profile.hpp"

#include <sched.h>
#include <unistd.h>

#include <cstring>
#include <fstream>
#include <functional>
#include <memory>

#include "gmock/gmock.h"
#include "gtest/gtest.h"

#include "Exception.hpp"
#include "Helper.hpp"
#include "MockComm.hpp"
#include "MockControlMessage.hpp"
#include "MockPlatformTopo.hpp"
#include "MockProfileTable.hpp"
#include "MockProfileThreadTable.hpp"
#include "MockSampleScheduler.hpp"
#include "SharedMemoryImp.hpp"
#include "geopm_internal.h"
#include "geopm_test.hpp"

using geopm::Exception;
using geopm::Profile;
using geopm::ProfileImp;
using geopm::SharedMemoryImp;
using geopm::PlatformTopo;

static size_t num_configured_cpus()
{
    ssize_t num_cpus = sysconf(_SC_NPROCESSORS_CONF);
    if (num_cpus < 0) {
        throw Exception("Unable to get cpu count for tests",
                        errno ? errno : GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
    }
    return num_cpus;
}

// Get the number of CPUs in the cpuset of the test process
static size_t num_affinitized_cpus()
{
    // The number of bits in the system's CPU set isn't known ahead of time.
    // We need to repeatedly try with increasing set sizes until we stop
    // seeing errors for sets that are too small, or until we give up.
    int cpus_in_set = num_configured_cpus();
    static const int MAX_CPUS = 1 << 30;
    while (cpus_in_set < MAX_CPUS) {
        auto cpu_set = std::unique_ptr<cpu_set_t, std::function<void(cpu_set_t *)> >(
            CPU_ALLOC(cpus_in_set), [](cpu_set_t *cpu_set) { CPU_FREE(cpu_set); });
        size_t cpu_set_size = CPU_ALLOC_SIZE(cpus_in_set);

        if (sched_getaffinity(0, cpu_set_size, cpu_set.get()) == -1) {
            if (errno != EINVAL) {
                throw Exception("Unable to get affinity mask for tests",
                                errno ? errno : GEOPM_ERROR_RUNTIME, __FILE__,
                                __LINE__);
            }
            // Otherwise, move on to the next-larger attempt
            cpus_in_set *= 2;
        }
        else {
            return CPU_COUNT_S(cpu_set_size, cpu_set.get());
        }
    }

    throw Exception("Unable to get cpu count for tests. Gave up at cpu set size of " +
                        std::to_string(cpus_in_set),
                    GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
}

class ProfileTestControlMessage : public MockControlMessage
{
    public:
        ProfileTestControlMessage()
        {
            EXPECT_CALL(*this, step())
                .WillRepeatedly(testing::Return());
            EXPECT_CALL(*this, wait())
                .WillRepeatedly(testing::Return());
            EXPECT_CALL(*this, cpu_rank(testing::_, testing::_))
                .WillRepeatedly(testing::Return());
            EXPECT_CALL(*this, cpu_rank(testing::_))
                .WillRepeatedly(testing::Return(0));
            EXPECT_CALL(*this, loop_begin())
                .WillRepeatedly(testing::Return());
        }
};

class ProfileTestSampleScheduler : public MockSampleScheduler
{
    public:
        ProfileTestSampleScheduler()
        {
            EXPECT_CALL(*this, clear())
                .WillRepeatedly(testing::Return());
            EXPECT_CALL(*this, do_sample())
                .WillRepeatedly(testing::Return(true));
        }
};

class ProfileTestPlatformTopo : public MockPlatformTopo
{
    public:
        ProfileTestPlatformTopo(int num_cpu)
        {
            EXPECT_CALL(*this, num_domain(GEOPM_DOMAIN_CPU))
                .WillRepeatedly(testing::Return(num_cpu));
        }
};

class ProfileTestProfileTable : public MockProfileTable
{
    public:
        ProfileTestProfileTable(std::function<uint64_t (const std::string &)> key_lambda,
                std::function<void (const struct geopm_prof_message_s &value)> insert_lambda)
        {
            EXPECT_CALL(*this, key(testing::_))
                .WillRepeatedly(testing::Invoke(key_lambda));
            EXPECT_CALL(*this, insert(testing::_))
                .WillRepeatedly(testing::Invoke(insert_lambda));
            EXPECT_CALL(*this, name_fill(testing::_))
                .WillRepeatedly(testing::Return(true));
        }
};

class ProfileTestProfileThreadTable : public MockProfileThreadTable
{
    public:
        ProfileTestProfileThreadTable(int num_cpu)
        {
            EXPECT_CALL(*this, num_cpu())
                .WillRepeatedly(testing::Return(num_cpu));
        }
};

class ProfileTestComm : public MockComm
{
    public:
        // COMM_WORLD
        ProfileTestComm(int world_rank, std::shared_ptr<MockComm> shm_comm)
        {
            EXPECT_CALL(*this, rank())
                .WillRepeatedly(testing::Return(world_rank));
            EXPECT_CALL(*this, split("prof", Comm::M_COMM_SPLIT_TYPE_SHARED))
                .WillOnce(testing::Return(shm_comm));
            EXPECT_CALL(*this, barrier())
                .WillRepeatedly(testing::Return());
        }

        ProfileTestComm(int shm_rank, int shm_size)
        {
            //rank, num_rank, barrier, test,
            EXPECT_CALL(*this, rank())
                .WillRepeatedly(testing::Return(shm_rank));
            EXPECT_CALL(*this, num_rank())
                .WillRepeatedly(testing::Return(shm_size));
            EXPECT_CALL(*this, barrier())
                .WillRepeatedly(testing::Return());
            EXPECT_CALL(*this, test(testing::_))
                .WillRepeatedly(testing::Return(true));
        }
};

class ProfileTest : public :: testing :: Test
{
    public:
        ProfileTest();
        ~ProfileTest();
    protected:
        const std::string M_SHM_KEY;
        const std::string M_PROF_NAME;
        const std::string M_REPORT;
        const double M_TIMEOUT;
        const bool M_DO_REGION_BARRIER;
        const size_t M_SHMEM_REGION_SIZE;
        const size_t M_SHM_COMM_SIZE;
        const int M_NUM_CPU;
        const std::vector<uint64_t> m_expected_rid;
        std::vector<std::string> m_region_names;
        std::vector<int> m_rank;
        std::unique_ptr<ProfileTestComm> m_world_comm;
        std::shared_ptr<ProfileTestComm> m_shm_comm;
        std::unique_ptr<ProfileTestProfileTable> m_table;
        std::unique_ptr<ProfileTestProfileThreadTable> m_tprof;
        std::unique_ptr<ProfileTestSampleScheduler> m_scheduler;
        std::unique_ptr<ProfileTestControlMessage> m_ctl_msg;
        ProfileTestPlatformTopo m_topo;
        std::shared_ptr<MockComm> m_comm;

        std::unique_ptr<Profile> m_profile;
};

class ProfileTestIntegration : public ProfileTest
{
    public:
        ProfileTestIntegration()
            : ProfileTest()
        {
        }
        ~ProfileTestIntegration() = default;
    protected:
        void test_all_cpus_are_assigned_a_rank(size_t cpu_count, size_t cpu_set_size);
};

ProfileTest::ProfileTest()
    : M_SHM_KEY("profile_test_shm_key")
    , M_PROF_NAME("profile_test")
    , M_REPORT("report_test")
    , M_TIMEOUT(0)
    , M_DO_REGION_BARRIER(false)
    , M_SHMEM_REGION_SIZE(12288)
    , M_SHM_COMM_SIZE(2)
    , M_NUM_CPU(2)
    , m_expected_rid({5599005, 3780331735, 3282504576})
    , m_region_names({"test_region_name", "test_other_name", "recursive_region"})
    , m_rank({0, 1})
    , m_topo(M_NUM_CPU)
    , m_comm(std::make_shared<MockComm>())
{
}

ProfileTest::~ProfileTest()
{
}

TEST_F(ProfileTest, region)
{
    int shm_rank = 0;
    int world_rank = 0;
    int idx = 0;
    for (auto region_name : m_region_names) {
        auto expected_rid = m_expected_rid[idx];
        auto key_lambda = [region_name, expected_rid] (const std::string &name)
        {
            EXPECT_EQ(region_name, name);
            return expected_rid;
        };
        auto insert_lambda = [] (const struct geopm_prof_message_s &value)
        {
        };
        m_table = geopm::make_unique<ProfileTestProfileTable>(key_lambda, insert_lambda);
        m_tprof = geopm::make_unique<ProfileTestProfileThreadTable>(M_NUM_CPU);

        m_ctl_msg = geopm::make_unique<ProfileTestControlMessage>();
        m_shm_comm = std::make_shared<ProfileTestComm>(shm_rank, M_SHM_COMM_SIZE);
        m_world_comm = geopm::make_unique<ProfileTestComm>(world_rank, m_shm_comm);

        m_profile = geopm::make_unique<ProfileImp>(M_PROF_NAME, M_SHM_KEY,
                                                   M_REPORT, M_TIMEOUT, M_DO_REGION_BARRIER,
                                                   std::move(m_world_comm),
                                                   std::move(m_ctl_msg), m_topo, std::move(m_table),
                                                   std::move(m_tprof), std::move(m_scheduler), m_comm);
        long hint = 0;
        uint64_t rid = m_profile->region(region_name, hint);
        EXPECT_EQ(expected_rid, rid);
        idx++;
    }

    GEOPM_EXPECT_THROW_MESSAGE(m_profile->region("multi_hint", (1ULL << 33) | (1ULL << 34)),
                               GEOPM_ERROR_RUNTIME, "multiple region hints set and only 1 at a time is supported.");
}

TEST_F(ProfileTest, enter_exit)
{
    int shm_rank = 0;
    int world_rank = 0;
    std::string region_name;
    uint64_t expected_rid;
    double prog_fraction;

    auto key_lambda = [&region_name, &expected_rid] (const std::string &name)
    {
        EXPECT_EQ(region_name, name);
        return expected_rid;
    };
    auto insert_lambda = [world_rank, &expected_rid, &prog_fraction] (const struct geopm_prof_message_s &value)
    {
        EXPECT_EQ(world_rank, value.rank);
        EXPECT_EQ(expected_rid, value.region_id);
        EXPECT_EQ(prog_fraction, value.progress);
    };

    m_table = geopm::make_unique<ProfileTestProfileTable>(key_lambda, insert_lambda);
    m_tprof = geopm::make_unique<ProfileTestProfileThreadTable>(M_NUM_CPU);
    EXPECT_CALL(*m_tprof, enable(testing::_))
        .WillRepeatedly(testing::Return());

    m_ctl_msg = geopm::make_unique<ProfileTestControlMessage>();
    m_shm_comm = std::make_shared<ProfileTestComm>(shm_rank, M_SHM_COMM_SIZE);
    m_world_comm = geopm::make_unique<ProfileTestComm>(world_rank, m_shm_comm);
    m_scheduler = geopm::make_unique<ProfileTestSampleScheduler>();

    m_profile = geopm::make_unique<ProfileImp>(M_PROF_NAME, M_SHM_KEY,
                                               M_REPORT, M_TIMEOUT, M_DO_REGION_BARRIER,
                                               std::move(m_world_comm),
                                               std::move(m_ctl_msg), m_topo, std::move(m_table),
                                               std::move(m_tprof), std::move(m_scheduler), m_comm);
    long hint = 0;
    for (size_t idx = 0; idx < m_region_names.size(); ++idx) {
        region_name = m_region_names[idx];
        expected_rid = m_expected_rid[idx];
        uint64_t rid = m_profile->region(region_name, hint);
        prog_fraction = 0.0;
        m_profile->enter(rid);
        if (!idx) { // MPI region
            expected_rid = m_expected_rid[idx] | GEOPM_REGION_ID_MPI;
            m_profile->enter(GEOPM_REGION_ID_MPI);
        }
        else if (idx == 2) {    // re-entrant region
            m_profile->enter(rid);
        }
        prog_fraction = 1.0;
        if (!idx) { // MPI region
            expected_rid = m_expected_rid[idx] | GEOPM_REGION_ID_MPI;
            m_profile->exit(GEOPM_REGION_ID_MPI);
        }
        else if (idx == 2) {    // re-entrant region
            m_profile->exit(rid);
        }
        expected_rid = m_expected_rid[idx];
        m_profile->exit(rid);
    }
    prog_fraction = 0.0;
    expected_rid = GEOPM_REGION_ID_MPI;
    m_profile->enter(GEOPM_REGION_ID_MPI);
    prog_fraction = 1.0;
    m_profile->exit(GEOPM_REGION_ID_MPI);
}

TEST_F(ProfileTest, progress)
{
    int shm_rank = 0;
    int world_rank = 0;
    std::string region_name;
    uint64_t expected_rid;
    double prog_fraction;

    auto key_lambda = [&region_name, &expected_rid] (const std::string &name)
    {
        EXPECT_EQ(region_name, name);
        return expected_rid;
    };
    auto insert_lambda = [world_rank, &expected_rid, &prog_fraction] (const struct geopm_prof_message_s &value)
    {
        EXPECT_EQ(world_rank, value.rank);
        EXPECT_EQ(expected_rid, value.region_id);
        EXPECT_EQ(prog_fraction, value.progress);
    };

    m_table = geopm::make_unique<ProfileTestProfileTable>(key_lambda, insert_lambda);
    m_tprof = geopm::make_unique<ProfileTestProfileThreadTable>(M_NUM_CPU);

    m_ctl_msg = geopm::make_unique<ProfileTestControlMessage>();
    m_shm_comm = std::make_shared<ProfileTestComm>(shm_rank, M_SHM_COMM_SIZE);
    m_world_comm = geopm::make_unique<ProfileTestComm>(world_rank, m_shm_comm);
    m_scheduler = geopm::make_unique<ProfileTestSampleScheduler>();
    EXPECT_CALL(*m_scheduler, record_exit())
        .WillOnce(testing::Return());

    m_profile = geopm::make_unique<ProfileImp>(M_PROF_NAME, M_SHM_KEY, M_REPORT, M_TIMEOUT, M_DO_REGION_BARRIER,
                                               std::move(m_world_comm),
                                               std::move(m_ctl_msg), m_topo, std::move(m_table),
                                               std::move(m_tprof), std::move(m_scheduler), m_comm);
    region_name = m_region_names[0];
    long hint = 0;
    uint64_t rid = m_profile->region(m_region_names[0], hint);
    prog_fraction = 0.0;
    m_profile->enter(rid);
    prog_fraction = 0.25;
    m_profile->progress(rid, prog_fraction);
}

TEST_F(ProfileTest, epoch)
{
    int shm_rank = 0;
    int world_rank = 0;
    std::string region_name;
    uint64_t expected_rid = GEOPM_REGION_ID_EPOCH;
    double prog_fraction = 0.0;

    auto key_lambda = [&region_name, &expected_rid] (const std::string &name)
    {
        EXPECT_EQ(region_name, name);
        return expected_rid;
    };
    auto insert_lambda = [world_rank, &expected_rid, &prog_fraction] (const struct geopm_prof_message_s &value)
    {
        EXPECT_EQ(world_rank, value.rank);
        EXPECT_EQ(expected_rid, value.region_id);
        EXPECT_EQ(prog_fraction, value.progress);
    };

    m_table = geopm::make_unique<ProfileTestProfileTable>(key_lambda, insert_lambda);
    m_tprof = geopm::make_unique<ProfileTestProfileThreadTable>(M_NUM_CPU);

    m_ctl_msg = geopm::make_unique<ProfileTestControlMessage>();
    m_shm_comm = std::make_shared<ProfileTestComm>(shm_rank, M_SHM_COMM_SIZE);
    m_world_comm = geopm::make_unique<ProfileTestComm>(world_rank, m_shm_comm);
    m_scheduler = geopm::make_unique<ProfileTestSampleScheduler>();

    m_profile = geopm::make_unique<ProfileImp>(M_PROF_NAME, M_SHM_KEY, M_REPORT, M_TIMEOUT, M_DO_REGION_BARRIER,
                                               std::move(m_world_comm),
                                               std::move(m_ctl_msg), m_topo, std::move(m_table),
                                               std::move(m_tprof), std::move(m_scheduler), m_comm);
    m_profile->epoch();
}

TEST_F(ProfileTest, shutdown)
{
    int shm_rank = 0;
    int world_rank = 0;

    auto key_lambda = [] (const std::string &name)
    {
        return (uint64_t) 0;
    };
    auto insert_lambda = [] (const struct geopm_prof_message_s &value)
    {
    };

    m_table = geopm::make_unique<ProfileTestProfileTable>(key_lambda, insert_lambda);
    m_tprof = geopm::make_unique<ProfileTestProfileThreadTable>(M_NUM_CPU);

    m_ctl_msg = geopm::make_unique<ProfileTestControlMessage>();
    m_shm_comm = std::make_shared<ProfileTestComm>(shm_rank, M_SHM_COMM_SIZE);
    m_world_comm = geopm::make_unique<ProfileTestComm>(world_rank, m_shm_comm);
    m_scheduler = geopm::make_unique<ProfileTestSampleScheduler>();

    m_profile = geopm::make_unique<ProfileImp>(M_PROF_NAME, M_SHM_KEY, M_REPORT, M_TIMEOUT, M_DO_REGION_BARRIER,
                                               std::move(m_world_comm),
                                               std::move(m_ctl_msg), m_topo, std::move(m_table),
                                               std::move(m_tprof), std::move(m_scheduler), m_comm);
    m_profile->shutdown();
    m_profile->region(m_region_names[0], 0);
    m_profile->enter(0);
    m_profile->exit(0);
    m_profile->epoch();
    m_profile->progress(0, 0.0);
    m_profile->tprof_table();
    m_profile->shutdown();
}

TEST_F(ProfileTest, tprof_table)
{
    int shm_rank = 0;
    int world_rank = 0;
    std::string region_name;
    uint64_t expected_rid = GEOPM_REGION_ID_EPOCH;
    double prog_fraction = 0.0;

    auto key_lambda = [&region_name, &expected_rid] (const std::string &name)
    {
        EXPECT_EQ(region_name, name);
        return expected_rid;
    };
    auto insert_lambda = [world_rank, &expected_rid, &prog_fraction] (const struct geopm_prof_message_s &value)
    {
        EXPECT_EQ(world_rank, value.rank);
        EXPECT_EQ(expected_rid, value.region_id);
        EXPECT_EQ(prog_fraction, value.progress);
    };

    m_table = geopm::make_unique<ProfileTestProfileTable>(key_lambda, insert_lambda);
    m_tprof = geopm::make_unique<ProfileTestProfileThreadTable>(M_NUM_CPU);

    m_ctl_msg = geopm::make_unique<ProfileTestControlMessage>();
    m_shm_comm = std::make_shared<ProfileTestComm>(shm_rank, M_SHM_COMM_SIZE);
    m_world_comm = geopm::make_unique<ProfileTestComm>(world_rank, m_shm_comm);
    m_scheduler = geopm::make_unique<ProfileTestSampleScheduler>();

    m_profile = geopm::make_unique<ProfileImp>(M_PROF_NAME, M_SHM_KEY, M_REPORT, M_TIMEOUT, M_DO_REGION_BARRIER,
                                               std::move(m_world_comm),
                                               std::move(m_ctl_msg), m_topo, std::move(m_table),
                                               std::move(m_tprof), std::move(m_scheduler), m_comm);
    EXPECT_EQ(M_NUM_CPU, m_profile->tprof_table()->num_cpu());
}

TEST_F(ProfileTestIntegration, config)
{
    // @todo
    // Create ProfileSampler integration test that creates a real
    // ControlMessage to interact with
    for (auto world_rank : m_rank) {
        for (auto shm_rank : m_rank) {
            m_ctl_msg = geopm::make_unique<ProfileTestControlMessage>();
            m_shm_comm = std::make_shared<ProfileTestComm>(shm_rank, M_SHM_COMM_SIZE);
            m_world_comm = geopm::make_unique<ProfileTestComm>(world_rank, m_shm_comm);

            auto tprof_shm = geopm::make_unique<SharedMemoryImp>(M_SHM_KEY + "-tprof", M_NUM_CPU * 64);
            std::string table_shm_key = M_SHM_KEY + "-sample-" + std::to_string(world_rank);
            auto table_shm = geopm::make_unique<SharedMemoryImp>(table_shm_key, M_SHMEM_REGION_SIZE);
            m_profile = geopm::make_unique<ProfileImp>(M_PROF_NAME, M_SHM_KEY, M_REPORT, M_TIMEOUT, M_DO_REGION_BARRIER,
                                                       std::move(m_world_comm),
                                                       std::move(m_ctl_msg), m_topo, nullptr, nullptr, nullptr, m_comm);
            table_shm->unlink();
            tprof_shm->unlink();
            table_shm.reset();
            tprof_shm.reset();
        }
    }
}

TEST_F(ProfileTestIntegration, misconfig_ctl_shmem)
{
    int world_rank = 0;
    int shm_rank = 0;
    m_shm_comm = std::make_shared<ProfileTestComm>(shm_rank, M_SHM_COMM_SIZE);
    m_world_comm = geopm::make_unique<ProfileTestComm>(world_rank, m_shm_comm);

    // no ctl_shmem
    ProfileImp(M_PROF_NAME, M_SHM_KEY, M_REPORT, M_TIMEOUT, M_DO_REGION_BARRIER,
               std::move(m_world_comm),
               nullptr, m_topo, nullptr, nullptr, nullptr, m_comm);

    // small ctl_shmem
    m_shm_comm = std::make_shared<ProfileTestComm>(shm_rank, M_SHM_COMM_SIZE);
    m_world_comm = geopm::make_unique<ProfileTestComm>(world_rank, m_shm_comm);
    auto ctl_shm = geopm::make_unique<SharedMemoryImp>(M_SHM_KEY + "-sample", 1);
    ProfileImp(M_PROF_NAME, M_SHM_KEY, M_REPORT, M_TIMEOUT, M_DO_REGION_BARRIER,
               std::move(m_world_comm),
               nullptr, m_topo, nullptr, nullptr, nullptr, m_comm);
    ctl_shm->unlink();
}

TEST_F(ProfileTestIntegration, misconfig_tprof_shmem)
{
    int world_rank = 0;
    int shm_rank = 0;
    m_ctl_msg = geopm::make_unique<ProfileTestControlMessage>();
    m_shm_comm = std::make_shared<ProfileTestComm>(shm_rank, M_SHM_COMM_SIZE);
    m_world_comm = geopm::make_unique<ProfileTestComm>(world_rank, m_shm_comm);

    // no tprof_shmem
    ProfileImp(M_PROF_NAME, M_SHM_KEY, M_REPORT, M_TIMEOUT, M_DO_REGION_BARRIER,
               std::move(m_world_comm),
               std::move(m_ctl_msg), m_topo, nullptr, nullptr, nullptr, m_comm);

    m_ctl_msg = geopm::make_unique<ProfileTestControlMessage>();
    m_shm_comm = std::make_shared<ProfileTestComm>(shm_rank, M_SHM_COMM_SIZE);
    m_world_comm = geopm::make_unique<ProfileTestComm>(world_rank, m_shm_comm);

    // small tprof_shmem
    auto tprof_shm = geopm::make_unique<SharedMemoryImp>(M_SHM_KEY + "-tprof", (M_NUM_CPU * 64) - 1);
    ProfileImp(M_PROF_NAME, M_SHM_KEY, M_REPORT, M_TIMEOUT, M_DO_REGION_BARRIER,
               std::move(m_world_comm),
               std::move(m_ctl_msg), m_topo, nullptr, nullptr, nullptr, m_comm);
    tprof_shm->unlink();
}

TEST_F(ProfileTestIntegration, misconfig_table_shmem)
{
    int world_rank = 0;
    int shm_rank = 0;
    m_ctl_msg = geopm::make_unique<ProfileTestControlMessage>();
    m_shm_comm = std::make_shared<ProfileTestComm>(shm_rank, M_SHM_COMM_SIZE);
    m_world_comm = geopm::make_unique<ProfileTestComm>(world_rank, m_shm_comm);
    m_tprof = geopm::make_unique<ProfileTestProfileThreadTable>(M_NUM_CPU);

    // no table_shmem
    ProfileImp(M_PROF_NAME, M_SHM_KEY, M_REPORT, M_TIMEOUT, M_DO_REGION_BARRIER,
               std::move(m_world_comm),
               std::move(m_ctl_msg), m_topo, nullptr, std::move(m_tprof), nullptr, m_comm);

    m_ctl_msg = geopm::make_unique<ProfileTestControlMessage>();
    m_shm_comm = std::make_shared<ProfileTestComm>(shm_rank, M_SHM_COMM_SIZE);
    m_world_comm = geopm::make_unique<ProfileTestComm>(world_rank, m_shm_comm);
    m_tprof = geopm::make_unique<ProfileTestProfileThreadTable>(M_NUM_CPU);
    std::string table_shm_key = M_SHM_KEY + "-sample-" + std::to_string(world_rank);
    auto table_shm = geopm::make_unique<SharedMemoryImp>(table_shm_key, 1);

    // small table_shmem
    ProfileImp(M_PROF_NAME, M_SHM_KEY, M_REPORT, M_TIMEOUT, M_DO_REGION_BARRIER,
               std::move(m_world_comm),
               std::move(m_ctl_msg), m_topo, nullptr, std::move(m_tprof), nullptr, m_comm);

    table_shm->unlink();
}

TEST_F(ProfileTestIntegration, misconfig_affinity)
{
    int world_rank = 0;
    int shm_rank = 0;
    m_ctl_msg = geopm::make_unique<ProfileTestControlMessage>();
    EXPECT_CALL(*m_ctl_msg, cpu_rank(testing::_))
        .WillRepeatedly(testing::Return(-2));
    m_shm_comm = std::make_shared<ProfileTestComm>(shm_rank, M_SHM_COMM_SIZE);
    m_world_comm = geopm::make_unique<ProfileTestComm>(world_rank, m_shm_comm);

    auto ctl_shm = geopm::make_unique<SharedMemoryImp>(M_SHM_KEY + "-sample", M_SHMEM_REGION_SIZE);
    auto tprof_shm = geopm::make_unique<SharedMemoryImp>(M_SHM_KEY + "-tprof", M_NUM_CPU * 64);
    std::string table_shm_key = M_SHM_KEY + "-sample-" + std::to_string(world_rank);
    auto table_shm = geopm::make_unique<SharedMemoryImp>(table_shm_key, M_SHMEM_REGION_SIZE);
    ProfileImp(M_PROF_NAME, M_SHM_KEY, M_REPORT, M_TIMEOUT, M_DO_REGION_BARRIER,
               std::move(m_world_comm),
               std::move(m_ctl_msg), m_topo, nullptr, nullptr, nullptr, m_comm);
    table_shm->unlink();
    tprof_shm->unlink();
    ctl_shm->unlink();
}

void ProfileTestIntegration::test_all_cpus_are_assigned_a_rank(size_t cpu_count, size_t cpu_set_size)
{
    const int world_rank = 0;
    const int shm_rank = 0;
    ProfileTestPlatformTopo test_topo(cpu_set_size);
    m_shm_comm = std::make_shared<ProfileTestComm>(shm_rank, M_SHM_COMM_SIZE);
    m_world_comm = geopm::make_unique<ProfileTestComm>(world_rank, m_shm_comm);
    m_ctl_msg = geopm::make_unique<ProfileTestControlMessage>();
    EXPECT_CALL(*m_ctl_msg, cpu_rank(testing::_, world_rank)).Times(cpu_count);
    ProfileImp(M_PROF_NAME, M_SHM_KEY, M_REPORT, M_TIMEOUT, M_DO_REGION_BARRIER,
               std::move(m_world_comm), std::move(m_ctl_msg), test_topo,
               nullptr, nullptr, nullptr, m_comm);
}

TEST_F(ProfileTestIntegration, cpu_set_size)
{
    size_t configured_cpu_count = num_configured_cpus();
    size_t affinitized_cpu_count = num_affinitized_cpus();

    // Test that all allowed CPUs have been assigned a rank.
    test_all_cpus_are_assigned_a_rank(affinitized_cpu_count, configured_cpu_count);

    // Test again with a larger cpuset size to demonstrate that the cpu_rank
    // calls don't simply happen once per allocated set entry.
    test_all_cpus_are_assigned_a_rank(affinitized_cpu_count, configured_cpu_count + 32);
}
