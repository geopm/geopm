/*
 * Copyright (c) 2015 - 2023, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "gtest/gtest.h"
#include "gmock/gmock.h"

#include "EpochIOGroup.hpp"
#include "record.hpp"
#include "geopm/Exception.hpp"
#include "geopm/PlatformTopo.hpp"
#include "MockPlatformTopo.hpp"
#include "MockApplicationSampler.hpp"
#include "geopm_prof.h"

using geopm::Exception;
using geopm::record_s;
using testing::Return;
using testing::_;

using geopm::EpochIOGroup;

class EpochIOGroupIntegrationTest : public ::testing::Test
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

void EpochIOGroupIntegrationTest::SetUp()
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

// shorter names for the enum event types
enum {
    REGION_ENTRY = geopm::EVENT_REGION_ENTRY,
    REGION_EXIT = geopm::EVENT_REGION_EXIT,
    EPOCH_COUNT = geopm::EVENT_EPOCH_COUNT,
};

TEST_F(EpochIOGroupIntegrationTest, read_batch_count)
{
    int idx0 = -1;
    int idx1 = -1;
    // expectations for push
    idx0 = m_group->push_signal("EPOCH_COUNT", GEOPM_DOMAIN_CPU, 0);
    int idx = m_group->push_signal("EPOCH::EPOCH_COUNT", GEOPM_DOMAIN_CPU, 0);
    EXPECT_NE(-1, idx0);
    EXPECT_EQ(idx0, idx);
    idx1 = m_group->push_signal("EPOCH_COUNT", GEOPM_DOMAIN_CPU, 2);
    EXPECT_NE(-1, idx1);
    EXPECT_NE(idx0, idx1);

    std::vector<record_s> records = {
        {0.2, m_pid_0, EPOCH_COUNT, 0x1},
        {1.2, m_pid_0, EPOCH_COUNT, 0x2},
        {1.2, m_pid_1, EPOCH_COUNT, 0x1},
    };
    m_app.inject_records(records);
    m_group->read_batch();

    double value = m_group->sample(idx0);
    EXPECT_EQ(2.0, value);
    value = m_group->sample(idx1);
    EXPECT_EQ(1.0, value);
}
