/*
 * Copyright (c) 2015 - 2023, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "gtest/gtest.h"
#include "gmock/gmock.h"

#include "EpochIOGroup.hpp"
#include "ApplicationSampler.hpp"
#include "record.hpp"
#include "geopm/Exception.hpp"
#include "geopm/PlatformTopo.hpp"
#include "MockPlatformTopo.hpp"
#include "MockApplicationSampler.hpp"
#include "geopm_test.hpp"

using geopm::Exception;
using geopm::ApplicationSampler;
using geopm::EpochIOGroup;
using geopm::IOGroup;
using testing::Return;
using testing::_;

class EpochIOGroupTest : public ::testing::Test
{
    protected:
        void SetUp(void);
        MockPlatformTopo m_topo;
        int m_num_cpu;
        int m_pid_0;
        int m_pid_1;
        MockApplicationSampler m_app;
        std::shared_ptr<EpochIOGroup> m_group;

};

void EpochIOGroupTest::SetUp()
{
    m_num_cpu = 4;
    m_pid_0 = 33;
    m_pid_1 = 42;
    std::vector<int> cpu_process { m_pid_0, m_pid_0, m_pid_1, m_pid_1 };
    ON_CALL(m_topo, num_domain(GEOPM_DOMAIN_CPU))
        .WillByDefault(Return(m_num_cpu));
    ON_CALL(m_app, per_cpu_process())
        .WillByDefault(Return(cpu_process));
    EXPECT_CALL(m_topo, num_domain(GEOPM_DOMAIN_CPU));
    m_group = std::make_shared<EpochIOGroup>(m_topo, m_app);
}

TEST_F(EpochIOGroupTest, valid_signals)
{
    std::vector<std::string> expected_names = {
        "EPOCH::EPOCH_COUNT",
        "EPOCH_COUNT",
    };
    // enable signals
    m_group->read_batch();

    auto signal_names = m_group->signal_names();
    for (const auto &name : expected_names) {
        EXPECT_TRUE(m_group->is_valid_signal(name));
        EXPECT_TRUE(signal_names.find(name) != signal_names.end());
        EXPECT_FALSE(m_group->signal_description(name).empty());
        // all signals are CPU domain
        EXPECT_EQ(GEOPM_DOMAIN_CPU, m_group->signal_domain_type(name));
        EXPECT_EQ(IOGroup::M_SIGNAL_BEHAVIOR_MONOTONE, m_group->signal_behavior(name));
        // read_signal is not supported
        EXPECT_THROW(m_group->read_signal(name, GEOPM_DOMAIN_CPU, 0), Exception);
    }

    // check aggregation
    EXPECT_TRUE(is_agg_min(m_group->agg_function("EPOCH_COUNT")));

    // check formatting
    EXPECT_TRUE(is_format_integer(m_group->format_function("EPOCH_COUNT")));

    // invalid inputs
    EXPECT_FALSE(m_group->is_valid_signal("INVALID"));
    EXPECT_FALSE(signal_names.find("INVALID") != signal_names.end());
    EXPECT_THROW(m_group->signal_description("INVALID"), Exception);
    EXPECT_EQ(GEOPM_DOMAIN_INVALID, m_group->signal_domain_type("INVALID"));
    EXPECT_THROW(m_group->push_signal("INVALID", GEOPM_DOMAIN_CPU, 0), Exception);
    EXPECT_THROW(m_group->push_signal("EPOCH_COUNT", GEOPM_DOMAIN_BOARD, 0), Exception);
    EXPECT_THROW(m_group->push_signal("EPOCH_COUNT", GEOPM_DOMAIN_CPU, -1), Exception);
    EXPECT_THROW(m_group->push_signal("EPOCH_COUNT", GEOPM_DOMAIN_CPU, m_num_cpu), Exception);
    EXPECT_THROW(m_group->read_signal("INVALID", GEOPM_DOMAIN_CPU, 0), Exception);
    EXPECT_THROW(m_group->read_signal("EPOCH_COUNT", GEOPM_DOMAIN_BOARD, 0), Exception);
    EXPECT_THROW(m_group->read_signal("EPOCH_COUNT", GEOPM_DOMAIN_CPU, -1), Exception);
    EXPECT_THROW(m_group->read_signal("EPOCH_COUNT", GEOPM_DOMAIN_CPU, m_num_cpu), Exception);
    EXPECT_THROW(m_group->agg_function("INVALID"), Exception);
    EXPECT_THROW(m_group->format_function("INVALID"), Exception);
}

TEST_F(EpochIOGroupTest, read_batch)
{
    // read_batch: distribute records to each process
    m_app.inject_records(R"(
# agent: monitor
TIME|PROCESS|EVENT|SIGNAL
0.286542262|33|EPOCH_COUNT|1
1.28657223|33|EPOCH_COUNT|2
1.286573997|42|EPOCH_COUNT|1
)");
    m_group->read_batch();
    // no more push allowed
    EXPECT_THROW(m_group->push_signal("EPOCH_COUNT", GEOPM_DOMAIN_CPU, 0), Exception);
}

TEST_F(EpochIOGroupTest, sample_count)
{
    int idx0 = -1;
    int idx1 = -1;
    idx0 = m_group->push_signal("EPOCH_COUNT", GEOPM_DOMAIN_CPU, 0);
    int idx = m_group->push_signal("EPOCH::EPOCH_COUNT", GEOPM_DOMAIN_CPU, 0);
    EXPECT_NE(-1, idx0);
    EXPECT_EQ(idx0, idx);
    idx1 = m_group->push_signal("EPOCH_COUNT", GEOPM_DOMAIN_CPU, 2);
    EXPECT_NE(-1, idx1);
    EXPECT_NE(idx0, idx1);

    // must read_batch once
    EXPECT_THROW(m_group->sample(idx0), Exception);

    m_app.inject_records(R"(
# agent: monitor
TIME|PROCESS|EVENT|SIGNAL
0.286542262|33|EPOCH_COUNT|1
1.28657223|33|EPOCH_COUNT|2
1.286573997|42|EPOCH_COUNT|1
)");
    m_group->read_batch();

    double value = m_group->sample(idx0);
    EXPECT_EQ(2.0, value);
    value = m_group->sample(idx1);
    EXPECT_EQ(1.0, value);

    // errors for sample
    EXPECT_THROW(m_group->sample(-1), Exception);
    EXPECT_THROW(m_group->sample(idx1 + 1), Exception);
}

TEST_F(EpochIOGroupTest, no_controls)
{
    EXPECT_NO_THROW(m_group->write_batch());
    EXPECT_EQ(0u, m_group->control_names().size());
    EXPECT_THROW(m_group->push_control("any", GEOPM_DOMAIN_CPU, 0), Exception);
    EXPECT_THROW(m_group->push_control("any", GEOPM_DOMAIN_CPU, 0), Exception);
}
