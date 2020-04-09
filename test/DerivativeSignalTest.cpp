/*
 * Copyright (c) 2015, 2016, 2017, 2018, 2019, 2020, Intel Corporation
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

#include <memory>

#include "gtest/gtest.h"
#include "gmock/gmock.h"

#include "DerivativeSignal.hpp"
#include "Helper.hpp"
#include "MockSignal.hpp"
#include "geopm_test.hpp"

using geopm::Signal;
using geopm::DerivativeSignal;

using testing::Return;
using testing::InvokeWithoutArgs;

class DerivativeSignalTest : public ::testing::Test
{
    protected:
        void SetUp(void);

        std::shared_ptr<MockSignal> m_time_sig;
        std::shared_ptr<MockSignal> m_y_sig;
        std::unique_ptr<Signal> m_sig;

        // example data
        std::vector<double> m_sample_values_0;
        double m_exp_slope_0;
        std::vector<double> m_sample_values_1;
        double m_exp_slope_1;
        std::vector<double> m_sample_values_2;
        double m_exp_slope_2;

        int m_num_history_sample = 8;
        double m_sleep_time = 0.001;
};

void DerivativeSignalTest::SetUp(void)
{
    m_time_sig = std::make_shared<MockSignal>();
    m_y_sig = std::make_shared<MockSignal>();
    m_sig = geopm::make_unique<DerivativeSignal>(m_time_sig, m_y_sig,
                                                 m_num_history_sample, m_sleep_time);

    // should have slope of 0.0
    m_sample_values_0 = {5.5, 5.5, 5.5, 5.5};
    m_exp_slope_0 = 0.0;

    // should have slope of 1.0
    m_sample_values_1 = {0.000001, 0.999999, 2.000001,
                         2.999999, 4.000001, 4.999999,
                         6.000001, 6.999999, 8.000001,
                         8.999999};
    m_exp_slope_1 = 1.0;

    // should have slope of .238 with least squares fit
    m_sample_values_2 = {0, 1, 2, 3, 0, 1, 2, 3};
    m_exp_slope_2 = 0.238;
}

TEST_F(DerivativeSignalTest, read_flat)
{
    size_t ii = 0;
    EXPECT_CALL(*m_time_sig, read()).Times(m_num_history_sample)
        .WillRepeatedly(InvokeWithoutArgs([&ii]() {
                    ++ii;
                    return ii;
                }));
    EXPECT_CALL(*m_y_sig, read()).Times(m_num_history_sample)
        .WillRepeatedly(Return(7.7));
    double result = m_sig->read();
    EXPECT_NEAR(m_exp_slope_0, result, 0.0001);
}

TEST_F(DerivativeSignalTest, read_slope_1)
{
    size_t ii = 0;
    double val = 2.5;
    EXPECT_CALL(*m_time_sig, read()).Times(m_num_history_sample)
        .WillRepeatedly(InvokeWithoutArgs([&ii]() {
                    ++ii;
                    return ii;
                }));
    EXPECT_CALL(*m_y_sig, read()).Times(m_num_history_sample)
        .WillRepeatedly(InvokeWithoutArgs([&val]() {
                    val += 1.0;;
                    return val;
                }));
    double result = m_sig->read();
    EXPECT_NEAR(m_exp_slope_1, result, 0.0001);
}

TEST_F(DerivativeSignalTest, read_batch_first)
{
    EXPECT_CALL(*m_time_sig, setup_batch());
    EXPECT_CALL(*m_y_sig, setup_batch());
    m_sig->setup_batch();

    EXPECT_CALL(*m_time_sig, sample()).WillOnce(Return(2.0));
    EXPECT_CALL(*m_y_sig, sample()).WillOnce(Return(7.7));
    double result = m_sig->sample();
    EXPECT_TRUE(std::isnan(result));
}

TEST_F(DerivativeSignalTest, read_batch_flat)
{
    EXPECT_CALL(*m_time_sig, setup_batch());
    EXPECT_CALL(*m_y_sig, setup_batch());
    m_sig->setup_batch();

    double result = NAN;
    for (size_t ii = 0; ii < m_sample_values_0.size(); ++ii) {
        EXPECT_CALL(*m_time_sig, sample()).WillOnce(Return(ii));
        EXPECT_CALL(*m_y_sig, sample()).WillOnce(Return(m_sample_values_0[ii]));
        result = m_sig->sample();
    }
    EXPECT_NEAR(m_exp_slope_0, result, 0.0001);
}

TEST_F(DerivativeSignalTest, read_batch_slope_1)
{
    EXPECT_CALL(*m_time_sig, setup_batch());
    EXPECT_CALL(*m_y_sig, setup_batch());
    m_sig->setup_batch();

    double result = NAN;
    for (size_t ii = 0; ii < m_sample_values_1.size(); ++ii) {
        EXPECT_CALL(*m_time_sig, sample()).WillOnce(Return(ii));
        EXPECT_CALL(*m_y_sig, sample()).WillOnce(Return(m_sample_values_1[ii]));
        result = m_sig->sample();
    }
    EXPECT_NEAR(m_exp_slope_1, result, 0.0001);
}

TEST_F(DerivativeSignalTest, read_batch_slope_2)
{
    EXPECT_CALL(*m_time_sig, setup_batch());
    EXPECT_CALL(*m_y_sig, setup_batch());
    m_sig->setup_batch();

    double result = NAN;
    for (size_t ii = 0; ii < m_sample_values_2.size(); ++ii) {
        EXPECT_CALL(*m_time_sig, sample()).WillOnce(Return(ii));
        EXPECT_CALL(*m_y_sig, sample()).WillOnce(Return(m_sample_values_2[ii]));
        result = m_sig->sample();
    }
    EXPECT_NEAR(m_exp_slope_2, result, 0.0001);
}

TEST_F(DerivativeSignalTest, setup_batch)
{
    // check that setup_batch can be safely called twice
    EXPECT_CALL(*m_time_sig, setup_batch()).Times(1);
    EXPECT_CALL(*m_y_sig, setup_batch()).Times(1);
    m_sig->setup_batch();
    m_sig->setup_batch();
}

TEST_F(DerivativeSignalTest, errors)
{
#ifdef GEOPM_DEBUG
    // cannot construct with null signals
    GEOPM_EXPECT_THROW_MESSAGE(DerivativeSignal(nullptr, m_y_sig, 0, 0),
                               GEOPM_ERROR_LOGIC,
                               "underlying Signals cannot be null");
    GEOPM_EXPECT_THROW_MESSAGE(DerivativeSignal(m_time_sig, nullptr, 0, 0),
                               GEOPM_ERROR_LOGIC,
                               "underlying Signals cannot be null");
#endif
    // cannot call sample without setup_batch
    GEOPM_EXPECT_THROW_MESSAGE(m_sig->sample(), GEOPM_ERROR_RUNTIME,
                               "setup_batch() must be called before sample()");

}
