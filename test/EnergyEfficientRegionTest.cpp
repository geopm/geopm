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
#include "config.h"

using ::testing::Return;
using geopm::EnergyEfficientRegionImp;

class EnergyEfficientRegionTest : public ::testing::Test
{
    public:
        EnergyEfficientRegionTest();
        virtual ~EnergyEfficientRegionTest() = default;
    protected:
        const double M_FREQ_MIN = 1.8e9;
        const double M_FREQ_MAX = 2.2e9;
        const double M_FREQ_STEP = 1.0e8;

        EnergyEfficientRegionImp m_freq_region;
};

EnergyEfficientRegionTest::EnergyEfficientRegionTest()
    : m_freq_region(M_FREQ_MIN, M_FREQ_MAX, M_FREQ_STEP, 0.10)
{

}

TEST_F(EnergyEfficientRegionTest, invalid_perf_margin)
{
#ifdef GEOPM_DEBUG
    EXPECT_THROW(EnergyEfficientRegionImp(M_FREQ_MIN, M_FREQ_MAX, M_FREQ_STEP, -0.7),
                 geopm::Exception);
    EXPECT_THROW(EnergyEfficientRegionImp(M_FREQ_MIN, M_FREQ_MAX, M_FREQ_STEP, 1.7),
                 geopm::Exception);
#endif
}

TEST_F(EnergyEfficientRegionTest, freq_starts_at_maximum)
{
    ASSERT_EQ(M_FREQ_MAX, m_freq_region.freq());
}

TEST_F(EnergyEfficientRegionTest, update_ignores_nan_sample)
{
    double start = m_freq_region.freq();
    m_freq_region.sample(NAN);
    m_freq_region.calc_next_freq();

    double end = m_freq_region.freq();
    ASSERT_EQ(start, end);
}

static void check_step(size_t step, double exp, double test)
{
    if (step <= 1) {
        EXPECT_EQ(exp, test);
    }
    else {
        EXPECT_LT(exp, test);
    }
}

TEST_F(EnergyEfficientRegionTest, freq_does_not_go_below_min)
{
    // run more times than the number of frequencies
    size_t num_steps = 5 + (size_t)(ceil((M_FREQ_MAX - M_FREQ_MIN) / M_FREQ_STEP));

    double start = m_freq_region.freq();
    for (size_t i = 0; i <= num_steps; ++i) {
        double curr_freq = m_freq_region.freq();
        check_step(i, curr_freq, start);
        m_freq_region.sample(-1);
        m_freq_region.calc_next_freq();
    }

    ASSERT_EQ(M_FREQ_MIN, m_freq_region.freq());

    double updated_min = M_FREQ_MIN + M_FREQ_STEP;
    EXPECT_THROW(m_freq_region.update_freq_range(updated_min, M_FREQ_MAX, M_FREQ_STEP), geopm::Exception);
}

TEST_F(EnergyEfficientRegionTest, freq_does_not_go_above_max)
{
    std::vector<double> perfs {-3, -3, -3, -5};
    double updated_max = M_FREQ_MAX - M_FREQ_STEP * 2;
    std::vector<double> expected {M_FREQ_MAX,
                                  M_FREQ_MAX - M_FREQ_STEP,
                                  M_FREQ_MAX - M_FREQ_STEP * 2,
                                  updated_max};
    for (size_t i = 0; i < perfs.size(); ++i) {
        if (i == 2) {
            EXPECT_THROW(m_freq_region.update_freq_range(M_FREQ_MIN, updated_max, M_FREQ_STEP), geopm::Exception);
            /// update not implemented.
            break;
        }
        m_freq_region.sample(perfs[i]);
        m_freq_region.calc_next_freq();
        EXPECT_EQ(expected[i], m_freq_region.freq());
    }
}

TEST_F(EnergyEfficientRegionTest, performance_decreases_freq_steps_back_up)
{
    std::vector<double> perfs {-3, -3, -3, -5};
    std::vector<double> expected {M_FREQ_MAX,
                                  M_FREQ_MAX,
                                  M_FREQ_MAX - M_FREQ_STEP,
                                  M_FREQ_MAX - M_FREQ_STEP * 2,
                                  M_FREQ_MAX - M_FREQ_STEP};
    for (size_t i = 0; i < perfs.size(); ++i) {
        EXPECT_EQ(expected[i], m_freq_region.freq());
        m_freq_region.sample(perfs[i]);
        m_freq_region.calc_next_freq();
    }
}

TEST_F(EnergyEfficientRegionTest, after_too_many_increase_freq_stays_at_higher)
{
    size_t max_increase = 4;
    double higher_freq = M_FREQ_MAX - M_FREQ_STEP;
    double lower_freq = M_FREQ_MAX - M_FREQ_STEP * 2;
    // run twice to step down from max
    m_freq_region.sample(-3);
    m_freq_region.calc_next_freq();
    m_freq_region.sample(-3);
    m_freq_region.calc_next_freq();
    EXPECT_EQ(higher_freq, m_freq_region.freq());

    // raise and lower bandwidth and alternate freq
    for (size_t i = 0; i < max_increase; ++i) {
        // lower freq
        m_freq_region.sample(-3);
        m_freq_region.calc_next_freq();
        EXPECT_EQ(lower_freq, m_freq_region.freq());
        // raise freq
        m_freq_region.sample(-5);
        m_freq_region.calc_next_freq();
        EXPECT_EQ(higher_freq, m_freq_region.freq());
    }

    // freq should stay at higher
    for (size_t i = 0; i < 3; ++i) {
        m_freq_region.sample(-3);
        m_freq_region.calc_next_freq();
        EXPECT_EQ(higher_freq, m_freq_region.freq());

        m_freq_region.sample(-5);
        m_freq_region.calc_next_freq();
        EXPECT_EQ(higher_freq, m_freq_region.freq());
    }
}
