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

#include "ScalabilityHintTimeSignal.hpp"
#include "Helper.hpp"
#include "MockSignal.hpp"
#include "geopm_test.hpp"

using geopm::Signal;
using geopm::ScalabilityHintTimeSignal;

using testing::Return;
using testing::InvokeWithoutArgs;

class ScalabilityHintTimeSignalTest : public ::testing::Test
{
    protected:
        void SetUp(void);

        std::shared_ptr<MockSignal> m_time_sig;
        std::shared_ptr<MockSignal> m_scal_sig;
        std::vector<std::unique_ptr<Signal>> m_sig;

        // example data
        std::vector<double> m_scal;

        double m_sleep_time;

        std::vector< std::pair<double, double> > m_range;
};

void ScalabilityHintTimeSignalTest::SetUp(void)
{
    m_scal = {0.75, 0.45, 0.01, NAN};
    m_sleep_time = 0.005;

    m_range = {std::make_pair(2.0,0.5),
               std::make_pair(0.5,0.05),
               std::make_pair(0.05,0)};


    m_time_sig = std::make_shared<MockSignal>();
    m_scal_sig = std::make_shared<MockSignal>();
    for (const auto &range : m_range) {
        m_sig.push_back(geopm::make_unique<ScalabilityHintTimeSignal>(m_scal_sig, m_time_sig,
                                                     range.first, range.second));
    }
}

TEST_F(ScalabilityHintTimeSignalTest, read_nan)
{
    for (int idx = 0; idx < (int) m_sig.size(); ++idx) {
        double actual = m_sig.at(idx)->read();
        EXPECT_TRUE(std::isnan(actual));
    }
}

TEST_F(ScalabilityHintTimeSignalTest, read_batch)
{
    double time = 0.00;

    EXPECT_CALL(*m_time_sig, setup_batch()).Times(m_sig.size());

    EXPECT_CALL(*m_scal_sig, setup_batch()).Times(m_sig.size());
    for (const auto &sig : m_sig) {
        sig->setup_batch();
    }

    for (int odx = 0; odx < (int) m_scal.size(); ++odx) {
        time += m_sleep_time;
        EXPECT_CALL(*m_time_sig, sample())
            .WillRepeatedly(Return(time));

        EXPECT_CALL(*m_scal_sig, sample()).Times(m_sig.size())
            .WillRepeatedly(Return(m_scal.at(odx)));

        for (int idx = 0; idx < (int) m_sig.size(); ++idx) {
            double actual = m_sig.at(idx)->sample();
            if (idx <= odx-1) {
                EXPECT_NEAR(m_sleep_time, actual, 0.00001);
            }
            else {
                EXPECT_NEAR(0.0, actual, 0.00001);
            }
        }
    }
}


TEST_F(ScalabilityHintTimeSignalTest, read_batch_upper_boundary)
{
    double time = 0.00;
    std::vector<double> scal;

    EXPECT_CALL(*m_time_sig, setup_batch()).Times(m_sig.size());

    EXPECT_CALL(*m_scal_sig, setup_batch()).Times(m_sig.size());
    for (const auto &sig : m_sig) {
        sig->setup_batch();
    }

    for (const auto &range : m_range) {
        scal.push_back(range.first);
    }
    scal.push_back(NAN);

    for (int odx = 0; odx < (int) scal.size(); ++odx) {
        time += m_sleep_time;

        EXPECT_CALL(*m_time_sig, sample())
            .WillRepeatedly(Return(time));

        EXPECT_CALL(*m_scal_sig, sample())
            .WillRepeatedly(Return(scal.at(odx)));

        for (int idx = 0; idx < (int) m_sig.size(); ++idx) {
            double actual = m_sig.at(idx)->sample();
            if  (idx < odx-1) {
                EXPECT_NEAR(m_sleep_time, actual, 0.00001);
            }
            else {
                EXPECT_NEAR(0.0, actual, 0.00001);
            }
        }
    }
}

TEST_F(ScalabilityHintTimeSignalTest, read_batch_lower_boundary)
{
    double time = 0.00;
    std::vector<double> scal;

    EXPECT_CALL(*m_time_sig, setup_batch()).Times(m_sig.size());

    EXPECT_CALL(*m_scal_sig, setup_batch()).Times(m_sig.size());
    for (const auto &sig : m_sig) {
        sig->setup_batch();
    }

    for (const auto &range : m_range) {
        scal.push_back(range.second);
    }
    scal.push_back(NAN);

    for (int odx = 0; odx < (int) scal.size(); ++odx) {
        time += m_sleep_time;

        EXPECT_CALL(*m_time_sig, sample())
            .WillRepeatedly(Return(time));

        EXPECT_CALL(*m_scal_sig, sample())
            .WillRepeatedly(Return(scal.at(odx)));

        for (int idx = 0; idx < (int) m_sig.size(); ++idx) {
            double actual = m_sig.at(idx)->sample();
            if (idx <= odx-1) {
                EXPECT_NEAR(m_sleep_time, actual, 0.00001);
            }
            else {
                EXPECT_NEAR(0.0, actual, 0.00001);
            }
        }
    }
}

TEST_F(ScalabilityHintTimeSignalTest, read_batch_nan)
{
    double time = 0.00;

    EXPECT_CALL(*m_time_sig, setup_batch()).Times(m_sig.size());
    EXPECT_CALL(*m_scal_sig, setup_batch()).Times(m_sig.size());

    EXPECT_CALL(*m_scal_sig, sample()).WillRepeatedly(Return(NAN));

    for (const auto &sig : m_sig) {
        sig->setup_batch();
    }

    time += m_sleep_time;
    EXPECT_CALL(*m_time_sig, sample())
        .WillRepeatedly(Return(time));

    for (int idx = 0; idx < (int) m_sig.size(); ++idx) {
        double actual = m_sig.at(idx)->sample();
        EXPECT_DOUBLE_EQ(0.0, actual);
    }
}

TEST_F(ScalabilityHintTimeSignalTest, read_batch_repeat)
{
    int repeated_samples = 5;
    double time = 0.00;

    EXPECT_CALL(*m_time_sig, setup_batch()).Times(m_sig.size());

    EXPECT_CALL(*m_scal_sig, setup_batch()).Times(m_sig.size());
    for (const auto &sig : m_sig) {
        sig->setup_batch();
    }

    for (int reps = 0; reps < repeated_samples; ++reps) {
        for (int odx = 0; odx < (int) m_scal.size(); ++odx) {
            time += m_sleep_time;

            EXPECT_CALL(*m_time_sig, sample())
                .WillRepeatedly(Return(time));

            EXPECT_CALL(*m_scal_sig, sample()).Times(m_sig.size())
                .WillRepeatedly(Return(m_scal.at(odx)));

            for (int idx = 0; idx < (int) m_sig.size(); ++idx) {
                double actual = m_sig.at(idx)->sample();
                if (idx <= odx-1) {
                    EXPECT_NEAR(m_sleep_time*(reps+1), actual, 0.00001);
                }
                else {
                    EXPECT_NEAR(m_sleep_time*(reps), actual, 0.00001);
                }
            }
        }
    }
}

TEST_F(ScalabilityHintTimeSignalTest, setup_batch)
{
    // check that setup_batch can be safely called twice

    for (const auto &sig : m_sig) {
        EXPECT_CALL(*m_time_sig, setup_batch()).Times(1);
        EXPECT_CALL(*m_scal_sig, setup_batch()).Times(1);
        sig->setup_batch();
        sig->setup_batch();
    }
}

TEST_F(ScalabilityHintTimeSignalTest, errors)
{
#ifdef GEOPM_DEBUG
    // cannot construct with null signals
    GEOPM_EXPECT_THROW_MESSAGE(ScalabilityHintTimeSignal(nullptr, m_scal_sig, 0, 0),
                               GEOPM_ERROR_LOGIC,
                               "Signal pointers for scalability and time cannot be null.");
    GEOPM_EXPECT_THROW_MESSAGE(ScalabilityHintTimeSignal(m_time_sig, nullptr, 0, 0),
                               GEOPM_ERROR_LOGIC,
                               "Signal pointers for scalability and time cannot be null.");
#endif
    // cannot call sample without setup_batch
    for (const auto &sig : m_sig) {
        GEOPM_EXPECT_THROW_MESSAGE(sig->sample(), GEOPM_ERROR_RUNTIME,
                                   "setup_batch() must be called before sample()");
    }

}
