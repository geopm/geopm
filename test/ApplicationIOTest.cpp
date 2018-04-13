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

#include <memory>

#include "gtest/gtest.h"
#include "gmock/gmock.h"

#include "ApplicationIO.hpp"
#include "Helper.hpp"
#include "MockPlatformIO.hpp"
#include "MockPlatformTopo.hpp"
#include "MockProfileSampler.hpp"
#include "MockKprofileIOSample.hpp"

using geopm::ApplicationIO;
using testing::Return;
using testing::_;

class ApplicationIOTest : public ::testing::Test
{
    protected:
        void SetUp();
        std::string m_shm_key = "test_shm";
        MockPlatformIO m_platform_io;
        MockPlatformTopo m_platform_topo;
        MockProfileSampler *m_sampler;
        MockKprofileIOSample *m_pio_sample;
        std::unique_ptr<ApplicationIO> m_app_io;
};

void ApplicationIOTest::SetUp()
{
    m_sampler = new MockProfileSampler;
    auto tmp_s = std::unique_ptr<MockProfileSampler>(m_sampler);
    m_pio_sample = new MockKprofileIOSample;
    auto tmp_pio = std::shared_ptr<MockKprofileIOSample>(m_pio_sample);

    EXPECT_CALL(*m_sampler, initialize());
    EXPECT_CALL(*m_sampler, rank_per_node());
    EXPECT_CALL(*m_sampler, capacity());
    std::vector<int> ranks {1, 2, 3, 4};
    EXPECT_CALL(*m_sampler, cpu_rank()).WillOnce(Return(ranks));

    EXPECT_CALL(m_platform_topo, num_domain(geopm::IPlatformTopo::M_DOMAIN_PACKAGE))
        .WillRepeatedly(Return(2));
    EXPECT_CALL(m_platform_topo, num_domain(geopm::IPlatformTopo::M_DOMAIN_BOARD_MEMORY))
        .WillRepeatedly(Return(2));

    EXPECT_CALL(m_platform_io, read_signal("ENERGY_PACKAGE", _, _)).Times(2);
    EXPECT_CALL(m_platform_io, read_signal("ENERGY_DRAM", _, _)).Times(2);
    m_app_io = geopm::make_unique<ApplicationIO>(m_shm_key, m_platform_io,
                                                 m_platform_topo,
                                                 std::move(tmp_s), tmp_pio);
}

TEST_F(ApplicationIOTest, passthrough)
{
    EXPECT_CALL(*m_sampler, do_shutdown()).WillOnce(Return(false));
    EXPECT_FALSE(m_app_io->do_shutdown());

    EXPECT_CALL(*m_sampler, report_name()).WillOnce(Return("my_report"));
    EXPECT_EQ("my_report", m_app_io->report_name());

    EXPECT_CALL(*m_sampler, profile_name()).WillOnce(Return("my_profile"));
    EXPECT_EQ("my_profile", m_app_io->profile_name());

    std::set<std::string> regions = {"region A", "region B"};
    EXPECT_CALL(*m_sampler, name_set()).WillOnce(Return(regions));
    EXPECT_EQ(regions, m_app_io->region_name_set());

    uint64_t rid = 0x8888;
    EXPECT_CALL(*m_pio_sample, total_region_runtime(rid))
        .WillOnce(Return(8080));
    EXPECT_EQ(8080, m_app_io->total_region_runtime(rid));

    EXPECT_CALL(*m_pio_sample, total_region_mpi_time(rid))
        .WillOnce(Return(909));
    EXPECT_EQ(909, m_app_io->total_region_mpi_runtime(rid));

    EXPECT_CALL(*m_pio_sample, total_epoch_runtime())
        .WillOnce(Return(123));
    EXPECT_EQ(123, m_app_io->total_epoch_runtime());

    EXPECT_CALL(*m_pio_sample, total_app_runtime())
        .WillOnce(Return(345));
    EXPECT_EQ(345, m_app_io->total_app_runtime());

    EXPECT_CALL(*m_pio_sample, total_app_mpi_time())
        .WillOnce(Return(456));
    EXPECT_EQ(456, m_app_io->total_app_mpi_runtime());

    EXPECT_CALL(*m_pio_sample, total_count(rid))
        .WillOnce(Return(77));
    EXPECT_EQ(77, m_app_io->total_count(rid));
}
