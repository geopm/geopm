/*
 * Copyright (c) 2015 - 2023, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include "config.h"

#include <sched.h>
#include <unistd.h>

#include <cstring>
#include <fstream>
#include <functional>
#include <memory>

#include "gmock/gmock.h"
#include "gtest/gtest.h"

#include "geopm_prof.h"
#include "geopm_hint.h"
#include "Profile.hpp"

#include "geopm_test.hpp"
#include "MockComm.hpp"
#include "MockApplicationRecordLog.hpp"
#include "MockApplicationStatus.hpp"
#include "MockServiceProxy.hpp"
#include "MockScheduler.hpp"


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
        std::shared_ptr<MockServiceProxy> m_service_proxy;
        std::unique_ptr<Profile> m_profile;
        std::shared_ptr<MockScheduler> m_scheduler;
};

void ProfileTest::SetUp()
{
    m_record_log = std::make_shared<MockApplicationRecordLog>();
    m_status = std::make_shared<MockApplicationStatus>();
    m_service_proxy = std::make_shared<MockServiceProxy>();
    EXPECT_CALL(*m_service_proxy, platform_start_profile("profile"));
    EXPECT_CALL(*m_service_proxy, platform_stop_profile(_));

    EXPECT_CALL(*m_status, set_hash(_, GEOPM_REGION_HASH_UNMARKED, GEOPM_REGION_HINT_UNSET));

    m_scheduler = std::make_shared<MockScheduler>();
    EXPECT_CALL(*m_scheduler, num_cpu())
       .WillRepeatedly(Return(4));
    EXPECT_CALL(*m_scheduler, proc_cpuset())
       .WillRepeatedly([](){return geopm::make_cpu_set(4, {2,3});});

    m_profile = geopm::make_unique<ProfileImp>("profile",
                                               "report",
                                               M_NUM_CPU,
                                               m_cpu_list,
                                               m_status,
                                               m_record_log,
                                               true,
                                               m_service_proxy,
                                               m_scheduler);
}

TEST_F(ProfileTest, enter_exit)
{
    std::string name = "test_region";
    uint64_t hint = GEOPM_REGION_HINT_COMPUTE;
    uint64_t region_id = m_profile->region(name, hint);
    uint64_t hash = geopm_region_id_hash(region_id);

    EXPECT_CALL(*m_record_log, enter(hash, _));
    EXPECT_CALL(*m_status, set_hash(2, hash, hint));
    EXPECT_CALL(*m_status, set_hash(3, hash, hint));
    m_profile->enter(region_id);

    EXPECT_CALL(*m_record_log, exit(hash, _));
    // hint is cleared when exiting top-level region
    EXPECT_CALL(*m_status, set_hash(2, GEOPM_REGION_HASH_UNMARKED, GEOPM_REGION_HINT_UNSET));
    EXPECT_CALL(*m_status, set_hash(3, GEOPM_REGION_HASH_UNMARKED, GEOPM_REGION_HINT_UNSET));
    // progress is cleared when exiting top-level region
    EXPECT_CALL(*m_status, reset_work_units(2));
    EXPECT_CALL(*m_status, reset_work_units(3));

    m_profile->exit(region_id);
}

// TODO: get rid of GEOPM_REGION_ID_MPI, epoch bit if still there
// TODO: fix geopm_mpi_region_enter/exit to set hint instead and
//       get rid of extra entry into GEOPM_REGION_ID_MPI

TEST_F(ProfileTest, enter_exit_nested)
{
    std::string usr_name = "usr_test_region";
    uint64_t usr_hint = GEOPM_REGION_HINT_COMPUTE;
    uint64_t usr_region_id = m_profile->region(usr_name, usr_hint);
    uint64_t usr_hash = geopm_region_id_hash(usr_region_id);

    std::string mpi_name = "mpi_test_region";
    uint64_t mpi_hint = GEOPM_REGION_HINT_NETWORK;
    uint64_t mpi_region_id = m_profile->region(mpi_name, mpi_hint);

    {
        // enter region and set hint
        EXPECT_CALL(*m_record_log, enter(usr_hash, _));
        EXPECT_CALL(*m_status, set_hash(2, usr_hash, usr_hint));
        EXPECT_CALL(*m_status, set_hash(3, usr_hash, usr_hint));
        m_profile->enter(usr_region_id);
    }
    {
        // don't enter a nested region, just update hint
        EXPECT_CALL(*m_record_log, enter(_, _)).Times(0);
        EXPECT_CALL(*m_status, set_hash(_, _, _)).Times(0);
        EXPECT_CALL(*m_status, set_hint(2, mpi_hint));
        EXPECT_CALL(*m_status, set_hint(3, mpi_hint));
        m_profile->enter(mpi_region_id);
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
        EXPECT_CALL(*m_status, set_hash(2, GEOPM_REGION_HASH_UNMARKED, GEOPM_REGION_HINT_UNSET));
        EXPECT_CALL(*m_status, set_hash(3, GEOPM_REGION_HASH_UNMARKED, GEOPM_REGION_HINT_UNSET));
        EXPECT_CALL(*m_status, reset_work_units(2));
        EXPECT_CALL(*m_status, reset_work_units(3));
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
    std::string name = "test_region";
    uint64_t hint = GEOPM_REGION_HINT_COMPUTE;
    uint64_t region_id = m_profile->region(name, hint);
    uint64_t hash = geopm_region_id_hash(region_id);
    {
        EXPECT_CALL(*m_record_log, enter(hash, _));
        EXPECT_CALL(*m_status, set_hash(_, _, _)).Times(2);
        m_profile->enter(region_id);
    }
    {
        EXPECT_CALL(*m_status, set_total_work_units(2, 6));
        EXPECT_CALL(*m_status, set_total_work_units(3, 6));
        m_profile->thread_init(6);
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
        EXPECT_CALL(*m_status, set_hash(_, _, _)).Times(2);
        // clear progress when exiting
        EXPECT_CALL(*m_status, reset_work_units(2));
        EXPECT_CALL(*m_status, reset_work_units(3));
        m_profile->exit(region_id);
    }
    // TODO: make it an error to set values for other CPUs not
    // assigned to this process.  Does it also make sense to provide
    // an API without CPU that calls through to all CPUs in cpu_set
    // for the Profile object?
}
