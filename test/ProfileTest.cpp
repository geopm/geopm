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

#include "gtest/gtest.h"
#include "gmock/gmock.h"
#include "geopm_test.hpp"
#include "geopm_internal.h"
#include "geopm_env.h"
#include "Helper.hpp"
#include "Profile.hpp"
#include "Exception.hpp"
#include "SharedMemory.hpp"
#include "MockComm.hpp"
#include "MockPlatformTopo.hpp"
#include "MockProfileTable.hpp"
#include "MockProfileThreadTable.hpp"
#include "MockSampleScheduler.hpp"
#include "MockControlMessage.hpp"
#include "MockComm.hpp"

using geopm::Exception;
using geopm::Profile;
using geopm::SharedMemory;
using geopm::IPlatformTopo;

extern "C"
{
    void geopm_env_load(void);
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
            EXPECT_CALL(*this, num_domain(IPlatformTopo::M_DOMAIN_CPU))
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
};

ProfileTest::ProfileTest()
    : M_SHM_KEY("profile_test_shm_key")
    , M_PROF_NAME("profile_test")
    , M_SHMEM_REGION_SIZE(12288)
    , M_SHM_COMM_SIZE(2)
    , M_NUM_CPU(2)
    , m_expected_rid({5599005, 3780331735, 3282504576})
    , m_region_names({"test_region_name", "test_other_name", "recursive_region"})
    , m_rank({0, 1})
    , m_topo(M_NUM_CPU)
{
    m_comm = std::make_shared<MockComm>();

    setenv("GEOPM_REGION_BARRIER", "1", 1);
    setenv("GEOPM_PROFILE_TIMEOUT", "1", 1);
    geopm_env_load();
}

ProfileTest::~ProfileTest()
{
    unsetenv("GEOPM_REGION_BARRIER");
    unsetenv("GEOPM_PROFILE_TIMEOUT");
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

        m_profile = geopm::make_unique<Profile>(M_PROF_NAME, M_SHM_KEY, std::move(m_world_comm),
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

    m_profile = geopm::make_unique<Profile>(M_PROF_NAME, M_SHM_KEY, std::move(m_world_comm),
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

    m_profile = geopm::make_unique<Profile>(M_PROF_NAME, M_SHM_KEY, std::move(m_world_comm),
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

    m_profile = geopm::make_unique<Profile>(M_PROF_NAME, M_SHM_KEY, std::move(m_world_comm),
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

    m_profile = geopm::make_unique<Profile>(M_PROF_NAME, M_SHM_KEY, std::move(m_world_comm),
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

    m_profile = geopm::make_unique<Profile>(M_PROF_NAME, M_SHM_KEY, std::move(m_world_comm),
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

            auto tprof_shm = geopm::make_unique<SharedMemory>(M_SHM_KEY + "-tprof", M_NUM_CPU * 64);
            std::string table_shm_key = M_SHM_KEY + "-sample-" + std::to_string(world_rank);
            auto table_shm = geopm::make_unique<SharedMemory>(table_shm_key, M_SHMEM_REGION_SIZE);
            m_profile = geopm::make_unique<Profile>(M_PROF_NAME, M_SHM_KEY, std::move(m_world_comm),
                                                    std::move(m_ctl_msg), m_topo, nullptr, nullptr, nullptr, m_comm);
            tprof_shm.reset();
            table_shm.reset();
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
    Profile(M_PROF_NAME, M_SHM_KEY, std::move(m_world_comm),
            nullptr, m_topo, nullptr, nullptr, nullptr, m_comm);

    // small ctl_shmem
    m_shm_comm = std::make_shared<ProfileTestComm>(shm_rank, M_SHM_COMM_SIZE);
    m_world_comm = geopm::make_unique<ProfileTestComm>(world_rank, m_shm_comm);
    auto ctl_shm = geopm::make_unique<SharedMemory>(M_SHM_KEY + "-sample", 1);
    Profile(M_PROF_NAME, M_SHM_KEY, std::move(m_world_comm),
            nullptr, m_topo, nullptr, nullptr, nullptr, m_comm);
}

TEST_F(ProfileTestIntegration, misconfig_tprof_shmem)
{
    int world_rank = 0;
    int shm_rank = 0;
    m_ctl_msg = geopm::make_unique<ProfileTestControlMessage>();
    m_shm_comm = std::make_shared<ProfileTestComm>(shm_rank, M_SHM_COMM_SIZE);
    m_world_comm = geopm::make_unique<ProfileTestComm>(world_rank, m_shm_comm);

    // no tprof_shmem
    Profile(M_PROF_NAME, M_SHM_KEY, std::move(m_world_comm),
            std::move(m_ctl_msg), m_topo, nullptr, nullptr, nullptr, m_comm);

    m_ctl_msg = geopm::make_unique<ProfileTestControlMessage>();
    m_shm_comm = std::make_shared<ProfileTestComm>(shm_rank, M_SHM_COMM_SIZE);
    m_world_comm = geopm::make_unique<ProfileTestComm>(world_rank, m_shm_comm);

    // small tprof_shmem
    auto tprof_shm = geopm::make_unique<SharedMemory>(M_SHM_KEY + "-tprof", (M_NUM_CPU * 64) - 1);
    Profile(M_PROF_NAME, M_SHM_KEY, std::move(m_world_comm),
            std::move(m_ctl_msg), m_topo, nullptr, nullptr, nullptr, m_comm);
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
    Profile(M_PROF_NAME, M_SHM_KEY, std::move(m_world_comm),
            std::move(m_ctl_msg), m_topo, nullptr, std::move(m_tprof), nullptr, m_comm);

    m_ctl_msg = geopm::make_unique<ProfileTestControlMessage>();
    m_shm_comm = std::make_shared<ProfileTestComm>(shm_rank, M_SHM_COMM_SIZE);
    m_world_comm = geopm::make_unique<ProfileTestComm>(world_rank, m_shm_comm);
    m_tprof = geopm::make_unique<ProfileTestProfileThreadTable>(M_NUM_CPU);
    std::string table_shm_key = M_SHM_KEY + "-sample-" + std::to_string(world_rank);
    auto table_shm = geopm::make_unique<SharedMemory>(table_shm_key, 1);

    // small table_shmem
    Profile(M_PROF_NAME, M_SHM_KEY, std::move(m_world_comm),
            std::move(m_ctl_msg), m_topo, nullptr, std::move(m_tprof), nullptr, m_comm);
}

TEST_F(ProfileTestIntegration, misconfig_affinity)
{
    int world_rank = 0;
    int shm_rank = 0;
    geopm_env_load();
    m_ctl_msg = geopm::make_unique<ProfileTestControlMessage>();
    EXPECT_CALL(*m_ctl_msg, cpu_rank(testing::_))
        .WillRepeatedly(testing::Return(-2));
    m_shm_comm = std::make_shared<ProfileTestComm>(shm_rank, M_SHM_COMM_SIZE);
    m_world_comm = geopm::make_unique<ProfileTestComm>(world_rank, m_shm_comm);

    auto ctl_shm = geopm::make_unique<SharedMemory>(M_SHM_KEY + "-sample", M_SHMEM_REGION_SIZE);
    auto tprof_shm = geopm::make_unique<SharedMemory>(M_SHM_KEY + "-tprof", M_NUM_CPU * 64);
    std::string table_shm_key = M_SHM_KEY + "-sample-" + std::to_string(world_rank);
    auto table_shm = geopm::make_unique<SharedMemory>(table_shm_key, M_SHMEM_REGION_SIZE);
    Profile(M_PROF_NAME, M_SHM_KEY, std::move(m_world_comm),
            std::move(m_ctl_msg), m_topo, nullptr, nullptr, nullptr, m_comm);
}
