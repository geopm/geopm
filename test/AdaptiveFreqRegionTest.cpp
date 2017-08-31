/*
 * Copyright (c) 2015, 2016, 2017, 2018, Intel Corporation
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

#include "AdaptiveFreqRegion.hpp"
#include "Region.hpp"
#include "Exception.hpp"

using geopm::AdaptiveFreqRegion;

class StubRegion : public geopm::Region
{
    public:
        StubRegion() : Region(0, 0, 0, nullptr) {}
        virtual ~StubRegion() {}

        // set the mocked value to be returned
        void set_runtime(double t)
        {
            m_mock_runtime = t;
        }
        void set_energy(double e)
        {
            m_mock_region_energy = e;
        }

        // increase mocked time
        void run_region()
        {
            m_current_time += m_mock_runtime;
            m_current_energy += m_mock_region_energy;
        }

        // force time to be mocked value
        struct geopm_time_s telemetry_timestamp(size_t sample_idx) override
        {
            return {m_current_time, 0};
        }

        double signal(int domain_idx, int signal_type) override
        {
            if (signal_type == GEOPM_TELEMETRY_TYPE_PKG_ENERGY) {
                return m_current_energy;
            }
            else if (signal_type == GEOPM_TELEMETRY_TYPE_DRAM_ENERGY) {
                return 1.0;
            }
            else {
                throw std::runtime_error("AdaptiveFreqRegion used unexpected signal: " +
                                         std::to_string(signal_type));
            }
        }

    private:
        time_t m_current_time = 0;
        time_t m_mock_runtime = 0;
        double m_current_energy = 0.0;
        double m_mock_region_energy = 0.0;
};

class AdaptiveFreqRegionTest : public ::testing::Test
{
    protected:
        AdaptiveFreqRegionTest();
        void SetUp();
        void TearDown();
        double m_freq_min = 1800000000.0;
        double m_freq_max = 2200000000.0;
        double m_freq_step = 100000000.0;
        int m_base_samples = 4;
        int m_num_domain = 1;

        StubRegion m_region;
        AdaptiveFreqRegion m_freq_region;

        void sample_to_set_baseline();
};

AdaptiveFreqRegionTest::AdaptiveFreqRegionTest()
    : m_freq_region(&m_region, m_freq_min, m_freq_max, m_freq_step, m_num_domain)
{

}

void AdaptiveFreqRegionTest::SetUp()
{
    ASSERT_NE(m_freq_min, m_freq_max);
    ASSERT_NE(0, m_freq_step);
}

void AdaptiveFreqRegionTest::TearDown()
{

}

void AdaptiveFreqRegionTest::sample_to_set_baseline()
{
    // freq stays unchanged for several samples to set target
    for (int i = 0; i < m_base_samples; ++i) {
        m_freq_region.update_entry();
        m_region.run_region();
        m_freq_region.update_exit();
        EXPECT_EQ(m_freq_max, m_freq_region.freq());
    }
}

TEST_F(AdaptiveFreqRegionTest, construct_with_null_throws)
{
    ASSERT_THROW(AdaptiveFreqRegion(nullptr, m_freq_min, m_freq_max, m_freq_step,
                                    m_num_domain),
                 geopm::Exception);
}

TEST_F(AdaptiveFreqRegionTest, freq_starts_at_maximum)
{
    ASSERT_EQ(m_freq_max, m_freq_region.freq());
}

TEST_F(AdaptiveFreqRegionTest, update_ignores_nan_sample)
{
    m_region.set_runtime(NAN);
    sample_to_set_baseline();

    double start = m_freq_region.freq();
    m_freq_region.update_entry();
    m_region.run_region();
    m_freq_region.update_exit();

    m_freq_region.update_entry();
    m_region.run_region();
    m_freq_region.update_exit();
    double end = m_freq_region.freq();
    ASSERT_EQ(start, end);

}

TEST_F(AdaptiveFreqRegionTest, only_changes_freq_after_enough_samples)
{
    m_region.set_runtime(2);
    sample_to_set_baseline();

    // freq decreases as runtime continues to hit target
    for (int i = 1; i <= 3; ++i) {
        m_freq_region.update_entry();
        m_region.run_region();
        m_freq_region.update_exit();
        EXPECT_EQ(m_freq_region.freq(), m_freq_max - (i*m_freq_step));
    }

    double end = m_freq_region.freq();
    ASSERT_EQ(end, m_freq_max - (3*m_freq_step));
}


TEST_F(AdaptiveFreqRegionTest, freq_does_not_go_below_min)
{
    // run more times than the number of frequencies
    size_t num_steps = 5 + (size_t)(ceil((m_freq_max-m_freq_min)/m_freq_step));

    m_region.set_runtime(2); // not senstive to freq

    sample_to_set_baseline();

    double start = m_freq_region.freq();
    for (size_t i = 1; i <= num_steps; ++i) {
        m_freq_region.update_entry();
        m_region.run_region();
        m_freq_region.update_exit();
        EXPECT_LT(m_freq_region.freq(), start);
    }

    double end = m_freq_region.freq();
    ASSERT_EQ(end, m_freq_min);

}

TEST_F(AdaptiveFreqRegionTest, performance_decreases_freq_steps_back_up)
{
    // 90% target will be 3.3
    m_region.set_runtime(3);

    sample_to_set_baseline();

    std::vector<time_t> samples {3, 3, 5};
    std::vector<double> expected {m_freq_max - m_freq_step,
                                  m_freq_max - m_freq_step*2,
                                  m_freq_max - m_freq_step};
    for (size_t i = 0; i < samples.size(); ++i) {
        m_region.set_runtime(samples[i]);
        m_freq_region.update_entry();
        m_region.run_region();
        m_freq_region.update_exit();
        EXPECT_EQ(expected[i], m_freq_region.freq());
    }
}

TEST_F(AdaptiveFreqRegionTest, energy_increases_freq_steps_back_up)
{
    m_region.set_runtime(3);
    m_region.set_energy(1);

    sample_to_set_baseline();

    std::vector<time_t> samples {1, 1, 5};
    std::vector<double> expected {m_freq_max - m_freq_step,
                                  m_freq_max - m_freq_step*2,
                                  m_freq_max - m_freq_step};
    for (size_t i = 0; i < samples.size(); ++i) {
        m_region.set_energy(samples[i]);
        m_freq_region.update_entry();
        m_region.run_region();
        m_freq_region.update_exit();
        EXPECT_EQ(expected[i], m_freq_region.freq());
    }
}

TEST_F(AdaptiveFreqRegionTest, after_too_many_increase_freq_stays_at_higher)
{
    m_region.set_runtime(3); // 90% target should be 2.7

    sample_to_set_baseline();

    size_t max_increase = 4;
    double higher_freq = m_freq_max - m_freq_step;
    double lower_freq = m_freq_max - m_freq_step*2;
    // run once to step down from max
    m_freq_region.update_entry();
    m_region.run_region();
    m_freq_region.update_exit();
    EXPECT_EQ(higher_freq, m_freq_region.freq());

    // raise and lower bandwidth and alternate freq
    for (size_t i = 0; i < max_increase; ++i) {
        // lower freq
        m_region.set_runtime(3);
        m_freq_region.update_entry();
        m_region.run_region();
        m_freq_region.update_exit();
        EXPECT_EQ(lower_freq, m_freq_region.freq());
        // raise freq
        m_region.set_runtime(5);
        m_freq_region.update_entry();
        m_region.run_region();
        m_freq_region.update_exit();
        EXPECT_EQ(higher_freq, m_freq_region.freq());
    }

    // freq should stay at higher
    for (size_t i = 0; i < 3; ++i) {
        m_region.set_runtime(3);
        m_freq_region.update_entry();
        m_region.run_region();
        m_freq_region.update_exit();
        EXPECT_EQ(higher_freq, m_freq_region.freq());

        m_region.set_runtime(5);
        m_freq_region.update_entry();
        m_region.run_region();
        m_freq_region.update_exit();
        EXPECT_EQ(higher_freq, m_freq_region.freq());
    }
}
