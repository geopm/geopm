/*
 * Copyright (c) 2015 - 2023, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <memory>

#include "gtest/gtest.h"
#include "gmock/gmock.h"

#include "geopm_test.hpp"
#include "PowerGovernorImp.hpp"
#include "MockPlatformIO.hpp"
#include "MockPlatformTopo.hpp"
#include "geopm/Helper.hpp"

using geopm::PowerGovernor;
using geopm::PowerGovernorImp;
using ::testing::_;
using ::testing::Return;

class PowerGovernorTest : public ::testing::Test
{
    protected:
        void SetUp(void);

        MockPlatformIO m_platform_io;
        MockPlatformTopo m_platform_topo;
        const double M_PKG_POWER_MIN = 50;
        const double M_PKG_POWER_MAX = 300;
        const double M_PKG_POWER_WIN = 0.015;
        int m_num_package = 2;
        std::vector<double> m_expected_power;
        std::unique_ptr<PowerGovernor> m_governor;
};

void PowerGovernorTest::SetUp(void)
{
    EXPECT_CALL(m_platform_io, control_domain_type("CPU_POWER_LIMIT_CONTROL"))
        .Times(1)
        .WillOnce(Return(GEOPM_DOMAIN_PACKAGE));
    EXPECT_CALL(m_platform_topo, num_domain(GEOPM_DOMAIN_PACKAGE))
        .Times(1)
        .WillOnce(Return(m_num_package));
    EXPECT_CALL(m_platform_io, push_control("CPU_POWER_LIMIT_CONTROL", GEOPM_DOMAIN_PACKAGE, _))
        .Times(m_num_package);

    EXPECT_CALL(m_platform_io, read_signal("CPU_POWER_MIN_AVAIL", GEOPM_DOMAIN_PACKAGE, 0))
        .Times(1)
        .WillOnce(Return(M_PKG_POWER_MIN));
    EXPECT_CALL(m_platform_io, read_signal("CPU_POWER_MAX_AVAIL", GEOPM_DOMAIN_PACKAGE, 0))
        .Times(1)
        .WillOnce(Return(M_PKG_POWER_MAX));
    EXPECT_CALL(m_platform_io, write_control("CPU_POWER_TIME_WINDOW_CONTROL", GEOPM_DOMAIN_PACKAGE, 0, M_PKG_POWER_WIN))
        .Times(1);
    EXPECT_CALL(m_platform_io, write_control("CPU_POWER_TIME_WINDOW_CONTROL", GEOPM_DOMAIN_PACKAGE, 1, M_PKG_POWER_WIN))
        .Times(1);

    m_governor = geopm::make_unique<PowerGovernorImp>(m_platform_io, m_platform_topo);
    m_governor->init_platform_io();
}

TEST_F(PowerGovernorTest, govern)
{
    m_governor->sample_platform();
    double node_power_set = 0.0;
    double node_power_request = m_num_package * (M_PKG_POWER_MAX - 1);
    EXPECT_CALL(m_platform_io, adjust(_, M_PKG_POWER_MAX - 1)).Times(m_num_package);
    m_governor->adjust_platform(node_power_request, node_power_set);
    EXPECT_TRUE(m_governor->do_write_batch());
    EXPECT_DOUBLE_EQ(node_power_request, node_power_set);

    node_power_request = m_num_package * M_PKG_POWER_MAX;
    node_power_set = 0.0;
    EXPECT_CALL(m_platform_io, adjust(_, M_PKG_POWER_MAX)).Times(m_num_package);
    m_governor->adjust_platform(node_power_request, node_power_set);
    EXPECT_TRUE(m_governor->do_write_batch());
    EXPECT_DOUBLE_EQ(node_power_request, node_power_set);

    node_power_set = 0.0;
    m_governor->adjust_platform(node_power_request, node_power_set);
    EXPECT_FALSE(m_governor->do_write_batch());
    EXPECT_EQ(0.0, node_power_set);
}

TEST_F(PowerGovernorTest, govern_min)
{
    /// min budget
    {
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
