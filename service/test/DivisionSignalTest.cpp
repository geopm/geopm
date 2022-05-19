/*
 * Copyright (c) 2015 - 2022, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "config.h"

#include <memory>

#include "gtest/gtest.h"
#include "gmock/gmock.h"

#include "DivisionSignal.hpp"
#include "geopm/Helper.hpp"
#include "geopm_test.hpp"
#include "MockSignal.hpp"

using geopm::DivisionSignal;
using testing::Return;
using testing::IsNan;

class DivisionSignalTest : public ::testing::Test
{
    protected:
        void SetUp();
        int m_domain;
        std::shared_ptr<MockSignal> m_numerator;
        std::shared_ptr<MockSignal> m_denominator;
        std::shared_ptr<DivisionSignal> m_sig;
};

void DivisionSignalTest::SetUp()
{
    m_numerator = std::make_shared<MockSignal>();
    m_denominator = std::make_shared<MockSignal>();
    m_sig = std::make_shared<DivisionSignal>(m_numerator, m_denominator);
}

TEST_F(DivisionSignalTest, read)
{
    double num = 67.8;
    double den = 34.11;
    double expected = num / den;
    EXPECT_CALL(*m_numerator, read()).WillOnce(Return(num));
    EXPECT_CALL(*m_denominator, read()).WillOnce(Return(den));
    double result = m_sig->read();
    EXPECT_NEAR(expected, result, 0.00001);
}

TEST_F(DivisionSignalTest, read_div_by_zero)
{
    double num = 67.8;
    double den = 0;
    EXPECT_CALL(*m_numerator, read()).WillOnce(Return(num));
    EXPECT_CALL(*m_denominator, read()).WillOnce(Return(den));
    double result = m_sig->read();
    EXPECT_THAT(result, IsNan());
}

TEST_F(DivisionSignalTest, read_batch)
{
    EXPECT_CALL(*m_numerator, setup_batch());
    EXPECT_CALL(*m_denominator, setup_batch());
    m_sig->setup_batch();

    double num = 67.8;
    double den = 34.11;
    double expected = num / den;
    EXPECT_CALL(*m_numerator, sample()).WillOnce(Return(num));
    EXPECT_CALL(*m_denominator, sample()).WillOnce(Return(den));
    double result = m_sig->sample();
    EXPECT_NEAR(expected, result, 0.00001);
}

TEST_F(DivisionSignalTest, read_batch_div_by_zero)
{
    EXPECT_CALL(*m_numerator, setup_batch());
    EXPECT_CALL(*m_denominator, setup_batch());
    m_sig->setup_batch();

    double num = 67.8;
    double den = 0;
    EXPECT_CALL(*m_numerator, sample()).WillOnce(Return(num));
    EXPECT_CALL(*m_denominator, sample()).WillOnce(Return(den));
    double result = m_sig->sample();
    EXPECT_THAT(result, IsNan());
}

TEST_F(DivisionSignalTest, setup_batch)
{
    // setup batch can be called multiple times without further side effects
    EXPECT_CALL(*m_numerator, setup_batch()).Times(1);
    EXPECT_CALL(*m_denominator, setup_batch()).Times(1);
    m_sig->setup_batch();
    m_sig->setup_batch();
}

TEST_F(DivisionSignalTest, errors)
{
#ifdef GEOPM_DEBUG
    // cannot construct with null signals
    GEOPM_EXPECT_THROW_MESSAGE(DivisionSignal(nullptr, m_denominator),
                               GEOPM_ERROR_LOGIC,
                               "numerator and denominator cannot be null");
    GEOPM_EXPECT_THROW_MESSAGE(DivisionSignal(m_numerator, nullptr),
                               GEOPM_ERROR_LOGIC,
                               "numerator and denominator cannot be null");
#endif

    // cannot call sample without batch setup
    GEOPM_EXPECT_THROW_MESSAGE(m_sig->sample(), GEOPM_ERROR_RUNTIME,
                               "setup_batch() must be called before sample()");

}
