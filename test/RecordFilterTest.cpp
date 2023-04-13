/*
 * Copyright (c) 2015 - 2023, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
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
    record_s record {{{0, 0}}, 0, geopm::EVENT_REGION_ENTRY, 0xabcd1234};
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
    record_s record {{{0, 0}}, 0, geopm::EVENT_REGION_ENTRY, 0xabcd1234};
    std::vector<record_s> result = filter->filter(record);
    ASSERT_EQ(1ULL, result.size());
    EXPECT_EQ(geopm::EVENT_REGION_ENTRY, result[0].event);
}
