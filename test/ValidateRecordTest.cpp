/*
 * Copyright (c) 2015 - 2023, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <cstdint>
#include "gtest/gtest.h"
#include "geopm_test.hpp"

#include "ValidateRecord.hpp"
#include "record.hpp"
#include "geopm_prof.h"
#include "geopm_hint.h"

using geopm::ValidateRecord;
using geopm::record_s;

class ValidateRecordTest : public ::testing::Test
{
    protected:
        void SetUp();
        ValidateRecord m_filter;
        record_s m_record;
};


void ValidateRecordTest::SetUp()
{
    m_record.time = {{2020, 0}};
    m_record.process = 42;
    m_record.event = -1;
    m_record.signal = GEOPM_REGION_HINT_UNKNOWN;
}

TEST_F(ValidateRecordTest, valid_stream)
{
    m_filter.check(m_record);

    m_record.time.t.tv_sec += 1;
    m_record.event = geopm::EVENT_REGION_ENTRY;
    m_record.signal = 0xabcd1234;
    m_filter.check(m_record);

    m_record.time.t.tv_sec += 1;
    m_record.event = geopm::EVENT_REGION_EXIT;
    m_record.signal = 0xabcd1234;
    m_filter.check(m_record);

    m_record.time.t.tv_sec += 1;
    m_record.event = geopm::EVENT_EPOCH_COUNT;
    m_record.signal = 1;
    m_filter.check(m_record);

    m_record.time.t.tv_sec += 1;
    m_record.event = geopm::EVENT_SHORT_REGION;
    m_record.signal = 2;
    m_filter.check(m_record);

    m_record.time.t.tv_sec += 1;
    m_record.signal += 1;
    m_filter.check(m_record);
}

TEST_F(ValidateRecordTest, process_change)
{
    m_filter.check(m_record);
    m_record.process = 1024;
    GEOPM_EXPECT_THROW_MESSAGE(m_filter.check(m_record),
                               GEOPM_ERROR_INVALID,
                               "Process has changed");
}

TEST_F(ValidateRecordTest, entry_exit_paired)
{
    m_record.event = geopm::EVENT_REGION_ENTRY;
    m_record.signal = 0xabcd1234ULL;
    m_filter.check(m_record);

    m_record.time.t.tv_sec += 1;
    m_record.event = geopm::EVENT_REGION_EXIT;
    m_filter.check(m_record);
}

TEST_F(ValidateRecordTest, entry_exit_unpaired)
{
    m_record.event = geopm::EVENT_REGION_ENTRY;
    m_record.signal = 0xabcd1234ULL;
    m_filter.check(m_record);

    m_record.time.t.tv_sec += 1;
    m_record.event = geopm::EVENT_REGION_EXIT;
    m_record.signal = 0xabcd1235ULL;
    GEOPM_EXPECT_THROW_MESSAGE(m_filter.check(m_record),
                               GEOPM_ERROR_INVALID,
                               "Region exited differs from last region entered");
}

TEST_F(ValidateRecordTest, double_entry)
{
    m_record.event = geopm::EVENT_REGION_ENTRY;
    m_record.signal = 0xabcd1234ULL;
    m_filter.check(m_record);

    m_record.time.t.tv_sec += 1;
    m_record.event = geopm::EVENT_REGION_ENTRY;
    m_record.signal = 0xabcd1235ULL;
    GEOPM_EXPECT_THROW_MESSAGE(m_filter.check(m_record),
                               GEOPM_ERROR_INVALID,
                               "Nested region entry detected");
}

TEST_F(ValidateRecordTest, exit_without_entry)
{
    m_record.event = geopm::EVENT_REGION_EXIT;
    m_record.signal = 0xabcd1234ULL;
    GEOPM_EXPECT_THROW_MESSAGE(m_filter.check(m_record),
                               GEOPM_ERROR_INVALID,
                               "Region exit without entry");
}

TEST_F(ValidateRecordTest, entry_exit_invalid_hash)
{
    m_record.event = geopm::EVENT_REGION_ENTRY;
    m_record.signal = UINT32_MAX + 1;
    GEOPM_EXPECT_THROW_MESSAGE(m_filter.check(m_record),
                               GEOPM_ERROR_INVALID,
                               "Region hash out of bounds");
}

TEST_F(ValidateRecordTest, epoch_count_monotone)
{
    m_record.event = geopm::EVENT_EPOCH_COUNT;
    m_record.signal = 1;
    m_filter.check(m_record);

    m_record.time.t.tv_sec += 1;
    GEOPM_EXPECT_THROW_MESSAGE(m_filter.check(m_record),
                               GEOPM_ERROR_INVALID,
                               "Epoch count not monotone and contiguous");
}

TEST_F(ValidateRecordTest, epoch_count_gap)
{
    m_record.event = geopm::EVENT_EPOCH_COUNT;
    m_record.signal = 1;
    m_filter.check(m_record);

    m_record.time.t.tv_sec += 1;
    m_record.signal += 2;
    GEOPM_EXPECT_THROW_MESSAGE(m_filter.check(m_record),
                               GEOPM_ERROR_INVALID,
                               "Epoch count not monotone and contiguous");
}


TEST_F(ValidateRecordTest, time_monotone)
{
    m_filter.check(m_record);

    m_record.time.t.tv_sec -= 1;
    m_record.event = geopm::EVENT_REGION_ENTRY;
    m_record.signal = 0xabcd1234;
    GEOPM_EXPECT_THROW_MESSAGE(m_filter.check(m_record),
                               GEOPM_ERROR_INVALID,
                               "Time value decreased");
}
