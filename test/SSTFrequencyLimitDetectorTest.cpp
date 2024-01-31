/*
 * Copyright (c) 2015 - 2024, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "gmock/gmock.h"
#include "gtest/gtest.h"

#include "MockPlatformIO.hpp"
#include "MockPlatformTopo.hpp"
#include "SSTFrequencyLimitDetector.hpp"
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

using geopm::SSTFrequencyLimitDetector;

class SSTFrequencyLimitDetectorTest : public ::testing::Test
{
    public:
        static const double CPU_FREQUENCY_MAX;
        static const double ALL_CORE_TURBO_LIMIT;
        static const double CPU_FREQUENCY_STICKER;
        static const double CPU_FREQUENCY_STEP;
        static const double LP_FREQ_SSE;
        static const double LP_FREQ_AVX2;
        static const double LP_FREQ_AVX512;

        static const int CORE_COUNT;
        static const std::vector<double> HP_CORES;
        static const std::vector<double> HP_FREQS_SSE;
        static const std::vector<double> HP_FREQS_AVX2;
        static const std::vector<double> HP_FREQS_AVX512;

        static const int CLOS_SIGNAL_INDEX_OFFSET;
        static const int SST_ENABLE_SIGNAL_INDEX_OFFSET;
        static const int FREQUENCY_CONTROL_SIGNAL_INDEX_OFFSET;
    protected:
        MockPlatformIO m_platform_io;
        MockPlatformTopo m_platform_topo;

        void SetUp();
};

const double SSTFrequencyLimitDetectorTest::CPU_FREQUENCY_MAX = 3.7e9;
const double SSTFrequencyLimitDetectorTest::ALL_CORE_TURBO_LIMIT = 2.7e9;
const double SSTFrequencyLimitDetectorTest::CPU_FREQUENCY_STICKER = 2.1e9;
const double SSTFrequencyLimitDetectorTest::CPU_FREQUENCY_STEP = 1e8;
const double SSTFrequencyLimitDetectorTest::LP_FREQ_SSE = 2.4e9;
const double SSTFrequencyLimitDetectorTest::LP_FREQ_AVX2 = 2.1e9;
const double SSTFrequencyLimitDetectorTest::LP_FREQ_AVX512 = 1.7e9;

const int SSTFrequencyLimitDetectorTest::CORE_COUNT = 4;
const std::vector<double> SSTFrequencyLimitDetectorTest::HP_CORES = {2, 3, 4};
const std::vector<double> SSTFrequencyLimitDetectorTest::HP_FREQS_SSE = {3.6e9, 3.3e9, 3.0e9};
const std::vector<double> SSTFrequencyLimitDetectorTest::HP_FREQS_AVX2 = {3.5e9, 3.2e9, 2.9e9};
const std::vector<double> SSTFrequencyLimitDetectorTest::HP_FREQS_AVX512 = {3.4e9, 3.1e9, 2.8e9};

const int SSTFrequencyLimitDetectorTest::CLOS_SIGNAL_INDEX_OFFSET = 100;
const int SSTFrequencyLimitDetectorTest::SST_ENABLE_SIGNAL_INDEX_OFFSET = 1000;
const int SSTFrequencyLimitDetectorTest::FREQUENCY_CONTROL_SIGNAL_INDEX_OFFSET = 2000;

void SSTFrequencyLimitDetectorTest::SetUp()
{
    ON_CALL(m_platform_io, read_signal("CPU_FREQUENCY_MAX_AVAIL", _, _))
        .WillByDefault(Return(CPU_FREQUENCY_MAX));
    ON_CALL(m_platform_io, read_signal("CPU_FREQUENCY_STICKER", _, _))
        .WillByDefault(Return(CPU_FREQUENCY_STICKER));
    ON_CALL(m_platform_io, read_signal("CPU_FREQUENCY_STEP", _, _))
        .WillByDefault(Return(CPU_FREQUENCY_STEP));
    ON_CALL(m_platform_io, read_signal("MSR::TURBO_RATIO_LIMIT:MAX_RATIO_LIMIT_7", _, _))
        .WillByDefault(Return(ALL_CORE_TURBO_LIMIT));
    for (size_t i = 0; i < HP_CORES.size(); ++i) {
        ON_CALL(m_platform_io, read_signal("SST::HIGHPRIORITY_NCORES:" + std::to_string(i), _, _))
            .WillByDefault(Return(HP_CORES[i]));
        ON_CALL(m_platform_io, read_signal("SST::HIGHPRIORITY_FREQUENCY_SSE:" + std::to_string(i), _, _))
            .WillByDefault(Return(HP_FREQS_SSE[i]));
        ON_CALL(m_platform_io, read_signal("SST::HIGHPRIORITY_FREQUENCY_AVX2:" + std::to_string(i), _, _))
            .WillByDefault(Return(HP_FREQS_AVX2[i]));
        ON_CALL(m_platform_io, read_signal("SST::HIGHPRIORITY_FREQUENCY_AVX512:" + std::to_string(i), _, _))
            .WillByDefault(Return(HP_FREQS_AVX512[i]));
    }
    ON_CALL(m_platform_io, read_signal("SST::LOWPRIORITY_FREQUENCY:SSE", _, _))
        .WillByDefault(Return(LP_FREQ_SSE));
    ON_CALL(m_platform_io, read_signal("SST::LOWPRIORITY_FREQUENCY:AVX2", _, _))
        .WillByDefault(Return(LP_FREQ_AVX2));
    ON_CALL(m_platform_io, read_signal("SST::LOWPRIORITY_FREQUENCY:AVX512", _, _))
        .WillByDefault(Return(LP_FREQ_AVX512));

    ON_CALL(m_platform_io, push_signal("SST::COREPRIORITY:ASSOCIATION", GEOPM_DOMAIN_CORE, _))
        .WillByDefault(Invoke(
            [](const std::string &, int, int core_idx) {
                return CLOS_SIGNAL_INDEX_OFFSET + core_idx;
            }));
    ON_CALL(m_platform_io, push_signal("SST::TURBO_ENABLE:ENABLE", GEOPM_DOMAIN_PACKAGE, _))
        .WillByDefault(Invoke(
            [](const std::string &, int, int package_idx) {
                return SST_ENABLE_SIGNAL_INDEX_OFFSET + package_idx;
            }));
    ON_CALL(m_platform_io, push_signal("CPU_FREQUENCY_MAX_CONTROL", GEOPM_DOMAIN_CORE, _))
        .WillByDefault(Invoke(
            [](const std::string &, int, int core_idx) {
                return FREQUENCY_CONTROL_SIGNAL_INDEX_OFFSET + core_idx;
            }));

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

TEST_F(SSTFrequencyLimitDetectorTest, returns_single_core_limit_by_default)
{
    SSTFrequencyLimitDetector sst_frequency_limit_detector(m_platform_io, m_platform_topo);

    for (size_t core_idx = 0; core_idx < CORE_COUNT; ++core_idx) {
        EXPECT_THAT(sst_frequency_limit_detector.get_core_frequency_limits(core_idx),
                    ElementsAre(Pair(CORE_COUNT, CPU_FREQUENCY_MAX)));

        EXPECT_EQ(CPU_FREQUENCY_STICKER,
                  sst_frequency_limit_detector.get_core_low_priority_frequency(core_idx));
    }
}

TEST_F(SSTFrequencyLimitDetectorTest, returns_max_observed_frequency_when_sst_disabled)
{
    SSTFrequencyLimitDetector sst_frequency_limit_detector(m_platform_io, m_platform_topo);

    EXPECT_CALL(m_platform_io, sample(SST_ENABLE_SIGNAL_INDEX_OFFSET)).WillOnce(Return(0));

    sst_frequency_limit_detector.update_max_frequency_estimates({1e9, 3e9, 2e9, 2.5e9});

    for (size_t core_idx = 0; core_idx < CORE_COUNT; ++core_idx) {
        EXPECT_THAT(sst_frequency_limit_detector.get_core_frequency_limits(core_idx),
                    ElementsAre(Pair(CORE_COUNT, 3e9)));

        EXPECT_EQ(CPU_FREQUENCY_STICKER,
                  sst_frequency_limit_detector.get_core_low_priority_frequency(core_idx));
    }
}

TEST_F(SSTFrequencyLimitDetectorTest, detects_nearest_license_level_limit_bucket_0)
{
    SSTFrequencyLimitDetector sst_frequency_limit_detector(m_platform_io, m_platform_topo);

    EXPECT_CALL(m_platform_io, sample(SST_ENABLE_SIGNAL_INDEX_OFFSET)).WillOnce(Return(1));
    EXPECT_CALL(m_platform_io,
                sample(AllOf(
                    Ge(FREQUENCY_CONTROL_SIGNAL_INDEX_OFFSET),
                    Lt(FREQUENCY_CONTROL_SIGNAL_INDEX_OFFSET + CORE_COUNT))))
        .WillRepeatedly(Return(CPU_FREQUENCY_MAX));

    // The first two cores are currently configured as high priority.
    const int hp_core_count = 2;
    EXPECT_CALL(m_platform_io,
                sample(AllOf(Ge(CLOS_SIGNAL_INDEX_OFFSET), Lt(CLOS_SIGNAL_INDEX_OFFSET + hp_core_count))))
        .Times(hp_core_count)
        .WillRepeatedly(Return(0));
    // Other cores are low priority
    EXPECT_CALL(m_platform_io,
                sample(AllOf(Ge(CLOS_SIGNAL_INDEX_OFFSET + hp_core_count), Lt(CLOS_SIGNAL_INDEX_OFFSET + CORE_COUNT))))
        .WillRepeatedly(Return(3));

    EXPECT_GE(HP_CORES[0], hp_core_count)
        << "Self-consistency check on the test configuration. This test case "
           "expects to be in SST-TF bucket 0.";
    sst_frequency_limit_detector.update_max_frequency_estimates({
        HP_FREQS_SSE[0] - 5e7,    // Just under the SSE limit, but not quite reaching AVX2
        HP_FREQS_AVX512[0] - 2e8, // Far under the AVX512 limit
        1e9, 1e9                  // Don't care about these cores for this test
    });

    EXPECT_THAT(sst_frequency_limit_detector.get_core_frequency_limits(0),
                ElementsAre(Pair(HP_CORES[0], HP_FREQS_SSE[0]),
                            Pair(HP_CORES[1], HP_FREQS_SSE[1]),
                            Pair(HP_CORES[2], HP_FREQS_SSE[2])));
    EXPECT_EQ(LP_FREQ_SSE,
              sst_frequency_limit_detector.get_core_low_priority_frequency(0));

    EXPECT_THAT(sst_frequency_limit_detector.get_core_frequency_limits(1),
                ElementsAre(Pair(HP_CORES[0], HP_FREQS_AVX512[0]),
                            Pair(HP_CORES[1], HP_FREQS_AVX512[1]),
                            Pair(HP_CORES[2], HP_FREQS_AVX512[2])));
    EXPECT_EQ(LP_FREQ_AVX512,
              sst_frequency_limit_detector.get_core_low_priority_frequency(1));
}

TEST_F(SSTFrequencyLimitDetectorTest, detects_nearest_license_level_limit_bucket_1)
{
    SSTFrequencyLimitDetector sst_frequency_limit_detector(m_platform_io, m_platform_topo);

    EXPECT_CALL(m_platform_io, sample(SST_ENABLE_SIGNAL_INDEX_OFFSET)).WillOnce(Return(1));
    EXPECT_CALL(m_platform_io,
                sample(AllOf(
                    Ge(FREQUENCY_CONTROL_SIGNAL_INDEX_OFFSET),
                    Lt(FREQUENCY_CONTROL_SIGNAL_INDEX_OFFSET + CORE_COUNT))))
        .WillRepeatedly(Return(CPU_FREQUENCY_MAX));

    // The first three cores are currently configured as high priority.
    const int hp_core_count = 3;
    EXPECT_CALL(m_platform_io,
                sample(AllOf(Ge(CLOS_SIGNAL_INDEX_OFFSET), Lt(CLOS_SIGNAL_INDEX_OFFSET + hp_core_count))))
        .Times(hp_core_count)
        .WillRepeatedly(Return(0));
    // Other cores are low priority
    EXPECT_CALL(m_platform_io,
                sample(AllOf(Ge(CLOS_SIGNAL_INDEX_OFFSET + hp_core_count), Lt(CLOS_SIGNAL_INDEX_OFFSET + CORE_COUNT))))
        .WillRepeatedly(Return(3));

    EXPECT_GE(HP_CORES[1], hp_core_count)
        << "Self-consistency check on the test configuration. This test case "
           "expects to be in SST-TF bucket 1.";
    sst_frequency_limit_detector.update_max_frequency_estimates({
        HP_FREQS_SSE[1] - 5e7,    // Just under the SSE limit, but not quite reaching AVX2
        HP_FREQS_AVX2[1],         // Equal to the AVX2 limit -- Assume our limit is AVX2
        HP_FREQS_AVX512[1] - 2e8, // Far under the AVX512 limit
        1e9                       // Don't care about this core for this test
    });

    EXPECT_THAT(sst_frequency_limit_detector.get_core_frequency_limits(0),
                ElementsAre(Pair(HP_CORES[0], HP_FREQS_SSE[0]),
                            Pair(HP_CORES[1], HP_FREQS_SSE[1]),
                            Pair(HP_CORES[2], HP_FREQS_SSE[2])));
    EXPECT_EQ(LP_FREQ_SSE,
              sst_frequency_limit_detector.get_core_low_priority_frequency(0));

    EXPECT_THAT(sst_frequency_limit_detector.get_core_frequency_limits(1),
                ElementsAre(Pair(HP_CORES[0], HP_FREQS_AVX2[0]),
                            Pair(HP_CORES[1], HP_FREQS_AVX2[1]),
                            Pair(HP_CORES[2], HP_FREQS_AVX2[2])));
    EXPECT_EQ(LP_FREQ_AVX2,
              sst_frequency_limit_detector.get_core_low_priority_frequency(1));

    EXPECT_THAT(sst_frequency_limit_detector.get_core_frequency_limits(2),
                ElementsAre(Pair(HP_CORES[0], HP_FREQS_AVX512[0]),
                            Pair(HP_CORES[1], HP_FREQS_AVX512[1]),
                            Pair(HP_CORES[2], HP_FREQS_AVX512[2])));
    EXPECT_EQ(LP_FREQ_AVX512,
              sst_frequency_limit_detector.get_core_low_priority_frequency(2));
}

TEST_F(SSTFrequencyLimitDetectorTest, limits_license_level_search_if_frequency_capped)
{
    SSTFrequencyLimitDetector sst_frequency_limit_detector(m_platform_io, m_platform_topo);

    EXPECT_CALL(m_platform_io, sample(SST_ENABLE_SIGNAL_INDEX_OFFSET)).WillOnce(Return(1));
    static const double AVX2_FREQUENCY_CAP = HP_FREQS_AVX2[0];
    EXPECT_CALL(m_platform_io,
                sample(AllOf(
                    Ge(FREQUENCY_CONTROL_SIGNAL_INDEX_OFFSET),
                    Lt(FREQUENCY_CONTROL_SIGNAL_INDEX_OFFSET + CORE_COUNT))))
        // Act like there's a CPU frequency cap at the AVX2 ceiling.
        .WillRepeatedly(Return(AVX2_FREQUENCY_CAP));

    // The first two cores are currently configured as high priority.
    const int hp_core_count = 2;
    EXPECT_CALL(m_platform_io,
                sample(AllOf(Ge(CLOS_SIGNAL_INDEX_OFFSET), Lt(CLOS_SIGNAL_INDEX_OFFSET + hp_core_count))))
        .Times(hp_core_count)
        .WillRepeatedly(Return(0));
    // Other cores are low priority
    EXPECT_CALL(m_platform_io,
                sample(AllOf(Ge(CLOS_SIGNAL_INDEX_OFFSET + hp_core_count), Lt(CLOS_SIGNAL_INDEX_OFFSET + CORE_COUNT))))
        .WillRepeatedly(Return(3));

    EXPECT_GE(HP_CORES[0], hp_core_count)
        << "Self-consistency check on the test configuration. This test case "
           "expects to be in SST-TF bucket 0.";
    sst_frequency_limit_detector.update_max_frequency_estimates({
        HP_FREQS_AVX2[0],   // Achieved everything we gave this core
        HP_FREQS_AVX512[0], // Not quite able to achieve the frequency cap
        1e9, 1e9            // Don't care about these cores for this test
    });

    // Core 0 exactly achieved the AVX2 ceiling, but that was also our cap.
    // Assume that it could potentially go faster if we relaxed the cap.
    EXPECT_THAT(sst_frequency_limit_detector.get_core_frequency_limits(0),
                ElementsAre(Pair(HP_CORES[0], HP_FREQS_SSE[0]),
                            Pair(HP_CORES[1], HP_FREQS_SSE[1]),
                            Pair(HP_CORES[2], HP_FREQS_SSE[2])));
    EXPECT_EQ(LP_FREQ_SSE,
              sst_frequency_limit_detector.get_core_low_priority_frequency(0));

    // Core 1 achieved less than our cap. So assume the nearest SST-TF limit is
    // the limiting factor.
    EXPECT_THAT(sst_frequency_limit_detector.get_core_frequency_limits(1),
                ElementsAre(Pair(HP_CORES[0], HP_FREQS_AVX512[0]),
                            Pair(HP_CORES[1], HP_FREQS_AVX512[1]),
                            Pair(HP_CORES[2], HP_FREQS_AVX512[2])));
    EXPECT_EQ(LP_FREQ_AVX512,
              sst_frequency_limit_detector.get_core_low_priority_frequency(1));
}

