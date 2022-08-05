/*
 * Copyright (c) 2015 - 2022, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "gtest/gtest.h"
#include "geopm/PluginFactory.hpp"
#include "TimeIOGroup.hpp"
#include "geopm/PlatformTopo.hpp"
#include "geopm/Exception.hpp"
#include "geopm_time.h"

using geopm::TimeIOGroup;
using geopm::PlatformTopo;
using geopm::IOGroup;
using geopm::Exception;

class TimeIOGroupTest : public :: testing :: Test
{
    protected:
        TimeIOGroupTest();

        TimeIOGroup m_group;
        const int m_time_domain;
        const double M_EPSILON = 0.1;
};

TimeIOGroupTest::TimeIOGroupTest()
    : m_time_domain(GEOPM_DOMAIN_CPU)
{

}

TEST_F(TimeIOGroupTest, is_valid)
{
    EXPECT_TRUE(m_group.is_valid_signal("TIME::ELAPSED"));
    EXPECT_FALSE(m_group.is_valid_signal("INVALID"));
    EXPECT_FALSE(m_group.is_valid_control("TIME::ELAPSED"));
    EXPECT_FALSE(m_group.is_valid_control("INVALID"));
    EXPECT_EQ(m_time_domain, m_group.signal_domain_type("TIME::ELAPSED"));
    EXPECT_EQ(GEOPM_DOMAIN_INVALID, m_group.signal_domain_type("INVALID"));
    EXPECT_EQ(GEOPM_DOMAIN_INVALID, m_group.control_domain_type("TIME::ELAPSED"));
    EXPECT_EQ(GEOPM_DOMAIN_INVALID, m_group.control_domain_type("INVALID"));

    // alias
    EXPECT_TRUE(m_group.is_valid_signal("TIME"));
    EXPECT_EQ(m_time_domain, m_group.signal_domain_type("TIME"));

    // all provided signals are valid
    EXPECT_NE(0u, m_group.signal_names().size());
    for (const auto &sig : m_group.signal_names()) {
        EXPECT_TRUE(m_group.is_valid_signal(sig));
        EXPECT_EQ(IOGroup::M_SIGNAL_BEHAVIOR_MONOTONE, m_group.signal_behavior(sig));
    }
    EXPECT_EQ(0u, m_group.control_names().size());
}

TEST_F(TimeIOGroupTest, push)
{
    int idx1 = m_group.push_signal("TIME::ELAPSED", m_time_domain, 0);
    int idx2 = m_group.push_signal("TIME::ELAPSED", m_time_domain, 0);
    EXPECT_EQ(idx1, idx2);
    EXPECT_THROW(m_group.push_signal("INVALID", m_time_domain, 0), Exception);
    EXPECT_THROW(m_group.push_control("TIME::ELAPSED", m_time_domain, 0), Exception);
    EXPECT_THROW(m_group.push_control("INVALID", m_time_domain, 0), Exception);

    // alias
    int idx3 = m_group.push_signal("TIME", m_time_domain, 0);
    EXPECT_EQ(idx3, idx1);

    // must push to correct domain
    EXPECT_THROW(m_group.push_signal("TIME", GEOPM_DOMAIN_PACKAGE, 0), Exception);
}

TEST_F(TimeIOGroupTest, read_nothing)
{
    // Can't sample before we push a signal
    EXPECT_THROW(m_group.sample(0), Exception);
    // Calling read_batch with no signals pushed is okay
    EXPECT_NO_THROW(m_group.read_batch());
    // Can't push signal after calling read_batch
    EXPECT_THROW(m_group.push_signal("TIME::ELAPSED", m_time_domain, 0), Exception);
}

TEST_F(TimeIOGroupTest, sample)
{
    // Push a signal and make sure the index comes back 0
    int signal_idx = m_group.push_signal("TIME::ELAPSED", m_time_domain, 0);
    EXPECT_EQ(0, signal_idx);
    // Pushing time twice should result in the same signal index
    signal_idx = m_group.push_signal("TIME::ELAPSED", m_time_domain, 0);
    EXPECT_EQ(0, signal_idx);
    int alias = m_group.push_signal("TIME", m_time_domain, 0);

    // Can't sample prior to reading
    EXPECT_THROW(m_group.sample(signal_idx), Exception);
    // Make sure that calling sample twice without calling
    // read_batch() in between results in the same answer.
    m_group.read_batch();
    double time0 = m_group.sample(signal_idx);
    double time0a = m_group.sample(alias);
    EXPECT_EQ(time0, time0a);
    sleep(1);
    double time1 = m_group.sample(signal_idx);
    EXPECT_EQ(time0, time1);
    m_group.read_batch();
    time1 = m_group.sample(signal_idx);
    double time1a = m_group.sample(alias);
    EXPECT_NE(time0, time1);
    EXPECT_EQ(time1, time1a);
    // Check that a one second spin is recorded as one second long.
    struct geopm_time_s spin0;
    struct geopm_time_s spin1;
    m_group.read_batch();
    geopm_time(&spin0);
    do {
        geopm_time(&spin1);
    }
    while (geopm_time_diff(&spin0, &spin1) < 1.0);
    time0 = m_group.sample(signal_idx);
    m_group.read_batch();
    time1 = m_group.sample(signal_idx);
    EXPECT_NEAR(time1 - time0, 1.0, M_EPSILON);
    // Check for throw if sample index is out of range
    EXPECT_THROW(m_group.sample(1), Exception);
    EXPECT_THROW(m_group.sample(-1), Exception);
}

TEST_F(TimeIOGroupTest, adjust)
{
    EXPECT_NO_THROW(m_group.write_batch());
    EXPECT_THROW(m_group.adjust(0, 0.0), Exception);
    EXPECT_THROW(m_group.write_control("TIME::ELAPSED", m_time_domain, 0, 0.0), Exception);
}

TEST_F(TimeIOGroupTest, read_signal)
{
    // Check that a one second spin is recorded as one second long.
    struct geopm_time_s spin0;
    struct geopm_time_s spin1;
    double time0 = m_group.read_signal("TIME::ELAPSED", m_time_domain, 0);
    double time0a = m_group.read_signal("TIME", m_time_domain, 0);
    EXPECT_NEAR(time0, time0a, M_EPSILON);
    geopm_time(&spin0);
    do {
        geopm_time(&spin1);
    }
    while (geopm_time_diff(&spin0, &spin1) < 1.0);
    double time1 = m_group.read_signal("TIME::ELAPSED", m_time_domain, 0);
    double time1a = m_group.read_signal("TIME", m_time_domain, 0);
    EXPECT_NEAR(time1, time1a, M_EPSILON);
    EXPECT_NEAR(time1 - time0, 1.0, M_EPSILON);
    EXPECT_THROW(m_group.read_signal("INVALID", m_time_domain, 0), Exception);

    // must read correct domain
    EXPECT_THROW(m_group.read_signal("TIME", GEOPM_DOMAIN_PACKAGE, 0), Exception);
}

TEST_F(TimeIOGroupTest, read_signal_and_batch)
{
    // Test that calling read_signal() does not modify the read_batch() values.
    int signal_idx = m_group.push_signal("TIME::ELAPSED", m_time_domain, 0);
    EXPECT_EQ(0, signal_idx);
    m_group.read_batch();
    double time0 = m_group.sample(0);
    sleep(1);
    double time1 = m_group.read_signal("TIME::ELAPSED", m_time_domain, 0);
    double time2 = m_group.sample(0);
    EXPECT_EQ(time0, time2);
    EXPECT_LT(0.9, time1 - time2);
}
