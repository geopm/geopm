/*
 * Copyright (c) 2015 - 2023, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "gtest/gtest.h"
#include "gmock/gmock.h"
#include "geopm_test.hpp"

#include "geopm_time.h"
#include "ApplicationRecordLog.hpp"
#include "record.hpp"
#include "MockSharedMemory.hpp"
#include "MockScheduler.hpp"

using geopm::ApplicationRecordLog;
using geopm::ApplicationRecordLogImp;
using geopm::SharedMemory;
using geopm::record_s;
using geopm::short_region_s;

using testing::AtLeast;

class ApplicationRecordLogTest : public ::testing::Test
{
    protected:
        void SetUp();
        std::shared_ptr<MockSharedMemory> m_mock_shared_memory;
        std::unique_ptr<ApplicationRecordLog> m_record_log;
        const int M_PROC_ID = 123;
        std::shared_ptr<MockScheduler> m_scheduler;
        // Note time_zero is one second after 1970
};

void ApplicationRecordLogTest::SetUp()
{
    size_t buffer_size = ApplicationRecordLog::buffer_size();
    m_mock_shared_memory = std::make_shared<MockSharedMemory>(buffer_size);
    m_scheduler = std::make_shared<MockScheduler>();
    m_record_log.reset(new ApplicationRecordLogImp(m_mock_shared_memory, M_PROC_ID, m_scheduler));
    //m_record_log = ApplicationRecordLog::make_unique(m_mock_shared_memory);

    EXPECT_CALL(*m_mock_shared_memory, get_scoped_lock()).Times(AtLeast(0));
}

TEST_F(ApplicationRecordLogTest, bad_shmem)
{
    size_t buffer_size = ApplicationRecordLog::buffer_size();
    std::shared_ptr<MockSharedMemory> shmem = std::make_shared<MockSharedMemory>(buffer_size - 1);
    GEOPM_EXPECT_THROW_MESSAGE(ApplicationRecordLog::make_unique(shmem),
                               GEOPM_ERROR_INVALID,
                               "Shared memory provided in constructor is too small");
}

TEST_F(ApplicationRecordLogTest, get_sizes)
{
    size_t buffer = ApplicationRecordLog::buffer_size();
    size_t record = ApplicationRecordLog::max_record();
    size_t region = ApplicationRecordLog::max_region();
    EXPECT_LT(0ULL, buffer);
    EXPECT_LT(0ULL, record);
    EXPECT_LT(0ULL, region);
    EXPECT_LT(record, region);
    EXPECT_GT(buffer, region * sizeof(geopm::short_region_s) + record * sizeof(geopm::record_s));
}

TEST_F(ApplicationRecordLogTest, empty_dump)
{
    std::vector<record_s> records;
    std::vector<short_region_s> short_regions;
    EXPECT_CALL(*m_mock_shared_memory, get_scoped_lock())
        .Times(1);
    m_record_log->dump(records, short_regions);
    EXPECT_EQ(0ULL, records.size());
    EXPECT_EQ(0ULL, short_regions.size());
}

TEST_F(ApplicationRecordLogTest, scoped_lock_test)
{
    uint64_t hash = 0x1234abcd;
    geopm_time_s time = {{2, 0}};

    {
        EXPECT_CALL(*m_mock_shared_memory, get_scoped_lock())
            .Times(1);
        m_record_log->enter(hash, time);
    }
    {
        EXPECT_CALL(*m_mock_shared_memory, get_scoped_lock())
            .Times(1);
        m_record_log->exit(hash, time);
    }
    {
        EXPECT_CALL(*m_mock_shared_memory, get_scoped_lock())
            .Times(1);
        m_record_log->epoch(time);
    }
    {
        std::vector<record_s> records;
        std::vector<short_region_s> short_regions;
        EXPECT_CALL(*m_mock_shared_memory, get_scoped_lock())
            .Times(1);
        m_record_log->dump(records, short_regions);
    }

}

TEST_F(ApplicationRecordLogTest, one_entry)
{
    std::vector<record_s> records;
    std::vector<short_region_s> short_regions;
    int M_PROC_ID = 123;
    uint64_t hash = 0x1234abcd;
    geopm_time_s time = {{2, 0}};

    m_record_log->enter(hash, time);
    m_record_log->dump(records, short_regions);
    EXPECT_EQ(0ULL, short_regions.size());
    ASSERT_EQ(1ULL, records.size());
    EXPECT_EQ(time.t.tv_sec, records[0].time.t.tv_sec);
    EXPECT_EQ(time.t.tv_nsec, records[0].time.t.tv_nsec);
    EXPECT_EQ(M_PROC_ID, records[0].process);
    EXPECT_EQ(geopm::EVENT_REGION_ENTRY, records[0].event);
    EXPECT_EQ(hash, records[0].signal);
}

TEST_F(ApplicationRecordLogTest, one_exit)
{
    std::vector<record_s> records;
    std::vector<short_region_s> short_regions;
    int M_PROC_ID = 123;
    uint64_t hash = 0x1234abcd;
    geopm_time_s time_1 = {{2, 0}};
    geopm_time_s time_2 = {{3, 0}};

    m_record_log->enter(hash, time_1);
    m_record_log->dump(records, short_regions);

    m_record_log->exit(hash, time_2);
    m_record_log->dump(records, short_regions);
    EXPECT_EQ(0ULL, short_regions.size());
    ASSERT_EQ(1ULL, records.size());
    EXPECT_EQ(time_2.t.tv_sec, records[0].time.t.tv_sec);
    EXPECT_EQ(time_2.t.tv_nsec, records[0].time.t.tv_nsec);
    EXPECT_EQ(M_PROC_ID, records[0].process);
    EXPECT_EQ(geopm::EVENT_REGION_EXIT, records[0].event);
    EXPECT_EQ(hash, records[0].signal);
}

TEST_F(ApplicationRecordLogTest, one_epoch)
{
    std::vector<record_s> records;
    std::vector<short_region_s> short_regions;
    int M_PROC_ID = 123;
    geopm_time_s time = {{2, 0}};

    m_record_log->epoch(time);
    m_record_log->dump(records, short_regions);
    EXPECT_EQ(0ULL, short_regions.size());
    ASSERT_EQ(1ULL, records.size());
    EXPECT_EQ(time.t.tv_sec, records[0].time.t.tv_sec);
    EXPECT_EQ(time.t.tv_nsec, records[0].time.t.tv_nsec);
    EXPECT_EQ(M_PROC_ID, records[0].process);
    EXPECT_EQ(geopm::EVENT_EPOCH_COUNT, records[0].event);
    EXPECT_EQ(1ULL, records[0].signal);
}

TEST_F(ApplicationRecordLogTest, short_region_entry_exit)
{
    std::vector<record_s> records;
    std::vector<short_region_s> short_regions;
    int M_PROC_ID = 123;
    uint64_t hash = 0x1234abcd;
    geopm_time_s time_entry1 = {{2, 0}};
    geopm_time_s time_exit1 = {{3, 0}};
    geopm_time_s time_entry2 = {{5, 0}};
    geopm_time_s time_exit2 = {{7, 0}};

    m_record_log->enter(hash, time_entry1);
    m_record_log->exit(hash, time_exit1);
    m_record_log->enter(hash, time_entry2);
    m_record_log->exit(hash, time_exit2);
    m_record_log->dump(records, short_regions);

    ASSERT_EQ(1ULL, records.size());
    EXPECT_EQ(time_entry1.t.tv_sec, records[0].time.t.tv_sec);
    EXPECT_EQ(time_entry1.t.tv_nsec, records[0].time.t.tv_nsec);
    EXPECT_EQ(M_PROC_ID, records[0].process);
    EXPECT_EQ(geopm::EVENT_SHORT_REGION, records[0].event);
    EXPECT_EQ(0ULL, records[0].signal);
    ASSERT_EQ(1ULL, short_regions.size());
    EXPECT_EQ(hash, short_regions[0].hash);
    EXPECT_EQ(2, short_regions[0].num_complete);
    EXPECT_EQ(3.0, short_regions[0].total_time);
}

TEST_F(ApplicationRecordLogTest, dump_twice)
{
    std::vector<record_s> records;
    std::vector<short_region_s> short_regions;
    geopm_time_s time_1 = {{2, 0}};
    geopm_time_s time_2 = {{3, 0}};

    m_record_log->enter(0x1234, time_1);
    m_record_log->exit(0x1234, time_2);
    m_record_log->epoch(time_2);

    m_record_log->dump(records, short_regions);
    EXPECT_EQ(2ULL, records.size());
    EXPECT_EQ(1ULL, short_regions.size());

    m_record_log->dump(records, short_regions);
    EXPECT_EQ(0ULL, records.size());
    EXPECT_EQ(0ULL, short_regions.size());
}

TEST_F(ApplicationRecordLogTest, dump_within_region)
{
    // This test shows the case where a region has been entered and
    // exited between two calls to dump to create a short region
    // event.  Additionally, in this case the region is entered and
    // then an epoch call is made prior to a call to dump(). Here the
    // entry call will not be noted by the caller of dump(), but
    // instead the closing of this region will be replaced by a short
    // region event in the subsequent call to dump().
    std::vector<record_s> records;
    std::vector<short_region_s> short_regions;
    uint64_t hash = 0xABCD;

    m_record_log->enter(hash, {2, 0});
    m_record_log->exit(hash, {3, 0});
    m_record_log->enter(hash, {4, 0});
    m_record_log->epoch({5, 0});
    m_record_log->dump(records, short_regions);
    ASSERT_EQ(2ULL, records.size());
    EXPECT_EQ(geopm::EVENT_SHORT_REGION, records[0].event);
    EXPECT_EQ(geopm::EVENT_EPOCH_COUNT, records[1].event);
    EXPECT_EQ(0ULL, records[0].signal);  // short region index
    EXPECT_EQ(1ULL, records[1].signal);  // epoch count
    ASSERT_EQ(1ULL, short_regions.size());
    EXPECT_EQ(hash, short_regions[0].hash);
    EXPECT_EQ(1, short_regions[0].num_complete);
    EXPECT_EQ(1.0, short_regions[0].total_time);

    m_record_log->epoch({6, 0});
    m_record_log->dump(records, short_regions);
    ASSERT_EQ(1ULL, records.size());
    EXPECT_EQ(geopm::EVENT_EPOCH_COUNT, records[0].event);
    EXPECT_EQ(2ULL, records[0].signal);
    ASSERT_EQ(0ULL, short_regions.size());

    m_record_log->epoch({7, 0});
    m_record_log->exit(hash, {8, 0});
    m_record_log->dump(records, short_regions);
    ASSERT_EQ(2ULL, records.size());
    EXPECT_EQ(geopm::EVENT_EPOCH_COUNT, records[0].event);
    EXPECT_EQ(geopm::EVENT_SHORT_REGION, records[1].event);
    EXPECT_EQ(3ULL, records[0].signal);
    EXPECT_EQ(0ULL, records[1].signal);
    ASSERT_EQ(1ULL, short_regions.size());
    EXPECT_EQ(hash, short_regions[0].hash);
    EXPECT_EQ(1, short_regions[0].num_complete);
    EXPECT_EQ(4.0, short_regions[0].total_time);
}

TEST_F(ApplicationRecordLogTest, overflow_record_table)
{
    std::vector<record_s> records;
    std::vector<short_region_s> short_regions;

    int max_size = 1024;
    for (int ii = 0; ii < max_size; ++ii) {
        m_record_log->epoch({ii, 0});
    }
    GEOPM_EXPECT_THROW_MESSAGE(m_record_log->epoch({max_size, 0}),
                               GEOPM_ERROR_RUNTIME, "maximum number of records reached");
}

TEST_F(ApplicationRecordLogTest, cannot_overflow_region_table)
{
    std::vector<record_s> records;
    std::vector<short_region_s> short_regions;
    uint64_t hash = 0xABCD;

    m_record_log->enter(hash, {2, 0});
    m_record_log->exit(hash, {3, 0});
    m_record_log->enter(hash, {4, 0});
    m_record_log->dump(records, short_regions);

    m_record_log->exit(hash, {5, 0});
    int max_size = 1024;
    for (int ii = 0; ii < max_size; ++ii) {
        m_record_log->enter(hash+ii, {6+ii, 0});
        m_record_log->exit(hash+ii, {6+ii, 0});
    }
    GEOPM_EXPECT_THROW_MESSAGE(m_record_log->enter(hash+max_size, {6+max_size, 0}),
                               GEOPM_ERROR_RUNTIME, "maximum number of records reached");
}
