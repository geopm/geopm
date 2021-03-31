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

#include "config.h"

#include <memory>

#include "gtest/gtest.h"
#include "gmock/gmock.h"

#include "DifferenceSignal.hpp"
#include "Helper.hpp"
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
