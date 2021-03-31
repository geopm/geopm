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
