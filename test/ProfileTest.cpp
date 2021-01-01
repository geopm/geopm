/*
 * Copyright (c) 2015, 2016, 2017, 2018, 2019, 2020, Intel Corporation
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

#include <sched.h>
#include <unistd.h>

#include <cstring>
#include <fstream>
#include <functional>
#include <memory>

#include "gmock/gmock.h"
#include "gtest/gtest.h"

#include "geopm.h"
#include "Profile.hpp"

#include "geopm_test.hpp"
#include "MockComm.hpp"
#include "MockProfileTable.hpp"
#include "MockControlMessage.hpp"
#include "MockApplicationRecordLog.hpp"
#include "MockApplicationStatus.hpp"


using geopm::Profile;
using geopm::ProfileImp;
using testing::_;
using testing::Return;
using testing::NiceMock;
using testing::AtLeast;

class ProfileTest : public ::testing::Test
{
    protected:
        void SetUp();
        const int m_process = 42;
        const int M_NUM_CPU = 4;
        std::set<int> m_cpu_list = {2, 3};
        std::shared_ptr<MockApplicationRecordLog> m_record_log;
        std::shared_ptr<MockApplicationStatus> m_status;

        // legacy mocks
        std::shared_ptr<MockComm> m_world_comm;
        std::shared_ptr<MockComm> m_shm_comm;
        std::shared_ptr<MockComm> m_comm;
        std::shared_ptr<MockProfileTable> m_table;
        std::shared_ptr<MockControlMessage> m_ctl_msg;

        std::unique_ptr<Profile> m_profile;
};

void ProfileTest::SetUp()
{
    m_record_log = std::make_shared<MockApplicationRecordLog>();
    m_status = std::make_shared<MockApplicationStatus>();

    EXPECT_CALL(*m_record_log, set_process(m_process));
    EXPECT_CALL(*m_record_log, set_time_zero(_));

    // legacy
    int shm_rank = 6;
    std::string M_PROF_NAME = "profile_test";
    std::string M_REPORT = "report_test";
    int M_SHM_COMM_SIZE = 2;
    m_ctl_msg = std::make_shared<NiceMock<MockControlMessage> >();
    m_shm_comm = std::make_shared<NiceMock<MockComm> >();
    ON_CALL(*m_shm_comm, rank()).WillByDefault(Return(shm_rank));
    ON_CALL(*m_shm_comm, num_rank()).WillByDefault(Return(M_SHM_COMM_SIZE));
    ON_CALL(*m_shm_comm, test(testing::_))
        .WillByDefault(testing::Return(true));

    m_world_comm = std::make_shared<NiceMock<MockComm> >();
    ON_CALL(*m_world_comm, rank()).WillByDefault(Return(m_process));
    ON_CALL(*m_world_comm, split("prof", geopm::Comm::M_COMM_SPLIT_TYPE_SHARED))
        .WillByDefault(Return(m_shm_comm));
    m_comm = std::make_shared<NiceMock<MockComm> >();
    m_table = std::make_shared<MockProfileTable>();
    ON_CALL(*m_table, name_fill(_))
        .WillByDefault(Return(true));

    m_profile = geopm::make_unique<ProfileImp>("profile",
                                               "shmem_key",
                                               "report",
                                               1,
                                               m_world_comm,
                                               m_ctl_msg,
                                               M_NUM_CPU,
                                               m_cpu_list,
                                               m_table,
                                               m_comm,
                                               m_status,
                                               m_record_log);
    EXPECT_CALL(*m_status, set_process(m_cpu_list, m_process));
    m_profile->init();
}

TEST_F(ProfileTest, enter_exit)
{
    uint64_t hash = 0xABCD;
    uint64_t hint = GEOPM_REGION_HINT_COMPUTE;
    uint64_t region_id = hint | hash;

    EXPECT_CALL(*m_record_log, enter(hash, _));
    EXPECT_CALL(*m_status, set_hash(2, hash));
    EXPECT_CALL(*m_status, set_hash(3, hash));
    EXPECT_CALL(*m_status, set_hint(2, hint));
    EXPECT_CALL(*m_status, set_hint(3, hint));
    m_profile->enter(region_id);

    EXPECT_CALL(*m_record_log, exit(hash, _));
    EXPECT_CALL(*m_status, set_hash(2, GEOPM_REGION_HASH_UNMARKED));
    EXPECT_CALL(*m_status, set_hash(3, GEOPM_REGION_HASH_UNMARKED));
    // hint is cleared when exiting top-level region
    EXPECT_CALL(*m_status, set_hint(2, GEOPM_REGION_HINT_UNSET));
    EXPECT_CALL(*m_status, set_hint(3, GEOPM_REGION_HINT_UNSET));
    // progress is cleared when exiting top-level region
    EXPECT_CALL(*m_status, set_total_work_units(2, 0));
    EXPECT_CALL(*m_status, set_total_work_units(3, 0));

    m_profile->exit(region_id);
}

// TODO: get rid of GEOPM_REGION_ID_MPI, epoch bit if still there
// TODO: fix geopm_mpi_region_enter/exit to set hint instead and
//       get rid of extra entry into GEOPM_REGION_ID_MPI

TEST_F(ProfileTest, enter_exit_nested)
{
    uint64_t usr_hash = 0xABCD;
    uint64_t usr_hint = GEOPM_REGION_HINT_COMPUTE;
    uint64_t usr_region_id = usr_hint | usr_hash;
    uint64_t mpi_hash = 0x5678;
    uint64_t mpi_hint = GEOPM_REGION_HINT_NETWORK;
    uint64_t mpi_region_id = mpi_hint | mpi_hash;
    {
        // enter region and set hint
        EXPECT_CALL(*m_record_log, enter(usr_hash, _));
        EXPECT_CALL(*m_status, set_hash(2, usr_hash));
        EXPECT_CALL(*m_status, set_hash(3, usr_hash));
        EXPECT_CALL(*m_status, set_hint(2, usr_hint));
        EXPECT_CALL(*m_status, set_hint(3, usr_hint));
        m_profile->enter(usr_hint | usr_hash);
    }
    {
        // don't enter a nested region, just update hint
        EXPECT_CALL(*m_record_log, enter(_, _)).Times(0);
        EXPECT_CALL(*m_status, set_hash(_, _)).Times(0);
        EXPECT_CALL(*m_status, set_hint(2, mpi_hint));
        EXPECT_CALL(*m_status, set_hint(3, mpi_hint));
        m_profile->enter(mpi_hint | mpi_hash);
    }
    {
        // don't exit, just restore hint
        EXPECT_CALL(*m_record_log, exit(_, _)).Times(0);
        EXPECT_CALL(*m_status, set_hint(2, usr_hint));
        EXPECT_CALL(*m_status, set_hint(3, usr_hint));
        m_profile->exit(mpi_region_id);
    }
    {
        // exit region and unset hint
        EXPECT_CALL(*m_record_log, exit(usr_hash, _));
        EXPECT_CALL(*m_status, set_hash(2, GEOPM_REGION_HASH_UNMARKED));
        EXPECT_CALL(*m_status, set_hash(3, GEOPM_REGION_HASH_UNMARKED));
        EXPECT_CALL(*m_status, set_hint(2, GEOPM_REGION_HINT_UNSET));
        EXPECT_CALL(*m_status, set_hint(3, GEOPM_REGION_HINT_UNSET));
        EXPECT_CALL(*m_status, set_total_work_units(2, 0));
        EXPECT_CALL(*m_status, set_total_work_units(3, 0));
        m_profile->exit(usr_region_id);
    }
}

TEST_F(ProfileTest, epoch)
{
    EXPECT_CALL(*m_record_log, epoch(_));
    m_profile->epoch();
}

TEST_F(ProfileTest, progress_multithread)
{
    uint64_t hash = 0xABCD;
    {
        EXPECT_CALL(*m_record_log, enter(hash, _));
        EXPECT_CALL(*m_status, set_hash(_, _)).Times(2);
        EXPECT_CALL(*m_status, set_hint(_, _)).Times(2);
        m_profile->enter(0xABCD);
    }
    {
        EXPECT_CALL(*m_status, set_total_work_units(2, 5));
        EXPECT_CALL(*m_status, set_total_work_units(3, 6));
        m_profile->thread_init(2, 5);
        m_profile->thread_init(3, 6);
    }
    {
        EXPECT_CALL(*m_status, increment_work_unit(3));
        EXPECT_CALL(*m_status, increment_work_unit(2));
        m_profile->thread_post(3);
        m_profile->thread_post(2);
    }
    {
        EXPECT_CALL(*m_status, increment_work_unit(3));
        m_profile->thread_post(3);
    }

    {
        EXPECT_CALL(*m_record_log, exit(hash, _));
        EXPECT_CALL(*m_status, set_hash(_, _)).Times(2);
        EXPECT_CALL(*m_status, set_hint(_, _)).Times(2);
        // clear progress when exiting
        EXPECT_CALL(*m_status, set_total_work_units(2, 0));
        EXPECT_CALL(*m_status, set_total_work_units(3, 0));
        m_profile->exit(0xABCD);
    }
    // TODO: make it an error to set values for other CPUs not
    // assigned to this process.  Does it also make sense to provide
    // an API without cpu that calls through to all CPUs in cpu_set
    // for the Profile object?
}
