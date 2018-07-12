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

#include "geopm_test.hpp"
#include "PowerGovernor.hpp"
#include "MockPlatformIO.hpp"
#include "MockPlatformTopo.hpp"
#include "Helper.hpp"

using geopm::PowerGovernor;
using geopm::IPlatformTopo;
using ::testing::_;
using ::testing::Return;

class PowerGovernorTest : public ::testing::Test
{
    protected:
        enum {
            M_SIGNAL_POWER_DRAM,
        };
        void SetUp(void);

        MockPlatformIO m_platform_io;
        MockPlatformTopo m_platform_topo;
        const double M_PKG_POWER_MIN = 50;
        const double M_PKG_POWER_MAX = 300;
        int m_num_package = 2;
        const int M_SAMPLES_PER_CONTROL = 3;
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
        .WillOnce(Return(M_PKG_POWER_MIN));
    EXPECT_CALL(m_platform_io, read_signal("POWER_PACKAGE_MAX", IPlatformTopo::M_DOMAIN_PACKAGE, 0))
        .Times(1)
        .WillOnce(Return(M_PKG_POWER_MAX));

    m_governor = geopm::make_unique<PowerGovernor>(m_platform_io, m_platform_topo, M_SAMPLES_PER_CONTROL);
    m_governor->init_platform_io();
}

TEST_F(PowerGovernorTest, govern)
{
    std::vector<double> dram_energy;
    dram_energy = {10.0, 20.0, 30.0, 40.0, 50.0};

    for (size_t x = 0; x < dram_energy.size(); ++x) {
        EXPECT_CALL(m_platform_io, sample(M_SIGNAL_POWER_DRAM))
            .Times(1)
            .WillOnce(Return(dram_energy[x]));
        if (!(x % M_SAMPLES_PER_CONTROL)) {
            double cur_max = *std::max_element(dram_energy.begin(), dram_energy.begin() + x + 1);
            double expect = (M_PKG_POWER_MAX - cur_max) / m_num_package;
            EXPECT_CALL(m_platform_io, adjust(_, expect))
                .Times(m_num_package);
        }

        m_governor->sample_platform();
        double node_power_set = 0.0;
        bool result = m_governor->adjust_platform(M_PKG_POWER_MAX, node_power_set);
        EXPECT_EQ(!(x % M_SAMPLES_PER_CONTROL), result);
        if (result) {
            EXPECT_EQ(M_PKG_POWER_MAX, node_power_set);
        }
        else {
            EXPECT_EQ(0.0, node_power_set);
        }
    }
}

TEST_F(PowerGovernorTest, govern_min)
{
    /// min budget
    {
        EXPECT_CALL(m_platform_io, sample(M_SIGNAL_POWER_DRAM))
            .Times(1)
            .WillOnce(Return(NAN));
        EXPECT_CALL(m_platform_io, adjust(_, M_PKG_POWER_MIN))
            .Times(m_num_package);

        m_governor->sample_platform();
        double node_power_set = 0.0;
        double node_power_min = m_num_package * M_PKG_POWER_MIN;
        m_governor->adjust_platform(node_power_min - 2, node_power_set);
        EXPECT_EQ(node_power_min, node_power_set);
    }

    /// policy below min HW support
    {
        GEOPM_EXPECT_THROW_MESSAGE(m_governor->set_power_bounds(M_PKG_POWER_MIN - 1, M_PKG_POWER_MAX),
                                   GEOPM_ERROR_RUNTIME, "invalid min_pkg_power bound.");
    }

    /// target below min policy
    {
        double new_pkg_power_min = M_PKG_POWER_MIN + 1;
        double new_node_power_min = new_pkg_power_min * m_num_package;
        EXPECT_CALL(m_platform_io, adjust(_, new_pkg_power_min))
            .Times(m_num_package);

        m_governor->set_power_bounds(new_pkg_power_min, M_PKG_POWER_MAX);
        double node_power_set = 0.0;
        m_governor->adjust_platform(m_num_package * (M_PKG_POWER_MIN - 2), node_power_set);
        EXPECT_EQ(new_node_power_min, node_power_set);
    }
}

TEST_F(PowerGovernorTest, govern_max)
{
    /// max budget
    {
        EXPECT_CALL(m_platform_io, sample(M_SIGNAL_POWER_DRAM))
            .Times(1)
            .WillOnce(Return(NAN));
        EXPECT_CALL(m_platform_io, adjust(_, M_PKG_POWER_MAX))
            .Times(m_num_package);

        m_governor->sample_platform();
        double node_power_set = 0.0;
        double node_power_max = m_num_package * M_PKG_POWER_MAX;
        m_governor->adjust_platform(node_power_max + 2, node_power_set);
        EXPECT_EQ(node_power_max, node_power_set);
    }

    /// policy above max HW support
    {
        GEOPM_EXPECT_THROW_MESSAGE(m_governor->set_power_bounds(M_PKG_POWER_MIN, M_PKG_POWER_MAX + 1),
                                   GEOPM_ERROR_RUNTIME, "invalid max_pkg_power bound.");
    }

    /// target above max policy
    {
        double new_pkg_power_max = M_PKG_POWER_MAX - 1;
        double new_node_power_max = new_pkg_power_max * m_num_package;
        EXPECT_CALL(m_platform_io, adjust(_, new_pkg_power_max))
            .Times(m_num_package);

        m_governor->set_power_bounds(M_PKG_POWER_MIN, new_pkg_power_max);
        double node_power_set = 0.0;
        m_governor->adjust_platform(new_node_power_max + 2, node_power_set);
        EXPECT_EQ(new_node_power_max, node_power_set);
    }
}
