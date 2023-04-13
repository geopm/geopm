/*
 * Copyright (c) 2015 - 2023, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <memory>

#include "gtest/gtest.h"
#include "gmock/gmock.h"

#include "ProcessRegionAggregator.hpp"
#include "record.hpp"
#include "geopm_test.hpp"
#include "MockApplicationSampler.hpp"

using geopm::ProcessRegionAggregator;
using geopm::ProcessRegionAggregatorImp;
using geopm::record_s;
using geopm::short_region_s;
using geopm::EVENT_REGION_ENTRY;
using geopm::EVENT_REGION_EXIT;
using geopm::EVENT_SHORT_REGION;
using testing::Return;

class ProcessRegionAggregatorTest : public ::testing::Test
{
    protected:
        void SetUp();

        MockApplicationSampler m_app_sampler;
        std::shared_ptr<ProcessRegionAggregator> m_account;
        int m_num_process = 4;
        double m_epsilon = 1e-7;
};

void ProcessRegionAggregatorTest::SetUp()
{
    EXPECT_CALL(m_app_sampler, client_pids())
        .WillOnce(Return(std::vector<int>({11, 12, 13, 14})));

    m_account = std::make_shared<ProcessRegionAggregatorImp>(m_app_sampler);
}

TEST_F(ProcessRegionAggregatorTest, entry_exit)
{
    std::vector<record_s> records;
    {
        // enter region
        records = {
            {{{1, 0}}, 12, EVENT_REGION_ENTRY, 0xDADA}
        };
        m_app_sampler.inject_records(records);
        m_account->update();
        EXPECT_DOUBLE_EQ(0.0, m_account->get_runtime_average(0xDADA));
        EXPECT_DOUBLE_EQ(0.0, m_account->get_count_average(0xDADA));
    }
    {
        // exit region
        records = {
            {{{2, 600000000}}, 12, EVENT_REGION_EXIT, 0xDADA}
        };
        m_app_sampler.inject_records(records);
        m_account->update();
        EXPECT_NEAR(0.4, m_account->get_runtime_average(0xDADA), m_epsilon);
        EXPECT_NEAR(0.25, m_account->get_count_average(0xDADA), m_epsilon);
    }
}

TEST_F(ProcessRegionAggregatorTest, short_region)
{
    std::vector<record_s> records;
    short_region_s short_region;

    {
        records = {
            {{{1, 0}}, 12, EVENT_SHORT_REGION, 0}
        };
        short_region = {0xDADA, 2, 1.0};
        m_app_sampler.inject_records(records);
        EXPECT_CALL(m_app_sampler, get_short_region(0))
            .WillOnce(Return(short_region));
        m_account->update();
        // average across 4 processes
        EXPECT_NEAR(0.25, m_account->get_runtime_average(0xDADA), m_epsilon);
        EXPECT_NEAR(0.5, m_account->get_count_average(0xDADA), m_epsilon);
    }
    {
        records = {
            {{{2, 0}}, 12, EVENT_SHORT_REGION, 0}
        };
        short_region = {0xDADA, 1, 0.5};
        m_app_sampler.inject_records(records);
        EXPECT_CALL(m_app_sampler, get_short_region(0))
            .WillOnce(Return(short_region));
        m_account->update();
        EXPECT_NEAR(1.5 / m_num_process, m_account->get_runtime_average(0xDADA), m_epsilon);
        EXPECT_NEAR(0.75, m_account->get_count_average(0xDADA), m_epsilon);

    }
}

TEST_F(ProcessRegionAggregatorTest, multiple_processes)
{
    std::vector<record_s> records;
    short_region_s short_region;

    {
        // enter region
        records = {
            {{{1, 100000000}}, 11, EVENT_REGION_ENTRY, 0xDADA},
            {{{1, 200000000}}, 12, EVENT_REGION_ENTRY, 0xDADA},
            {{{1, 300000000}}, 13, EVENT_REGION_ENTRY, 0xDADA},
            {{{1, 400000000}}, 14, EVENT_REGION_ENTRY, 0xDADA},
        };
        m_app_sampler.inject_records(records);
        m_account->update();
        EXPECT_NEAR(0.0, m_account->get_runtime_average(0xDADA), m_epsilon);
        EXPECT_NEAR(0.0, m_account->get_count_average(0xDADA), m_epsilon);
        EXPECT_NEAR(0.0, m_account->get_runtime_average(0xBEAD), m_epsilon);
        EXPECT_NEAR(0.0, m_account->get_count_average(0xBEAD), m_epsilon);
    }
    {
        records = {
            {{{2, 200000000}}, 11, EVENT_REGION_EXIT, 0xDADA},
            {{{2, 400000000}}, 11, EVENT_SHORT_REGION, 0},
            {{{2, 0}}, 12, EVENT_SHORT_REGION, 1},
            {{{2, 0}}, 13, EVENT_SHORT_REGION, 2},
            {{{2, 0}}, 14, EVENT_SHORT_REGION, 3},
            {{{2, 800000000}}, 14, EVENT_REGION_EXIT, 0xDADA},
        };
        m_app_sampler.inject_records(records);
        short_region = {0xBEAD, 2, 0.15};
        EXPECT_CALL(m_app_sampler, get_short_region(0))
            .WillOnce(Return(short_region));
        short_region = {0xBEAD, 2, 0.25};
        EXPECT_CALL(m_app_sampler, get_short_region(1))
            .WillOnce(Return(short_region));
        short_region = {0xBEAD, 1, 0.35};
        EXPECT_CALL(m_app_sampler, get_short_region(2))
            .WillOnce(Return(short_region));
        short_region = {0xBEAD, 1, 0.45};
        EXPECT_CALL(m_app_sampler, get_short_region(3))
            .WillOnce(Return(short_region));

        m_account->update();
        EXPECT_NEAR((1.1 + 1.4) / m_num_process, m_account->get_runtime_average(0xDADA), m_epsilon);
        EXPECT_NEAR(2.0 / m_num_process, m_account->get_count_average(0xDADA), m_epsilon);

        EXPECT_NEAR((0.15 + 0.25 + 0.35 + 0.45) / m_num_process,
                  m_account->get_runtime_average(0xBEAD), m_epsilon);
        EXPECT_NEAR( 6.0 / m_num_process, m_account->get_count_average(0xBEAD), m_epsilon);
    }
    {
        records = {
            {{{3, 200000000}}, 12, EVENT_REGION_EXIT, 0xDADA},
            {{{3, 300000000}}, 13, EVENT_REGION_EXIT, 0xDADA},
            {{{2, 0}}, 12, EVENT_SHORT_REGION, 0},
            {{{2, 0}}, 13, EVENT_SHORT_REGION, 1},
        };
        m_app_sampler.inject_records(records);
        short_region = {0xBEAD, 1, 0.15};
        EXPECT_CALL(m_app_sampler, get_short_region(0))
            .WillOnce(Return(short_region));
        short_region = {0xBEAD, 2, 0.25};
        EXPECT_CALL(m_app_sampler, get_short_region(1))
            .WillOnce(Return(short_region));
        m_account->update();

        // average of all procs
        EXPECT_NEAR((1.1 + 2.0 + 2.0 + 1.4) / m_num_process,
                         m_account->get_runtime_average(0xDADA), m_epsilon);
        EXPECT_NEAR(1.0, m_account->get_count_average(0xDADA), m_epsilon);

        EXPECT_NEAR((0.15 + 0.25 + 0.35 + 0.45 + 0.15 + 0.25) / m_num_process,
                         m_account->get_runtime_average(0xBEAD), m_epsilon);
        EXPECT_NEAR((6.0 + 3.0) / m_num_process, m_account->get_count_average(0xBEAD), m_epsilon);
    }

}
