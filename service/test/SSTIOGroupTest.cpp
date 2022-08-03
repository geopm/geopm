/*
 * Copyright (c) 2015 - 2022, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <memory>
#include <set>
#include <algorithm>

#include "gtest/gtest.h"
#include "gmock/gmock.h"

#include "geopm/Helper.hpp"
#include "SSTIOGroup.hpp"
#include "MockPlatformTopo.hpp"
#include "MockSSTIO.hpp"
#include "MockSaveControl.hpp"
#include "geopm_topo.h"

using geopm::SSTIOGroup;
using testing::Return;
using testing::_;
using testing::AtLeast;
using testing::AnyOf;
using testing::Throw;
using testing::InSequence;
using testing::Invoke;
using testing::Gt;

class SSTIOGroupTest : public :: testing :: Test
{
    protected:
        void SetUp();
        std::shared_ptr<MockSSTIO> m_sstio;
        std::unique_ptr<SSTIOGroup> m_group;
        std::shared_ptr<MockPlatformTopo> m_topo;
        std::shared_ptr<MockSaveControl> m_mock_save_ctl;
        int m_num_package = 2;
        int m_num_core = 4;
        int m_num_cpu = 16;
};

void SSTIOGroupTest::SetUp()
{
    m_topo = make_topo(m_num_package, m_num_core, m_num_cpu);
    EXPECT_CALL(*m_topo, domain_nested(_, _, _)).Times(AtLeast(0));
    EXPECT_CALL(*m_topo, num_domain(_)).Times(AtLeast(0));

    m_sstio = std::make_shared<MockSSTIO>();
    m_mock_save_ctl = std::make_shared<MockSaveControl>();

    for (int i = 0; i < m_num_cpu; ++i) {
        /* Punit index doesn't necessarily equal CPU index. Make them different
         * to make sure we calculate offsets based on punit instead of CPU.
         */
        ON_CALL(*m_sstio, get_punit_from_cpu(i)).WillByDefault(Return(i*2));
    }
    EXPECT_CALL(*m_sstio, get_punit_from_cpu(_)).Times(m_num_package * m_num_core);

    m_group = geopm::make_unique<SSTIOGroup>(*m_topo, m_sstio, m_mock_save_ctl);
}

TEST_F(SSTIOGroupTest, valid_signal_names)
{
    auto names = m_group->signal_names();
    for (auto nn : names) {
        EXPECT_TRUE(m_group->is_valid_signal(nn)) << "nn = " << nn;
    }
    EXPECT_FALSE(m_group->is_valid_signal("SST::TOTALLY_MADE_UP:SIGNAL"));
}

TEST_F(SSTIOGroupTest, valid_control_names)
{
    auto names = m_group->control_names();
    for (auto name : names) {
        EXPECT_TRUE(m_group->is_valid_control(name)) << "name = " << name;
    }
    EXPECT_FALSE(m_group->is_valid_control("SST::TOTALLY_MADE_UP:CONTROL"));
}

TEST_F(SSTIOGroupTest, valid_signal_domains)
{
    auto names = m_group->signal_names();
    for (auto name : names) {
        if (name == "SST::COREPRIORITY:ASSOCIATION" || name == "SST::COREPRIORITY_0x00020#") {
            // These are the only signals that have per-core handling. If this
            // test fails, then a new per-core signal was added. Make sure you
            // handle any new special cases that appear.
            EXPECT_EQ(GEOPM_DOMAIN_CORE, m_group->signal_domain_type(name))
                    << "name = " << name;
        }
        else {
            EXPECT_EQ(GEOPM_DOMAIN_PACKAGE, m_group->signal_domain_type(name))
                    << "name = " << name;
        }
    }
}

TEST_F(SSTIOGroupTest, valid_control_domains)
{
    auto names = m_group->control_names();
    for (auto name : names) {
        if (name == "SST::COREPRIORITY:ASSOCIATION" || name == "SST::COREPRIORITY_0x00020#") {
            // These are the only controls that have per-core handling. If this
            // test fails, then a new per-core control was added. Make sure you
            // handle any new special cases that appear.
            EXPECT_EQ(GEOPM_DOMAIN_CORE, m_group->control_domain_type(name))
                    << "name = " << name;
        }
        else {
            EXPECT_EQ(GEOPM_DOMAIN_PACKAGE, m_group->control_domain_type(name))
                    << "name = " << name;
        }
    }
}

TEST_F(SSTIOGroupTest, sample_mbox_signal)
{
    enum sst_idx_e {
        CONFIG_LEVEL_0,
        CONFIG_LEVEL_1
    };

    int pkg_0_cpu = 0;
    int pkg_1_cpu = 2;

    EXPECT_CALL(*m_sstio, add_mbox_read(pkg_0_cpu, 0x7F, 0x00, 0x00))
    .WillOnce(Return(CONFIG_LEVEL_0));
    EXPECT_CALL(*m_sstio, add_mbox_read(pkg_1_cpu, 0x7F, 0x00, 0x00))
    .WillOnce(Return(CONFIG_LEVEL_1));

    int idx0 = m_group->push_signal("SST::CONFIG_LEVEL:LEVEL", GEOPM_DOMAIN_PACKAGE, 0);
    int idx1 = m_group->push_signal("SST::CONFIG_LEVEL:LEVEL", GEOPM_DOMAIN_PACKAGE, 1);
    EXPECT_NE(idx0, idx1);

    EXPECT_CALL(*m_sstio, read_batch());
    m_group->read_batch();
    uint32_t raw0 = 0x1428000;
    uint32_t raw1 = 0x1678000;
    uint32_t expected0 = 0x42;
    uint32_t expected1 = 0x67;
    EXPECT_CALL(*m_sstio, sample(CONFIG_LEVEL_0)).WillOnce(Return(raw0));
    EXPECT_CALL(*m_sstio, sample(CONFIG_LEVEL_1)).WillOnce(Return(raw1));
    uint32_t result = m_group->sample(idx0);
    EXPECT_EQ(expected0, result);
    result = m_group->sample(idx1);
    EXPECT_EQ(expected1, result);
}

// This tests a different path from sample_mbox_signal. While both cover signals
// that go through the mailbox interface, this test covers signals that are
// generated from a definition for a mailbox control.
TEST_F(SSTIOGroupTest, sample_mbox_control)
{
    enum sst_idx_e {
        CONFIG_LEVEL_0,
        CONFIG_LEVEL_1
    };

    int pkg_0_cpu = 0;
    int pkg_1_cpu = 2;

    EXPECT_CALL(*m_sstio, add_mbox_read(pkg_0_cpu, 0x7f, 0x01, 0x00))
    .WillOnce(Return(CONFIG_LEVEL_0));
    EXPECT_CALL(*m_sstio, add_mbox_read(pkg_1_cpu, 0x7f, 0x01, 0x00))
    .WillOnce(Return(CONFIG_LEVEL_1));

    int idx0 = m_group->push_signal("SST::TURBO_ENABLE:ENABLE", GEOPM_DOMAIN_PACKAGE, 0);
    int idx1 = m_group->push_signal("SST::TURBO_ENABLE:ENABLE", GEOPM_DOMAIN_PACKAGE, 1);
    EXPECT_NE(idx0, idx1);

    EXPECT_CALL(*m_sstio, read_batch());
    m_group->read_batch();
    // Should only read bit 16
    uint32_t raw0 = 0xffffff;
    uint32_t raw1 = 0xfeffff;
    uint32_t expected0 = 0x1;
    uint32_t expected1 = 0x0;
    EXPECT_CALL(*m_sstio, sample(CONFIG_LEVEL_0)).WillOnce(Return(raw0));
    EXPECT_CALL(*m_sstio, sample(CONFIG_LEVEL_1)).WillOnce(Return(raw1));
    uint32_t result = m_group->sample(idx0);
    EXPECT_EQ(expected0, result);
    result = m_group->sample(idx1);
    EXPECT_EQ(expected1, result);
}

// There aren't currently any MMIO signals, except those that are generated
// from MMIO controls. This tests an MMIO signal generated from a control.
// Specifically, this tests one that operates in the core domain.
TEST_F(SSTIOGroupTest, sample_mmio_percore_control)
{
    enum sst_idx_e {
        COREPRIORITY_0 = 10,
        COREPRIORITY_1 = 20
    };

    int core_0_cpu = 0;
    int core_1_cpu = 1;

    EXPECT_CALL(*m_sstio, add_mmio_read(core_0_cpu, 0x20))
    .WillOnce(Return(COREPRIORITY_0));
    EXPECT_CALL(*m_sstio, add_mmio_read(core_1_cpu, 0x28 /* punit 2 */))
    .WillOnce(Return(COREPRIORITY_1));

    int idx0 = m_group->push_signal("SST::COREPRIORITY:ASSOCIATION", GEOPM_DOMAIN_CORE, 0);
    int idx1 = m_group->push_signal("SST::COREPRIORITY:ASSOCIATION", GEOPM_DOMAIN_CORE, 1);
    EXPECT_NE(idx0, idx1);

    EXPECT_CALL(*m_sstio, read_batch());
    m_group->read_batch();

    // It should read bits 16..17 (lower 2 bits of e and 1)
    uint32_t raw0 = 0xfeffff;
    uint32_t raw1 = 0xf1ffff;
    uint32_t expected0 = 0x2;
    uint32_t expected1 = 0x1;

    EXPECT_CALL(*m_sstio, sample(COREPRIORITY_0)).WillOnce(Return(raw0));
    EXPECT_CALL(*m_sstio, sample(COREPRIORITY_1)).WillOnce(Return(raw1));
    uint32_t result = m_group->sample(idx0);
    EXPECT_EQ(expected0, result);
    result = m_group->sample(idx1);
    EXPECT_EQ(expected1, result);
}

TEST_F(SSTIOGroupTest, adjust_mbox_control)
{
    // The only mailbox-based controls are the ENABLE ones. These two controls
    // are co-dependent on each other, which complicates batch operations. For
    // now, let's simplify things by disallowing these controls in batch operations.
    EXPECT_THROW(m_group->push_control("SST::TURBO_ENABLE:ENABLE", GEOPM_DOMAIN_PACKAGE, 0),
                 geopm::Exception);
    EXPECT_THROW(m_group->push_control("SST::COREPRIORITY_ENABLE:ENABLE", GEOPM_DOMAIN_PACKAGE, 0),
                 geopm::Exception);
}


TEST_F(SSTIOGroupTest, adjust_mmio_control)
{
    enum sst_idx_e {
        /* Arbitrary values. Just make them different from other offsets in this
         * test to reduce chances of false passes.
         */
        FREQ_0 = 10,
        FREQ_1 = 20
    };

    int pkg_0_cpu = 0;
    int pkg_1_cpu = 2;

    // Expectations for SST::COREPRIORITY:1:FREQUENCY_MIN
    EXPECT_CALL(*m_sstio, add_mmio_write(pkg_0_cpu, 0x0c, 0, 0x00fffff0 /* bits 4..23. All known fields */))
    .WillOnce(Return(FREQ_0));
    EXPECT_CALL(*m_sstio, add_mmio_write(pkg_1_cpu, 0x0c, 0, 0x00fffff0 /* bits 4..23. All known fields */))
    .WillOnce(Return(FREQ_1));

    int idx0 = m_group->push_control("SST::COREPRIORITY:1:FREQUENCY_MIN", GEOPM_DOMAIN_PACKAGE, 0);
    int idx1 = m_group->push_control("SST::COREPRIORITY:1:FREQUENCY_MIN", GEOPM_DOMAIN_PACKAGE, 1);
    EXPECT_NE(idx0, idx1);

    int shift = 8;  // bits 8-15
    EXPECT_CALL(*m_sstio, adjust(FREQ_0, 10 /* 100s of MHz */ << shift, 0xff00 /* just this field */));
    m_group->adjust(idx0, 1e9);
    EXPECT_CALL(*m_sstio, adjust(FREQ_1, 21 /* 100s of MHz */ << shift, 0xff00 /* just this field */));
    m_group->adjust(idx1, 2.1e9);
}


TEST_F(SSTIOGroupTest, error_in_save_removes_control)
{
    auto names = m_group->control_names();
    int pkg_0_cpu = 0;
    const std::array<std::string, 3> broken_controls{ {
            "SST::COREPRIORITY:1:PRIORITY",
            "SST::COREPRIORITY:1:FREQUENCY_MIN",
            "SST::COREPRIORITY:1:FREQUENCY_MAX",
        } };
    const std::string unimpacted_control{"SST::COREPRIORITY:2:FREQUENCY_MIN"};

    for (const auto &control_name : broken_controls) {
        EXPECT_TRUE(m_group->is_valid_control(control_name))
                << control_name << " before failed save";
    }
    EXPECT_TRUE(m_group->is_valid_control(unimpacted_control))
            << unimpacted_control << " before failed save";

    // save_control will hit a lot of other controls. Let them all succeed
    // except for the ones we are testing. Google Test docs recommend using
    // ON_CALL for don't-care cases like this, but the EXPECT_CALL we do later
    // on a subest of these calls will not work with that pattern.
    EXPECT_CALL(*m_sstio, write_mmio_once(_, _, _, _, _, _)).WillRepeatedly(Return());
    EXPECT_CALL(*m_sstio, write_mbox_once(_, _, _, _, _, _, _, _, _)).WillRepeatedly(Return());

    // Fail writes in the SST::COREPRIORITY:1:* fields
    EXPECT_CALL(*m_sstio, write_mmio_once(pkg_0_cpu, 0x0c, 0, 0x00fffff0, _, _))
    .Times(3)
    .WillRepeatedly(Throw(std::runtime_error("Test-injected failure")));

    EXPECT_CALL(*m_sstio, read_mmio_once(_, _)).Times(AtLeast(0));
    EXPECT_CALL(*m_sstio, read_mbox_once(_, _, _, _)).Times(AtLeast(0));

    m_group->save_control();

    for (const auto &control_name : broken_controls) {
        EXPECT_FALSE(m_group->is_valid_control(control_name))
                << control_name << " after failed save";
    }
    EXPECT_TRUE(m_group->is_valid_control(unimpacted_control))
            << unimpacted_control << " after failed save";
}

TEST_F(SSTIOGroupTest, save_restore_control)
{
    // Verify that all controls can be read as signals
    auto control_set = m_group->control_names();
    auto signal_set = m_group->signal_names();
    std::vector<std::string> difference(control_set.size());

    auto it = std::set_difference(control_set.cbegin(), control_set.cend(),
                                  signal_set.cbegin(), signal_set.cend(),
                                  difference.begin());
    difference.resize(it - difference.begin());

    std::string err_msg = "The following controls are not readable as signals: \n";
    for (auto &sig : difference) {
        err_msg += "    " + sig + '\n';
    }
    EXPECT_EQ((unsigned int) 0, difference.size()) << err_msg;

    std::string file_name = "tmp_file";
    EXPECT_CALL(*m_mock_save_ctl, write_json(file_name));
    m_group->save_control(file_name);
    EXPECT_CALL(*m_mock_save_ctl, restore(_));
    m_group->restore_control(file_name);
}

TEST_F(SSTIOGroupTest, enable_sst_tf_implies_enable_sst_cp_write_once)
{
    const auto SST_TF_COMMAND = static_cast<uint16_t>(SSTIOGroup::SSTMailboxCommand::M_TURBO_FREQUENCY);
    const auto SST_CP_COMMAND = static_cast<uint16_t>(SSTIOGroup::SSTMailboxCommand::M_CORE_PRIORITY);
    const uint16_t ENABLE_SUBCOMMAND = 0x02;
    const int SST_TF_ENABLE_BIT = 16;
    const int SST_CP_ENABLE_BIT = 1;

    InSequence seq; // SST-CP must be enabled before enabling SST-TF
    EXPECT_CALL(*m_sstio, write_mbox_once(_, SST_CP_COMMAND, ENABLE_SUBCOMMAND,
                                          _, _, _, _, 1 << SST_CP_ENABLE_BIT, _)).WillOnce(Return());
    EXPECT_CALL(*m_sstio, write_mbox_once(_, SST_TF_COMMAND, ENABLE_SUBCOMMAND,
                                          _, _, _, _, 1 << SST_TF_ENABLE_BIT, _)).WillOnce(Return());

    m_group->write_control("SST::TURBO_ENABLE:ENABLE", GEOPM_DOMAIN_PACKAGE, 0, 1);
}

TEST_F(SSTIOGroupTest, disable_sst_cp_implies_disable_sst_tf_write_once)
{
    const auto SST_TF_COMMAND = static_cast<uint16_t>(SSTIOGroup::SSTMailboxCommand::M_TURBO_FREQUENCY);
    const auto SST_CP_COMMAND = static_cast<uint16_t>(SSTIOGroup::SSTMailboxCommand::M_CORE_PRIORITY);
    const uint16_t ENABLE_SUBCOMMAND = 0x02;
    const int SST_TF_ENABLE_BIT = 16;
    const int SST_CP_ENABLE_BIT = 1;

    InSequence seq; // SST-TF must be disabled before disabling SST-CP
    EXPECT_CALL(*m_sstio, write_mbox_once(_, SST_TF_COMMAND, ENABLE_SUBCOMMAND,
                                          _, _, _, _, 0 << SST_TF_ENABLE_BIT, _)).WillOnce(Return());
    EXPECT_CALL(*m_sstio, write_mbox_once(_, SST_CP_COMMAND, ENABLE_SUBCOMMAND,
                                          _, _, _, _, 0 << SST_CP_ENABLE_BIT, _)).WillOnce(Return());

    m_group->write_control("SST::COREPRIORITY_ENABLE:ENABLE", GEOPM_DOMAIN_PACKAGE, 0, 0);
}

TEST_F(SSTIOGroupTest, restored_controls_follow_ordered_dependencies_disabled)
{
    const auto SST_TF_COMMAND = static_cast<uint16_t>(SSTIOGroup::SSTMailboxCommand::M_TURBO_FREQUENCY);
    const auto SST_CP_COMMAND = static_cast<uint16_t>(SSTIOGroup::SSTMailboxCommand::M_CORE_PRIORITY);
    const uint16_t ENABLE_SUBCOMMAND = 0x02;

    // Act like both SST-CP and SST-TF are disabled
    EXPECT_CALL(*m_sstio, read_mbox_once(_, _, _, _)).Times(AtLeast(0)).WillRepeatedly(Return(0));
    m_group->save_control();


    std::vector<uint16_t> disable_commands;
    EXPECT_CALL(*m_sstio, write_mbox_once(_, AnyOf(SST_TF_COMMAND, SST_CP_COMMAND),
                                          ENABLE_SUBCOMMAND, _, _, _, _, 0, _)).WillRepeatedly(Invoke(
                                                      [&disable_commands] (uint32_t, uint16_t command, uint16_t, uint32_t, uint16_t,
    uint32_t, uint32_t, uint64_t, uint64_t) {
        disable_commands.push_back(command);
    }));
    m_group->restore_control();

    // disable_commands may include a direct attempt to disable SST-TF, which
    // is fine. It must include an attempt to disable SST-TF as a prerequisite
    // to disabling SST-CP.
    auto sst_cp_it = std::find(disable_commands.begin(), disable_commands.end(), SST_CP_COMMAND);
    ASSERT_TRUE(sst_cp_it != disable_commands.end())
            << "Expected SST-CP to be disabled in restore()";
    ASSERT_TRUE(sst_cp_it != disable_commands.begin()
                && *std::prev(sst_cp_it) == SST_TF_COMMAND)
            << "Expected SST-CP to be disabled after SST-TF in restore()";
}

TEST_F(SSTIOGroupTest, restored_controls_follow_ordered_dependencies_enabled)
{
    const auto SST_TF_COMMAND = static_cast<uint16_t>(SSTIOGroup::SSTMailboxCommand::M_TURBO_FREQUENCY);
    const auto SST_CP_COMMAND = static_cast<uint16_t>(SSTIOGroup::SSTMailboxCommand::M_CORE_PRIORITY);
    const uint16_t ENABLE_SUBCOMMAND = 0x02;

    // Act like both SST-CP and SST-TF are enabled
    EXPECT_CALL(*m_sstio, read_mbox_once(_, _, _, _)).Times(AtLeast(0)).WillRepeatedly(Return(0xffffffff));
    m_group->save_control();

    std::vector<uint16_t> enable_commands;
    EXPECT_CALL(*m_sstio, write_mbox_once(_, AnyOf(SST_TF_COMMAND, SST_CP_COMMAND),
                                          ENABLE_SUBCOMMAND, _, _, _, _, Gt(0u), _)).WillRepeatedly(Invoke(
                                                      [&enable_commands] (uint32_t, uint16_t command, uint16_t, uint32_t, uint16_t,
    uint32_t, uint32_t, uint64_t, uint64_t) {
        enable_commands.push_back(command);
    }));
    m_group->restore_control();

    // enable_commands may include a direct attempt to enable SST-CP, which
    // is fine. It must include an attempt to enable SST-CP as a prerequisite
    // to enabling SST-TF.
    auto sst_tf_it = std::find(enable_commands.begin(), enable_commands.end(), SST_TF_COMMAND);
    ASSERT_TRUE(sst_tf_it != enable_commands.end())
            << "Expected SST-TF to be enabled in restore()";
    ASSERT_TRUE(sst_tf_it != enable_commands.begin()
                && *std::prev(sst_tf_it) == SST_CP_COMMAND)
            << "Expected SST-TF to be enabled after SST-CP in restore()";
}
