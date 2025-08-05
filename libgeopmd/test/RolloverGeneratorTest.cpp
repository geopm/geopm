/*
 * Copyright (c) 2015 - 2025 Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include <RolloverGenerator.hpp>


#include "geopm/SaveControl.hpp"

#include <iostream>

#include "gtest/gtest.h"

#include "geopm_test.hpp"
#include "geopm/Helper.hpp"
#include "MockIOGroup.hpp"
#include "MockPlatformTopo.hpp"

using geopm::RolloverGenerator;

class RolloverGeneratorTest: public ::testing::Test
{
};

TEST_F(RolloverGeneratorTest, test_no_rollover)
{
    auto gen = RolloverGenerator();
    gen.set_factor(pow(2, 64));
    for (int idx = 0; idx < 4; ++idx) {
        EXPECT_EQ(idx, gen.update(idx)) << "Initial count up to 3";
    }
}

TEST_F(RolloverGeneratorTest, test_twos)
{
    auto gen = RolloverGenerator();
    gen.set_factor(pow(2, 64));
    for (int idx = 0; idx < 4; ++idx) {
        EXPECT_EQ(2 * idx, gen.update(2 * idx))  << "New generator count up to 6 by 2";
    }
}

TEST_F(RolloverGeneratorTest, test_one_bit_width)
{
    auto gen = RolloverGenerator();
    gen.set_factor(pow(2, 1));
    for (int idx = 0; idx < 4; ++idx) {
        int value = idx % 2;
        EXPECT_EQ(idx, gen.update(value)) << "One bit counter";
    }
}

TEST_F(RolloverGeneratorTest, test_two_bit_width)
{
    auto gen = RolloverGenerator();
    gen.set_factor(pow(2, 2));
    for (int idx = 0; idx < 4; ++idx) {
        int value = idx % 4;
        EXPECT_EQ(idx, gen.update(value))  << "Two bit counter";
    }
}

TEST_F(RolloverGeneratorTest, test_32_bit_width)
{
    auto gen = RolloverGenerator();
    gen.set_factor(pow(2, 32));
    EXPECT_EQ(1, gen.update(1));
    double rollover_factor = static_cast<double>(UINT32_MAX) + 1.0;
    EXPECT_EQ(rollover_factor, gen.update(0));
    EXPECT_EQ(rollover_factor + 1.0, gen.update(1));
    EXPECT_NEAR(2.0 * rollover_factor + 1.0, gen.update(0), 1);
}

TEST_F(RolloverGeneratorTest, test_64_bit_width)
{
    auto gen = RolloverGenerator();
    gen.set_factor(pow(2, 64));
    double rollover_factor = static_cast<double>(UINT64_MAX) + 1.0;
    EXPECT_EQ(1, gen.update(1));
    EXPECT_EQ(rollover_factor, gen.update(0));
    EXPECT_EQ(rollover_factor + 1.0, gen.update(1));
    double expected = 2.0 * rollover_factor + 1.0;
    double error = 1 << (64 - 51);
    EXPECT_NEAR(expected, gen.update(0), error);
}

TEST_F(RolloverGeneratorTest, test_set_factor)
{
    auto gen = RolloverGenerator();
    double rollover_factor = 12345;
    gen.set_factor(rollover_factor);
    EXPECT_EQ(1, gen.update(1));
    EXPECT_EQ(rollover_factor, gen.update(0));
    EXPECT_EQ(rollover_factor + 1.0, gen.update(1));
    double expected = 2.0 * rollover_factor;
    EXPECT_EQ(expected, gen.update(0));
}
