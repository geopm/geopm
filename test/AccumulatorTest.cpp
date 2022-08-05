/*
 * Copyright (c) 2015 - 2022, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "gtest/gtest.h"
#include "gmock/gmock.h"

#include "Accumulator.hpp"

using geopm::SumAccumulator;
using geopm::AvgAccumulator;

class AccumulatorTest : public ::testing::Test
{
    protected:
        void SetUp();
        std::shared_ptr<SumAccumulator> m_accum_sum;
        std::shared_ptr<AvgAccumulator> m_accum_avg;
};

void AccumulatorTest::SetUp()
{
    m_accum_sum = SumAccumulator::make_unique();
    m_accum_avg = AvgAccumulator::make_unique();
}

TEST_F(AccumulatorTest, empty)
{
    EXPECT_EQ(0.0, m_accum_sum->total());
    EXPECT_EQ(0.0, m_accum_sum->interval_total());
    EXPECT_EQ(0.0, m_accum_avg->average());
    EXPECT_EQ(0.0, m_accum_avg->interval_average());
}

TEST_F(AccumulatorTest, sum_ones)
{
    m_accum_sum->enter();
    for (int idx = 0; idx != 10; ++idx) {
        EXPECT_EQ(idx, m_accum_sum->total());
        m_accum_sum->update(1.0);
        EXPECT_EQ(idx + 1.0, m_accum_sum->total());
        EXPECT_EQ(0.0, m_accum_sum->interval_total());
    }
    m_accum_sum->exit();
    m_accum_sum->enter();
    for (int idx = 0; idx != 9; ++idx) {
        EXPECT_EQ(idx + 10.0, m_accum_sum->total());
        m_accum_sum->update(1.0);
        EXPECT_EQ(idx + 11.0, m_accum_sum->total());
        EXPECT_EQ(10.0, m_accum_sum->interval_total());
    }
    m_accum_sum->exit();
    m_accum_sum->enter();
    for (int idx = 0; idx != 8; ++idx) {
        EXPECT_EQ(idx + 19.0, m_accum_sum->total());
        m_accum_sum->update(1.0);
        EXPECT_EQ(idx + 20.0, m_accum_sum->total());
        EXPECT_EQ(9.0, m_accum_sum->interval_total());
    }
    m_accum_sum->exit();
    m_accum_sum->enter();
    for (int idx = 0; idx != 7; ++idx) {
        EXPECT_EQ(idx + 27.0, m_accum_sum->total());
        m_accum_sum->update(1.0);
        EXPECT_EQ(idx + 28.0, m_accum_sum->total());
        EXPECT_EQ(8.0, m_accum_sum->interval_total());
    }
    m_accum_sum->exit();
    m_accum_sum->enter();
    for (int idx = 0; idx != 6; ++idx) {
        EXPECT_EQ(idx + 34.0, m_accum_sum->total());
        m_accum_sum->update(1.0);
        EXPECT_EQ(idx + 35.0, m_accum_sum->total());
        EXPECT_EQ(7.0, m_accum_sum->interval_total());
    }
    m_accum_sum->exit();
}


TEST_F(AccumulatorTest, sum_idx)
{
    m_accum_sum->enter();
    for (int idx = 0; idx != 10; ++idx) {
        m_accum_sum->update(idx);
    }
    EXPECT_EQ(45.0, m_accum_sum->total());
    EXPECT_EQ(0.0, m_accum_sum->interval_total());
    m_accum_sum->exit();
    EXPECT_EQ(45.0, m_accum_sum->total());
    EXPECT_EQ(45.0, m_accum_sum->interval_total());
    m_accum_sum->enter();
    for (int idx = 0; idx != 9; ++idx) {
        m_accum_sum->update(idx);
    }
    EXPECT_EQ(81.0, m_accum_sum->total());
    EXPECT_EQ(45.0, m_accum_sum->interval_total());
    m_accum_sum->exit();
    EXPECT_EQ(81.0, m_accum_sum->total());
    EXPECT_EQ(36.0, m_accum_sum->interval_total());
    m_accum_sum->enter();
    for (int idx = 0; idx != 8; ++idx) {
        m_accum_sum->update(idx);
    }
    EXPECT_EQ(109.0, m_accum_sum->total());
    EXPECT_EQ(36.0, m_accum_sum->interval_total());
    m_accum_sum->exit();
    EXPECT_EQ(109.0, m_accum_sum->total());
    EXPECT_EQ(28.0, m_accum_sum->interval_total());
}


TEST_F(AccumulatorTest, avg_ones)
{
    m_accum_avg->enter();
    for (int idx = 0; idx != 10; ++idx) {
        m_accum_avg->update(1.0, 1.0);
        EXPECT_EQ(1.0, m_accum_avg->average());
        EXPECT_EQ(0.0, m_accum_avg->interval_average());
    }
    m_accum_avg->exit();
    m_accum_avg->enter();
    for (int idx = 0; idx != 9; ++idx) {
        m_accum_avg->update(1.0, 1.0);
        EXPECT_EQ(1.0, m_accum_avg->average());
        EXPECT_EQ(1.0, m_accum_avg->interval_average());
    }
    m_accum_avg->exit();
    m_accum_avg->enter();
    for (int idx = 0; idx != 8; ++idx) {
        m_accum_avg->update(1.0, 1.0);
        EXPECT_EQ(1.0, m_accum_avg->average());
        EXPECT_EQ(1.0, m_accum_avg->interval_average());
    }
    m_accum_avg->exit();
}


TEST_F(AccumulatorTest, avg_idx_signal)
{
    m_accum_avg->enter();
    for (int idx = 0; idx != 10; ++idx) {
        m_accum_avg->update(1.0, idx);
    }
    EXPECT_EQ(4.5, m_accum_avg->average());
    EXPECT_EQ(0.0, m_accum_avg->interval_average());
    m_accum_avg->exit();
    EXPECT_EQ(4.5, m_accum_avg->average());
    EXPECT_EQ(4.5, m_accum_avg->interval_average());
    m_accum_avg->enter();
    for (int idx = 0; idx != 5; ++idx) {
        m_accum_avg->update(1.0, idx);
    }
    EXPECT_DOUBLE_EQ(11.0 / 3.0, m_accum_avg->average());
    EXPECT_EQ(4.5, m_accum_avg->interval_average());
    m_accum_avg->exit();
    EXPECT_EQ(11.0 / 3.0, m_accum_avg->average());
    EXPECT_EQ(2.0, m_accum_avg->interval_average());
}
