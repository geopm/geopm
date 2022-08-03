/*
 * Copyright (c) 2015 - 2022, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <memory>

#include "gtest/gtest.h"
#include "gmock/gmock.h"

#include "SSTControl.hpp"
#include "MockSSTIO.hpp"
#include "geopm_hash.h"

using geopm::SSTControl;
using testing::Return;

class SSTControlTest : public :: testing :: Test
{
    protected:
        void SetUp(void);
        void TearDown(void);
        std::shared_ptr<MockSSTIO> m_sstio;
        int m_num_cpu = 4;
};

void SSTControlTest::SetUp(void)
{
    m_sstio = std::make_shared<MockSSTIO>();
}

void SSTControlTest::TearDown(void)
{

}

TEST_F(SSTControlTest, mailbox_adjust_batch)
{
    int cpu = 3;
    uint16_t command = 0x7f;
    uint16_t subcommand = 0x33;
    uint32_t interface_param = 0x93;
    uint32_t write_value = 0x56;
    uint32_t begin_bit = 4;
    uint32_t end_bit = 5;
    double scale = 2.0;

    uint32_t read_subcommand = 0x34;
    uint32_t read_interface_param = 0x94;
    uint32_t read_mask = 0xf0;

    SSTControl control{ m_sstio, SSTControl::M_MBOX, cpu, command, subcommand,
                        interface_param, write_value, begin_bit, end_bit, scale,
                        read_subcommand, read_interface_param, read_mask };

    int batch_idx = 42;
    EXPECT_CALL(*m_sstio,
                add_mbox_write(cpu, command, subcommand, interface_param,
                               read_subcommand, read_interface_param, read_mask))
    .WillOnce(Return(batch_idx));

    control.setup_batch();

    double user_write_value = 1.0;
    uint32_t internal_write_value = 2 /* scaled */ << begin_bit;
    uint32_t write_mask = 0x30;
    EXPECT_CALL(*m_sstio, adjust(batch_idx, internal_write_value, write_mask));
    control.adjust(user_write_value);
}

TEST_F(SSTControlTest, mmio_adjust_batch)
{
    int cpu = 3;
    uint16_t command = 0x7f;
    uint16_t subcommand = 0x33;
    uint32_t interface_param = 0x93;
    uint32_t write_value = 0x56;
    uint32_t begin_bit = 4;
    uint32_t end_bit = 5;
    double scale = 2.0;

    uint32_t read_subcommand = 0x34;
    uint32_t read_interface_param = 0x94;
    uint32_t read_mask = 0xf0;

    SSTControl control{ m_sstio, SSTControl::M_MMIO, cpu, command, subcommand,
                        interface_param, write_value, begin_bit, end_bit, scale,
                        read_subcommand, read_interface_param, read_mask };

    int batch_idx = 42;
    EXPECT_CALL(*m_sstio, add_mmio_write(cpu, interface_param, write_value, read_mask))
    .WillOnce(Return(batch_idx));

    control.setup_batch();

    double user_write_value = 1.0;
    uint32_t internal_write_value = 2 /* scaled */ << begin_bit;
    uint32_t write_mask = 0x30;
    EXPECT_CALL(*m_sstio, adjust(batch_idx, internal_write_value, write_mask));
    control.adjust(user_write_value);
}

TEST_F(SSTControlTest, save_restore_mmio)
{
    int cpu = 3;
    uint16_t command = 0x7f;
    uint16_t subcommand = 0x33;
    uint32_t interface_param = 0x93;
    uint32_t write_value = 0x56;
    uint32_t begin_bit = 4;
    uint32_t end_bit = 5;
    double scale = 2.0;

    uint32_t read_subcommand = 0x34;
    uint32_t read_interface_param = 0x94;
    /* read mask is typically going to be a superset of the bits in the write
     * mask for MMIO-type signals (will read back the whole control to do RMW
     * on a field in that control)
     */
    uint32_t read_mask = 0xf0;
    uint32_t write_mask = 0x30; /* bits 4-5 */

    SSTControl control{ m_sstio, SSTControl::M_MMIO, cpu, command, subcommand,
                        interface_param, write_value, begin_bit, end_bit, scale,
                        read_subcommand, read_interface_param, read_mask };

    uint64_t read_value = 0x1234;
    uint64_t restored_bits = read_value & write_mask;
    EXPECT_CALL(*m_sstio, read_mmio_once(cpu, interface_param))
    .WillOnce(Return(read_value));  // extra bits should be masked off for write
    control.save();
    EXPECT_CALL(*m_sstio, write_mmio_once(cpu, interface_param, write_value,
                                          read_mask, restored_bits, write_mask));
    control.restore();
}

TEST_F(SSTControlTest, save_restore_mbox)
{
    int cpu = 3;
    uint16_t command = 0x7f;
    uint16_t subcommand = 0x33;
    uint32_t interface_param = 0x93;
    uint32_t write_value = 0x56;
    uint32_t begin_bit = 4;
    uint32_t end_bit = 5;
    double scale = 2.0;

    uint32_t read_subcommand = 0x34;
    uint32_t read_interface_param = 0x94;
    /* read mask is typically going to be a superset of the bits in the write
     * mask for MBOX-type signals (will read back the whole control to do RMW
     * on a field in that control)
     */
    uint32_t read_mask = 0xf0;
    uint32_t write_mask = 0x30; /* bits 4-5 */

    SSTControl control{ m_sstio, SSTControl::M_MBOX, cpu, command, subcommand,
                        interface_param, write_value, begin_bit, end_bit, scale,
                        read_subcommand, read_interface_param, read_mask };

    uint64_t read_value = 0x1234;
    uint64_t restored_bits = read_value & write_mask;
    EXPECT_CALL(*m_sstio, read_mbox_once(cpu, command, read_subcommand, read_interface_param))
    .WillOnce(Return(read_value));  // extra bits should be masked off for write
    control.save();
    EXPECT_CALL(*m_sstio, write_mbox_once(cpu, command, subcommand,
                                          interface_param, read_subcommand,
                                          read_interface_param, read_mask,
                                          restored_bits, write_mask));
    control.restore();
}
