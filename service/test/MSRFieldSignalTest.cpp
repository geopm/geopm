/*
 * Copyright (c) 2015 - 2024, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "config.h"

#include <memory>

#include "gtest/gtest.h"
#include "gmock/gmock.h"

#include "MSRFieldSignal.hpp"
#include "MSR.hpp"
#include "geopm/Helper.hpp"
#include "geopm_hash.h"
#include "geopm_field.h"
#include "MockSignal.hpp"
#include "geopm_test.hpp"

using geopm::Signal;
using geopm::MSRFieldSignal;
using geopm::MSR;
using geopm::string_format_float;
using testing::Return;

class MSRFieldSignalTest : public ::testing::Test
{
    protected:
        void SetUp();

        std::shared_ptr<MockSignal> m_raw;
        int m_start;
        int m_end;
        uint64_t m_mask;
};

void MSRFieldSignalTest::SetUp()
{
    m_raw = std::make_shared<MockSignal>();
    m_start = 16;
    m_end = 23;
}

TEST_F(MSRFieldSignalTest, read_scale)
{
    double scalar = 1.5;
    auto sig = geopm::make_unique<MSRFieldSignal>(m_raw, m_start, m_end,
                                                  MSR::M_FUNCTION_SCALE, scalar);
    uint64_t raw_val = 0xF1458321;
    EXPECT_CALL(*m_raw, read())
        .WillOnce(Return(geopm_field_to_signal(raw_val)));
    double expected = 0x45 * scalar;
    double result = sig->read();
    EXPECT_EQ(expected, result);
}

TEST_F(MSRFieldSignalTest, read_batch_scale)
{
    double scalar = 2.7;
    auto sig = geopm::make_unique<MSRFieldSignal>(m_raw, m_start, m_end,
                                                  MSR::M_FUNCTION_SCALE, scalar);
    EXPECT_CALL(*m_raw, setup_batch());
    sig->setup_batch();
    uint64_t raw_val = 0xF1678321;
    EXPECT_CALL(*m_raw, sample())
        .WillOnce(Return(geopm_field_to_signal(raw_val)));
    double expected = 0x67 * scalar;
    double result = sig->sample();
    EXPECT_EQ(expected, result);
}

TEST_F(MSRFieldSignalTest, read_log_half)
{
    double scalar = 1.0;
    auto sig = geopm::make_unique<MSRFieldSignal>(m_raw, m_start, m_end,
                                                  MSR::M_FUNCTION_LOG_HALF,
                                                  scalar);
    uint64_t raw_val = 0xF1028321;  // field is 0x02
    EXPECT_CALL(*m_raw, read())
        .WillOnce(Return(geopm_field_to_signal(raw_val)));
    double expected = 0.25;
    double result = sig->read();
    EXPECT_EQ(expected, result);
}

TEST_F(MSRFieldSignalTest, read_batch_log_half)
{
    double scalar = 1.0;
    auto sig = geopm::make_unique<MSRFieldSignal>(m_raw, m_start, m_end,
                                                  MSR::M_FUNCTION_LOG_HALF,
                                                  scalar);
    EXPECT_CALL(*m_raw, setup_batch());
    sig->setup_batch();
    uint64_t raw_val = 0xF1028321;  // field is 0x02
    EXPECT_CALL(*m_raw, sample())
        .WillOnce(Return(geopm_field_to_signal(raw_val)));
    double expected = 0.25;
    double result = sig->sample();
    EXPECT_EQ(expected, result);

}

TEST_F(MSRFieldSignalTest, read_7_bit_float)
{
    double scalar = 3.0;
    auto sig = geopm::make_unique<MSRFieldSignal>(m_raw, m_start, m_end,
                                                  MSR::M_FUNCTION_7_BIT_FLOAT,
                                                  scalar);
    uint64_t raw_val = 0xF1418321;  // field is 0x41
    EXPECT_CALL(*m_raw, read())
        .WillOnce(Return(geopm_field_to_signal(raw_val)));
    double expected = 9.0;
    double result = sig->read();
    EXPECT_EQ(expected, result);
}

TEST_F(MSRFieldSignalTest, read_batch_7_bit_float)
{
    double scalar = 3.0;
    auto sig = geopm::make_unique<MSRFieldSignal>(m_raw, m_start, m_end,
                                                  MSR::M_FUNCTION_7_BIT_FLOAT,
                                                  scalar);
    EXPECT_CALL(*m_raw, setup_batch());
    sig->setup_batch();
    uint64_t raw_val = 0xF1418321;  // field is 0x41
    EXPECT_CALL(*m_raw, sample())
        .WillOnce(Return(geopm_field_to_signal(raw_val)));
    double expected = 9.0;
    double result = sig->sample();
    EXPECT_EQ(expected, result);

}

TEST_F(MSRFieldSignalTest, read_overflow)
{
    std::unique_ptr<Signal> sig = geopm::make_unique<MSRFieldSignal>(m_raw, 0, 3,
                                                                     MSR::M_FUNCTION_OVERFLOW, 1.0);
    double result = NAN, expected = NAN;
    // no overflow for any sequence of values
    expected = 5.0;
    EXPECT_CALL(*m_raw, read())
        .WillOnce(Return(geopm_field_to_signal(0x0005)));
    result = sig->read();
    EXPECT_DOUBLE_EQ(expected, result);

    expected = 4.0;
    EXPECT_CALL(*m_raw, read())
        .WillOnce(Return(geopm_field_to_signal(0x0004)));
    result = sig->read();
    EXPECT_DOUBLE_EQ(expected, result);

    expected = 10.0;
    EXPECT_CALL(*m_raw, read())
        .WillOnce(Return(geopm_field_to_signal(0x000A)));
    result = sig->read();
    EXPECT_DOUBLE_EQ(expected, result);

    expected = 1.0;
    EXPECT_CALL(*m_raw, read())
        .WillOnce(Return(geopm_field_to_signal(0x0001)));
    result = sig->read();
    EXPECT_DOUBLE_EQ(expected, result);
}

TEST_F(MSRFieldSignalTest, read_batch_overflow)
{
    auto sig = geopm::make_unique<MSRFieldSignal>(m_raw, 0, 3,
                                                  MSR::M_FUNCTION_OVERFLOW, 1.0);
    EXPECT_CALL(*m_raw, setup_batch());
    sig->setup_batch();
    double result = NAN, expected = NAN;
    // no overflow
    expected = 5.0;
    EXPECT_CALL(*m_raw, sample())
        .WillOnce(Return(geopm_field_to_signal(0x0005)));
    result = sig->sample();
    EXPECT_DOUBLE_EQ(expected, result);
    // one overflow
    expected = 20.0;  // 4 + 16
    EXPECT_CALL(*m_raw, sample())
        .WillOnce(Return(geopm_field_to_signal(0x0004)));
    result = sig->sample();
    EXPECT_DOUBLE_EQ(expected, result);
    // still one overflow
    expected = 26.0;  // 10 + 16
    EXPECT_CALL(*m_raw, sample())
        .WillOnce(Return(geopm_field_to_signal(0x000A)));
    result = sig->sample();
    EXPECT_DOUBLE_EQ(expected, result);
    // multiple overflow
    expected = 33.0;  // 1 + 16 + 16
    EXPECT_CALL(*m_raw, sample())
        .WillOnce(Return(geopm_field_to_signal(0x0001)));
    result = sig->sample();
    EXPECT_DOUBLE_EQ(expected, result);
}

TEST_F(MSRFieldSignalTest, real_counter)
{
    // Test with real counter values
    auto sig = geopm::make_unique<MSRFieldSignal>(m_raw, 0, 47,
                                                  MSR::M_FUNCTION_OVERFLOW, 1.0);
    EXPECT_CALL(*m_raw, setup_batch());
    sig->setup_batch();

    uint64_t input_value = 0xFFFFFF27AAE8;
    EXPECT_CALL(*m_raw, sample())
        .WillOnce(Return(geopm_field_to_signal(input_value)));
    double of_value = sig->sample();
    EXPECT_DOUBLE_EQ((double)input_value, of_value);

    // Setup funky rollover
    input_value = 0xFFFF000DD5D0;
    uint64_t expected_value = input_value + (1ull << 48); // i.e. 0x1FFFF000DD5D0
    EXPECT_CALL(*m_raw, sample())
        .WillOnce(Return(geopm_field_to_signal(input_value)));
    of_value = sig->sample();
    EXPECT_DOUBLE_EQ((double)expected_value, of_value)
                     << "\nActual is : 0x" << std::hex << (uint64_t)of_value << std::endl
                     << "Expected is : 0x" << std::hex << expected_value << std::endl;
                }

TEST_F(MSRFieldSignalTest, setup_batch)
{
    auto sig = geopm::make_unique<MSRFieldSignal>(m_raw, m_start, m_end,
                                                  MSR::M_FUNCTION_SCALE, 1.0);
    // setup batch can be called multiple times without further side effects
    EXPECT_CALL(*m_raw, setup_batch()).Times(1);
    sig->setup_batch();
    sig->setup_batch();

}

TEST_F(MSRFieldSignalTest, errors)
{
    // logic errors in constructor because this class is internal to MSRIOGroup.
    // @todo: consider whether Signal should be public to help writers of IOGroups
#ifdef GEOPM_DEBUG
    // cannot construct with null underlying signal
    GEOPM_EXPECT_THROW_MESSAGE(MSRFieldSignal(nullptr, 0, 0,
                                              MSR::M_FUNCTION_SCALE, 1.0),
                               GEOPM_ERROR_LOGIC, "raw_msr cannot be null");

    // invalid number of bits
    GEOPM_EXPECT_THROW_MESSAGE(MSRFieldSignal(m_raw, 0, 63,
                                              MSR::M_FUNCTION_SCALE, 1.0),
                               GEOPM_ERROR_LOGIC, "64-bit fields are not supported");
    GEOPM_EXPECT_THROW_MESSAGE(MSRFieldSignal(m_raw, 4, 0,
                                              MSR::M_FUNCTION_SCALE, 1.0),
                               GEOPM_ERROR_LOGIC, "begin bit must be <= end bit");

    // invalid encode function
    GEOPM_EXPECT_THROW_MESSAGE(MSRFieldSignal(m_raw, 0, 0,
                                              99, 1.0),
                               GEOPM_ERROR_LOGIC, "invalid encoding function");
#endif

    // cannot call sample without batch setup
    auto sig = geopm::make_unique<MSRFieldSignal>(m_raw, m_start, m_end,
                                                  MSR::M_FUNCTION_SCALE, 1.0);
    GEOPM_EXPECT_THROW_MESSAGE(sig->sample(), GEOPM_ERROR_RUNTIME,
                               "setup_batch() must be called before sample()");

}
