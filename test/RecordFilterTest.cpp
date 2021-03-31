/*
 * Copyright (c) 2015 - 2021, Intel Corporation
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

#include <memory>

#include "gtest/gtest.h"
#include "geopm_test.hpp"

#include "RecordFilter.hpp"
#include "record.hpp"

using geopm::RecordFilter;
using geopm::record_s;

class RecordFilterTest : public ::testing::Test
{

};


TEST_F(RecordFilterTest, invalid_filter_name)
{
    GEOPM_EXPECT_THROW_MESSAGE(RecordFilter::make_unique("invalid_filter_name"),
                               GEOPM_ERROR_INVALID,"parse name");
}

TEST_F(RecordFilterTest, make_proxy_epoch)
{
    std::shared_ptr<RecordFilter> filter = RecordFilter::make_unique("proxy_epoch,0xabcd1234");
    // Assert that the pointer is non-null
    ASSERT_TRUE(filter);
    record_s record {0.0, 0, geopm::EVENT_REGION_ENTRY, 0xabcd1234};
    std::vector<record_s> result = filter->filter(record);
    ASSERT_EQ(2ULL, result.size());
    EXPECT_EQ(geopm::EVENT_REGION_ENTRY, result[0].event);
    EXPECT_EQ(geopm::EVENT_EPOCH_COUNT, result[1].event);
}

TEST_F(RecordFilterTest, make_edit_distance)
{
    std::shared_ptr<RecordFilter> filter = RecordFilter::make_unique("edit_distance,10");
    // Assert that the pointer is non-null
    ASSERT_TRUE(filter);
    record_s record {0.0, 0, geopm::EVENT_REGION_ENTRY, 0xabcd1234};
    std::vector<record_s> result = filter->filter(record);
    ASSERT_EQ(1ULL, result.size());
    EXPECT_EQ(geopm::EVENT_REGION_ENTRY, result[0].event);
}
