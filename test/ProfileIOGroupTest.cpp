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
#include "geopm_test.hpp"
#include "ProfileIOGroup.hpp"
#include "ProfileIORuntime.hpp"
#include "ProfileIOSample.hpp"
#include "PlatformTopo.hpp"
#include "MockProfileIORuntime.hpp"
#include "MockProfileIOSample.hpp"
#include "MockPlatformTopo.hpp"
#include "Exception.hpp"
#include "config.h"

using ::testing::_;
using ::testing::Return;
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
        std::shared_ptr<MockProfileIORuntime> m_mock_runtime;
        ProfileIOGroup m_piog;
};


ProfileIOGroupTest::ProfileIOGroupTest()
    : m_mock_pios(std::make_shared<MockProfileIOSample>())
    , m_mock_runtime(std::make_shared<MockProfileIORuntime>())
    , m_piog(m_mock_pios, m_mock_runtime, m_mock_topo)
{
    // ProfileIOGroup should never call update; only Controller will update
    EXPECT_CALL(*m_mock_pios, update(_, _)).Times(0);
    EXPECT_CALL(*m_mock_runtime, insert_regulator(_, _)).Times(0);
}

ProfileIOGroupTest::~ProfileIOGroupTest()
{

}

TEST_F(ProfileIOGroupTest, is_valid)
{
    EXPECT_TRUE(m_piog.is_valid_signal("PROFILE::REGION_ID#"));
    EXPECT_TRUE(m_piog.is_valid_signal("PROFILE::REGION_PROGRESS"));
    EXPECT_FALSE(m_piog.is_valid_signal("PROFILE::INVALID_SIGNAL"));
    EXPECT_FALSE(m_piog.is_valid_control("PROFILE::INVALID_CONTROL"));

    // aliases
    EXPECT_TRUE(m_piog.is_valid_signal("REGION_ID#"));
    EXPECT_TRUE(m_piog.is_valid_signal("REGION_PROGRESS"));

    // all provided signals are valid
    EXPECT_NE(0u, m_piog.signal_names().size());
    for (const auto &sig : m_piog.signal_names()) {
        EXPECT_TRUE(m_piog.is_valid_signal(sig));
    }
    EXPECT_EQ(0u, m_piog.control_names().size());
}

TEST_F(ProfileIOGroupTest, domain_type)
{
    EXPECT_EQ(PlatformTopo::M_DOMAIN_CPU, m_piog.signal_domain_type("PROFILE::REGION_ID#"));
    EXPECT_EQ(PlatformTopo::M_DOMAIN_CPU, m_piog.signal_domain_type("PROFILE::REGION_PROGRESS"));
    EXPECT_EQ(PlatformTopo::M_DOMAIN_INVALID, m_piog.signal_domain_type("PROFILE::INVALID_SIGNAL"));
    EXPECT_EQ(PlatformTopo::M_DOMAIN_INVALID, m_piog.control_domain_type("PROFILE::INVALID_CONTROL"));

    // aliases
    EXPECT_EQ(PlatformTopo::M_DOMAIN_CPU, m_piog.signal_domain_type("REGION_ID#"));
    EXPECT_EQ(PlatformTopo::M_DOMAIN_CPU, m_piog.signal_domain_type("REGION_PROGRESS"));
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
    std::vector<std::vector<uint64_t> > expected_rid = {{777, 888},
                                                        {555, 444}};
    std::vector<uint64_t> expected_read_rid = {888, 555};
    ASSERT_EQ(expected_rid[0].size(), expected_read_rid.size());
    int num_cpu = expected_read_rid.size();

    EXPECT_CALL(*m_mock_pios, per_cpu_region_id()).Times(4)
        .WillOnce(Return(expected_rid[0]))
        .WillOnce(Return(expected_rid[1]))
        .WillRepeatedly(Return(expected_read_rid));

    // push_signal
    std::vector<int> rid_idx;
    for (int cpu = 0; cpu < num_cpu; ++cpu) {
        rid_idx.push_back(m_piog.push_signal("PROFILE::REGION_ID#", PlatformTopo::M_DOMAIN_CPU, cpu));
    }
    int dup_idx0 = m_piog.push_signal("PROFILE::REGION_ID#", PlatformTopo::M_DOMAIN_CPU, 0);
    EXPECT_EQ(rid_idx[0], dup_idx0);
    int alias = m_piog.push_signal("REGION_ID#", PlatformTopo::M_DOMAIN_CPU, 0);
    EXPECT_EQ(rid_idx[0], alias);

    GEOPM_EXPECT_THROW_MESSAGE(m_piog.sample(rid_idx[0]), GEOPM_ERROR_INVALID,
                               "signal has not been read");

    // samples
    for (auto expected : expected_rid) {
        m_piog.read_batch();
        for (int cpu = 0; cpu < num_cpu; ++cpu) {
            EXPECT_EQ(expected[cpu],
                      geopm_signal_to_field(m_piog.sample(rid_idx[cpu])));
        }
    }
    // read_signal
    for (int cpu = 0; cpu < num_cpu; ++cpu) {
        EXPECT_EQ(expected_read_rid[cpu],
                  geopm_signal_to_field(m_piog.read_signal("PROFILE::REGION_ID#",
                                                           PlatformTopo::M_DOMAIN_CPU, cpu)));
    }
    // errors
    GEOPM_EXPECT_THROW_MESSAGE(m_piog.push_signal("PROFILE::REGION_ID#", PlatformTopo::M_DOMAIN_CPU, 0),
                               GEOPM_ERROR_INVALID, "cannot push signal after call to read_batch");
}

TEST_F(ProfileIOGroupTest, progress)
{
    std::vector<std::vector<double> > expected_progress = {{0.5, 0.3, 0.9},
                                                           {0.1, 0.0, 0.4}};
    std::vector<double> expected_read_progress = {0.1, 0.3, 0.2};
    ASSERT_EQ(expected_progress[0].size(), expected_read_progress.size());
    int num_cpu = expected_read_progress.size();
    EXPECT_CALL(*m_mock_pios, per_cpu_progress(_)).Times(5)
        .WillOnce(Return(expected_progress[0]))
        .WillOnce(Return(expected_progress[1]))
        .WillRepeatedly(Return(expected_read_progress));

    // push_signal
    std::vector<int> prog_idx;
    for (int cpu = 0; cpu < num_cpu; ++cpu) {
        prog_idx.push_back(m_piog.push_signal("PROFILE::REGION_PROGRESS", PlatformTopo::M_DOMAIN_CPU, cpu));
    }
    int dup_idx0 = m_piog.push_signal("PROFILE::REGION_PROGRESS", PlatformTopo::M_DOMAIN_CPU, 0);
    EXPECT_EQ(prog_idx[0], dup_idx0);
    int alias = m_piog.push_signal("REGION_PROGRESS", PlatformTopo::M_DOMAIN_CPU, 0);
    EXPECT_EQ(prog_idx[0], alias);

    GEOPM_EXPECT_THROW_MESSAGE(m_piog.sample(prog_idx[0]), GEOPM_ERROR_INVALID,
                               "signal has not been read");

    // sample
    for (auto expected : expected_progress) {
        m_piog.read_batch();
        for (int cpu = 0; cpu < num_cpu; ++cpu) {
            EXPECT_DOUBLE_EQ(expected[cpu], m_piog.sample(prog_idx[cpu]));
        }
    }
    // read_signal
    for (int cpu = 0; cpu < num_cpu; ++cpu) {
        EXPECT_DOUBLE_EQ(expected_read_progress[cpu],
                         m_piog.read_signal("PROFILE::REGION_PROGRESS",
                                            PlatformTopo::M_DOMAIN_CPU, cpu));
    }
}

TEST_F(ProfileIOGroupTest, runtime_sample)
{
    uint64_t region_id_1 = 4444;
    uint64_t region_id_2 = 5555;

    std::vector<double> region_runtime_1 = {5, 6, 7, 8};
    std::vector<double> region_runtime_2 = {4, 3, 2, 3};
    std::vector<uint64_t> region_cpu = {5555, 4444, 4444, 5555};
    std::vector<double> expected_runtime = {4, 6, 7, 3};

    EXPECT_CALL(*m_mock_pios, per_cpu_region_id())
        .WillOnce(Return(region_cpu));
    EXPECT_CALL(*m_mock_runtime, per_cpu_runtime(region_id_1))
        .WillOnce(Return(region_runtime_1));
    EXPECT_CALL(*m_mock_runtime, per_cpu_runtime(region_id_2))
        .WillOnce(Return(region_runtime_2));

    // push_signal
    std::vector<int> runtime_idx;
    for (size_t ii = 0; ii < region_cpu.size(); ++ii) {
        runtime_idx.push_back(m_piog.push_signal("PROFILE::REGION_RUNTIME", PlatformTopo::M_DOMAIN_CPU, ii));
    }
    int dup_idx = m_piog.push_signal("PROFILE::REGION_RUNTIME", PlatformTopo::M_DOMAIN_CPU, 0);
    int alias = m_piog.push_signal("REGION_RUNTIME", PlatformTopo::M_DOMAIN_CPU, 0);
    EXPECT_EQ(runtime_idx[0], dup_idx);
    EXPECT_EQ(runtime_idx[0], alias);

    m_piog.read_batch();

    // sample
    std::vector<double> result;
    for (size_t ii = 0; ii < region_cpu.size(); ++ii) {
        result.push_back(m_piog.sample(runtime_idx[ii]));
    }
    EXPECT_EQ(expected_runtime, result);
}

TEST_F(ProfileIOGroupTest, runtime_read_signal)
{
    uint64_t region_id_1 = 4444;
    uint64_t region_id_2 = 5555;

    std::vector<double> region_runtime_1 = {5, 6, 7, 8};
    std::vector<double> region_runtime_2 = {4, 3, 2, 3};
    std::vector<uint64_t> region_cpu = {5555, 4444, 4444, 5555};
    std::vector<double> expected_runtime = {4, 6, 7, 3};

    EXPECT_CALL(*m_mock_pios, per_cpu_region_id()).Times(4)
        .WillRepeatedly(Return(region_cpu));
    EXPECT_CALL(*m_mock_runtime, per_cpu_runtime(region_id_1)).Times(2)
        .WillRepeatedly(Return(region_runtime_1));
    EXPECT_CALL(*m_mock_runtime, per_cpu_runtime(region_id_2)).Times(2)
        .WillRepeatedly(Return(region_runtime_2));

    // read_signal
    std::vector<double> result;
    for (size_t ii = 0; ii < region_cpu.size(); ++ii) {
        result.push_back(m_piog.read_signal("PROFILE::REGION_RUNTIME", PlatformTopo::M_DOMAIN_CPU, ii));
    }
    EXPECT_EQ(expected_runtime, result);
}
