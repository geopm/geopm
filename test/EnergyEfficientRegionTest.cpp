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
    m_freq_region.update_exit(NAN);

    double end = m_freq_region.freq();
    ASSERT_EQ(start, end);
}
