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

//#include <sys/types.h>
//#include <unistd.h>
//#include <sys/mman.h>
//#include <sys/stat.h>
//#include <fcntl.h>
#include <sstream>

#include "gtest/gtest.h"
#include "gmock/gmock.h"
#include "geopm_env.h"
#include "geopm_sched.h"
#include "Profile.hpp"
#include "Exception.hpp"
#include "SharedMemory.hpp"
#include "MockComm.hpp"
#include "MockProfileTable.hpp"
#include "MockProfileThreadTable.hpp"
#include "MockSampleScheduler.hpp"
#include "MockControlMessage.hpp"
#include "MockSharedMemoryUser.hpp"

using geopm::Exception;
using geopm::Profile;
using geopm::IProfileThreadTable;
using geopm::ISharedMemory;
using geopm::SharedMemory;
using geopm::ISharedMemoryUser;
using geopm::SharedMemoryUser;
using geopm::IProfileTable;
using geopm::ISampleScheduler;
using geopm::IControlMessage;

struct free_delete
{
    void operator()(void* x)
    {
        free(x);
    }
};

class ProfileTestSharedMemoryUser : public MockSharedMemoryUser
{
    protected:
        std::unique_ptr<void, free_delete> m_buffer;
    public:
        ProfileTestSharedMemoryUser()
        {
        }

        ProfileTestSharedMemoryUser(size_t size)
        {
            m_buffer = std::unique_ptr<void, free_delete>(malloc(size));
            EXPECT_CALL(*this, size())
                .WillRepeatedly(testing::Return(size));
            EXPECT_CALL(*this, pointer())
                .WillRepeatedly(testing::Return(m_buffer.get()));
            EXPECT_CALL(*this, unlink())
                .WillRepeatedly(testing::Return());
        }
};

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

class ProfileTestProfileTable : public MockProfileTable
{
    public:
        ProfileTestProfileTable(std::function<uint64_t (const std::string &)> key_lambda, std::function<void (uint64_t key, const struct geopm_prof_message_s &value)> insert_lambda)
        {
            EXPECT_CALL(*this, key(testing::_))
                .WillRepeatedly(testing::Invoke(key_lambda));
            EXPECT_CALL(*this, insert(testing::_, testing::_))
                .WillRepeatedly(testing::Invoke(insert_lambda));
        }
};

class ProfileTestProfileThreadTable : public MockProfileThreadTable
{
    public:
        ProfileTestProfileThreadTable()
        {
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
            EXPECT_CALL(*this, split("prof", IComm::M_COMM_SPLIT_TYPE_SHARED))
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
        const double M_OVERHEAD_FRAC;
        const std::vector<uint64_t> m_expected_rid;
        std::vector<std::string> m_region_names;
        std::vector<int> m_rank;
        std::shared_ptr<ProfileTestComm> m_world_comm;
        std::shared_ptr<ProfileTestComm> m_shm_comm;
        std::unique_ptr<ProfileTestProfileTable> m_table;
        std::unique_ptr<ProfileTestProfileThreadTable> m_tprof;
        std::unique_ptr<ProfileTestSampleScheduler> m_scheduler;
        std::unique_ptr<ProfileTestControlMessage> m_ctl_msg;

        std::unique_ptr<Profile> m_profile;
};

ProfileTest::ProfileTest()
    : M_SHM_KEY ("profilte_test_shm_key")
    , M_PROF_NAME ("profile_test")
    , M_SHMEM_REGION_SIZE (12288)
    , M_SHM_COMM_SIZE (2)
    , M_OVERHEAD_FRAC (0.01)
    , m_expected_rid({5599005, 3780331735})
    , m_region_names({"test_region_name", "test_other_name"})
    , m_rank ({0, 1})
{
    setenv("GEOPM_ERROR_AFFINITY_IGNORE", "1", 1);
    setenv("GEOPM_REGION_BARRIER", "1", 1);
    setenv("GEOPM_PROFILE_TIMEOUT", std::to_string(1).c_str(), 1);
    setenv("GEOPM_REPORT_VERBOSITY", std::to_string(1).c_str(), 1);
}

ProfileTest::~ProfileTest()
{
    unsetenv("GEOPM_ERROR_AFFINITY_IGNORE");
    unsetenv("GEOPM_REGION_BARRIER");
    unsetenv("GEOPM_PROFILE_TIMEOUT");
    unsetenv("GEOPM_REPORT_VERBOSITY");
}

// TODO cleanup env var in destructor
// TODO fixtures need to either create shared mem user mock OR config_*_shm() call, not both
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
        auto insert_lambda = [] (uint64_t key, const struct geopm_prof_message_s &value)
        {
        };
        std::unique_ptr<ProfileTestSharedMemoryUser> table_shmem(new ProfileTestSharedMemoryUser(M_SHMEM_REGION_SIZE));
        m_table = std::unique_ptr<ProfileTestProfileTable>(new ProfileTestProfileTable(key_lambda, insert_lambda));

        m_ctl_msg = std::unique_ptr<ProfileTestControlMessage>(new ProfileTestControlMessage());
        m_shm_comm = std::make_shared<ProfileTestComm>(shm_rank, M_SHM_COMM_SIZE);
        m_world_comm = std::make_shared<ProfileTestComm>(world_rank, m_shm_comm);
        m_scheduler = std::unique_ptr<ProfileTestSampleScheduler>(new ProfileTestSampleScheduler());

        m_profile = std::unique_ptr<Profile>(new Profile(M_PROF_NAME, M_SHM_KEY, M_OVERHEAD_FRAC, nullptr,
                    nullptr, std::move(m_table),
                    std::move(table_shmem), std::move(m_scheduler),
                    std::move(m_ctl_msg), nullptr,
                    m_world_comm.get()));
        m_profile->config_prof_comm();
        long hint = 0;
        uint64_t rid = m_profile->region(region_name, hint);
        // call region with a new string
        // make it an mpi region for 2 birds
        EXPECT_EQ(expected_rid, rid);
        idx++;
    }
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
    auto insert_lambda = [world_rank, &expected_rid, &prog_fraction] (uint64_t key, const struct geopm_prof_message_s &value)
    {
        EXPECT_EQ(expected_rid, key);
        EXPECT_EQ(world_rank, value.rank);
        EXPECT_EQ(expected_rid, value.region_id);
        EXPECT_EQ(prog_fraction, value.progress);
    };

    std::unique_ptr<ProfileTestSharedMemoryUser> table_shmem(new ProfileTestSharedMemoryUser(M_SHMEM_REGION_SIZE));
    m_table = std::unique_ptr<ProfileTestProfileTable>(new ProfileTestProfileTable(key_lambda, insert_lambda));
    m_tprof = std::unique_ptr<ProfileTestProfileThreadTable>(new ProfileTestProfileThreadTable());
    EXPECT_CALL(*m_tprof, enable(testing::_))
        .WillRepeatedly(testing::Return());

    m_ctl_msg = std::unique_ptr<ProfileTestControlMessage>(new ProfileTestControlMessage());
    m_shm_comm = std::make_shared<ProfileTestComm>(shm_rank, M_SHM_COMM_SIZE);
    m_world_comm = std::make_shared<ProfileTestComm>(world_rank, m_shm_comm);
    m_scheduler = std::unique_ptr<ProfileTestSampleScheduler>(new ProfileTestSampleScheduler());

    m_profile = std::unique_ptr<Profile>(new Profile(M_PROF_NAME, M_SHM_KEY, M_OVERHEAD_FRAC, std::move(m_tprof),
                nullptr, std::move(m_table),
                std::move(table_shmem), std::move(m_scheduler),
                std::move(m_ctl_msg), nullptr,
                m_world_comm.get()));
    m_profile->config_prof_comm();
    long hint = 0;
    for (size_t idx = 0; idx < m_region_names.size(); ++idx) {
        region_name = m_region_names[idx];
        expected_rid = m_expected_rid[idx];
        uint64_t rid = m_profile->region(region_name, hint);
        prog_fraction = 0.0;
        m_profile->enter(rid);
        if (!idx) {
            expected_rid = m_expected_rid[idx] | GEOPM_REGION_ID_MPI;
            m_profile->enter(GEOPM_REGION_ID_MPI);
        }
        prog_fraction = 1.0;
        if (!idx) {
            expected_rid = m_expected_rid[idx] | GEOPM_REGION_ID_MPI;
            m_profile->exit(GEOPM_REGION_ID_MPI);
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
    auto insert_lambda = [world_rank, &expected_rid, &prog_fraction] (uint64_t key, const struct geopm_prof_message_s &value)
    {
        EXPECT_EQ(expected_rid, key);
        EXPECT_EQ(world_rank, value.rank);
        EXPECT_EQ(expected_rid, value.region_id);
        EXPECT_EQ(prog_fraction, value.progress);
    };

    std::unique_ptr<ProfileTestSharedMemoryUser> table_shmem(new ProfileTestSharedMemoryUser(M_SHMEM_REGION_SIZE));
    m_table = std::unique_ptr<ProfileTestProfileTable>(new ProfileTestProfileTable(key_lambda, insert_lambda));

    m_ctl_msg = std::unique_ptr<ProfileTestControlMessage>(new ProfileTestControlMessage());
    m_shm_comm = std::make_shared<ProfileTestComm>(shm_rank, M_SHM_COMM_SIZE);
    m_world_comm = std::make_shared<ProfileTestComm>(world_rank, m_shm_comm);
    m_scheduler = std::unique_ptr<ProfileTestSampleScheduler>(new ProfileTestSampleScheduler());
    EXPECT_CALL(*m_scheduler, record_exit())
        .WillOnce(testing::Return());

    m_profile = std::unique_ptr<Profile>(new Profile(M_PROF_NAME, M_SHM_KEY, M_OVERHEAD_FRAC, nullptr,
                nullptr, std::move(m_table),
                std::move(table_shmem), std::move(m_scheduler),
                std::move(m_ctl_msg), nullptr,
                m_world_comm.get()));
    m_profile->config_prof_comm();
    region_name = m_region_names[0];
    long hint = 0;
    uint64_t rid = m_profile->region(m_region_names[0], hint);
    prog_fraction = 0.0;
    m_profile->enter(rid);
    prog_fraction = 0.25;
    m_profile->progress(rid, prog_fraction);
    hint++;
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
    auto insert_lambda = [world_rank, &expected_rid, &prog_fraction] (uint64_t key, const struct geopm_prof_message_s &value)
    {
        EXPECT_EQ(expected_rid, key);
        EXPECT_EQ(world_rank, value.rank);
        EXPECT_EQ(expected_rid, value.region_id);
        EXPECT_EQ(prog_fraction, value.progress);
    };

    std::unique_ptr<ProfileTestSharedMemoryUser> table_shmem(new ProfileTestSharedMemoryUser(M_SHMEM_REGION_SIZE));
    m_table = std::unique_ptr<ProfileTestProfileTable>(new ProfileTestProfileTable(key_lambda, insert_lambda));

    m_ctl_msg = std::unique_ptr<ProfileTestControlMessage>(new ProfileTestControlMessage());
    m_shm_comm = std::make_shared<ProfileTestComm>(shm_rank, M_SHM_COMM_SIZE);
    m_world_comm = std::make_shared<ProfileTestComm>(world_rank, m_shm_comm);
    m_scheduler = std::unique_ptr<ProfileTestSampleScheduler>(new ProfileTestSampleScheduler());

    m_profile = std::unique_ptr<Profile>(new Profile(M_PROF_NAME, M_SHM_KEY, M_OVERHEAD_FRAC, nullptr,
                nullptr, std::move(m_table),
                std::move(table_shmem), std::move(m_scheduler),
                std::move(m_ctl_msg), nullptr,
                m_world_comm.get()));
    m_profile->config_prof_comm();
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
    auto insert_lambda = [] (uint64_t key, const struct geopm_prof_message_s &value)
    {
    };

    std::unique_ptr<ProfileTestSharedMemoryUser> table_shmem(new ProfileTestSharedMemoryUser(M_SHMEM_REGION_SIZE));
    m_table = std::unique_ptr<ProfileTestProfileTable>(new ProfileTestProfileTable(key_lambda, insert_lambda));

    m_ctl_msg = std::unique_ptr<ProfileTestControlMessage>(new ProfileTestControlMessage());
    m_shm_comm = std::make_shared<ProfileTestComm>(shm_rank, M_SHM_COMM_SIZE);
    m_world_comm = std::make_shared<ProfileTestComm>(world_rank, m_shm_comm);
    m_scheduler = std::unique_ptr<ProfileTestSampleScheduler>(new ProfileTestSampleScheduler());

    m_profile = std::unique_ptr<Profile>(new Profile(M_PROF_NAME, M_SHM_KEY, M_OVERHEAD_FRAC, nullptr,
                nullptr, std::move(m_table),
                std::move(table_shmem), std::move(m_scheduler),
                std::move(m_ctl_msg), nullptr,
                m_world_comm.get()));
    m_profile->config_prof_comm();
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
    auto insert_lambda = [world_rank, &expected_rid, &prog_fraction] (uint64_t key, const struct geopm_prof_message_s &value)
    {
        EXPECT_EQ(expected_rid, key);
        EXPECT_EQ(world_rank, value.rank);
        EXPECT_EQ(expected_rid, value.region_id);
        EXPECT_EQ(prog_fraction, value.progress);
    };

    std::unique_ptr<ProfileTestSharedMemoryUser> table_shmem(new ProfileTestSharedMemoryUser(M_SHMEM_REGION_SIZE));
    m_table = std::unique_ptr<ProfileTestProfileTable>(new ProfileTestProfileTable(key_lambda, insert_lambda));
    m_tprof = std::unique_ptr<ProfileTestProfileThreadTable>(new ProfileTestProfileThreadTable());

    m_ctl_msg = std::unique_ptr<ProfileTestControlMessage>(new ProfileTestControlMessage());
    m_shm_comm = std::make_shared<ProfileTestComm>(shm_rank, M_SHM_COMM_SIZE);
    m_world_comm = std::make_shared<ProfileTestComm>(world_rank, m_shm_comm);
    m_scheduler = std::unique_ptr<ProfileTestSampleScheduler>(new ProfileTestSampleScheduler());

    m_profile = std::unique_ptr<Profile>(new Profile(M_PROF_NAME, M_SHM_KEY, M_OVERHEAD_FRAC, std::move(m_tprof),
                nullptr, std::move(m_table),
                std::move(table_shmem), std::move(m_scheduler),
                std::move(m_ctl_msg), nullptr,
                m_world_comm.get()));
    m_profile->config_prof_comm();
    m_profile->tprof_table();//can't do comparison as local m_tprof was std::move'd
}

TEST_F(ProfileTest, config)
{
    auto key_lambda = [] (const std::string &name)
    {
        return (uint64_t) 0;
    };
    auto insert_lambda = [] (uint64_t key, const struct geopm_prof_message_s &value)
    {
    };
    // TODO add logic to change ctl_msg->cpu_rank(_) return { -1, -2}
    for (auto world_rank : m_rank) {
        for (auto shm_rank : m_rank) {
            std::unique_ptr<ProfileTestSharedMemoryUser> table_shmem(new ProfileTestSharedMemoryUser(M_SHMEM_REGION_SIZE));
            m_ctl_msg = std::unique_ptr<ProfileTestControlMessage>(new ProfileTestControlMessage());
            m_shm_comm = std::make_shared<ProfileTestComm>(shm_rank, M_SHM_COMM_SIZE);
            m_world_comm = std::make_shared<ProfileTestComm>(world_rank, m_shm_comm);
            m_table = std::unique_ptr<ProfileTestProfileTable>(new ProfileTestProfileTable(key_lambda, insert_lambda));
            m_scheduler = std::unique_ptr<ProfileTestSampleScheduler>(new ProfileTestSampleScheduler());

            m_profile = std::unique_ptr<Profile>(new Profile(M_PROF_NAME, M_SHM_KEY, M_OVERHEAD_FRAC, /*std::shared_ptr<IProfileThreadTable>*/ nullptr,
                        /*std::unique_ptr<ISharedMemoryUser>*/ nullptr, std::move(m_table),
                        std::move(table_shmem), std::move(m_scheduler),
                        std::move(m_ctl_msg), std::unique_ptr<ProfileTestSharedMemoryUser>(new ProfileTestSharedMemoryUser(M_SHMEM_REGION_SIZE)),
                        m_world_comm.get()));
            m_profile->config_prof_comm();
            auto sample_shm = std::unique_ptr<SharedMemory>(new SharedMemory(M_SHM_KEY + "-sample", M_SHMEM_REGION_SIZE));
            m_profile->config_ctl_shm();
            // TODO this call creates a real ControlMessage that attempts to step/wait on destruction
            // either work around or simulate this step/wait interaction to fix induced hang
            // create fixture that forks creating a profile and profile sampler?
            //m_profile->config_ctl_msg();
            m_profile->config_cpu_affinity();
            size_t tprof_shm_size = geopm_sched_num_cpu() * 64;
            auto tprof_shm = std::unique_ptr<SharedMemory>(new SharedMemory(M_SHM_KEY + "-tprof", tprof_shm_size));
            m_profile->config_tprof_table();
            std::ostringstream table_shm_key;
            table_shm_key << M_SHM_KEY <<  "-sample-" << world_rank;
            auto table_shm = std::unique_ptr<SharedMemory>(new SharedMemory(table_shm_key.str(), M_SHMEM_REGION_SIZE));
            m_profile->config_table();
            sample_shm.reset();
            tprof_shm.reset();
            table_shm.reset();
        }
    }
}

TEST_F(ProfileTest, config_throws)
{
    int world_rank = 0;
    int shm_rank = 0;
    auto key_lambda = [] (const std::string &name)
    {
        return (uint64_t) 0;
    };
    auto insert_lambda = [] (uint64_t key, const struct geopm_prof_message_s &value)
    {
    };
    std::unique_ptr<ProfileTestSharedMemoryUser> table_shmem(new ProfileTestSharedMemoryUser(M_SHMEM_REGION_SIZE));
    m_ctl_msg = std::unique_ptr<ProfileTestControlMessage>(new ProfileTestControlMessage());
    EXPECT_CALL(*m_ctl_msg, cpu_rank(testing::_))
        .WillRepeatedly(testing::Return(-2));
    m_shm_comm = std::make_shared<ProfileTestComm>(shm_rank, M_SHM_COMM_SIZE);
    m_world_comm = std::make_shared<ProfileTestComm>(world_rank, m_shm_comm);
    m_table = std::unique_ptr<ProfileTestProfileTable>(new ProfileTestProfileTable(key_lambda, insert_lambda));
    m_scheduler = std::unique_ptr<ProfileTestSampleScheduler>(new ProfileTestSampleScheduler());

    m_profile = std::unique_ptr<Profile>(new Profile(M_PROF_NAME, M_SHM_KEY, M_OVERHEAD_FRAC, /*std::shared_ptr<IProfileThreadTable>*/ nullptr,
                /*std::unique_ptr<ISharedMemoryUser>*/ nullptr, std::move(m_table),
                std::move(table_shmem), std::move(m_scheduler),
                std::move(m_ctl_msg), std::unique_ptr<ProfileTestSharedMemoryUser>(new ProfileTestSharedMemoryUser(M_SHMEM_REGION_SIZE)),
                m_world_comm.get()));
    m_profile->config_prof_comm();
    //EXPECT_THROW(m_profile->config_ctl_shm(), Exception);// TODO will need work to produce this throw
    auto sample_shm = std::unique_ptr<SharedMemory>(new SharedMemory(M_SHM_KEY + "-sample", M_SHMEM_REGION_SIZE));
    m_profile->config_ctl_shm();
    EXPECT_THROW(m_profile->config_cpu_affinity(), Exception);
    //EXPECT_THROW(m_profile->config_tprof_shm(), Exception);// TODO will need work to produce this throw
    size_t tprof_shm_size = 4;
    auto tprof_shm = std::unique_ptr<SharedMemory>(new SharedMemory(M_SHM_KEY + "-tprof", tprof_shm_size));
    EXPECT_THROW(m_profile->config_tprof_table(), Exception);
}
