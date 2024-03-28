/*
 * Copyright (c) 2015 - 2024, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "gmock/gmock.h"
#include "gtest/gtest.h"

#include "MockPlatformIO.hpp"
#include "MockPlatformTopo.hpp"
#include "TRLFrequencyLimitDetector.hpp"
#include "geopm/Helper.hpp"
#include "geopm/PlatformIO.hpp"
#include "geopm/PlatformTopo.hpp"
#include "geopm_test.hpp"

using ::testing::_;
using testing::AllOf;
using testing::ElementsAre;
using testing::Ge;
using ::testing::Invoke;
using testing::Lt;
using testing::Pair;
using ::testing::Return;

using geopm::TRLFrequencyLimitDetector;

static const double CPU_FREQUENCY_MAX = 3.7e9;
static const double ALL_CORE_TURBO_LIMIT = 2.7e9;
static const double CPU_FREQUENCY_STICKER = 2.1e9;
static const double CPU_FREQUENCY_STEP = 1e8;

static const int CORE_COUNT = 4;

class TRLFrequencyLimitDetectorTest : public ::testing::Test
{
    protected:
        MockPlatformIO m_platform_io;
        MockPlatformTopo m_platform_topo;

        void SetUp();
};

void TRLFrequencyLimitDetectorTest::SetUp()
{
    ON_CALL(m_platform_io, read_signal("CPU_FREQUENCY_MAX_AVAIL", _, _))
        .WillByDefault(Return(CPU_FREQUENCY_MAX));
    ON_CALL(m_platform_io, read_signal("CPU_FREQUENCY_STICKER", _, _))
        .WillByDefault(Return(CPU_FREQUENCY_STICKER));
    ON_CALL(m_platform_io, read_signal("CPU_FREQUENCY_STEP", _, _))
        .WillByDefault(Return(CPU_FREQUENCY_STEP));
    ON_CALL(m_platform_io, read_signal("MSR::TURBO_RATIO_LIMIT:MAX_RATIO_LIMIT_7", _, _))
        .WillByDefault(Return(ALL_CORE_TURBO_LIMIT));

    ON_CALL(m_platform_topo, num_domain(GEOPM_DOMAIN_PACKAGE))
        .WillByDefault(Return(1));
    ON_CALL(m_platform_topo, num_domain(GEOPM_DOMAIN_CORE))
        .WillByDefault(Return(CORE_COUNT));
    std::set<int> cores_in_package;
    for (int i = 0; i < CORE_COUNT; ++i) {
        cores_in_package.insert(i);
    }
    ON_CALL(m_platform_topo, domain_nested(GEOPM_DOMAIN_CORE, GEOPM_DOMAIN_PACKAGE, _))
        .WillByDefault(Return(cores_in_package));
}

TEST_F(TRLFrequencyLimitDetectorTest, returns_single_core_limit_by_default)
{
    TRLFrequencyLimitDetector sst_frequency_limit_detector(m_platform_io, m_platform_topo);

    for (size_t core_idx = 0; core_idx < CORE_COUNT; ++core_idx) {
        EXPECT_THAT(sst_frequency_limit_detector.get_core_frequency_limits(core_idx),
                    ElementsAre(Pair(CORE_COUNT, CPU_FREQUENCY_MAX)));

        EXPECT_EQ(CPU_FREQUENCY_STICKER,
                  sst_frequency_limit_detector.get_core_low_priority_frequency(core_idx));
    }
}

TEST_F(TRLFrequencyLimitDetectorTest, returns_max_observed_frequency_after_update)
{
    TRLFrequencyLimitDetector sst_frequency_limit_detector(m_platform_io, m_platform_topo);

    sst_frequency_limit_detector.update_max_frequency_estimates({1e9, 3e9, 2e9, 2.5e9});

    for (size_t core_idx = 0; core_idx < CORE_COUNT; ++core_idx) {
        EXPECT_THAT(sst_frequency_limit_detector.get_core_frequency_limits(core_idx),
                    ElementsAre(Pair(CORE_COUNT, 3e9)));

        EXPECT_EQ(3e9,
                  sst_frequency_limit_detector.get_core_low_priority_frequency(core_idx));
    }
}
