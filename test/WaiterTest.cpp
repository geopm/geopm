/*
 * Copyright (c) 2015 - 2023, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <memory>

#include "gtest/gtest.h"
#include "geopm_test.hpp"

#include "Waiter.hpp"

using geopm::Waiter;

class WaiterTest : public ::testing::Test
{
    protected:
        double m_period = 0.1;
        double m_epsilon = 0.01;
};


TEST_F(WaiterTest, invalid_strategy_name)
{
    GEOPM_EXPECT_THROW_MESSAGE(Waiter::make_unique(1.0, "invalid_strategy_name"),
                               GEOPM_ERROR_INVALID, "Unknown strategy");
}

TEST_F(WaiterTest, make_unique)
{
    std::shared_ptr<Waiter> waiter = Waiter::make_unique(1.0);
    ASSERT_EQ(1.0, waiter->period());
    waiter = Waiter::make_unique(2.0, "sleep");
    ASSERT_EQ(2.0, waiter->period());
}

TEST_F(WaiterTest, reset)
{
    std::shared_ptr<Waiter> waiter = Waiter::make_unique(m_period);
    geopm_time_s time_0;
    geopm_time_s time_1;
    timespec delay = {0,100000000};
    nanosleep(&delay, nullptr);
    waiter->reset();
    geopm_time(&time_0);
    waiter->wait();
    geopm_time(&time_1);
    EXPECT_NEAR(m_period, geopm_time_diff(&time_0, &time_1), m_epsilon);
}

TEST_F(WaiterTest, wait)
{
    geopm_time_s time_0;
    geopm_time_s time_1;
    std::shared_ptr<Waiter> waiter = Waiter::make_unique(m_period);
    timespec delay = {0,100000000};
    nanosleep(&delay, nullptr);
    for (int count = 0; count < 10; ++count) {
        geopm_time(&time_0);
        waiter->wait();
        geopm_time(&time_1);
        EXPECT_NEAR(m_period, geopm_time_diff(&time_0, &time_1), m_epsilon);
    }
}
