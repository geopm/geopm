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

#include "PowerGovernor.hpp"
#include "MockPlatformIO.hpp"
#include "MockPlatformTopo.hpp"
#include "Helper.hpp"

using geopm::PowerGovernor;
using geopm::IPlatformTopo;
using ::testing::_;
using ::testing::Return;
using ::testing::Sequence;

class PowerGovernorTest : public ::testing::Test
{
    protected:
        enum {
            M_SIGNAL_POWER_DRAM,
        };
        void SetUp(void);

        MockPlatformIO m_platform_io;
        MockPlatformTopo m_platform_topo;
        double m_pkg_power_min = 50;
        double m_pkg_power_max = 300;
        int m_num_package = 2;
        double m_target_node_power = m_pkg_power_max * m_num_package;
        std::vector<double> m_dram_energy;
        std::vector<double> m_expected_power;
        std::unique_ptr<PowerGovernor> m_governor;
};

void PowerGovernorTest::SetUp(void)
{
    EXPECT_CALL(m_platform_io, push_signal("POWER_DRAM", IPlatformTopo::M_DOMAIN_BOARD, 0))
        .Times(1)
        .WillOnce(Return(M_SIGNAL_POWER_DRAM));
    EXPECT_CALL(m_platform_io, control_domain_type("POWER_PACKAGE"))
        .Times(1)
        .WillOnce(Return(IPlatformTopo::M_DOMAIN_PACKAGE));
    EXPECT_CALL(m_platform_topo, num_domain(IPlatformTopo::M_DOMAIN_PACKAGE))
        .Times(1)
        .WillOnce(Return(m_num_package));
    EXPECT_CALL(m_platform_io, push_control("POWER_PACKAGE", IPlatformTopo::M_DOMAIN_PACKAGE, _))
        .Times(m_num_package);

    EXPECT_CALL(m_platform_io, read_signal("POWER_PACKAGE_MIN", IPlatformTopo::M_DOMAIN_PACKAGE, 0))
        .Times(1)
        .WillOnce(Return(m_pkg_power_min));
    EXPECT_CALL(m_platform_io, read_signal("POWER_PACKAGE_MAX", IPlatformTopo::M_DOMAIN_PACKAGE, 0))
        .Times(1)
        .WillOnce(Return(m_pkg_power_max));

    m_governor = geopm::make_unique<PowerGovernor>(m_platform_io, m_platform_topo);
    m_governor->init_platform_io();
}

TEST_F(PowerGovernorTest, govern)
{
    Sequence seq;
    m_dram_energy = {10.0, 20.0, 30.0, 40.0, 50.0};
    for (size_t x = 0; x < m_dram_energy.size(); ++x) {
        EXPECT_CALL(m_platform_io, sample(M_SIGNAL_POWER_DRAM))
            .Times(1)
            .InSequence(seq)
            .WillOnce(Return(m_dram_energy[x]));
        EXPECT_CALL(m_platform_io, adjust(_, (m_target_node_power - m_dram_energy[x]) / m_num_package))
            .Times(m_num_package)
            .InSequence(seq);
    }
    for (size_t x = 0; x < m_dram_energy.size(); ++x) {
        m_governor->sample_platform();
        m_governor->adjust_platform(m_target_node_power);
    }
}

TEST_F(PowerGovernorTest, govern_min)
{
    Sequence seq;
    EXPECT_CALL(m_platform_io, sample(M_SIGNAL_POWER_DRAM))
        .Times(1)
        .InSequence(seq)
        .WillOnce(Return(NAN));
    EXPECT_CALL(m_platform_io, adjust(_, m_pkg_power_min))
        .Times(m_num_package)
        .InSequence(seq);

    m_governor->sample_platform();
    m_governor->adjust_platform((m_num_package * m_pkg_power_min) - 2);
}

TEST_F(PowerGovernorTest, govern_max)
{
    Sequence seq;
    EXPECT_CALL(m_platform_io, sample(M_SIGNAL_POWER_DRAM))
        .Times(1)
        .InSequence(seq)
        .WillOnce(Return(NAN));
    EXPECT_CALL(m_platform_io, adjust(_, m_pkg_power_max))
        .Times(m_num_package)
        .InSequence(seq);

    m_governor->sample_platform();
    m_governor->adjust_platform((m_num_package * m_pkg_power_max) + 2);
}
