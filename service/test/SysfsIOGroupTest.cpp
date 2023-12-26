/*
 * Copyright (c) 2015 - 2023, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "SysfsIOGroup.hpp"

#include "MockIOUring.hpp"
#include "MockPlatformTopo.hpp"
#include "MockSaveControl.hpp"
#include "MockSysfsDriver.hpp"
#include "SysfsDriver.hpp"
#include "geopm/Helper.hpp"

#include "gmock/gmock.h"
#include "gtest/gtest.h"

#include <memory>

using geopm::IOGroup;
using geopm::SysfsIOGroup;
using geopm::SysfsDriver;
using testing::Return;
using testing::_;
using testing::AtLeast;
using testing::AnyOf;
using testing::Throw;
using testing::InSequence;
using testing::Invoke;
using testing::Gt;

class SysfsIOGroupTest : public :: testing :: Test
{
    protected:
        void SetUp();
        std::shared_ptr<MockSysfsDriver> m_driver;
        std::shared_ptr<MockIOUring> m_batch_io;
        std::unique_ptr<SysfsIOGroup> m_group;
        std::shared_ptr<MockPlatformTopo> m_topo;
        std::shared_ptr<MockSaveControl> m_mock_save_ctl;
        int m_num_package = 1;
        int m_num_core = 2;
        int m_num_cpu = 4;
};

void SysfsIOGroupTest::SetUp()
{
    m_topo = make_topo(m_num_package, m_num_core, m_num_cpu);

    m_driver = std::make_shared<MockSysfsDriver>();
    ON_CALL(*m_driver, driver()).WillByDefault(Return("TESTIOGROUP"));
    ON_CALL(*m_driver, properties()).WillByDefault(Return(std::map<std::string, SysfsDriver::properties_s>{
       {"TESTIOGROUP::SIGNAL1", SysfsDriver::properties_s{
        "TESTIOGROUP::SIGNAL1", false, "signal1", "Signal1's description", 2.0,
        IOGroup::M_UNITS_NONE, [](const std::vector<double> &) -> double { return 1.0; },
        IOGroup::M_SIGNAL_BEHAVIOR_CONSTANT, [](double) { return std::string("99"); },
        ""}
       },{"TESTIOGROUP::CONTROL1", SysfsDriver::properties_s{
        "TESTIOGROUP::CONTROL1", true, "control1", "Control1's description", 4.0,
        IOGroup::M_UNITS_NONE, [](const std::vector<double> &) -> double { return 1.0; },
        IOGroup::M_SIGNAL_BEHAVIOR_CONSTANT, [](double) { return std::string("99"); },
        ""}
       }
    }));

    m_mock_save_ctl = std::make_shared<MockSaveControl>();

    m_group = geopm::make_unique<SysfsIOGroup>(m_driver, *m_topo, m_mock_save_ctl, m_batch_io, m_batch_io);
}

TEST_F(SysfsIOGroupTest, valid_signal_names)
{
    EXPECT_CALL(*m_driver, attribute_path(_, _)).Times(AtLeast(0)).WillRepeatedly(Return("/dev/null"));
    auto names = m_group->signal_names();
    EXPECT_GT(names.size(), 0ull);
    for (auto name : names) {
        EXPECT_TRUE(m_group->is_valid_signal(name)) << "name = " << name;
    }

    EXPECT_CALL(*m_driver, attribute_path(_, _)).Times(AtLeast(0)).WillRepeatedly(Return("./completely/made/up/path/that/i/hope/does/not/exist"));
    for (auto name : names) {
        EXPECT_FALSE(m_group->is_valid_signal(name)) << "name = " << name;
    }

    EXPECT_FALSE(m_group->is_valid_signal("CPUFREQ::TOTALLY_MADE_UP:SIGNAL"));
}

TEST_F(SysfsIOGroupTest, valid_control_names)
{
    EXPECT_CALL(*m_driver, attribute_path(_, _)).Times(AtLeast(0)).WillRepeatedly(Return("/dev/null"));
    auto names = m_group->control_names();
    EXPECT_GT(names.size(), 0ull);
    for (auto name : names) {
        EXPECT_TRUE(m_group->is_valid_control(name)) << "name = " << name;
    }

    EXPECT_CALL(*m_driver, attribute_path(_, _)).Times(AtLeast(0)).WillRepeatedly(Return("./completely/made/up/path/that/i/hope/does/not/exist"));
    for (auto name : names) {
        EXPECT_FALSE(m_group->is_valid_control(name)) << "name = " << name;
    }

    EXPECT_FALSE(m_group->is_valid_control("CPUFREQ::TOTALLY_MADE_UP:CONTROL"));
}
