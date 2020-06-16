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
#include "gmock/gmock.h"

#include "EpochIOGroup.hpp"
#include "ApplicationSampler.hpp"
#include "record.hpp"
#include "Exception.hpp"
#include "PlatformTopo.hpp"
#include "MockPlatformTopo.hpp"
#include "MockApplicationSampler.hpp"
#include "MockProcessEpoch.hpp"
#include "geopm_test.hpp"

using geopm::Exception;
using geopm::ApplicationSampler;
using geopm::EpochIOGroup;
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
        std::shared_ptr<MockProcessEpoch> m_epoch_0;
        std::shared_ptr<MockProcessEpoch> m_epoch_1;
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
    m_epoch_0 = std::make_shared<MockProcessEpoch>();
    m_epoch_1 = std::make_shared<MockProcessEpoch>();
    std::map<int, std::shared_ptr<geopm::ProcessEpoch> > process_map {
        {m_pid_0, m_epoch_0},
        {m_pid_1, m_epoch_1},
    };

    EXPECT_CALL(m_topo, num_domain(GEOPM_DOMAIN_CPU));
    m_group = std::make_shared<EpochIOGroup>(m_topo, m_app, process_map);
}

MATCHER_P(IsRecordEqual, other, "")
{
    return arg.time == other.time &&
           arg.process == other.process &&
           arg.event == other.event &&
           arg.signal == other.signal;
}

TEST_F(EpochIOGroupTest, valid_signals)
{
    std::vector<std::string> expected_names = {
        "EPOCH::EPOCH_COUNT",
        "EPOCH::EPOCH_RUNTIME",
        "EPOCH::EPOCH_RUNTIME_NETWORK",
        "EPOCH::EPOCH_RUNTIME_IGNORE",
        "EPOCH_COUNT",
        "EPOCH_RUNTIME",
        "EPOCH_RUNTIME_NETWORK",
        "EPOCH_RUNTIME_IGNORE",
    };
    // enable signals
    EXPECT_CALL(m_app, per_cpu_process());
    m_group->read_batch();

    auto signal_names = m_group->signal_names();
    for (const auto &name : expected_names) {
        EXPECT_TRUE(m_group->is_valid_signal(name));
        EXPECT_TRUE(signal_names.find(name) != signal_names.end());
        EXPECT_FALSE(m_group->signal_description(name).empty());
        // all signals are CPU domain
        EXPECT_EQ(GEOPM_DOMAIN_CPU, m_group->signal_domain_type(name));
        // read_signal is not supported
        EXPECT_THROW(m_group->read_signal(name, GEOPM_DOMAIN_CPU, 0), Exception);
    }

    // check aggregation
    EXPECT_TRUE(is_agg_min(m_group->agg_function("EPOCH_COUNT")));
    EXPECT_TRUE(is_agg_max(m_group->agg_function("EPOCH_RUNTIME")));
    EXPECT_TRUE(is_agg_max(m_group->agg_function("EPOCH_RUNTIME_NETWORK")));
    EXPECT_TRUE(is_agg_max(m_group->agg_function("EPOCH_RUNTIME_IGNORE")));

    // check formatting
    EXPECT_TRUE(is_format_integer(m_group->format_function("EPOCH_COUNT")));
    EXPECT_TRUE(is_format_double(m_group->format_function("EPOCH_RUNTIME")));
    EXPECT_TRUE(is_format_double(m_group->format_function("EPOCH_RUNTIME_NETWORK")));
    EXPECT_TRUE(is_format_double(m_group->format_function("EPOCH_RUNTIME_IGNORE")));

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
0.286542262|33|EPOCH_COUNT|0x1
1.28657223|33|EPOCH_COUNT|0x2
1.286573997|42|EPOCH_COUNT|0x1
)");
    EXPECT_CALL(m_app, per_cpu_process());
    EXPECT_CALL(*m_epoch_0, update(_)).Times(2);
    EXPECT_CALL(*m_epoch_1, update(_)).Times(1);
    m_group->read_batch();
    // no more push allowed
    EXPECT_THROW(m_group->push_signal("EPOCH_COUNT", GEOPM_DOMAIN_CPU, 0), Exception);
}

TEST_F(EpochIOGroupTest, sample_count)
{
    EXPECT_CALL(m_app, per_cpu_process());
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
    m_group->read_batch();

    EXPECT_CALL(*m_epoch_0, epoch_count())
        .WillOnce(Return(3));
    double value = m_group->sample(idx0);
    EXPECT_EQ(3.0, value);
    EXPECT_CALL(*m_epoch_1, epoch_count())
        .WillOnce(Return(4));
    value = m_group->sample(idx1);
    EXPECT_EQ(4.0, value);

    // errors for sample
    EXPECT_THROW(m_group->sample(-1), Exception);
    EXPECT_THROW(m_group->sample(idx1 + 1), Exception);
}

TEST_F(EpochIOGroupTest, sample_runtime)
{
    EXPECT_CALL(m_app, per_cpu_process());
    int idx0 = -1;
    int idx1 = -1;
    idx0 = m_group->push_signal("EPOCH_RUNTIME", GEOPM_DOMAIN_CPU, 0);
    int idx = m_group->push_signal("EPOCH::EPOCH_RUNTIME", GEOPM_DOMAIN_CPU, 0);
    EXPECT_NE(-1, idx0);
    EXPECT_EQ(idx0, idx);
    idx1 = m_group->push_signal("EPOCH_RUNTIME", GEOPM_DOMAIN_CPU, 2);
    EXPECT_NE(-1, idx1);
    EXPECT_NE(idx0, idx1);

    // must read_batch once
    EXPECT_THROW(m_group->sample(idx0), Exception);
    m_group->read_batch();

    EXPECT_CALL(*m_epoch_0, last_epoch_runtime())
        .WillOnce(Return(4.5));
    double value = m_group->sample(idx0);
    EXPECT_EQ(4.5, value);
    EXPECT_CALL(*m_epoch_1, last_epoch_runtime())
        .WillOnce(Return(9.8));
    value = m_group->sample(idx1);
    EXPECT_EQ(9.8, value);
}

TEST_F(EpochIOGroupTest, sample_runtime_network)
{
    EXPECT_CALL(m_app, per_cpu_process());
    int idx0 = -1;
    int idx1 = -1;
    idx0 = m_group->push_signal("EPOCH_RUNTIME_NETWORK", GEOPM_DOMAIN_CPU, 0);
    int idx = m_group->push_signal("EPOCH::EPOCH_RUNTIME_NETWORK", GEOPM_DOMAIN_CPU, 0);
    EXPECT_NE(-1, idx0);
    EXPECT_EQ(idx0, idx);
    idx1 = m_group->push_signal("EPOCH_RUNTIME_NETWORK", GEOPM_DOMAIN_CPU, 2);
    EXPECT_NE(-1, idx1);
    EXPECT_NE(idx0, idx1);

    // must read_batch once
    EXPECT_THROW(m_group->sample(idx0), Exception);
    m_group->read_batch();

    EXPECT_CALL(*m_epoch_0, last_epoch_runtime_network())
        .WillOnce(Return(0.003));
    double value = m_group->sample(idx0);
    EXPECT_EQ(0.003, value);
    EXPECT_CALL(*m_epoch_1, last_epoch_runtime_network())
        .WillOnce(Return(0.004));
    value = m_group->sample(idx1);
    EXPECT_EQ(0.004, value);
}

TEST_F(EpochIOGroupTest, sample_runtime_ignore)
{
    EXPECT_CALL(m_app, per_cpu_process());
    int idx0 = -1;
    int idx1 = -1;
    idx0 = m_group->push_signal("EPOCH_RUNTIME_IGNORE", GEOPM_DOMAIN_CPU, 0);
    int idx = m_group->push_signal("EPOCH::EPOCH_RUNTIME_IGNORE", GEOPM_DOMAIN_CPU, 0);
    EXPECT_NE(-1, idx0);
    EXPECT_EQ(idx0, idx);
    idx1 = m_group->push_signal("EPOCH_RUNTIME_IGNORE", GEOPM_DOMAIN_CPU, 2);
    EXPECT_NE(-1, idx1);
    EXPECT_NE(idx0, idx1);

    // must read_batch once
    EXPECT_THROW(m_group->sample(idx0), Exception);
    m_group->read_batch();

    EXPECT_CALL(*m_epoch_0, last_epoch_runtime_ignore())
        .WillOnce(Return(55.66));
    double value = m_group->sample(idx0);
    EXPECT_EQ(55.66, value);
    EXPECT_CALL(*m_epoch_1, last_epoch_runtime_ignore())
        .WillOnce(Return(34.34));
    value = m_group->sample(idx1);
    EXPECT_EQ(34.34, value);
}

TEST_F(EpochIOGroupTest, no_controls)
{
    EXPECT_NO_THROW(m_group->write_batch());
    EXPECT_EQ(0u, m_group->control_names().size());
    EXPECT_THROW(m_group->push_control("any", GEOPM_DOMAIN_CPU, 0), Exception);
    EXPECT_THROW(m_group->push_control("any", GEOPM_DOMAIN_CPU, 0), Exception);
}
