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

#include <cstdint>
#include "gtest/gtest.h"
#include "geopm_test.hpp"

#include "ValidateRecord.hpp"
#include "ApplicationSampler.hpp"
#include "geopm.h"

using geopm::ApplicationSampler;
using geopm::ValidateRecord;

class ValidateRecordTest : public ::testing::Test
{
    protected:
        void SetUp();
        ValidateRecord m_filter;
        ApplicationSampler::m_record_s m_record;
};


void ValidateRecordTest::SetUp()
{
    m_record.time = 2020.0;
    m_record.process = 42;
    m_record.event = ApplicationSampler::M_EVENT_HINT;
    m_record.signal = GEOPM_REGION_HINT_UNKNOWN;
}

TEST_F(ValidateRecordTest, valid_stream)
{
    m_filter.check(m_record);

    m_record.time += 1.0;
    m_record.event = ApplicationSampler::M_EVENT_REGION_ENTRY;
    m_record.signal = 0xabcd1234;
    m_filter.check(m_record);

    m_record.time += 1.0;
    m_record.event = ApplicationSampler::M_EVENT_REGION_EXIT;
    m_record.signal = 0xabcd1234;
    m_filter.check(m_record);

    m_record.time += 1.0;
    m_record.event = ApplicationSampler::M_EVENT_EPOCH_COUNT;
    m_record.signal = 1;
    m_filter.check(m_record);

    m_record.time += 1.0;
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

TEST_F(ValidateRecordTest, hint_invalid)
{
    m_record.signal = 1ULL;
    GEOPM_EXPECT_THROW_MESSAGE(m_filter.check(m_record),
                               GEOPM_ERROR_INVALID,
                               "Hint out of range");
    m_record.signal = GEOPM_REGION_HINT_NETWORK |
                      GEOPM_REGION_HINT_COMPUTE;
    GEOPM_EXPECT_THROW_MESSAGE(m_filter.check(m_record),
                               GEOPM_ERROR_INVALID,
                               "Hint out of range");
}

TEST_F(ValidateRecordTest, entry_exit_paired)
{
    m_record.event = ApplicationSampler::M_EVENT_REGION_ENTRY;
    m_record.signal = 0xabcd1234ULL;
    m_filter.check(m_record);

    m_record.time += 1.0;
    m_record.event = ApplicationSampler::M_EVENT_REGION_EXIT;
    m_filter.check(m_record);
}

TEST_F(ValidateRecordTest, entry_exit_unpaired)
{
    m_record.event = ApplicationSampler::M_EVENT_REGION_ENTRY;
    m_record.signal = 0xabcd1234ULL;
    m_filter.check(m_record);

    m_record.time += 1.0;
    m_record.event = ApplicationSampler::M_EVENT_REGION_EXIT;
    m_record.signal = 0xabcd1235ULL;
    GEOPM_EXPECT_THROW_MESSAGE(m_filter.check(m_record),
                               GEOPM_ERROR_INVALID,
                               "Region exited differs from last region entered");
}

TEST_F(ValidateRecordTest, double_entry)
{
    m_record.event = ApplicationSampler::M_EVENT_REGION_ENTRY;
    m_record.signal = 0xabcd1234ULL;
    m_filter.check(m_record);

    m_record.time += 1.0;
    m_record.event = ApplicationSampler::M_EVENT_REGION_ENTRY;
    m_record.signal = 0xabcd1235ULL;
    GEOPM_EXPECT_THROW_MESSAGE(m_filter.check(m_record),
                               GEOPM_ERROR_INVALID,
                               "Nested region entry detected");
}

TEST_F(ValidateRecordTest, exit_without_entry)
{
    m_record.event = ApplicationSampler::M_EVENT_REGION_EXIT;
    m_record.signal = 0xabcd1234ULL;
    GEOPM_EXPECT_THROW_MESSAGE(m_filter.check(m_record),
                               GEOPM_ERROR_INVALID,
                               "Region exit without entry");
}

TEST_F(ValidateRecordTest, entry_exit_invalid_hash)
{
    m_record.event = ApplicationSampler::M_EVENT_REGION_ENTRY;
    m_record.signal = UINT32_MAX + 1;
    GEOPM_EXPECT_THROW_MESSAGE(m_filter.check(m_record),
                               GEOPM_ERROR_INVALID,
                               "Region hash out of bounds");
}

TEST_F(ValidateRecordTest, epoch_count_monotone)
{
    m_record.event = ApplicationSampler::M_EVENT_EPOCH_COUNT;
    m_record.signal = 1;
    m_filter.check(m_record);

    m_record.time += 1.0;
    GEOPM_EXPECT_THROW_MESSAGE(m_filter.check(m_record),
                               GEOPM_ERROR_INVALID,
                               "Epoch count not monotone and contiguous");
}

TEST_F(ValidateRecordTest, epoch_count_gap)
{
    m_record.event = ApplicationSampler::M_EVENT_EPOCH_COUNT;
    m_record.signal = 1;
    m_filter.check(m_record);

    m_record.time += 1.0;
    m_record.signal += 2;
    GEOPM_EXPECT_THROW_MESSAGE(m_filter.check(m_record),
                               GEOPM_ERROR_INVALID,
                               "Epoch count not monotone and contiguous");
}


TEST_F(ValidateRecordTest, time_monotone)
{
    m_filter.check(m_record);

    m_record.time -= 1.0;
    m_record.event = ApplicationSampler::M_EVENT_REGION_ENTRY;
    m_record.signal = 0xabcd1234;
    GEOPM_EXPECT_THROW_MESSAGE(m_filter.check(m_record),
                               GEOPM_ERROR_INVALID,
                               "Time value decreased");
}
