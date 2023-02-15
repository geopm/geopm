/*
 * Copyright (c) 2015 - 2023, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "config.h"

#include <memory>

#include "gtest/gtest.h"
#include "gmock/gmock.h"

#include "DifferenceSignal.hpp"
#include "geopm/Helper.hpp"
#include "geopm_test.hpp"
#include "MockSignal.hpp"

using geopm::DifferenceSignal;
using testing::Return;

class DifferenceSignalTest : public ::testing::Test
{
    protected:
        void SetUp();
        int m_domain;
        std::shared_ptr<MockSignal> m_minuend;
        std::shared_ptr<MockSignal> m_subtrahend;
        std::shared_ptr<DifferenceSignal> m_sig;
};

void DifferenceSignalTest::SetUp()
{
    m_minuend = std::make_shared<MockSignal>();
    m_subtrahend = std::make_shared<MockSignal>();
    m_sig = std::make_shared<DifferenceSignal>(m_minuend, m_subtrahend);
}

TEST_F(DifferenceSignalTest, read)
{
    double min = 67.8;
    double sub = 34.11;
    double expected = min - sub;
    EXPECT_CALL(*m_minuend, read()).WillOnce(Return(min));
    EXPECT_CALL(*m_subtrahend, read()).WillOnce(Return(sub));
    double result = m_sig->read();
    EXPECT_NEAR(expected, result, 0.00001);
}

TEST_F(DifferenceSignalTest, read_batch)
{
    EXPECT_CALL(*m_minuend, setup_batch());
    EXPECT_CALL(*m_subtrahend, setup_batch());
    m_sig->setup_batch();

    double min = 67.8;
    double sub = 34.11;
    double expected = min - sub;
    EXPECT_CALL(*m_minuend, sample()).WillOnce(Return(min));
    EXPECT_CALL(*m_subtrahend, sample()).WillOnce(Return(sub));
    double result = m_sig->sample();
    EXPECT_NEAR(expected, result, 0.00001);
}

TEST_F(DifferenceSignalTest, setup_batch)
{
    // setup batch can be called multiple times without further side effects
    EXPECT_CALL(*m_minuend, setup_batch()).Times(1);
    EXPECT_CALL(*m_subtrahend, setup_batch()).Times(1);
    m_sig->setup_batch();
    m_sig->setup_batch();
}

TEST_F(DifferenceSignalTest, errors)
{
#ifdef GEOPM_DEBUG
    // cannot construct with null signals
    GEOPM_EXPECT_THROW_MESSAGE(DifferenceSignal(nullptr, m_subtrahend),
                               GEOPM_ERROR_LOGIC,
                               "minuend and subtrahend cannot be null");
    GEOPM_EXPECT_THROW_MESSAGE(DifferenceSignal(m_minuend, nullptr),
                               GEOPM_ERROR_LOGIC,
                               "minuend and subtrahend cannot be null");
#endif

    // cannot call sample without batch setup
    GEOPM_EXPECT_THROW_MESSAGE(m_sig->sample(), GEOPM_ERROR_RUNTIME,
                               "setup_batch() must be called before sample()");

}
