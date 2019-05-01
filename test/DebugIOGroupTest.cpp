/*
 * Copyright (c) 2015, 2016, 2017, 2018, 2019, Intel Corporation
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

#include "gtest/gtest.h"
#include "gmock/gmock.h"

#include "PluginFactory.hpp"
#include "DebugIOGroup.hpp"
#include "PlatformTopo.hpp"
#include "Exception.hpp"
#include "MockPlatformTopo.hpp"
#include "geopm_hash.h"

using geopm::DebugIOGroup;
//using geopm::PlatformTopo;
using geopm::Exception;
using testing::Return;
using testing::NiceMock;

class DebugIOGroupTest : public :: testing :: Test
{
    protected:
        DebugIOGroupTest();
        void SetUp();

        NiceMock<MockPlatformTopo> m_topo;
        DebugIOGroup m_group;
        // memory locations to be mapped to signals
        double m_val1_0;
        double m_val1_1;
        double m_val2;
        uint64_t m_int_val;
        std::vector<double*> m_core_signals;
        std::vector<double*> m_board_signals;
        std::vector<uint64_t*> m_cpu_signals;
};

DebugIOGroupTest::DebugIOGroupTest()
    : m_group(m_topo)
    , m_core_signals{&m_val1_0, &m_val1_1}
    , m_board_signals{&m_val2}
    , m_cpu_signals{&m_int_val}
{

}

void DebugIOGroupTest::SetUp()
{
    ON_CALL(m_topo, num_domain(GEOPM_DOMAIN_CORE))
        .WillByDefault(Return(2));
    ON_CALL(m_topo, num_domain(GEOPM_DOMAIN_CPU))
        .WillByDefault(Return(1)); // todo
    ON_CALL(m_topo, num_domain(GEOPM_DOMAIN_BOARD))
        .WillByDefault(Return(1));
    ON_CALL(m_topo, num_domain(GEOPM_DOMAIN_PACKAGE))
        .WillByDefault(Return(1));
}

TEST_F(DebugIOGroupTest, is_valid)
{
    EXPECT_FALSE(m_group.is_valid_signal("VAL_1"));
    EXPECT_FALSE(m_group.is_valid_signal("VAL_2"));
    EXPECT_FALSE(m_group.is_valid_signal("VAL#"));
    EXPECT_FALSE(m_group.is_valid_signal("BAD"));
    m_group.register_signal("VAL_1", GEOPM_DOMAIN_CORE, m_core_signals);
    EXPECT_TRUE(m_group.is_valid_signal("VAL_1"));
    EXPECT_FALSE(m_group.is_valid_signal("VAL_2"));
    EXPECT_FALSE(m_group.is_valid_signal("VAL#"));
    EXPECT_FALSE(m_group.is_valid_signal("BAD"));
    m_group.register_signal("VAL_2", GEOPM_DOMAIN_BOARD, m_board_signals);
    EXPECT_TRUE(m_group.is_valid_signal("VAL_1"));
    EXPECT_TRUE(m_group.is_valid_signal("VAL_2"));
    EXPECT_FALSE(m_group.is_valid_signal("VAL#"));
    EXPECT_FALSE(m_group.is_valid_signal("BAD"));
    m_group.register_signal("VAL#", GEOPM_DOMAIN_CPU, m_cpu_signals);
    EXPECT_TRUE(m_group.is_valid_signal("VAL_1"));
    EXPECT_TRUE(m_group.is_valid_signal("VAL_2"));
    EXPECT_TRUE(m_group.is_valid_signal("VAL#"));
    EXPECT_FALSE(m_group.is_valid_signal("BAD"));

    EXPECT_EQ(GEOPM_DOMAIN_CORE, m_group.signal_domain_type("VAL_1"));
    EXPECT_EQ(GEOPM_DOMAIN_BOARD, m_group.signal_domain_type("VAL_2"));
    EXPECT_EQ(GEOPM_DOMAIN_CPU, m_group.signal_domain_type("VAL#"));

    // all provided signals are valid
    EXPECT_NE(0u, m_group.signal_names().size());
    for (const auto &sig : m_group.signal_names()) {
        EXPECT_TRUE(m_group.is_valid_signal(sig));
    }
    EXPECT_EQ(0u, m_group.control_names().size());
}


TEST_F(DebugIOGroupTest, register_signal_error)
{
    // cannot register the same signal twice
    m_group.register_signal("VAL_2", GEOPM_DOMAIN_BOARD, m_board_signals);
    EXPECT_THROW(m_group.register_signal("VAL_2", GEOPM_DOMAIN_BOARD, m_board_signals), Exception);
    // cannot register the same signal name with a different domain
    EXPECT_THROW(m_group.register_signal("VAL_2", GEOPM_DOMAIN_CPU, m_board_signals), Exception);
    // vector size must equal the domain size
    EXPECT_THROW(m_group.register_signal("VAL_1", GEOPM_DOMAIN_CORE, m_board_signals), Exception);
    // cannot register with null pointers
    EXPECT_THROW(m_group.register_signal("VAL#", GEOPM_DOMAIN_CPU, {(uint64_t*)nullptr}), Exception);
}

TEST_F(DebugIOGroupTest, push)
{
    m_group.register_signal("VAL_1", GEOPM_DOMAIN_CORE, m_core_signals);
    m_group.register_signal("VAL_2", GEOPM_DOMAIN_BOARD, m_board_signals);
    m_group.register_signal("VAL#", GEOPM_DOMAIN_CPU, m_cpu_signals);

    int idx1a = m_group.push_signal("VAL_1", GEOPM_DOMAIN_CORE, 0);
    int idx1b = m_group.push_signal("VAL_1", GEOPM_DOMAIN_CORE, 0);
    int idx1c = m_group.push_signal("VAL_1", GEOPM_DOMAIN_CORE, 1);
    int idx2 = m_group.push_signal("VAL_2", GEOPM_DOMAIN_BOARD, 0);
    int idx3 = m_group.push_signal("VAL#", GEOPM_DOMAIN_CPU, 0);
    EXPECT_EQ(idx1a, idx1b);
    EXPECT_NE(idx1a, idx2);
    EXPECT_NE(idx1a, idx3);

    EXPECT_THROW(m_group.push_signal("INVALID", GEOPM_DOMAIN_BOARD, 0), Exception);
    EXPECT_THROW(m_group.push_control("VAL_1", GEOPM_DOMAIN_BOARD, 0), Exception);
    // must push to correct domain
    EXPECT_THROW(m_group.push_signal("VAL_1", GEOPM_DOMAIN_PACKAGE, 0), Exception);
}

TEST_F(DebugIOGroupTest, read_nothing)
{
    // Can't sample before we push a signal
    EXPECT_THROW(m_group.sample(0), Exception);
    // Calling read_batch with no signals pushed is okay
    EXPECT_NO_THROW(m_group.read_batch());
}

TEST_F(DebugIOGroupTest, sample)
{
    m_group.register_signal("VAL_1", GEOPM_DOMAIN_CORE, m_core_signals);
    m_group.register_signal("VAL_2", GEOPM_DOMAIN_BOARD, m_board_signals);
    m_group.register_signal("VAL#", GEOPM_DOMAIN_CPU, m_cpu_signals);

    int idx1a = m_group.push_signal("VAL_1", GEOPM_DOMAIN_CORE, 0);
    int idx1b = m_group.push_signal("VAL_1", GEOPM_DOMAIN_CORE, 1);
    int idx2 = m_group.push_signal("VAL_2", GEOPM_DOMAIN_BOARD, 0);
    int idx3 = m_group.push_signal("VAL#", GEOPM_DOMAIN_CPU, 0);

    m_val1_0 = 10;
    m_val1_1 = 11;
    m_val2 = 20;
    m_int_val = 0x1234567812345678;
    EXPECT_EQ(m_val1_0, m_group.sample(idx1a));
    EXPECT_EQ(m_val1_1, m_group.sample(idx1b));
    EXPECT_EQ(m_val2, m_group.sample(idx2));
    EXPECT_EQ(m_int_val, geopm_signal_to_field(m_group.sample(idx3)));

    m_val1_0 = 15;
    m_val1_1 = 16;
    m_val2 = 25;
    m_int_val = 0x9876543298765432;
    EXPECT_EQ(m_val1_0, m_group.sample(idx1a));
    EXPECT_EQ(m_val1_1, m_group.sample(idx1b));
    EXPECT_EQ(m_val2, m_group.sample(idx2));
    EXPECT_EQ(m_int_val, geopm_signal_to_field(m_group.sample(idx3)));
}

TEST_F(DebugIOGroupTest, read_signal)
{
    m_group.register_signal("VAL_1", GEOPM_DOMAIN_CORE, m_core_signals);
    m_group.register_signal("VAL_2", GEOPM_DOMAIN_BOARD, m_board_signals);
    m_group.register_signal("VAL#", GEOPM_DOMAIN_CPU, m_cpu_signals);

    m_val1_0 = 10;
    m_val1_1 = 11;
    m_val2 = 20;
    m_int_val = 0x1234567812345678;
    EXPECT_EQ(m_val1_0, m_group.read_signal("VAL_1", GEOPM_DOMAIN_CORE, 0));
    EXPECT_EQ(m_val1_1, m_group.read_signal("VAL_1", GEOPM_DOMAIN_CORE, 1));
    EXPECT_EQ(m_val2, m_group.read_signal("VAL_2", GEOPM_DOMAIN_BOARD, 0));
    EXPECT_EQ(m_int_val, geopm_signal_to_field(m_group.read_signal("VAL#", GEOPM_DOMAIN_CPU, 0)));

    m_val1_0 = 15;
    m_val1_1 = 16;
    m_val2 = 25;
    m_int_val = 0x9876543298765432;
    EXPECT_EQ(m_val1_0, m_group.read_signal("VAL_1", GEOPM_DOMAIN_CORE, 0));
    EXPECT_EQ(m_val1_1, m_group.read_signal("VAL_1", GEOPM_DOMAIN_CORE, 1));
    EXPECT_EQ(m_val2, m_group.read_signal("VAL_2", GEOPM_DOMAIN_BOARD, 0));
    EXPECT_EQ(m_int_val, geopm_signal_to_field(m_group.read_signal("VAL#", GEOPM_DOMAIN_CPU, 0)));
}
