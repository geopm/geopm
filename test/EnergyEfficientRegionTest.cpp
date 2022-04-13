/*
 * Copyright (c) 2015 - 2022, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <cmath>

#include "gtest/gtest.h"
#include "gmock/gmock.h"

#include "EnergyEfficientRegion.hpp"
#include "geopm/Exception.hpp"
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
