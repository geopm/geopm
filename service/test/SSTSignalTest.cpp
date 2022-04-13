/*
 * Copyright (c) 2015 - 2022, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <memory>

#include "gtest/gtest.h"
#include "gmock/gmock.h"

#include "geopm_hash.h"
#include "geopm_field.h"
#include "SSTSignal.hpp"
#include "MockSSTIO.hpp"

using geopm::SSTSignal;
using testing::Return;
using testing::_;

class SSTSignalTest : public :: testing :: Test
{
    protected:
        void SetUp(void);
        void TearDown(void);
        std::shared_ptr<MockSSTIO> m_sstio;
        int m_num_cpu = 4;
};

void SSTSignalTest::SetUp(void)
{
    m_sstio = std::make_shared<MockSSTIO>();
}

void SSTSignalTest::TearDown(void)
{

}

TEST_F(SSTSignalTest, mailbox_read_batch)
{
    int cpu = 3;
    uint16_t command = 0x7f;
    uint16_t subcommand = 0x33;
    uint32_t sub_arg = 0x56;
    uint32_t interface_param = 0x93;

    SSTSignal sig {m_sstio, SSTSignal::M_MBOX, cpu, command, subcommand, sub_arg,
                   interface_param};

    int batch_idx = 42;
    EXPECT_CALL(*m_sstio, add_mbox_read(cpu, command, subcommand, sub_arg))
        .WillOnce(Return(batch_idx));

    sig.setup_batch();

    double expected = 6.0;
    EXPECT_CALL(*m_sstio, sample(batch_idx)).WillOnce(Return(geopm_signal_to_field(expected)));

    double result = sig.sample();
    EXPECT_EQ(expected, result);
}

TEST_F(SSTSignalTest, mmio_read_batch)
{
    int cpu = 3;
    uint16_t command = 0x7f;
    uint16_t subcommand = 0x33;
    uint32_t sub_arg = 0x56;
    uint32_t interface_param = 0x93;

    SSTSignal sig {m_sstio, SSTSignal::M_MMIO, cpu, command, subcommand, sub_arg,
                   interface_param};

    int batch_idx = 42;
    EXPECT_CALL(*m_sstio, add_mmio_read(cpu, sub_arg))
        .WillOnce(Return(batch_idx));

    sig.setup_batch();

    double expected = 6.0;
    EXPECT_CALL(*m_sstio, sample(batch_idx)).WillOnce(Return(geopm_signal_to_field(expected)));

    double result = sig.sample();
    EXPECT_EQ(expected, result);
}
