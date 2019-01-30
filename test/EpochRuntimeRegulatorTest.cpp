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

#include <map>

#include "gtest/gtest.h"
#include "gmock/gmock.h"

#include "geopm.h"
#include "geopm_internal.h"
#include "EpochRuntimeRegulator.hpp"
#include "MockPlatformIO.hpp"
#include "MockPlatformTopo.hpp"
#include "geopm_test.hpp"

using geopm::EpochRuntimeRegulator;
using geopm::IPlatformTopo;
using testing::Return;
using testing::_;

class EpochRuntimeRegulatorTest : public ::testing::Test
{
    protected:
        EpochRuntimeRegulatorTest();
        void SetUp();
        std::map<uint64_t, std::vector<double> > m_runtime_steps;
        std::map<uint64_t, double> m_total_runtime;
        static constexpr int M_NUM_RANK = 2;
        EpochRuntimeRegulator m_regulator;
        MockPlatformIO m_platform_io;
        MockPlatformTopo m_platform_topo;
};

//constexpr EpochRuntimeRegulatorTest::M_NUM_RANKS;
EpochRuntimeRegulatorTest::EpochRuntimeRegulatorTest()
    : m_regulator(M_NUM_RANK, m_platform_io, m_platform_topo)
{

}

void EpochRuntimeRegulatorTest::SetUp()
{

}

TEST_F(EpochRuntimeRegulatorTest, invalid_ranks)
{
    GEOPM_EXPECT_THROW_MESSAGE(EpochRuntimeRegulator(-1, m_platform_io, m_platform_topo),
                               GEOPM_ERROR_RUNTIME, "invalid max rank count");
    GEOPM_EXPECT_THROW_MESSAGE(EpochRuntimeRegulator(0, m_platform_io, m_platform_topo),
                               GEOPM_ERROR_RUNTIME, "invalid max rank count");
    GEOPM_EXPECT_THROW_MESSAGE(m_regulator.record_entry(GEOPM_REGION_HASH_UNMARKED, -1, {{1,1}}),
                               GEOPM_ERROR_RUNTIME, "invalid rank value");
    GEOPM_EXPECT_THROW_MESSAGE(m_regulator.record_entry(GEOPM_REGION_HASH_UNMARKED, 99, {{1,1}}),
                               GEOPM_ERROR_RUNTIME, "invalid rank value");
    GEOPM_EXPECT_THROW_MESSAGE(m_regulator.record_exit(GEOPM_REGION_HASH_UNMARKED, -1, {{1,1}}),
                               GEOPM_ERROR_RUNTIME, "invalid rank value");
    GEOPM_EXPECT_THROW_MESSAGE(m_regulator.record_exit(GEOPM_REGION_HASH_UNMARKED, 99, {{1,1}}),
                               GEOPM_ERROR_RUNTIME, "invalid rank value");

}

TEST_F(EpochRuntimeRegulatorTest, unknown_region)
{
    uint64_t region_id = 0x98765432;
    EXPECT_FALSE(m_regulator.is_regulated(region_id));
    GEOPM_EXPECT_THROW_MESSAGE(m_regulator.region_regulator(region_id),
                               GEOPM_ERROR_RUNTIME, "unknown region detected");
    GEOPM_EXPECT_THROW_MESSAGE(m_regulator.record_exit(region_id, 0, {{1,1}}),
                               GEOPM_ERROR_RUNTIME, "unknown region detected");

    m_regulator.record_entry(region_id, 0, {{1,1}});
    EXPECT_TRUE(m_regulator.is_regulated(region_id));
}

TEST_F(EpochRuntimeRegulatorTest, rank_enter_exit_trace)
{
    return;
    uint64_t region_id = 0x98765432;
    geopm_time_s start0 {{1, 0}};
    geopm_time_s start1 {{2, 1}};
    geopm_time_s end0 {{11, 0}};
    geopm_time_s end1 {{12, 1}};

    m_regulator.record_entry(region_id, 0, start0);
    m_regulator.record_entry(region_id, 1, start1);
    m_regulator.record_exit(region_id, 0, end0);
    m_regulator.record_exit(region_id, 1, end1);
    auto region_info = m_regulator.region_info();
    ASSERT_EQ(2u, region_info.size());
    std::vector<double> expected_progress = {0.0, 1.0};
    std::vector<double> expected_runtime = {0.0, 10.0};
    size_t idx = 0;
    // region info should be based on last entry and first exit
    // for time all the ranks were in the region
    for (const auto &info : region_info) {
        EXPECT_EQ(region_id, info.region_hash);
        EXPECT_EQ(region_id, info.region_hint);
        EXPECT_EQ(expected_progress[idx], info.progress);
        EXPECT_EQ(expected_runtime[idx], info.runtime);
        ++idx;
    }
    EXPECT_EQ(1, m_regulator.total_count(region_id));

    m_regulator.clear_region_info();
    // single ranks do not change region info list
    m_regulator.record_entry(region_id, 0, start0);
    m_regulator.record_exit(region_id, 0, end0);
    region_info = m_regulator.region_info();
    EXPECT_EQ(0u, region_info.size());
}

TEST_F(EpochRuntimeRegulatorTest, all_ranks_enter_exit)
{
    uint64_t region_id = 0x98765432;
    geopm_time_s start {{1, 0}};
    geopm_time_s end[M_NUM_RANK] = {
        {{10, 0}}, {{12, 0}}
    };

    for (int rank = 0; rank < M_NUM_RANK; ++rank) {
        m_regulator.record_entry(region_id, rank, start);
        m_regulator.record_exit(region_id, rank, end[rank]);
    }

    EXPECT_DOUBLE_EQ(10.0, m_regulator.total_region_runtime(region_id));
}

TEST_F(EpochRuntimeRegulatorTest, epoch_runtime)
{
    return;
    int num_package = 2;
    int num_memory = 1;
    EXPECT_CALL(m_platform_topo, num_domain(IPlatformTopo::M_DOMAIN_PACKAGE)).Times(6)
        .WillRepeatedly(Return(num_package));
    EXPECT_CALL(m_platform_topo, num_domain(IPlatformTopo::M_DOMAIN_BOARD_MEMORY)).Times(6)
        .WillRepeatedly(Return(num_memory));
    EXPECT_CALL(m_platform_io, read_signal("ENERGY_PACKAGE", _, _))
        .Times(num_package * 6);
    EXPECT_CALL(m_platform_io, read_signal("ENERGY_DRAM", _, _))
        .Times(num_memory * 6);

    uint64_t region_id = 0x98765432;
    m_regulator.record_entry(region_id, 0, {{1, 0}});
    m_regulator.record_entry(region_id, 1, {{1, 0}});
    m_regulator.record_exit(region_id, 0, {{2, 0}});
    m_regulator.record_exit(region_id, 1, {{2, 0}});
    m_regulator.epoch(0, {{2, 0}});
    m_regulator.epoch(1, {{2, 0}});
    m_regulator.record_entry(region_id, 0, {{2, 0}});
    m_regulator.record_entry(region_id, 1, {{2, 0}});
    m_regulator.record_exit(region_id, 0, {{3, 0}});
    m_regulator.record_exit(region_id, 1, {{3, 0}});
    m_regulator.epoch(0, {{3, 0}});
    m_regulator.epoch(1, {{3, 0}});
    m_regulator.record_entry(region_id, 0, {{3, 0}});
    m_regulator.record_entry(region_id, 1, {{3, 0}});
    m_regulator.record_exit(region_id, 0, {{4, 0}});
    m_regulator.record_exit(region_id, 1, {{4, 0}});
    m_regulator.epoch(0, {{4, 0}});
    m_regulator.epoch(1, {{4, 0}});

    EXPECT_EQ(3, m_regulator.total_count(region_id));
    EXPECT_EQ(2, m_regulator.total_count(GEOPM_REGION_HASH_EPOCH));
    EXPECT_DOUBLE_EQ(3.0, m_regulator.total_region_runtime(region_id));
    EXPECT_DOUBLE_EQ(2.0, m_regulator.total_region_runtime(GEOPM_REGION_HASH_EPOCH));
}
