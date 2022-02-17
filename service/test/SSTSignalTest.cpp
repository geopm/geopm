/*
 * Copyright (c) 2015 - 2022, Intel Corporation
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

#include "geopm_hash.h"
#include "geopm_internal.h"
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
