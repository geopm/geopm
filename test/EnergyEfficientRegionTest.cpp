/*
 * Copyright (c) 2015, 2016, 2017, 2018, 2019, Intel Corporation
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

#include <cmath>

#include "gtest/gtest.h"
#include "gmock/gmock.h"

#include "EnergyEfficientRegion.hpp"
#include "Exception.hpp"
#include "MockPlatformIO.hpp"

using ::testing::Return;
using geopm::EnergyEfficientRegion;

class EnergyEfficientRegionTest : public ::testing::Test
{
    public:
        EnergyEfficientRegionTest();
        virtual ~EnergyEfficientRegionTest() = default;
    protected:
        enum {
            M_SIGNAL_RUNTIME,
        };

        double M_FREQ_MIN = 1.8e9;
        double M_FREQ_MAX = 2.2e9;
        double M_FREQ_STEP = 1.0e8;

        MockPlatformIO m_platform_io;
        EnergyEfficientRegion m_freq_region;
};

EnergyEfficientRegionTest::EnergyEfficientRegionTest()
    : m_freq_region(m_platform_io,
                    M_FREQ_MIN, M_FREQ_MAX, M_FREQ_STEP,
                    M_SIGNAL_RUNTIME)
{
    //ASSERT_NE(M_FREQ_MIN, M_FREQ_MAX);
    //ASSERT_NE(0, M_FREQ_STEP);
}

TEST_F(EnergyEfficientRegionTest, freq_starts_at_maximum)
{
    ASSERT_EQ(M_FREQ_MAX, m_freq_region.freq());
}

TEST_F(EnergyEfficientRegionTest, update_ignores_nan_sample)
{
    EXPECT_CALL(m_platform_io, sample(M_SIGNAL_RUNTIME))
                .WillOnce(Return(NAN));
    double start = m_freq_region.freq();
    m_freq_region.update_exit();

    double end = m_freq_region.freq();
    ASSERT_EQ(start, end);
}

TEST_F(EnergyEfficientRegionTest, freq_does_not_go_below_min)
{
    // run more times than the number of frequencies
    size_t num_steps = 5 + (size_t)(ceil((M_FREQ_MAX - M_FREQ_MIN) / M_FREQ_STEP));

    double start = m_freq_region.freq();
    for (size_t i = 1; i <= num_steps; ++i) {
        EXPECT_CALL(m_platform_io, sample(M_SIGNAL_RUNTIME))
                    .WillOnce(Return(1));
        m_freq_region.update_exit();
        EXPECT_LT(m_freq_region.freq(), start);
    }

    ASSERT_EQ(M_FREQ_MIN, m_freq_region.freq());

    double updated_min = M_FREQ_MIN + M_FREQ_STEP;
    EXPECT_THROW(m_freq_region.update_freq_range(updated_min, M_FREQ_MAX, M_FREQ_STEP), geopm::Exception);
    /// @todo logic below is to be implemented
    //m_freq_region.update_freq_range(updated_min, M_FREQ_MAX, M_FREQ_STEP);
    //ASSERT_EQ(updated_min, m_freq_region.freq());
}

TEST_F(EnergyEfficientRegionTest, freq_does_not_go_above_max)
{
    std::vector<time_t> samples {3, 3, 5};
    double updated_max = M_FREQ_MAX - M_FREQ_STEP * 2;
    std::vector<double> expected {M_FREQ_MAX - M_FREQ_STEP,
                                  M_FREQ_MAX - M_FREQ_STEP * 2,
                                  updated_max};
    for (size_t i = 0; i < samples.size(); ++i) {
        if (i == 2) {
            EXPECT_THROW(m_freq_region.update_freq_range(M_FREQ_MIN, updated_max, M_FREQ_STEP), geopm::Exception);
            /// @todo logic below is to be implemented
            //m_freq_region.update_freq_range(M_FREQ_MIN, updated_max, M_FREQ_STEP);
            return;
        }
        EXPECT_CALL(m_platform_io, sample(M_SIGNAL_RUNTIME))
                    .WillOnce(Return(samples[i]));
        m_freq_region.update_exit();
        EXPECT_EQ(expected[i], m_freq_region.freq());
    }
}

TEST_F(EnergyEfficientRegionTest, performance_decreases_freq_steps_back_up)
{
    std::vector<time_t> samples {3, 3, 5};
    std::vector<double> expected {M_FREQ_MAX - M_FREQ_STEP,
                                  M_FREQ_MAX - M_FREQ_STEP * 2,
                                  M_FREQ_MAX - M_FREQ_STEP};
    for (size_t i = 0; i < samples.size(); ++i) {
        EXPECT_CALL(m_platform_io, sample(M_SIGNAL_RUNTIME))
                    .WillOnce(Return(samples[i]));
        m_freq_region.update_exit();
        EXPECT_EQ(expected[i], m_freq_region.freq());
    }
}

TEST_F(EnergyEfficientRegionTest, after_too_many_increase_freq_stays_at_higher)
{
    EXPECT_CALL(m_platform_io, sample(M_SIGNAL_RUNTIME))
                .WillOnce(Return(3));

    size_t max_increase = 4;
    double higher_freq = M_FREQ_MAX - M_FREQ_STEP;
    double lower_freq = M_FREQ_MAX - M_FREQ_STEP * 2;
    // run once to step down from max
    m_freq_region.update_exit();
    EXPECT_EQ(higher_freq, m_freq_region.freq());

    // raise and lower bandwidth and alternate freq
    for (size_t i = 0; i < max_increase; ++i) {
        // lower freq
        EXPECT_CALL(m_platform_io, sample(M_SIGNAL_RUNTIME))
                    .WillOnce(Return(3));
        m_freq_region.update_exit();
        EXPECT_EQ(lower_freq, m_freq_region.freq());
        // raise freq
        EXPECT_CALL(m_platform_io, sample(M_SIGNAL_RUNTIME))
                    .WillOnce(Return(5));
        m_freq_region.update_exit();
        EXPECT_EQ(higher_freq, m_freq_region.freq());
    }

    // freq should stay at higher
    for (size_t i = 0; i < 3; ++i) {
        EXPECT_CALL(m_platform_io, sample(M_SIGNAL_RUNTIME))
                    .WillOnce(Return(3));
        m_freq_region.update_exit();
        EXPECT_EQ(higher_freq, m_freq_region.freq());

        EXPECT_CALL(m_platform_io, sample(M_SIGNAL_RUNTIME))
                    .WillOnce(Return(5));
        m_freq_region.update_exit();
        EXPECT_EQ(higher_freq, m_freq_region.freq());
    }
}
