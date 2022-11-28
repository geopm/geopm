/*
 * Copyright (c) 2015 - 2023, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <sched.h>
#include <unistd.h>

#include <cstring>
#include <fstream>
#include <functional>
#include <memory>

#include "gmock/gmock.h"
#include "gtest/gtest.h"

#include "geopm_hint.h"
#include "geopm_prof.h"
#include "Profile.hpp"
#include "geopm/SharedMemory.hpp"
#include "ApplicationRecordLog.hpp"
#include "ApplicationStatus.hpp"

#include "geopm_test.hpp"
#include "MockComm.hpp"

#include "MockControlMessage.hpp"
#include "MockProfileTable.hpp"


using geopm::Profile;
using geopm::ProfileImp;
using geopm::SharedMemory;
using geopm::ApplicationRecordLog;
using geopm::record_s;
using geopm::short_region_s;
using geopm::ApplicationStatus;
using testing::Return;
using testing::NiceMock;
using testing::_;


class ProfileTestIntegration : public ::testing::Test
{
    protected:
        void SetUp();
        void TearDown();

        const int M_NUM_CPU = 4;
        const int m_process = 42;
        std::set<int> m_cpu_list = {2, 3};
        const int m_timeout = 1;
        std::string M_SHM_KEY = "ProfileTestIntegration";

        // legacy code path
        std::shared_ptr<MockComm> m_world_comm;
        std::shared_ptr<MockComm> m_shm_comm;
        std::shared_ptr<MockComm> m_comm;
        std::shared_ptr<SharedMemory> m_table_shm;
        std::shared_ptr<MockProfileTable> m_table;
        std::shared_ptr<MockControlMessage> m_ctl_msg;

        // new code path
        std::shared_ptr<SharedMemory> m_ctl_record_shmem;
        std::shared_ptr<SharedMemory> m_ctl_status_shmem;
        std::shared_ptr<ApplicationRecordLog> m_ctl_record_log;
        std::shared_ptr<ApplicationStatus> m_ctl_status;
        std::shared_ptr<Profile> m_profile;
};

void ProfileTestIntegration::SetUp()
{
    int shm_rank = 6;
    std::string M_PROF_NAME = "profile_test";
    std::string M_REPORT = "report_test";
    int M_SHM_COMM_SIZE = 2;

    // new code path
    m_ctl_record_shmem = SharedMemory::make_unique_owner(M_SHM_KEY + "-record-log-42",
                                                         ApplicationRecordLog::buffer_size());
    m_ctl_status_shmem = SharedMemory::make_unique_owner(M_SHM_KEY + "-status",
                                                         ApplicationStatus::buffer_size(M_NUM_CPU));
    m_ctl_record_log = ApplicationRecordLog::make_unique(m_ctl_record_shmem);
    m_ctl_status = ApplicationStatus::make_unique(M_NUM_CPU, m_ctl_status_shmem);

    // legacy
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

    m_profile = geopm::make_unique<ProfileImp>(M_PROF_NAME,
                                               M_SHM_KEY,
                                               M_REPORT,
                                               m_timeout,
                                               m_world_comm,
                                               m_ctl_msg,
                                               M_NUM_CPU,
                                               m_cpu_list,
                                               m_table,
                                               m_comm,
                                               nullptr,  // status
                                               nullptr,  // record_log
                                               true);    // do_profile

    m_profile->init();
}

void ProfileTestIntegration::TearDown()
{
    // Note: owner side sharedmemory unlink won't throw
    if (m_table_shm) {
        m_table_shm->unlink();
    }
    if (m_ctl_record_shmem) {
        m_ctl_record_shmem->unlink();
    }
    if (m_ctl_status_shmem) {
        m_ctl_status_shmem->unlink();
    }
}

TEST_F(ProfileTestIntegration, enter_exit)
{
    EXPECT_CALL(*m_table, key(_)).WillOnce(Return(0xABCD));
    std::string name = "test_region";
    uint64_t hint = GEOPM_REGION_HINT_COMPUTE;
    uint64_t region_id = m_profile->region(name, hint);
    uint64_t hash = geopm_region_id_hash(region_id);

    std::vector<record_s> records;
    std::vector<short_region_s> short_regions;

    m_profile->enter(region_id);
    m_ctl_record_log->dump(records, short_regions);
    m_ctl_status->update_cache();
    ASSERT_EQ(1ull, records.size());
    EXPECT_EQ(m_process, records[0].process);
    EXPECT_EQ(geopm::EVENT_REGION_ENTRY, records[0].event);
    EXPECT_EQ(hash, records[0].signal);
    EXPECT_EQ(hint, m_ctl_status->get_hint(2));
    EXPECT_EQ(hint, m_ctl_status->get_hint(3));
    // do not change other CPUs
    EXPECT_EQ(GEOPM_REGION_HINT_INACTIVE, m_ctl_status->get_hint(0));
    EXPECT_EQ(GEOPM_REGION_HINT_INACTIVE, m_ctl_status->get_hint(1));

    m_profile->exit(region_id);
    m_ctl_record_log->dump(records, short_regions);
    m_ctl_status->update_cache();
    ASSERT_EQ(1ull, records.size());
    EXPECT_EQ(m_process, records[0].process);
    EXPECT_EQ(geopm::EVENT_REGION_EXIT, records[0].event);
    EXPECT_EQ(hash, records[0].signal);
    EXPECT_EQ(GEOPM_REGION_HINT_UNSET, m_ctl_status->get_hint(2));
    EXPECT_EQ(GEOPM_REGION_HINT_UNSET, m_ctl_status->get_hint(3));
}

TEST_F(ProfileTestIntegration, enter_exit_short)
{
    EXPECT_CALL(*m_table, key(_)).WillOnce(Return(0xABCD));
    std::string name = "test_region";
    uint64_t hint = GEOPM_REGION_HINT_COMPUTE;
    uint64_t region_id = m_profile->region(name, hint);
    uint64_t hash = geopm_region_id_hash(region_id);
    std::vector<record_s> records;
    std::vector<short_region_s> short_regions;

    m_profile->enter(region_id);
    m_profile->exit(region_id);
    m_ctl_record_log->dump(records, short_regions);
    m_ctl_status->update_cache();
    ASSERT_EQ(1ULL, records.size());
    EXPECT_EQ(m_process, records[0].process);
    EXPECT_EQ(geopm::EVENT_SHORT_REGION, records[0].event);
    EXPECT_EQ(0ULL, records[0].signal);
    ASSERT_EQ(1ULL, short_regions.size());
    EXPECT_EQ(hash, short_regions[0].hash);
    EXPECT_EQ(1, short_regions[0].num_complete);
    // exited region
    EXPECT_EQ(GEOPM_REGION_HINT_UNSET, m_ctl_status->get_hint(2));
    EXPECT_EQ(GEOPM_REGION_HINT_UNSET, m_ctl_status->get_hint(3));

    m_profile->enter(region_id);
    m_profile->exit(region_id);
    m_profile->enter(region_id);
    m_profile->exit(region_id);
    m_profile->enter(region_id);
    m_ctl_record_log->dump(records, short_regions);
    m_ctl_status->update_cache();
    ASSERT_EQ(1ULL, records.size());
    EXPECT_EQ(m_process, records[0].process);
    EXPECT_EQ(geopm::EVENT_SHORT_REGION, records[0].event);
    EXPECT_EQ(0ULL, records[0].signal);
    ASSERT_EQ(1ULL, short_regions.size());
    EXPECT_EQ(hash, short_regions[0].hash);
    EXPECT_EQ(2, short_regions[0].num_complete);
    // still in region
    EXPECT_EQ(hint, m_ctl_status->get_hint(2));
    EXPECT_EQ(hint, m_ctl_status->get_hint(3));

    m_profile->exit(region_id);
    m_ctl_record_log->dump(records, short_regions);
    m_ctl_status->update_cache();
    ASSERT_EQ(1ULL, records.size());
    EXPECT_EQ(m_process, records[0].process);
    EXPECT_EQ(geopm::EVENT_SHORT_REGION, records[0].event);
    EXPECT_EQ(0ULL, records[0].signal);
    ASSERT_EQ(1ULL, short_regions.size());
    EXPECT_EQ(hash, short_regions[0].hash);
    EXPECT_EQ(1, short_regions[0].num_complete);
    // exited region
    EXPECT_EQ(GEOPM_REGION_HINT_UNSET, m_ctl_status->get_hint(2));
    EXPECT_EQ(GEOPM_REGION_HINT_UNSET, m_ctl_status->get_hint(3));
}

TEST_F(ProfileTestIntegration, enter_exit_nested)
{
    EXPECT_CALL(*m_table, key(_)).WillOnce(Return(0xABCD));
    std::string usr_name = "usr_test_region";
    uint64_t usr_hint = GEOPM_REGION_HINT_COMPUTE;
    uint64_t usr_region_id = m_profile->region(usr_name, usr_hint);
    uint64_t usr_hash = geopm_region_id_hash(usr_region_id);

    EXPECT_CALL(*m_table, key(_)).WillOnce(Return(0x5678));
    std::string mpi_name = "mpi_test_region";
    uint64_t mpi_hint = GEOPM_REGION_HINT_NETWORK;
    uint64_t mpi_region_id = m_profile->region(mpi_name, mpi_hint);

    std::vector<record_s> records;
    std::vector<short_region_s> short_regions;

    m_profile->enter(usr_region_id);
    m_ctl_record_log->dump(records, short_regions);
    m_ctl_status->update_cache();
    ASSERT_EQ(1ULL, records.size());
    EXPECT_EQ(geopm::EVENT_REGION_ENTRY, records[0].event);
    EXPECT_EQ(usr_hash, records[0].signal);
    EXPECT_EQ(usr_hint, m_ctl_status->get_hint(2));
    EXPECT_EQ(usr_hint, m_ctl_status->get_hint(3));

    m_profile->enter(mpi_region_id);
    m_ctl_record_log->dump(records, short_regions);
    m_ctl_status->update_cache();
    // no entry for nested region
    ASSERT_EQ(0ULL, records.size());
    EXPECT_EQ(mpi_hint, m_ctl_status->get_hint(2));
    EXPECT_EQ(mpi_hint, m_ctl_status->get_hint(3));

    m_profile->exit(mpi_region_id);
    m_ctl_record_log->dump(records, short_regions);
    m_ctl_status->update_cache();
    // no exit for nested region
    ASSERT_EQ(0ULL, records.size());
    EXPECT_EQ(usr_hint, m_ctl_status->get_hint(2));
    EXPECT_EQ(usr_hint, m_ctl_status->get_hint(3));

    m_profile->exit(usr_region_id);
    m_ctl_record_log->dump(records, short_regions);
    m_ctl_status->update_cache();
    ASSERT_EQ(1ULL, records.size());
    EXPECT_EQ(geopm::EVENT_REGION_EXIT, records[0].event);
    EXPECT_EQ(usr_hash, records[0].signal);
    EXPECT_EQ(GEOPM_REGION_HINT_UNSET, m_ctl_status->get_hint(2));
    EXPECT_EQ(GEOPM_REGION_HINT_UNSET, m_ctl_status->get_hint(3));
}

TEST_F(ProfileTestIntegration, epoch)
{
    std::vector<record_s> records;
    std::vector<short_region_s> short_regions;

    m_profile->epoch();

    m_ctl_record_log->dump(records, short_regions);
    m_ctl_status->update_cache();
    ASSERT_EQ(1ULL, records.size());
    EXPECT_EQ(m_process, records[0].process);
    EXPECT_EQ(geopm::EVENT_EPOCH_COUNT, records[0].event);
    EXPECT_EQ(1ULL, records[0].signal);
    ASSERT_EQ(0ULL, short_regions.size());
}

TEST_F(ProfileTestIntegration, progress_multithread)
{
    EXPECT_CALL(*m_table, key(_)).WillOnce(Return(0xABCD));
    std::string name = "test_region";
    uint64_t hint = GEOPM_REGION_HINT_COMPUTE;
    uint64_t region_id = m_profile->region(name, hint);

    m_profile->enter(region_id);
    m_profile->thread_init(8);
    m_ctl_status->update_cache();
    EXPECT_EQ(0.0, m_ctl_status->get_progress_cpu(2));
    EXPECT_EQ(0.0, m_ctl_status->get_progress_cpu(3));

    m_profile->thread_post(3);
    m_profile->thread_post(2);
    m_ctl_status->update_cache();
    EXPECT_EQ(0.125, m_ctl_status->get_progress_cpu(2));
    EXPECT_EQ(0.125, m_ctl_status->get_progress_cpu(3));

    m_profile->thread_post(3);
    m_ctl_status->update_cache();
    EXPECT_EQ(0.125, m_ctl_status->get_progress_cpu(2));
    EXPECT_EQ(0.25, m_ctl_status->get_progress_cpu(3));

    m_profile->thread_post(2);
    m_profile->thread_post(2);
    m_profile->thread_post(3);
    m_ctl_status->update_cache();
    EXPECT_EQ(0.375, m_ctl_status->get_progress_cpu(2));
    EXPECT_EQ(0.375, m_ctl_status->get_progress_cpu(3));

    m_profile->thread_post(3);
    m_profile->thread_post(2);
    m_ctl_status->update_cache();
    EXPECT_EQ(0.5, m_ctl_status->get_progress_cpu(2));
    EXPECT_EQ(0.5, m_ctl_status->get_progress_cpu(3));
    m_profile->exit(region_id);
}
