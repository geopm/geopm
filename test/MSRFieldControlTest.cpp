/*
 * Copyright (c) 2015, 2016, 2017, 2018, 2019, 2020, Intel Corporation
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

#include "config.h"

#include <memory>

#include "gtest/gtest.h"
#include "gmock/gmock.h"

#include "MSRFieldControl.hpp"
#include "MSR.hpp"
#include "Helper.hpp"
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

        std::shared_ptr<MockMSRIO> m_msrio;
        int m_cpu;
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
    m_offset = 0xABC;
    m_begin_bit = 16;
    m_end_bit = 23;
    m_mask = 0xFF0000;
    m_idx = 42;
}

TEST_F(MSRFieldControlTest, write_scale)
{
    double scalar = 1.5;
    auto ctl = geopm::make_unique<MSRFieldControl>(m_msrio, m_cpu, m_offset,
                                                   m_begin_bit, m_end_bit,
                                                   MSR::M_FUNCTION_SCALE, scalar);
    double value = 150;
    EXPECT_CALL(*m_msrio, write_msr(m_cpu, m_offset, 0x640000, m_mask));
    ctl->write(value);
}

TEST_F(MSRFieldControlTest, write_batch_scale)
{
    double scalar = 1.5;
    auto ctl = geopm::make_unique<MSRFieldControl>(m_msrio, m_cpu, m_offset,
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
    double scalar = 1.0;
    auto ctl = geopm::make_unique<MSRFieldControl>(m_msrio, m_cpu, m_offset,
                                                   m_begin_bit, m_end_bit,
                                                   MSR::M_FUNCTION_LOG_HALF,
                                                   scalar);
    double value = 0.25;
    EXPECT_CALL(*m_msrio, write_msr(m_cpu, m_offset, 0x020000, m_mask));
    ctl->write(value);
}

TEST_F(MSRFieldControlTest, write_batch_log_half)
{
    double scalar = 1.0;
    auto ctl = geopm::make_unique<MSRFieldControl>(m_msrio, m_cpu, m_offset,
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
    double scalar = 3.0;
    auto ctl = geopm::make_unique<MSRFieldControl>(m_msrio, m_cpu, m_offset,
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
    double scalar = 3.0;
    auto ctl = geopm::make_unique<MSRFieldControl>(m_msrio, m_cpu, m_offset,
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
    auto ctl = geopm::make_unique<MSRFieldControl>(m_msrio, m_cpu, m_offset,
                                                   m_begin_bit, m_end_bit,
                                                   MSR::M_FUNCTION_SCALE, 1.0);
    // setup batch can be called multiple times without further side effects
    EXPECT_CALL(*m_msrio, add_write(_, _)).Times(1);
    ctl->setup_batch();
    ctl->setup_batch();
}

TEST_F(MSRFieldControlTest, errors)
{
    // cannot constuct with null msrio
    GEOPM_EXPECT_THROW_MESSAGE(MSRFieldControl(nullptr, m_cpu, m_offset,
                                               m_begin_bit, m_end_bit,
                                               geopm::MSR::M_FUNCTION_SCALE, 1.0),
                               GEOPM_ERROR_INVALID, "null MSRIO");

    // cannot call adjust without setup batch
    auto ctl = geopm::make_unique<MSRFieldControl>(m_msrio, m_cpu, m_offset,
                                                   m_begin_bit, m_end_bit,
                                                   MSR::M_FUNCTION_SCALE, 1.0);
    GEOPM_EXPECT_THROW_MESSAGE(ctl->adjust(123), GEOPM_ERROR_RUNTIME,
                               "adjust() before setup_batch()");

    // invalid encode function
    GEOPM_EXPECT_THROW_MESSAGE(MSRFieldControl(m_msrio, m_cpu, m_offset,
                                               m_begin_bit, m_end_bit,
                                               -1, 1.0),
                               GEOPM_ERROR_INVALID, "unsupported encode function");
    GEOPM_EXPECT_THROW_MESSAGE(MSRFieldControl(m_msrio, m_cpu, m_offset,
                                               m_begin_bit, m_end_bit,
                                               MSR::M_FUNCTION_OVERFLOW, 1.0),
                               GEOPM_ERROR_INVALID, "unsupported encode function");

    // invalid number of bits
    GEOPM_EXPECT_THROW_MESSAGE(MSRFieldControl(m_msrio, m_cpu, m_offset,
                                               4, 0,
                                               MSR::M_FUNCTION_SCALE, 1.0),
                               GEOPM_ERROR_INVALID, "begin bit must be <= end bit");

}

TEST_F(MSRFieldControlTest, save_restore)
{
    auto ctl = geopm::make_unique<MSRFieldControl>(m_msrio, m_cpu, m_offset,
                                                   m_begin_bit, m_end_bit,
                                                   MSR::M_FUNCTION_SCALE, 1.0);
    uint64_t saved_value = 0x420000;
    EXPECT_CALL(*m_msrio, read_msr(m_cpu, m_offset))
        .WillOnce(Return(saved_value | 0x12));  // extra bits should be masked off for write
    ctl->save();
    EXPECT_CALL(*m_msrio, write_msr(m_cpu, m_offset, saved_value, m_mask));
    ctl->restore();
}
