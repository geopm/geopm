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

#include "EfficientFreqRegion.hpp"
#include "Region.hpp"
#include "Exception.hpp"
#include "MockPlatformIO.hpp"

using geopm::EfficientFreqRegion;

class StubPlatformIO : public MockPlatformIO
{
    public:
        StubPlatformIO()
        {
            m_values[ENERGY_PKG] = 0.0;
            m_values[ENERGY_DRAM] = 0.0;
            m_values[RUNTIME] = NAN;
        }
        virtual ~StubPlatformIO() {}

        enum mock_signal_idx_e {
            ENERGY_PKG,
            ENERGY_DRAM,
            RUNTIME,
        };

        // set the mocked value to be returned
        void set_runtime(double t)
        {
            m_values[RUNTIME] = t;
        }
        void set_energy(double e)
        {
            m_mock_region_energy = e;
        }

        // increase mocked energy
        void run_region()
        {
            if (std::isnan(m_values.at(ENERGY_PKG))) {
                throw std::runtime_error("Mock energy was not given a value.");
            }
            m_values[ENERGY_PKG] += m_mock_region_energy;
        }

        double sample(int signal_idx) override
        {
            return m_values.at(signal_idx);
        }
    private:
        std::map<int, double> m_values;
        double m_mock_region_energy = 0.0;
};

class EfficientFreqRegionTest : public ::testing::Test
{
    protected:
        EfficientFreqRegionTest();
        void SetUp();
        void TearDown();
        double m_freq_min = 1800000000.0;
        double m_freq_max = 2200000000.0;
        double m_freq_step = 100000000.0;
        int m_base_samples = 3;
        int m_num_domain = 1;

        StubPlatformIO m_platform_io;
        EfficientFreqRegion m_freq_region;

        void sample_to_set_baseline();
};

EfficientFreqRegionTest::EfficientFreqRegionTest()
    : m_freq_region(m_platform_io, m_freq_min, m_freq_max, m_freq_step, m_num_domain,
                    {StubPlatformIO::RUNTIME},
                    {StubPlatformIO::ENERGY_PKG},
                    {StubPlatformIO::ENERGY_DRAM})
{

}

void EfficientFreqRegionTest::SetUp()
{
    ASSERT_NE(m_freq_min, m_freq_max);
    ASSERT_NE(0, m_freq_step);
}

void EfficientFreqRegionTest::TearDown()
{

}

void EfficientFreqRegionTest::sample_to_set_baseline()
{
    // freq stays unchanged for several samples to set target
    for (int i = 0; i < m_base_samples; ++i) {
        m_freq_region.update_entry();
        m_platform_io.run_region();
        m_freq_region.update_exit();
        EXPECT_EQ(m_freq_max, m_freq_region.freq());
    }
}

TEST_F(EfficientFreqRegionTest, freq_starts_at_maximum)
{
    ASSERT_EQ(m_freq_max, m_freq_region.freq());
}

TEST_F(EfficientFreqRegionTest, update_ignores_nan_sample)
{
    m_platform_io.set_runtime(NAN);
    sample_to_set_baseline();

    double start = m_freq_region.freq();
    m_freq_region.update_entry();
    m_platform_io.run_region();
    m_freq_region.update_exit();

    m_freq_region.update_entry();
    m_platform_io.run_region();
    m_freq_region.update_exit();
    double end = m_freq_region.freq();
    ASSERT_EQ(start, end);

}

TEST_F(EfficientFreqRegionTest, only_changes_freq_after_enough_samples)
{
    m_platform_io.set_runtime(2);
    sample_to_set_baseline();

    // freq decreases as runtime continues to hit target
    for (int i = 1; i <= 3; ++i) {
        m_freq_region.update_entry();
        m_platform_io.run_region();
        m_freq_region.update_exit();
        EXPECT_EQ(m_freq_region.freq(), m_freq_max - (i*m_freq_step));
    }

    double end = m_freq_region.freq();
    ASSERT_EQ(end, m_freq_max - (3*m_freq_step));
}


TEST_F(EfficientFreqRegionTest, freq_does_not_go_below_min)
{
    // run more times than the number of frequencies
    size_t num_steps = 5 + (size_t)(ceil((m_freq_max-m_freq_min)/m_freq_step));

    m_platform_io.set_runtime(2); // not senstive to freq

    sample_to_set_baseline();

    double start = m_freq_region.freq();
    for (size_t i = 1; i <= num_steps; ++i) {
        m_freq_region.update_entry();
        m_platform_io.run_region();
        m_freq_region.update_exit();
        EXPECT_LT(m_freq_region.freq(), start);
    }

    double end = m_freq_region.freq();
    ASSERT_EQ(end, m_freq_min);

}

TEST_F(EfficientFreqRegionTest, performance_decreases_freq_steps_back_up)
{
    // 90% target will be 3.3
    m_platform_io.set_runtime(3);

    sample_to_set_baseline();

    std::vector<time_t> samples {3, 3, 5};
    std::vector<double> expected {m_freq_max - m_freq_step,
                                  m_freq_max - m_freq_step*2,
                                  m_freq_max - m_freq_step};
    for (size_t i = 0; i < samples.size(); ++i) {
        m_platform_io.set_runtime(samples[i]);
        m_freq_region.update_entry();
        m_platform_io.run_region();
        m_freq_region.update_exit();
        EXPECT_EQ(expected[i], m_freq_region.freq());
    }
}

TEST_F(EfficientFreqRegionTest, energy_increases_freq_steps_back_up)
{
    m_platform_io.set_runtime(3);
    m_platform_io.set_energy(1);

    sample_to_set_baseline();

    std::vector<time_t> samples {1, 1, 5};
    std::vector<double> expected {m_freq_max - m_freq_step,
                                  m_freq_max - m_freq_step*2,
                                  m_freq_max - m_freq_step};
    for (size_t i = 0; i < samples.size(); ++i) {
        m_platform_io.set_energy(samples[i]);
        m_freq_region.update_entry();
        m_platform_io.run_region();
        m_freq_region.update_exit();
        EXPECT_EQ(expected[i], m_freq_region.freq());
    }
}

TEST_F(EfficientFreqRegionTest, after_too_many_increase_freq_stays_at_higher)
{
    m_platform_io.set_runtime(3); // 90% target should be 2.7

    sample_to_set_baseline();

    size_t max_increase = 4;
    double higher_freq = m_freq_max - m_freq_step;
    double lower_freq = m_freq_max - m_freq_step*2;
    // run once to step down from max
    m_freq_region.update_entry();
    m_platform_io.run_region();
    m_freq_region.update_exit();
    EXPECT_EQ(higher_freq, m_freq_region.freq());

    // raise and lower bandwidth and alternate freq
    for (size_t i = 0; i < max_increase; ++i) {
        // lower freq
        m_platform_io.set_runtime(3);
        m_freq_region.update_entry();
        m_platform_io.run_region();
        m_freq_region.update_exit();
        EXPECT_EQ(lower_freq, m_freq_region.freq());
        // raise freq
        m_platform_io.set_runtime(5);
        m_freq_region.update_entry();
        m_platform_io.run_region();
        m_freq_region.update_exit();
        EXPECT_EQ(higher_freq, m_freq_region.freq());
    }

    // freq should stay at higher
    for (size_t i = 0; i < 3; ++i) {
        m_platform_io.set_runtime(3);
        m_freq_region.update_entry();
        m_platform_io.run_region();
        m_freq_region.update_exit();
        EXPECT_EQ(higher_freq, m_freq_region.freq());

        m_platform_io.set_runtime(5);
        m_freq_region.update_entry();
        m_platform_io.run_region();
        m_freq_region.update_exit();
        EXPECT_EQ(higher_freq, m_freq_region.freq());
    }
}
