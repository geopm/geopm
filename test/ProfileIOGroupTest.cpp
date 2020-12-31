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

#include "gtest/gtest.h"

#include "geopm.h"
#include "ProfileIOGroup.hpp"
#include "Exception.hpp"
#include "Helper.hpp"
#include "MockPlatformTopo.hpp"
#include "MockApplicationSampler.hpp"
#include "geopm_test.hpp"

using geopm::ProfileIOGroup;
using geopm::Exception;
using testing::Return;
using testing::AtLeast;
using testing::_;

class ProfileIOGroupTest : public :: testing :: Test
{
    protected:
        ProfileIOGroupTest();

        MockPlatformTopo m_topo;
        MockApplicationSampler m_sampler;
        std::shared_ptr<ProfileIOGroup> m_group;

        std::vector<int> m_cpu_rank;
        int m_num_cpu;
};

ProfileIOGroupTest::ProfileIOGroupTest()
    : m_cpu_rank({11, 11, 42, 42})
    , m_num_cpu(m_cpu_rank.size())
{

    ON_CALL(m_topo, num_domain(GEOPM_DOMAIN_CPU))
        .WillByDefault(Return(m_num_cpu));
    EXPECT_CALL(m_topo, num_domain(_)).Times(AtLeast(1));

    m_group = std::make_shared<ProfileIOGroup>(m_topo, m_sampler);

    // no signals before connection
    EXPECT_EQ(0u, m_group->signal_names().size());

    EXPECT_CALL(m_sampler, per_cpu_process())
        .WillOnce(Return(m_cpu_rank));
    m_group->connect();
}

// TODO: test list
// read signal invalid domain and index
// push invalid domain type and index
// push invalid name
// push signal and alias, same result
// region count, region hash
// push after read batch

TEST_F(ProfileIOGroupTest, is_valid)
{
    // all provided signals are valid and CPU domain
    EXPECT_NE(0u, m_group->signal_names().size());
    for (const auto &sig : m_group->signal_names()) {
        EXPECT_TRUE(m_group->is_valid_signal(sig));
        EXPECT_EQ(GEOPM_DOMAIN_CPU, m_group->signal_domain_type(sig));
    }

    // no controls
    EXPECT_EQ(0u, m_group->control_names().size());

    // invalid signal
    EXPECT_FALSE(m_group->is_valid_signal("INVALID"));
    EXPECT_EQ(GEOPM_DOMAIN_INVALID, m_group->signal_domain_type("INVALID"));
}

TEST_F(ProfileIOGroupTest, aliases)
{
    auto all_names = m_group->signal_names();
    int alias_count = 0;
    for (const auto &name : all_names) {
        if (! geopm::string_begins_with(name, m_group->plugin_name())) {
            int idx0 = m_group->push_signal(name, GEOPM_DOMAIN_CPU, 0);
            int idx1 = m_group->push_signal("PROFILE::" + name, GEOPM_DOMAIN_CPU, 0);
            EXPECT_EQ(idx0, idx1);
            ++alias_count;
        }
    }
    EXPECT_LT(0, alias_count) << "Expected some signal aliases";
}

TEST_F(ProfileIOGroupTest, read_signal_region_hash)
{
    uint64_t reg_a = 0xAAAA;
    uint64_t reg_b = 0xBBBB;
    EXPECT_CALL(m_sampler, cpu_region_hash(0))
        .WillOnce(Return(reg_a));
    EXPECT_CALL(m_sampler, cpu_region_hash(1))
        .WillOnce(Return(reg_b));
    EXPECT_EQ(reg_a, m_group->read_signal("REGION_HASH", GEOPM_DOMAIN_CPU, 0));
    EXPECT_EQ(reg_b, m_group->read_signal("REGION_HASH", GEOPM_DOMAIN_CPU, 1));
}

TEST_F(ProfileIOGroupTest, read_signal_hint)
{
    EXPECT_CALL(m_sampler, cpu_hint(0))
        .WillOnce(Return(GEOPM_REGION_HINT_IGNORE));
    EXPECT_CALL(m_sampler, cpu_hint(1))
        .WillOnce(Return(GEOPM_REGION_HINT_MEMORY));
    EXPECT_EQ(GEOPM_REGION_HINT_IGNORE, m_group->read_signal("REGION_HINT", GEOPM_DOMAIN_CPU, 0));
    EXPECT_EQ(GEOPM_REGION_HINT_MEMORY, m_group->read_signal("REGION_HINT", GEOPM_DOMAIN_CPU, 1));
}

TEST_F(ProfileIOGroupTest, read_signal_thread_progress)
{
    EXPECT_CALL(m_sampler, cpu_progress(0))
        .WillOnce(Return(0.25));
    EXPECT_CALL(m_sampler, cpu_progress(1))
        .WillOnce(Return(0.75));
    EXPECT_EQ(0.25, m_group->read_signal("REGION_THREAD_PROGRESS", GEOPM_DOMAIN_CPU, 0));
    EXPECT_EQ(0.75, m_group->read_signal("REGION_THREAD_PROGRESS", GEOPM_DOMAIN_CPU, 1));
}

TEST_F(ProfileIOGroupTest, read_signal_hint_time)
{
    double expected = 2.25;
    EXPECT_CALL(m_sampler, cpu_hint_time(2, GEOPM_REGION_HINT_NETWORK))
        .WillOnce(Return(expected));
    double result = m_group->read_signal("TIME_HINT_NETWORK", GEOPM_DOMAIN_CPU, 2);
    EXPECT_EQ(expected, result);

    expected = 8.88;
    EXPECT_CALL(m_sampler, cpu_hint_time(1, GEOPM_REGION_HINT_IGNORE))
        .WillOnce(Return(expected));
    result = m_group->read_signal("TIME_HINT_IGNORE", GEOPM_DOMAIN_CPU, 1);
    EXPECT_EQ(expected, result);
}

TEST_F(ProfileIOGroupTest, batch_signal_region_hash)
{
    uint64_t reg_a = 0xAAAA;
    uint64_t reg_b = 0xBBBB;
    int idx0 = m_group->push_signal("REGION_HASH", GEOPM_DOMAIN_CPU, 0);
    int idx1 = m_group->push_signal("REGION_HASH", GEOPM_DOMAIN_CPU, 1);
    EXPECT_NE(idx0, idx1);

    // before batch
    GEOPM_EXPECT_THROW_MESSAGE(m_group->sample(idx0), GEOPM_ERROR_INVALID,
                               "signal has not been read");

    // first batch
    {
        EXPECT_CALL(m_sampler, cpu_region_hash(0))
            .WillOnce(Return(reg_a));
        EXPECT_CALL(m_sampler, cpu_region_hash(1))
            .WillOnce(Return(reg_b));
        EXPECT_CALL(m_sampler, cpu_region_hash(2))
            .WillOnce(Return(GEOPM_REGION_HASH_INVALID));
        EXPECT_CALL(m_sampler, cpu_region_hash(3))
            .WillOnce(Return(GEOPM_REGION_HASH_INVALID));
        m_group->read_batch();

        EXPECT_EQ(reg_a, m_group->sample(idx0));
        EXPECT_EQ(reg_b, m_group->sample(idx1));
    }
    {
        EXPECT_CALL(m_sampler, cpu_region_hash(_)).Times(0);
        EXPECT_EQ(reg_a, m_group->sample(idx0));
        EXPECT_EQ(reg_b, m_group->sample(idx1));
    }

    // second batch
    {
        EXPECT_CALL(m_sampler, cpu_region_hash(0))
            .WillOnce(Return(reg_b));
        EXPECT_CALL(m_sampler, cpu_region_hash(1))
            .WillOnce(Return(GEOPM_REGION_HASH_INVALID));
        EXPECT_CALL(m_sampler, cpu_region_hash(2))
            .WillOnce(Return(GEOPM_REGION_HASH_INVALID));
        EXPECT_CALL(m_sampler, cpu_region_hash(3))
            .WillOnce(Return(GEOPM_REGION_HASH_INVALID));
        m_group->read_batch();

        EXPECT_EQ(reg_b, m_group->sample(idx0));
        EXPECT_EQ(GEOPM_REGION_HASH_INVALID, m_group->sample(idx1));
    }
}

TEST_F(ProfileIOGroupTest, batch_signal_hint)
{
    FAIL() << "TODO";
}

TEST_F(ProfileIOGroupTest, batch_signal_thread_progress)
{
    FAIL() << "TODO";
}


TEST_F(ProfileIOGroupTest, batch_signal_hint_time)
{
    int idx0 = m_group->push_signal("TIME_HINT_NETWORK", GEOPM_DOMAIN_CPU, 2);
    int idx0a = m_group->push_signal("PROFILE::TIME_HINT_NETWORK", GEOPM_DOMAIN_CPU, 2);
    EXPECT_EQ(idx0, idx0a);
    int idx1 = m_group->push_signal("TIME_HINT_NETWORK", GEOPM_DOMAIN_CPU, 3);
    EXPECT_NE(idx0, idx1);
    int idx2 = m_group->push_signal("TIME_HINT_IGNORE", GEOPM_DOMAIN_CPU, 2);
    EXPECT_NE(idx0, idx2);

    // TODO: read_batch not used for caching; sample calls through directly
    m_group->read_batch();

    double expected = 7.77;
    EXPECT_CALL(m_sampler, cpu_hint_time(2, GEOPM_REGION_HINT_NETWORK))
        .WillOnce(Return(expected));
    double result = m_group->sample(idx0);
    EXPECT_EQ(expected, result);

    expected = 9.99;
    EXPECT_CALL(m_sampler, cpu_hint_time(2, GEOPM_REGION_HINT_IGNORE))
        .WillOnce(Return(expected));
    result = m_group->sample(idx2);
    EXPECT_EQ(expected, result);

}
