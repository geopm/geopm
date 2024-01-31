/*
 * Copyright (c) 2015 - 2024, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "config.h"

#include <memory>

#include "gtest/gtest.h"
#include "gmock/gmock.h"

#include "MSRFieldControl.hpp"
#include "MSR.hpp"
#include "geopm/Helper.hpp"
#include "MockMSRIO.hpp"
#include "geopm_test.hpp"

using geopm::MSRFieldControl;
using geopm::MSR;
using testing::Return;
using testing::_;

class MSRFieldControlTest : public ::testing::Test
{
    protected:
        void SetUp();
        void set_up_default_expectations();

        std::shared_ptr<MockMSRIO> m_msrio;
        int m_cpu;
        int m_save_restore_ctx;
        int m_save_idx;
        int m_restore_idx;
        uint64_t m_offset;
        int m_begin_bit;
        int m_end_bit;
        uint64_t m_mask;
        int m_idx;
};

void MSRFieldControlTest::SetUp()
{
    m_msrio = std::make_shared<MockMSRIO>();
    m_cpu = 1;
    m_save_restore_ctx = 1;
    m_save_idx = 1;
    m_restore_idx = 1;
    m_offset = 0xABC;
    m_begin_bit = 16;
    m_end_bit = 23;
    m_mask = 0xFF0000;
    m_idx = 42;
}

void MSRFieldControlTest::set_up_default_expectations()
{
    EXPECT_CALL(*m_msrio, add_read(m_cpu, m_offset, m_save_restore_ctx))
        .WillOnce(Return(m_save_idx));
    EXPECT_CALL(*m_msrio, add_write(m_cpu, m_offset, m_save_restore_ctx))
        .WillOnce(Return(m_restore_idx));
}

TEST_F(MSRFieldControlTest, write_scale)
{
    double scalar = 1.5;
    set_up_default_expectations();
    auto ctl = geopm::make_unique<MSRFieldControl>(m_msrio, m_save_restore_ctx, m_cpu, m_offset,
                                                   m_begin_bit, m_end_bit,
                                                   MSR::M_FUNCTION_SCALE, scalar);
    double value = 150;
    EXPECT_CALL(*m_msrio, write_msr(m_cpu, m_offset, 0x640000, m_mask));
    ctl->write(value);
}

TEST_F(MSRFieldControlTest, write_batch_scale)
{
    set_up_default_expectations();
    double scalar = 1.5;
    auto ctl = geopm::make_unique<MSRFieldControl>(m_msrio, m_save_restore_ctx, m_cpu, m_offset,
                                                   m_begin_bit, m_end_bit,
                                                   MSR::M_FUNCTION_SCALE, scalar);
    double value = 150;
    EXPECT_CALL(*m_msrio, add_write(m_cpu, m_offset))
        .WillOnce(Return(m_idx));
    ctl->setup_batch();
    EXPECT_CALL(*m_msrio, adjust(m_idx, 0x640000, m_mask));
    ctl->adjust(value);
}

TEST_F(MSRFieldControlTest, write_log_half)
{
    set_up_default_expectations();
    double scalar = 1.0;
    auto ctl = geopm::make_unique<MSRFieldControl>(m_msrio, m_save_restore_ctx, m_cpu, m_offset,
                                                   m_begin_bit, m_end_bit,
                                                   MSR::M_FUNCTION_LOG_HALF,
                                                   scalar);
    double value = 0.25;
    EXPECT_CALL(*m_msrio, write_msr(m_cpu, m_offset, 0x020000, m_mask));
    ctl->write(value);
}

TEST_F(MSRFieldControlTest, write_batch_log_half)
{
    set_up_default_expectations();
    double scalar = 1.0;
    auto ctl = geopm::make_unique<MSRFieldControl>(m_msrio, m_save_restore_ctx, m_cpu, m_offset,
                                                   m_begin_bit, m_end_bit,
                                                   MSR::M_FUNCTION_LOG_HALF,
                                                   scalar);
    double value = 0.25;
    EXPECT_CALL(*m_msrio, add_write(m_cpu, m_offset))
        .WillOnce(Return(m_idx));
    ctl->setup_batch();
    EXPECT_CALL(*m_msrio, adjust(m_idx, 0x020000, m_mask));
    ctl->adjust(value);
}

TEST_F(MSRFieldControlTest, write_7_bit_float)
{
    set_up_default_expectations();
    double scalar = 3.0;
    auto ctl = geopm::make_unique<MSRFieldControl>(m_msrio, m_save_restore_ctx, m_cpu, m_offset,
                                                   m_begin_bit, m_end_bit,
                                                   MSR::M_FUNCTION_7_BIT_FLOAT,
                                                   scalar);
    double value = 9.0;
    EXPECT_CALL(*m_msrio, write_msr(m_cpu, m_offset, 0x410000, m_mask));
    ctl->write(value);

    // value must be > 0
    GEOPM_EXPECT_THROW_MESSAGE(ctl->write(0), GEOPM_ERROR_INVALID,
                               "input value <= 0 for M_FUNCTION_7_BIT_FLOAT");
}

TEST_F(MSRFieldControlTest, write_batch_7_bit_float)
{
    set_up_default_expectations();
    double scalar = 3.0;
    auto ctl = geopm::make_unique<MSRFieldControl>(m_msrio, m_save_restore_ctx, m_cpu, m_offset,
                                                   m_begin_bit, m_end_bit,
                                                   MSR::M_FUNCTION_7_BIT_FLOAT,
                                                   scalar);
    double value = 9.0;
    EXPECT_CALL(*m_msrio, add_write(m_cpu, m_offset))
        .WillOnce(Return(m_idx));
    ctl->setup_batch();
    EXPECT_CALL(*m_msrio, adjust(m_idx, 0x410000, m_mask));
    ctl->adjust(value);

    // value must be > 0
    GEOPM_EXPECT_THROW_MESSAGE(ctl->adjust(0), GEOPM_ERROR_INVALID,
                               "input value <= 0 for M_FUNCTION_7_BIT_FLOAT");

}

TEST_F(MSRFieldControlTest, setup_batch)
{
    set_up_default_expectations();
    auto ctl = geopm::make_unique<MSRFieldControl>(m_msrio, m_save_restore_ctx, m_cpu, m_offset,
                                                   m_begin_bit, m_end_bit,
                                                   MSR::M_FUNCTION_SCALE, 1.0);
    // setup batch can be called multiple times without further side effects
    EXPECT_CALL(*m_msrio, add_write(_, _)).Times(1);
    ctl->setup_batch();
    ctl->setup_batch();
}

TEST_F(MSRFieldControlTest, errors)
{
    // cannot construct with null msrio
    GEOPM_EXPECT_THROW_MESSAGE(MSRFieldControl(nullptr, m_save_restore_ctx, m_cpu, m_offset,
                                               m_begin_bit, m_end_bit,
                                               geopm::MSR::M_FUNCTION_SCALE, 1.0),
                               GEOPM_ERROR_INVALID, "null MSRIO");

    // cannot call adjust without setup batch
    set_up_default_expectations();
    auto ctl = geopm::make_unique<MSRFieldControl>(m_msrio, m_save_restore_ctx, m_cpu, m_offset,
                                                   m_begin_bit, m_end_bit,
                                                   MSR::M_FUNCTION_SCALE, 1.0);
    GEOPM_EXPECT_THROW_MESSAGE(ctl->adjust(123), GEOPM_ERROR_RUNTIME,
                               "adjust() before setup_batch()");

    // invalid encode function
    GEOPM_EXPECT_THROW_MESSAGE(MSRFieldControl(m_msrio, m_save_restore_ctx, m_cpu, m_offset,
                                               m_begin_bit, m_end_bit,
                                               -1, 1.0),
                               GEOPM_ERROR_INVALID, "unsupported encode function");
    GEOPM_EXPECT_THROW_MESSAGE(MSRFieldControl(m_msrio, m_save_restore_ctx, m_cpu, m_offset,
                                               m_begin_bit, m_end_bit,
                                               MSR::M_FUNCTION_OVERFLOW, 1.0),
                               GEOPM_ERROR_INVALID, "unsupported encode function");

    // invalid number of bits
    GEOPM_EXPECT_THROW_MESSAGE(MSRFieldControl(m_msrio, m_save_restore_ctx, m_cpu, m_offset,
                                               4, 0,
                                               MSR::M_FUNCTION_SCALE, 1.0),
                               GEOPM_ERROR_INVALID, "begin bit must be <= end bit");

}

TEST_F(MSRFieldControlTest, save_restore)
{
    set_up_default_expectations();
    auto ctl = geopm::make_unique<MSRFieldControl>(m_msrio, m_save_restore_ctx, m_cpu, m_offset,
                                                   m_begin_bit, m_end_bit,
                                                   MSR::M_FUNCTION_SCALE, 1.0);
    uint64_t saved_value = 0x420000;
    EXPECT_CALL(*m_msrio, sample(m_save_idx, m_save_restore_ctx))
        .WillOnce(Return(saved_value | 0x12));  // extra bits should be masked off for write
    ctl->save();
    EXPECT_CALL(*m_msrio, adjust(m_restore_idx, saved_value, m_mask, m_save_restore_ctx));
    ctl->restore();
}
