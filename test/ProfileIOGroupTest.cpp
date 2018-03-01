/*
 * Copyright (c) 2015, 2016, 2017, 2018, Intel Corporation
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

#include "geopm_hash.h"
#include "ProfileIOGroup.hpp"
#include "PlatformTopo.hpp"
#include "MockProfileIOSample.hpp"
#include "MockPlatformTopo.hpp"
#include "Exception.hpp"

using ::testing::_;
using ::testing::Return;
using geopm::IProfileIOSample;
using geopm::ProfileIOGroup;
using geopm::PlatformTopo;
using geopm::Exception;

constexpr int TEST_NUM_CPU = 4;

class MockPlatformTopoCpu : public MockPlatformTopo
{
    public:
        int num_domain(int domain_type) const override {
            if (domain_type == PlatformTopo::M_DOMAIN_CPU) {
                return TEST_NUM_CPU;
            } else {
                throw geopm::Exception("ProfileIOGroupTest: not expected to call num_domain with non-cpu domain",
                                       GEOPM_ERROR_INVALID, __FILE__, __LINE__);
            }
        }
};

class ProfileIOGroupTest : public :: testing:: Test
{
    public:
        ProfileIOGroupTest();
        virtual ~ProfileIOGroupTest();
    protected:
        MockPlatformTopoCpu m_mock_topo;
        std::shared_ptr<MockProfileIOSample> m_mock_pios;
        ProfileIOGroup m_piog;
};


ProfileIOGroupTest::ProfileIOGroupTest()
    : m_mock_pios(std::shared_ptr<MockProfileIOSample>(new MockProfileIOSample))
    , m_piog(m_mock_pios, m_mock_topo)
{
    // ProfileIOGroup should never call update; only Controller will update
    EXPECT_CALL(*m_mock_pios, update(_, _)).Times(0);

}

ProfileIOGroupTest::~ProfileIOGroupTest()
{

}

TEST_F(ProfileIOGroupTest, is_valid)
{
    EXPECT_TRUE(m_piog.is_valid_signal("PROFILE::REGION_ID#"));
    EXPECT_TRUE(m_piog.is_valid_signal("PROFILE::PROGRESS"));
    EXPECT_FALSE(m_piog.is_valid_signal("PROFILE::INVALID_SIGNAL"));
    EXPECT_FALSE(m_piog.is_valid_control("PROFILE::INVALID_CONTROL"));
}

TEST_F(ProfileIOGroupTest, domain_type)
{
    EXPECT_EQ(PlatformTopo::M_DOMAIN_CPU, m_piog.signal_domain_type("PROFILE::REGION_ID#"));
    EXPECT_EQ(PlatformTopo::M_DOMAIN_CPU, m_piog.signal_domain_type("PROFILE::PROGRESS"));
    EXPECT_EQ(PlatformTopo::M_DOMAIN_INVALID, m_piog.signal_domain_type("PROFILE::INVALID_SIGNAL"));
    EXPECT_EQ(PlatformTopo::M_DOMAIN_INVALID, m_piog.control_domain_type("PROFILE::INVALID_CONTROL"));
}

TEST_F(ProfileIOGroupTest, invalid_signal)
{
    EXPECT_THROW(m_piog.push_signal("INVALID", PlatformTopo::M_DOMAIN_CPU, 0), Exception); // "signal name \"INVALID\" not valid"
    EXPECT_THROW(m_piog.push_signal("PROFILE::REGION_ID#", PlatformTopo::M_DOMAIN_BOARD, 0), Exception); // "non-CPU domains are not supported"
    EXPECT_THROW(m_piog.push_signal("PROFILE::REGION_ID#", PlatformTopo::M_DOMAIN_CPU, 9999), Exception); // "domain index out of range
    EXPECT_THROW(m_piog.read_signal("INVALID", PlatformTopo::M_DOMAIN_CPU, 0), Exception); // "signal name \"INVALID\" not valid"
    EXPECT_THROW(m_piog.read_signal("PROFILE::REGION_ID#", PlatformTopo::M_DOMAIN_BOARD, 0), Exception); // "non-CPU domains are not supported"
    EXPECT_THROW(m_piog.read_signal("PROFILE::REGION_ID#", PlatformTopo::M_DOMAIN_CPU, 9999), Exception); // "domain index out of range
}

TEST_F(ProfileIOGroupTest, control)
{
    EXPECT_THROW(m_piog.push_control("PROFILE::REGION_ID#", PlatformTopo::M_DOMAIN_CPU, 0), Exception); // "no controls supported"
    EXPECT_THROW(m_piog.write_control("PROFILE::REGION_ID#", PlatformTopo::M_DOMAIN_CPU, 0, 0.0), Exception); // "no controls supported"
}

TEST_F(ProfileIOGroupTest, region_id)
{
    std::vector<uint64_t> expected_rid[4] = {{777, 888},
                                             {555, 444},
                                             {888, 555}, {888, 555}};
    EXPECT_CALL(*m_mock_pios, per_cpu_region_id()).Times(4)
        .WillOnce(Return(expected_rid[0]))
        .WillOnce(Return(expected_rid[1]))
        .WillOnce(Return(expected_rid[2]))
        .WillOnce(Return(expected_rid[3]));

    // push_signal
    int rid_idx0 = m_piog.push_signal("PROFILE::REGION_ID#", PlatformTopo::M_DOMAIN_CPU, 0);
    int dup_idx0 = m_piog.push_signal("PROFILE::REGION_ID#", PlatformTopo::M_DOMAIN_CPU, 0);
    EXPECT_EQ(rid_idx0, dup_idx0);
    int rid_idx1 = m_piog.push_signal("PROFILE::REGION_ID#", PlatformTopo::M_DOMAIN_CPU, 1);
    EXPECT_THROW(m_piog.sample(rid_idx0), Exception); // "sample has not been read"

    // sample
    m_piog.read_batch();
    uint64_t rid0 = geopm_signal_to_field(m_piog.sample(rid_idx0));
    uint64_t rid1 = geopm_signal_to_field(m_piog.sample(rid_idx1));
    EXPECT_EQ(777ULL, rid0);
    EXPECT_EQ(888ULL, rid1);

    // second sample
    m_piog.read_batch();
    rid0 = geopm_signal_to_field(m_piog.sample(rid_idx0));
    rid1 = geopm_signal_to_field(m_piog.sample(rid_idx1));
    EXPECT_EQ(555ULL, rid0);
    EXPECT_EQ(444ULL, rid1);

    // read_signal
    rid0 = geopm_signal_to_field(m_piog.read_signal("PROFILE::REGION_ID#", PlatformTopo::M_DOMAIN_CPU, 0));
    rid1 = geopm_signal_to_field(m_piog.read_signal("PROFILE::REGION_ID#", PlatformTopo::M_DOMAIN_CPU, 1));
    EXPECT_EQ(888ULL, rid0);
    EXPECT_EQ(555ULL, rid1);

    // errors
    EXPECT_THROW(m_piog.push_signal("PROFILE::REGION_ID#", PlatformTopo::M_DOMAIN_CPU, 0),
                 Exception); // "cannot push signal after call to read_batch"
}

// TODO: use vectors and loop for index, prog values
TEST_F(ProfileIOGroupTest, progress)
{
    std::vector<double> expected_progress[5] = {{0.5, 0.3, 0.9},
                                                {0.1, 0.0, 0.4},
                                                {0.1, 0.3, 0.2}, {0.1, 0.3, 0.2}, {0.1, 0.3, 0.2}};
    EXPECT_CALL(*m_mock_pios, per_cpu_progress(_)).Times(5)
        .WillOnce(Return(expected_progress[0]))
        .WillOnce(Return(expected_progress[1]))
        .WillOnce(Return(expected_progress[2]))
        .WillOnce(Return(expected_progress[3]))
        .WillOnce(Return(expected_progress[4]));

    // push_signal
    int prog_idx0 = m_piog.push_signal("PROFILE::PROGRESS", PlatformTopo::M_DOMAIN_CPU, 0);
    int prog_idx1 = m_piog.push_signal("PROFILE::PROGRESS", PlatformTopo::M_DOMAIN_CPU, 1);
    int prog_idx2 = m_piog.push_signal("PROFILE::PROGRESS", PlatformTopo::M_DOMAIN_CPU, 2);
    int dup_idx0 = m_piog.push_signal("PROFILE::PROGRESS", PlatformTopo::M_DOMAIN_CPU, 0);
    EXPECT_EQ(prog_idx0, dup_idx0);
    EXPECT_THROW(m_piog.sample(prog_idx0), Exception); // "sample has not been read"

    // sample
    m_piog.read_batch();
    double prog0 = m_piog.sample(prog_idx0);
    double prog1 = m_piog.sample(prog_idx1);
    double prog2 = m_piog.sample(prog_idx2);
    EXPECT_DOUBLE_EQ(0.5, prog0);
    EXPECT_DOUBLE_EQ(0.3, prog1);
    EXPECT_DOUBLE_EQ(0.9, prog2);

    // second sample
    m_piog.read_batch();
    prog0 = m_piog.sample(prog_idx0);
    prog1 = m_piog.sample(prog_idx1);
    prog2 = m_piog.sample(prog_idx2);
    EXPECT_DOUBLE_EQ(0.1, prog0);
    EXPECT_DOUBLE_EQ(0.0, prog1);
    EXPECT_DOUBLE_EQ(0.4, prog2);

    // read_signal
    prog0 = m_piog.read_signal("PROFILE::PROGRESS", PlatformTopo::M_DOMAIN_CPU, 0);
    prog1 = m_piog.read_signal("PROFILE::PROGRESS", PlatformTopo::M_DOMAIN_CPU, 1);
    prog2 = m_piog.read_signal("PROFILE::PROGRESS", PlatformTopo::M_DOMAIN_CPU, 2);
    EXPECT_DOUBLE_EQ(0.1, prog0);
    EXPECT_DOUBLE_EQ(0.3, prog1);
    EXPECT_DOUBLE_EQ(0.2, prog2);
}
