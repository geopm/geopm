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

#include <vector>

#include "gtest/gtest.h"
#include "gmock/gmock.h"

#include "RegionAggregator.hpp"
#include "Helper.hpp"
#include "PlatformTopo.hpp"
#include "Agg.hpp"
#include "MockPlatformIO.hpp"
#include "geopm.h"
#include "geopm_hash.h"
#include "geopm_internal.h"
#include "geopm_hash.h"
#include "geopm_test.hpp"

using geopm::RegionAggregator;
using geopm::IPlatformIO;
using geopm::IPlatformTopo;
using testing::_;
using testing::Return;

class RegionAggregatorTest : public ::testing::Test
{
    protected:
        void SetUp(void);

        std::unique_ptr<RegionAggregator> m_agg;
        MockPlatformIO m_platio;
        enum M_SIGNAL {
            M_SIGNAL_TIME,
            M_SIGNAL_ENERGY_0,
            M_SIGNAL_ENERGY_1,
            M_SIGNAL_CYCLES_0,
            M_SIGNAL_CYCLES_1,
            M_SIGNAL_CYCLES_2,
            M_SIGNAL_CYCLES_3,
            M_SIGNAL_R_HASH_BOARD,
            M_SIGNAL_R_HASH_PKG_0,
            M_SIGNAL_R_HASH_PKG_1,
            M_SIGNAL_R_HASH_CPU_0,
            M_SIGNAL_R_HASH_CPU_1,
            M_SIGNAL_R_HASH_CPU_2,
            M_SIGNAL_R_HASH_CPU_3,
            M_SIGNAL_EPOCH_COUNT
        };
};

void RegionAggregatorTest::SetUp(void)
{
    ON_CALL(m_platio, push_signal("TIME", IPlatformTopo::M_DOMAIN_BOARD, 0))
        .WillByDefault(Return(M_SIGNAL_TIME));
    ON_CALL(m_platio, push_signal("ENERGY", IPlatformTopo::M_DOMAIN_PACKAGE, 0))
        .WillByDefault(Return(M_SIGNAL_ENERGY_0));
    ON_CALL(m_platio, push_signal("ENERGY", IPlatformTopo::M_DOMAIN_PACKAGE, 1))
        .WillByDefault(Return(M_SIGNAL_ENERGY_1));
    ON_CALL(m_platio, push_signal("CYCLES", IPlatformTopo::M_DOMAIN_CPU, 0))
        .WillByDefault(Return(M_SIGNAL_CYCLES_0));
    ON_CALL(m_platio, push_signal("CYCLES", IPlatformTopo::M_DOMAIN_CPU, 1))
        .WillByDefault(Return(M_SIGNAL_CYCLES_1));
    ON_CALL(m_platio, push_signal("CYCLES", IPlatformTopo::M_DOMAIN_CPU, 2))
        .WillByDefault(Return(M_SIGNAL_CYCLES_2));
    ON_CALL(m_platio, push_signal("CYCLES", IPlatformTopo::M_DOMAIN_CPU, 3))
        .WillByDefault(Return(M_SIGNAL_CYCLES_3));
    ON_CALL(m_platio, push_signal("REGION_HASH", IPlatformTopo::M_DOMAIN_BOARD, 0))
        .WillByDefault(Return(M_SIGNAL_R_HASH_BOARD));
    ON_CALL(m_platio, push_signal("REGION_HASH", IPlatformTopo::M_DOMAIN_PACKAGE, 0))
        .WillByDefault(Return(M_SIGNAL_R_HASH_PKG_0));
    ON_CALL(m_platio, push_signal("REGION_HASH", IPlatformTopo::M_DOMAIN_PACKAGE, 1))
        .WillByDefault(Return(M_SIGNAL_R_HASH_PKG_1));
    ON_CALL(m_platio, push_signal("REGION_HASH", IPlatformTopo::M_DOMAIN_CPU, 0))
        .WillByDefault(Return(M_SIGNAL_R_HASH_CPU_0));
    ON_CALL(m_platio, push_signal("REGION_HASH", IPlatformTopo::M_DOMAIN_CPU, 1))
        .WillByDefault(Return(M_SIGNAL_R_HASH_CPU_1));
    ON_CALL(m_platio, push_signal("REGION_HASH", IPlatformTopo::M_DOMAIN_CPU, 2))
        .WillByDefault(Return(M_SIGNAL_R_HASH_CPU_2));
    ON_CALL(m_platio, push_signal("REGION_HASH", IPlatformTopo::M_DOMAIN_CPU, 3))
        .WillByDefault(Return(M_SIGNAL_R_HASH_CPU_3));

    EXPECT_CALL(m_platio, push_signal("EPOCH_COUNT", _, _))
        .WillOnce(Return(M_SIGNAL_EPOCH_COUNT));
    m_agg = geopm::make_unique<RegionAggregator>(m_platio);
    m_agg->init();
}

TEST_F(RegionAggregatorTest, sample_total)
{
    uint64_t regionA = 0x4444;
    uint64_t regionB = 0x5555;
    double regA = regionA;
    double regB = regionB;

    // sampled values for REGION_HASH on each CPU, package, and board
    std::vector<double> rid_cpu_0 { regA, regA, regA, regB, regB, regA, regA, regA };
    std::vector<double> rid_cpu_1 { regA, regA, regA, regB, regB, regB, regA, regA };
    std::vector<double> rid_cpu_2 { regA, regA, regB, regB, regB, regB, regB, regA };
    std::vector<double> rid_cpu_3 { regA, regA, regB, regB, regA, regA, regA, regA };
    int num_sample = rid_cpu_0.size();
    std::vector<double> rid_pkg_0(num_sample);
    std::vector<double> rid_pkg_1(num_sample);
    std::vector<double> rid_board(num_sample);
    auto agg = geopm::Agg::region_hash;
    // set up regions for larger domains
    // 2 CPU per package, 2 packages, 1 board
    for (int idx = 0; idx < num_sample; ++idx) {
        rid_pkg_0[idx] = agg({rid_cpu_0[idx], rid_cpu_1[idx]});
        rid_pkg_1[idx] = agg({rid_cpu_2[idx], rid_cpu_3[idx]});
        rid_board[idx] = agg({rid_cpu_0[idx], rid_cpu_1[idx], rid_cpu_2[idx], rid_cpu_3[idx]});
    }

    // sampled values and expected totals
    std::vector<double> time[1] {{0, 1, 2, 3, 4, 5, 6, 7}};
    std::map<uint64_t, double> exp_time[1] {
        { // board 0
            {regionA, 2.0},
            {regionB, 1.0},
            {GEOPM_REGION_HASH_UNMARKED, 4.0}
        }
    };

    std::vector<double> energy[2] {{0, 100, 200, 300, 400, 500, 600, 700},
                                   {0, 101, 202, 303, 404, 505, 606, 707}};
    std::map<uint64_t, double> exp_energy[2] {
        { // package 0
            {regionA, 400},
            {regionB, 200},
            {GEOPM_REGION_HASH_UNMARKED, 100}
        },
        { // package 1
            {regionA, 202},
            {regionB, 202},
            {GEOPM_REGION_HASH_UNMARKED, 303},
        }
    };

    std::vector<double> cycles[4] {{0, 1111, 2222, 3333, 4400, 5500, 6600, 7700},
                                   {0, 1000, 2000, 3003, 4004, 5000, 6000, 7007},
                                   {0, 1010, 2000, 3000, 4040, 5000, 6000, 7070},
                                   {0, 1111, 2200, 3333, 4400, 5555, 6600, 7777}};
    // CPU-scoped signals will have no unmarked because each CPU always has a rank at every step
    std::map<uint64_t, double> exp_cycles[4] {
        { // cpu 0
            {regionA, 1111 + 1111 + 1111 +    0 +    0 + 1100 + 1100},
            {regionB,    0 +    0 +    0 + 1067 + 1100 +    0 +    0},
            {GEOPM_REGION_HASH_UNMARKED, 0.0}
        },
        { // cpu 1
            {regionA, 1000 + 1000 + 1003 +    0 +    0 +    0 + 1007},
            {regionB,    0 +    0 +    0 + 1001 +  996 + 1000 +    0},
            {GEOPM_REGION_HASH_UNMARKED, 0.0}
        },
        { // cpu 2
            {regionA, 1010 +  990 +    0 +    0 +    0 +    0 +    0},
            {regionB,    0 +    0 + 1000 + 1040 +  960 + 1000 + 1070},
            {GEOPM_REGION_HASH_UNMARKED, 0.0}
        },
        { // cpu3
            {regionA, 1111 + 1089 +    0 +    0 + 1155 + 1045 + 1177},
            {regionB,    0 +    0 + 1133 + 1067 +    0 +    0 +    0},
            {GEOPM_REGION_HASH_UNMARKED, 0.0}
        }
    };

    EXPECT_CALL(m_platio, push_signal("TIME", IPlatformTopo::M_DOMAIN_BOARD, 0));
    EXPECT_CALL(m_platio, push_signal("ENERGY", IPlatformTopo::M_DOMAIN_PACKAGE, 0));
    EXPECT_CALL(m_platio, push_signal("ENERGY", IPlatformTopo::M_DOMAIN_PACKAGE, 1));
    EXPECT_CALL(m_platio, push_signal("CYCLES", IPlatformTopo::M_DOMAIN_CPU, 0));
    EXPECT_CALL(m_platio, push_signal("CYCLES", IPlatformTopo::M_DOMAIN_CPU, 1));
    EXPECT_CALL(m_platio, push_signal("CYCLES", IPlatformTopo::M_DOMAIN_CPU, 2));
    EXPECT_CALL(m_platio, push_signal("CYCLES", IPlatformTopo::M_DOMAIN_CPU, 3));
    EXPECT_CALL(m_platio, push_signal("REGION_HASH", IPlatformTopo::M_DOMAIN_BOARD, 0));
    EXPECT_CALL(m_platio, push_signal("REGION_HASH", IPlatformTopo::M_DOMAIN_PACKAGE, 0));
    EXPECT_CALL(m_platio, push_signal("REGION_HASH", IPlatformTopo::M_DOMAIN_PACKAGE, 1));
    EXPECT_CALL(m_platio, push_signal("REGION_HASH", IPlatformTopo::M_DOMAIN_CPU, 0));
    EXPECT_CALL(m_platio, push_signal("REGION_HASH", IPlatformTopo::M_DOMAIN_CPU, 1));
    EXPECT_CALL(m_platio, push_signal("REGION_HASH", IPlatformTopo::M_DOMAIN_CPU, 2));
    EXPECT_CALL(m_platio, push_signal("REGION_HASH", IPlatformTopo::M_DOMAIN_CPU, 3));
    EXPECT_EQ(M_SIGNAL_TIME, m_agg->push_signal_total("TIME", IPlatformTopo::M_DOMAIN_BOARD, 0));
    EXPECT_EQ(M_SIGNAL_ENERGY_0, m_agg->push_signal_total("ENERGY", IPlatformTopo::M_DOMAIN_PACKAGE, 0));
    EXPECT_EQ(M_SIGNAL_ENERGY_1, m_agg->push_signal_total("ENERGY", IPlatformTopo::M_DOMAIN_PACKAGE, 1));
    EXPECT_EQ(M_SIGNAL_CYCLES_0, m_agg->push_signal_total("CYCLES", IPlatformTopo::M_DOMAIN_CPU, 0));
    EXPECT_EQ(M_SIGNAL_CYCLES_1, m_agg->push_signal_total("CYCLES", IPlatformTopo::M_DOMAIN_CPU, 1));
    EXPECT_EQ(M_SIGNAL_CYCLES_2, m_agg->push_signal_total("CYCLES", IPlatformTopo::M_DOMAIN_CPU, 2));
    EXPECT_EQ(M_SIGNAL_CYCLES_3, m_agg->push_signal_total("CYCLES", IPlatformTopo::M_DOMAIN_CPU, 3));

    for (int idx = 0; idx < num_sample; ++idx) {
        // expected sample values
        EXPECT_CALL(m_platio, sample(M_SIGNAL_TIME))
            .WillRepeatedly(Return(time[0][idx]));
        EXPECT_CALL(m_platio, sample(M_SIGNAL_ENERGY_0))
            .WillRepeatedly(Return(energy[0][idx]));
        EXPECT_CALL(m_platio, sample(M_SIGNAL_ENERGY_1))
            .WillRepeatedly(Return(energy[1][idx]));
        EXPECT_CALL(m_platio, sample(M_SIGNAL_CYCLES_0))
            .WillRepeatedly(Return(cycles[0][idx]));
        EXPECT_CALL(m_platio, sample(M_SIGNAL_CYCLES_1))
            .WillRepeatedly(Return(cycles[1][idx]));
        EXPECT_CALL(m_platio, sample(M_SIGNAL_CYCLES_2))
            .WillRepeatedly(Return(cycles[2][idx]));
        EXPECT_CALL(m_platio, sample(M_SIGNAL_CYCLES_3))
            .WillRepeatedly(Return(cycles[3][idx]));

        // expected region IDs
        EXPECT_CALL(m_platio, sample(M_SIGNAL_R_HASH_BOARD))
            .WillRepeatedly(Return(rid_board[idx]));
        EXPECT_CALL(m_platio, sample(M_SIGNAL_R_HASH_PKG_0))
            .WillRepeatedly(Return(rid_pkg_0[idx]));
        EXPECT_CALL(m_platio, sample(M_SIGNAL_R_HASH_PKG_1))
            .WillRepeatedly(Return(rid_pkg_1[idx]));
        EXPECT_CALL(m_platio, sample(M_SIGNAL_R_HASH_CPU_0))
            .WillRepeatedly(Return(rid_cpu_0[idx]));
        EXPECT_CALL(m_platio, sample(M_SIGNAL_R_HASH_CPU_1))
            .WillRepeatedly(Return(rid_cpu_1[idx]));
        EXPECT_CALL(m_platio, sample(M_SIGNAL_R_HASH_CPU_2))
            .WillRepeatedly(Return(rid_cpu_2[idx]));
        EXPECT_CALL(m_platio, sample(M_SIGNAL_R_HASH_CPU_3))
            .WillRepeatedly(Return(rid_cpu_3[idx]));

        // epoch count - no epoch
        EXPECT_CALL(m_platio, sample(M_SIGNAL_EPOCH_COUNT))
            .WillRepeatedly(Return(-1));
        m_agg->read_batch();
    }
    std::set<uint64_t> regions = {regionA, regionB, GEOPM_REGION_HASH_UNMARKED};

    for (auto region : regions) {
        EXPECT_EQ(exp_time[0][region], m_agg->sample_total(M_SIGNAL_TIME, region));
        EXPECT_EQ(exp_energy[0][region], m_agg->sample_total(M_SIGNAL_ENERGY_0, region));
        EXPECT_EQ(exp_energy[1][region], m_agg->sample_total(M_SIGNAL_ENERGY_1, region));
        EXPECT_EQ(exp_cycles[0][region], m_agg->sample_total(M_SIGNAL_CYCLES_0, region));
        EXPECT_EQ(exp_cycles[1][region], m_agg->sample_total(M_SIGNAL_CYCLES_1, region));
        EXPECT_EQ(exp_cycles[2][region], m_agg->sample_total(M_SIGNAL_CYCLES_2, region));
        EXPECT_EQ(exp_cycles[3][region], m_agg->sample_total(M_SIGNAL_CYCLES_3, region));
    }
    std::set<uint64_t> result_regions = m_agg->tracked_region_hash();
    EXPECT_EQ(regions, result_regions);

    // Invalid index
    GEOPM_EXPECT_THROW_MESSAGE(m_agg->sample_total(-1, regionA), GEOPM_ERROR_INVALID,
                               "Invalid signal index");
    // Unpushed signal index
    GEOPM_EXPECT_THROW_MESSAGE(m_agg->sample_total(9999, regionA), GEOPM_ERROR_INVALID,
                               "signal index not pushed with push_signal_total");
    // Unseen region
    EXPECT_DOUBLE_EQ(0.0, m_agg->sample_total(M_SIGNAL_TIME, 0x9999));
}

TEST_F(RegionAggregatorTest, epoch_total)
{
    uint64_t reg_normal = 0x3333;

    EXPECT_CALL(m_platio, push_signal("TIME", IPlatformTopo::M_DOMAIN_BOARD, 0));
    EXPECT_CALL(m_platio, push_signal("REGION_HASH", IPlatformTopo::M_DOMAIN_BOARD, 0));
    m_agg->push_signal_total("TIME", IPlatformTopo::M_DOMAIN_BOARD, 0);
    // regions before first epoch
    std::vector<uint64_t> pre_epoch_regions {reg_normal, GEOPM_REGION_HASH_UNMARKED};
    int step = 0;
    for (auto region : pre_epoch_regions) {
        EXPECT_CALL(m_platio, sample(M_SIGNAL_TIME))
            .WillRepeatedly(Return(step));
        EXPECT_CALL(m_platio, sample(M_SIGNAL_R_HASH_BOARD))
            .WillRepeatedly(Return(region));
        EXPECT_CALL(m_platio, sample(M_SIGNAL_EPOCH_COUNT))
            .WillRepeatedly(Return(-1));

        ++step;

        m_agg->read_batch();
    }

    EXPECT_DOUBLE_EQ(1.0, m_agg->sample_total(M_SIGNAL_TIME, reg_normal));
    EXPECT_DOUBLE_EQ(0.0, m_agg->sample_total(M_SIGNAL_TIME, GEOPM_REGION_HASH_UNMARKED));
    EXPECT_DOUBLE_EQ(0.0, m_agg->sample_total(M_SIGNAL_TIME, GEOPM_REGION_HASH_EPOCH));

    // only time from non-MPI, non-ignore regions will go in epoch
    // unmarked region is also included in epoch
    std::vector<uint64_t> epoch_regions {GEOPM_REGION_HASH_UNMARKED,
                                         reg_normal,
                                         GEOPM_REGION_HASH_UNMARKED};
    for (auto region : epoch_regions) {
        EXPECT_CALL(m_platio, sample(M_SIGNAL_TIME))
            .WillRepeatedly(Return(step));
        EXPECT_CALL(m_platio, sample(M_SIGNAL_R_HASH_BOARD))
            .WillRepeatedly(Return(region));
        // after first epoch()
        EXPECT_CALL(m_platio, sample(M_SIGNAL_EPOCH_COUNT))
            .WillRepeatedly(Return(0));

        ++step;

        m_agg->read_batch();
    }

    EXPECT_DOUBLE_EQ(2.0, m_agg->sample_total(M_SIGNAL_TIME, reg_normal));
    EXPECT_DOUBLE_EQ(2.0, m_agg->sample_total(M_SIGNAL_TIME, GEOPM_REGION_HASH_UNMARKED));
    // should have 1 from reg_normal, 2 from unmarked
    EXPECT_DOUBLE_EQ(3.0, m_agg->sample_total(M_SIGNAL_TIME, GEOPM_REGION_HASH_EPOCH));
}
