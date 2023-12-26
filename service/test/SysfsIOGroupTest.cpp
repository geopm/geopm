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
#include "geopm_topo.h"
#include <cstring>

#include "gmock/gmock.h"
#include "gtest/gtest.h"

#include <memory>
#include <algorithm>

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
        "TEST_SIGNAL_ALIAS"}
       },{"TESTIOGROUP::CONTROL1", SysfsDriver::properties_s{
        "TESTIOGROUP::CONTROL1", true, "control1", "Control1's description", 4.0,
        IOGroup::M_UNITS_NONE, [](const std::vector<double> &) -> double { return 1.0; },
        IOGroup::M_SIGNAL_BEHAVIOR_CONSTANT, [](double) { return std::string("99"); },
        "TEST_CONTROL_ALIAS"}
       }
    }));
    // Mock to make all attributes map to a readable and writable file so they
    // all appear accessible by default.
    ON_CALL(*m_driver, attribute_path(_, _)).WillByDefault(Return("/dev/null"));

    m_mock_save_ctl = std::make_shared<MockSaveControl>();

    m_batch_io = std::make_shared<MockIOUring>();
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

    // Signal maps to a file that cannot be accessed for reads
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

    // Control maps to a file that cannot be accessed for writes
    EXPECT_CALL(*m_driver, attribute_path(_, _)).Times(AtLeast(0)).WillRepeatedly(Return("./completely/made/up/path/that/i/hope/does/not/exist"));
    for (auto name : names) {
        EXPECT_FALSE(m_group->is_valid_control(name)) << "name = " << name;
    }

    EXPECT_FALSE(m_group->is_valid_control("CPUFREQ::TOTALLY_MADE_UP:CONTROL"));
}

TEST_F(SysfsIOGroupTest, signal_domain_type)
{
    EXPECT_EQ(GEOPM_DOMAIN_INVALID, m_group->signal_domain_type("MADE_UP_SIGNAL_NAME"));

    EXPECT_CALL(*m_driver, domain_type("TESTIOGROUP::SIGNAL1"))
        .Times(2) // Once for a raw signal name test and once for an alias test
        .WillRepeatedly(Return(GEOPM_DOMAIN_CORE));

    EXPECT_EQ(GEOPM_DOMAIN_CORE, m_group->signal_domain_type("TESTIOGROUP::SIGNAL1"));
    EXPECT_EQ(GEOPM_DOMAIN_CORE, m_group->signal_domain_type("TEST_SIGNAL_ALIAS"));
}

TEST_F(SysfsIOGroupTest, control_domain_type)
{
    EXPECT_EQ(GEOPM_DOMAIN_INVALID, m_group->control_domain_type("MADE_UP_CONTROL_NAME"));

    EXPECT_CALL(*m_driver, domain_type("TESTIOGROUP::CONTROL1"))
        .Times(2) // Once for a raw control name test and once for an alias test
        .WillRepeatedly(Return(GEOPM_DOMAIN_CORE));

    EXPECT_EQ(GEOPM_DOMAIN_CORE, m_group->control_domain_type("TESTIOGROUP::CONTROL1"));
    EXPECT_EQ(GEOPM_DOMAIN_CORE, m_group->control_domain_type("TEST_CONTROL_ALIAS"));
}

TEST_F(SysfsIOGroupTest, batch_reads)
{
    auto read_value = [](std::shared_ptr<int> ret, int, void *buf, unsigned nbytes, off_t) {
        std::strncpy(static_cast<char*>(buf), "1.25", nbytes);
        if (ret) {
            *ret = std::min(static_cast<size_t>(nbytes), sizeof "1.25");
        }
    };
    // Mock the file read
    EXPECT_CALL(*m_batch_io, prep_read(_, _, _, _, _)).WillRepeatedly(Invoke(read_value));
    // Mock the translation from file contents to a number
    EXPECT_CALL(*m_driver, signal_parse("TESTIOGROUP::SIGNAL1"))
        .WillRepeatedly(Return([](const std::string& value)->double {return std::stod(value);}));
    auto signal_idx = m_group->push_signal("TESTIOGROUP::SIGNAL1", GEOPM_DOMAIN_BOARD, 0);
    m_group->read_batch();
    EXPECT_EQ(1.25, m_group->sample(signal_idx));
}

TEST_F(SysfsIOGroupTest, batch_writes)
{
    std::string written = "";
    auto write_value = [&written](std::shared_ptr<int>, int, const void *buf, unsigned nbytes, off_t) {
        written = std::string(static_cast<const char*>(buf), nbytes-1);
    };
    auto double_to_3dec_string = [](const double value) -> std::string {
        std::ostringstream ostream;
        ostream.precision(3);
        ostream << value;
        return ostream.str();
    };
    // Mock the translation from a number to desired file contents
    EXPECT_CALL(*m_driver, control_gen("TESTIOGROUP::CONTROL1"))
        .WillRepeatedly(Return(double_to_3dec_string));
    // Mock the file write
    EXPECT_CALL(*m_batch_io, prep_write(_, _, _, _, _)).WillRepeatedly(Invoke(write_value));
    auto control_idx = m_group->push_control("TESTIOGROUP::CONTROL1", GEOPM_DOMAIN_BOARD, 0);
    m_group->adjust(control_idx, 1.25);
    m_group->write_batch();
    EXPECT_EQ("1.25", written);
}
