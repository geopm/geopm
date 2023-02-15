/*
 * Copyright (c) 2015 - 2023, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include "SSTClosGovernor.hpp"

#include <memory>
#include <stdexcept>

#include "gmock/gmock-matchers.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"

#include "MockPlatformIO.hpp"
#include "MockPlatformTopo.hpp"
#include "SSTClosGovernorImp.hpp"
#include "geopm/Helper.hpp"
#include "geopm_test.hpp"
#include "geopm_topo.h"

using geopm::SSTClosGovernor;
using geopm::SSTClosGovernorImp;
using ::testing::_;
using ::testing::Expectation;
using ::testing::Invoke;
using ::testing::Return;
using ::testing::Throw;

const int CLOS_CONTROL_IDX = 100;

const int CORE_COUNT = 4;
const int PACKAGE_COUNT = 1;
const double MIN_FREQ = 1e9;
const double STICKER_FREQ = 2e9;
const double MAX_FREQ = 3e9;

class SSTClosGovernorTest : public ::testing::Test
{
    protected:
        void SetUp(void);

        MockPlatformIO m_platform_io;
        MockPlatformTopo m_platform_topo;
        const double M_PKG_POWER_MIN = 50;
        const double M_PKG_POWER_MAX = 300;
        const double M_PKG_POWER_WIN = 0.015;
        std::vector<double> m_expected_power;
        std::unique_ptr<SSTClosGovernor> m_governor;
};

void SSTClosGovernorTest::SetUp(void)
{
    ON_CALL(m_platform_io, control_domain_type("SST::COREPRIORITY:ASSOCIATION"))
        .WillByDefault(Return(GEOPM_DOMAIN_CORE));
    ON_CALL(m_platform_topo, num_domain(GEOPM_DOMAIN_CORE))
        .WillByDefault(Return(CORE_COUNT));
    ON_CALL(m_platform_io, control_domain_type("SST::COREPRIORITY:0:FREQUENCY_MIN"))
        .WillByDefault(Return(GEOPM_DOMAIN_PACKAGE));
    ON_CALL(m_platform_topo, num_domain(GEOPM_DOMAIN_PACKAGE))
        .WillByDefault(Return(PACKAGE_COUNT));

    ON_CALL(m_platform_io, read_signal("CPU_FREQUENCY_MIN_AVAIL", _, _))
        .WillByDefault(Return(MIN_FREQ));
    ON_CALL(m_platform_io, read_signal("CPU_FREQUENCY_STICKER", _, _))
        .WillByDefault(Return(STICKER_FREQ));
    ON_CALL(m_platform_io, read_signal("CPU_FREQUENCY_MAX_AVAIL", _, _))
        .WillByDefault(Return(MAX_FREQ));

    ON_CALL(m_platform_io, push_control("SST::COREPRIORITY:ASSOCIATION", GEOPM_DOMAIN_CORE, _))
        .WillByDefault(Invoke([](const std::string &, int, int core) { return CLOS_CONTROL_IDX + core; }));

    m_governor = geopm::make_unique<SSTClosGovernorImp>(m_platform_io, m_platform_topo);
    m_governor->init_platform_io();
}

TEST_F(SSTClosGovernorTest, is_supported)
{
    EXPECT_CALL(m_platform_io, read_signal("SST::COREPRIORITY_SUPPORT:CAPABILITIES", _, _))
        .WillOnce(Return(1))
        .WillOnce(Return(1))
        .WillOnce(Throw(std::runtime_error("injected error")));
    EXPECT_CALL(m_platform_io, read_signal("SST::TURBOFREQ_SUPPORT:SUPPORTED", _, _))
        .WillOnce(Return(1))
        .WillOnce(Return(0));

    EXPECT_TRUE(SSTClosGovernor::is_supported(m_platform_io));
    EXPECT_FALSE(SSTClosGovernor::is_supported(m_platform_io));
    EXPECT_FALSE(SSTClosGovernor::is_supported(m_platform_io));
}

TEST_F(SSTClosGovernorTest, govern)
{
    EXPECT_FALSE(m_governor->do_write_batch());
    EXPECT_CALL(m_platform_io, adjust(CLOS_CONTROL_IDX + 0, 3));
    EXPECT_CALL(m_platform_io, adjust(CLOS_CONTROL_IDX + 1, 2));
    EXPECT_CALL(m_platform_io, adjust(CLOS_CONTROL_IDX + 2, 1));
    EXPECT_CALL(m_platform_io, adjust(CLOS_CONTROL_IDX + 3, 0));
    m_governor->adjust_platform({3, 2, 1, 0});
    EXPECT_TRUE(m_governor->do_write_batch());

    // Wrong-sized inputs
    EXPECT_THROW(m_governor->adjust_platform({1, 2, 3}), geopm::Exception);
    EXPECT_THROW(m_governor->adjust_platform({1, 2, 3, 2, 1}), geopm::Exception);
}

TEST_F(SSTClosGovernorTest, enable)
{

    Expectation sst_cp_enable = EXPECT_CALL(m_platform_io, write_control("SST::COREPRIORITY_ENABLE:ENABLE", _, _, 1));
    EXPECT_CALL(m_platform_io, write_control("SST::TURBO_ENABLE:ENABLE", _, _, 1))
        .After(sst_cp_enable);
    m_governor->enable_sst_turbo_prioritization();
}

TEST_F(SSTClosGovernorTest, disable)
{
    Expectation sst_tf_disable = EXPECT_CALL(m_platform_io, write_control("SST::TURBO_ENABLE:ENABLE", _, _, 0));
    EXPECT_CALL(m_platform_io, write_control("SST::COREPRIORITY_ENABLE:ENABLE", _, _, 0))
        .After(sst_tf_disable);
    m_governor->disable_sst_turbo_prioritization();
}
