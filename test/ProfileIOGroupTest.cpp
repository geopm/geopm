/*
 * Copyright (c) 2015 - 2023, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "gtest/gtest.h"

#include "geopm_prof.h"
#include "geopm_hint.h"
#include "geopm_hash.h"
#include "ProfileIOGroup.hpp"
#include "geopm/Exception.hpp"
#include "geopm/Helper.hpp"
#include "MockPlatformTopo.hpp"
#include "MockApplicationSampler.hpp"
#include "geopm_test.hpp"

using geopm::ProfileIOGroup;
using geopm::Exception;
using geopm::IOGroup;
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

    m_group->read_batch();
}

TEST_F(ProfileIOGroupTest, is_valid)
{
    // all provided signals are valid and CPU domain
    EXPECT_NE(0u, m_group->signal_names().size());
    for (const auto &sig : m_group->signal_names()) {
        EXPECT_TRUE(m_group->is_valid_signal(sig));
        EXPECT_EQ(GEOPM_DOMAIN_CPU, m_group->signal_domain_type(sig));
        EXPECT_LT(-1, m_group->signal_behavior(sig));
    }

    EXPECT_EQ(IOGroup::M_SIGNAL_BEHAVIOR_LABEL,
              m_group->signal_behavior("REGION_HASH"));
    EXPECT_EQ(IOGroup::M_SIGNAL_BEHAVIOR_LABEL,
              m_group->signal_behavior("REGION_HINT"));
    EXPECT_EQ(IOGroup::M_SIGNAL_BEHAVIOR_VARIABLE,
              m_group->signal_behavior("REGION_PROGRESS"));
    EXPECT_EQ(IOGroup::M_SIGNAL_BEHAVIOR_MONOTONE,
              m_group->signal_behavior("TIME_HINT_UNKNOWN"));
    EXPECT_EQ(IOGroup::M_SIGNAL_BEHAVIOR_MONOTONE,
              m_group->signal_behavior("TIME_HINT_UNSET"));
    EXPECT_EQ(IOGroup::M_SIGNAL_BEHAVIOR_MONOTONE,
              m_group->signal_behavior("TIME_HINT_COMPUTE"));
    EXPECT_EQ(IOGroup::M_SIGNAL_BEHAVIOR_MONOTONE,
              m_group->signal_behavior("TIME_HINT_MEMORY"));
    EXPECT_EQ(IOGroup::M_SIGNAL_BEHAVIOR_MONOTONE,
              m_group->signal_behavior("TIME_HINT_NETWORK"));
    EXPECT_EQ(IOGroup::M_SIGNAL_BEHAVIOR_MONOTONE,
              m_group->signal_behavior("TIME_HINT_IO"));
    EXPECT_EQ(IOGroup::M_SIGNAL_BEHAVIOR_MONOTONE,
              m_group->signal_behavior("TIME_HINT_SERIAL"));
    EXPECT_EQ(IOGroup::M_SIGNAL_BEHAVIOR_MONOTONE,
              m_group->signal_behavior("TIME_HINT_PARALLEL"));
    EXPECT_EQ(IOGroup::M_SIGNAL_BEHAVIOR_MONOTONE,
              m_group->signal_behavior("TIME_HINT_IGNORE"));

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
    EXPECT_EQ(0.25, m_group->read_signal("REGION_PROGRESS", GEOPM_DOMAIN_CPU, 0));
    EXPECT_EQ(0.75, m_group->read_signal("REGION_PROGRESS", GEOPM_DOMAIN_CPU, 1));
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
    int idx2 = m_group->push_signal("REGION_HASH", GEOPM_DOMAIN_CPU, 2);
    int idx3 = m_group->push_signal("REGION_HASH", GEOPM_DOMAIN_CPU, 3);
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
        EXPECT_TRUE(std::isnan(m_group->sample(idx2)));
        EXPECT_TRUE(std::isnan(m_group->sample(idx3)));
    }
    {
        EXPECT_CALL(m_sampler, cpu_region_hash(_)).Times(0);
        EXPECT_EQ(reg_a, m_group->sample(idx0));
        EXPECT_EQ(reg_b, m_group->sample(idx1));
        EXPECT_TRUE(std::isnan(m_group->sample(idx2)));
        EXPECT_TRUE(std::isnan(m_group->sample(idx3)));
    }

    // second batch
    {
        EXPECT_CALL(m_sampler, cpu_region_hash(0))
            .WillOnce(Return(reg_b));
        EXPECT_CALL(m_sampler, cpu_region_hash(1))
            .WillOnce(Return(GEOPM_REGION_HASH_UNMARKED));
        EXPECT_CALL(m_sampler, cpu_region_hash(2))
            .WillOnce(Return(GEOPM_REGION_HASH_INVALID));
        EXPECT_CALL(m_sampler, cpu_region_hash(3))
            .WillOnce(Return(GEOPM_REGION_HASH_INVALID));
        m_group->read_batch();

        EXPECT_EQ(reg_b, m_group->sample(idx0));
        EXPECT_EQ(GEOPM_REGION_HASH_UNMARKED, m_group->sample(idx1));
        EXPECT_TRUE(std::isnan(m_group->sample(idx2)));
        EXPECT_TRUE(std::isnan(m_group->sample(idx3)));
    }
}

TEST_F(ProfileIOGroupTest, batch_signal_hint)
{
    uint64_t hint_a = GEOPM_REGION_HINT_MEMORY;
    uint64_t hint_b = GEOPM_REGION_HINT_NETWORK;
    int idx0 = m_group->push_signal("REGION_HINT", GEOPM_DOMAIN_CPU, 0);
    int idx1 = m_group->push_signal("REGION_HINT", GEOPM_DOMAIN_CPU, 1);
    EXPECT_NE(idx0, idx1);

    // before batch
    GEOPM_EXPECT_THROW_MESSAGE(m_group->sample(idx0), GEOPM_ERROR_INVALID,
                               "signal has not been read");

    // first batch
    {
        EXPECT_CALL(m_sampler, cpu_hint(0))
            .WillOnce(Return(hint_a));
        EXPECT_CALL(m_sampler, cpu_hint(1))
            .WillOnce(Return(hint_b));
        EXPECT_CALL(m_sampler, cpu_hint(2))
            .WillOnce(Return(GEOPM_REGION_HINT_UNSET));
        EXPECT_CALL(m_sampler, cpu_hint(3))
            .WillOnce(Return(GEOPM_REGION_HINT_UNSET));
        m_group->read_batch();

        EXPECT_EQ(hint_a, m_group->sample(idx0));
        EXPECT_EQ(hint_b, m_group->sample(idx1));
    }
    {
        EXPECT_CALL(m_sampler, cpu_hint(_)).Times(0);
        EXPECT_EQ(hint_a, m_group->sample(idx0));
        EXPECT_EQ(hint_b, m_group->sample(idx1));
    }

    // second batch
    {
        EXPECT_CALL(m_sampler, cpu_hint(0))
            .WillOnce(Return(hint_b));
        EXPECT_CALL(m_sampler, cpu_hint(1))
            .WillOnce(Return(GEOPM_REGION_HINT_UNSET));
        EXPECT_CALL(m_sampler, cpu_hint(2))
            .WillOnce(Return(GEOPM_REGION_HINT_UNSET));
        EXPECT_CALL(m_sampler, cpu_hint(3))
            .WillOnce(Return(GEOPM_REGION_HINT_UNSET));
        m_group->read_batch();

        EXPECT_EQ(hint_b, m_group->sample(idx0));
        EXPECT_EQ(GEOPM_REGION_HINT_UNSET, m_group->sample(idx1));
    }

}

TEST_F(ProfileIOGroupTest, batch_signal_thread_progress)
{
    int idx0 = m_group->push_signal("REGION_PROGRESS", GEOPM_DOMAIN_CPU, 0);
    int idx1 = m_group->push_signal("REGION_PROGRESS", GEOPM_DOMAIN_CPU, 1);
    EXPECT_NE(idx0, idx1);

    // before batch
    GEOPM_EXPECT_THROW_MESSAGE(m_group->sample(idx0), GEOPM_ERROR_INVALID,
                               "signal has not been read");

    // first batch
    {
        EXPECT_CALL(m_sampler, cpu_progress(0))
            .WillOnce(Return(0.5));
        EXPECT_CALL(m_sampler, cpu_progress(1))
            .WillOnce(Return(0.125));
        EXPECT_CALL(m_sampler, cpu_progress(2))
            .WillOnce(Return(NAN));
        EXPECT_CALL(m_sampler, cpu_progress(3))
            .WillOnce(Return(NAN));
        m_group->read_batch();

        EXPECT_EQ(0.5, m_group->sample(idx0));
        EXPECT_EQ(0.125, m_group->sample(idx1));
    }
    {
        EXPECT_CALL(m_sampler, cpu_progress(_)).Times(0);
        EXPECT_EQ(0.5, m_group->sample(idx0));
        EXPECT_EQ(0.125, m_group->sample(idx1));
    }

    // second batch
    {
        EXPECT_CALL(m_sampler, cpu_progress(0))
            .WillOnce(Return(0.75));
        EXPECT_CALL(m_sampler, cpu_progress(1))
            .WillOnce(Return(0.5));
        EXPECT_CALL(m_sampler, cpu_progress(2))
            .WillOnce(Return(NAN));
        EXPECT_CALL(m_sampler, cpu_progress(3))
            .WillOnce(Return(NAN));
        m_group->read_batch();

        EXPECT_EQ(0.75, m_group->sample(idx0));
        EXPECT_EQ(0.5, m_group->sample(idx1));
    }
}

TEST_F(ProfileIOGroupTest, batch_signal_hint_time)
{
    int idx0 = m_group->push_signal("TIME_HINT_NETWORK", GEOPM_DOMAIN_CPU, 2);
    int idx1 = m_group->push_signal("TIME_HINT_NETWORK", GEOPM_DOMAIN_CPU, 3);
    EXPECT_NE(idx0, idx1);
    int idx2 = m_group->push_signal("TIME_HINT_IGNORE", GEOPM_DOMAIN_CPU, 2);
    EXPECT_NE(idx0, idx2);

    // before batch
    GEOPM_EXPECT_THROW_MESSAGE(m_group->sample(idx0), GEOPM_ERROR_INVALID,
                               "signal has not been read");

    // first batch
    {
        // others in batch
        EXPECT_CALL(m_sampler, cpu_hint_time(_, _)).WillRepeatedly(Return(0.0));

        EXPECT_CALL(m_sampler, cpu_hint_time(2, GEOPM_REGION_HINT_NETWORK))
            .WillOnce(Return(7.77));
        EXPECT_CALL(m_sampler, cpu_hint_time(3, GEOPM_REGION_HINT_NETWORK))
            .WillOnce(Return(8.88));
        EXPECT_CALL(m_sampler, cpu_hint_time(2, GEOPM_REGION_HINT_IGNORE))
            .WillOnce(Return(9.99));
        m_group->read_batch();

        EXPECT_EQ(7.77, m_group->sample(idx0));
        EXPECT_EQ(8.88, m_group->sample(idx1));
        EXPECT_EQ(9.99, m_group->sample(idx2));
    }
    {
        EXPECT_CALL(m_sampler, cpu_hint_time(_, _)).Times(0);
        EXPECT_EQ(7.77, m_group->sample(idx0));
        EXPECT_EQ(8.88, m_group->sample(idx1));
        EXPECT_EQ(9.99, m_group->sample(idx2));
    }

    // second batch
    {
        // others in batch
        EXPECT_CALL(m_sampler, cpu_hint_time(_, _)).WillRepeatedly(Return(0.0));

        EXPECT_CALL(m_sampler, cpu_hint_time(2, GEOPM_REGION_HINT_NETWORK))
            .WillOnce(Return(3.33));
        EXPECT_CALL(m_sampler, cpu_hint_time(3, GEOPM_REGION_HINT_NETWORK))
            .WillOnce(Return(4.44));
        EXPECT_CALL(m_sampler, cpu_hint_time(2, GEOPM_REGION_HINT_IGNORE))
            .WillOnce(Return(5.55));
        m_group->read_batch();

        EXPECT_EQ(3.33, m_group->sample(idx0));
        EXPECT_EQ(4.44, m_group->sample(idx1));
        EXPECT_EQ(5.55, m_group->sample(idx2));
    }
}

TEST_F(ProfileIOGroupTest, errors)
{
    GEOPM_EXPECT_THROW_MESSAGE(m_group->push_signal("INVALID", GEOPM_DOMAIN_CPU, 0),
                               GEOPM_ERROR_INVALID, "signal_name INVALID not valid");
    GEOPM_EXPECT_THROW_MESSAGE(m_group->push_signal("REGION_HASH", GEOPM_DOMAIN_BOARD, 0),
                               GEOPM_ERROR_INVALID, "non-CPU domains are not supported");
    GEOPM_EXPECT_THROW_MESSAGE(m_group->push_signal("REGION_HASH", GEOPM_DOMAIN_CPU, -1),
                               GEOPM_ERROR_INVALID, "domain index out of range");
    GEOPM_EXPECT_THROW_MESSAGE(m_group->push_signal("REGION_HASH", GEOPM_DOMAIN_CPU, m_num_cpu),
                               GEOPM_ERROR_INVALID, "domain index out of range");
    GEOPM_EXPECT_THROW_MESSAGE(m_group->read_signal("INVALID", GEOPM_DOMAIN_CPU, 0),
                               GEOPM_ERROR_INVALID, "signal_name INVALID not valid");
    GEOPM_EXPECT_THROW_MESSAGE(m_group->read_signal("REGION_HASH", GEOPM_DOMAIN_BOARD, 0),
                               GEOPM_ERROR_INVALID, "non-CPU domains are not supported");
    GEOPM_EXPECT_THROW_MESSAGE(m_group->read_signal("REGION_HASH", GEOPM_DOMAIN_CPU, -1),
                               GEOPM_ERROR_INVALID, "domain index out of range");
    GEOPM_EXPECT_THROW_MESSAGE(m_group->read_signal("REGION_HASH", GEOPM_DOMAIN_CPU, m_num_cpu),
                               GEOPM_ERROR_INVALID, "domain index out of range");

    // push after read_batch
    m_group->read_batch();
    GEOPM_EXPECT_THROW_MESSAGE(m_group->push_signal("REGION_HASH", GEOPM_DOMAIN_CPU, 0),
                               GEOPM_ERROR_INVALID, "cannot push signal after call to read_batch");
}
