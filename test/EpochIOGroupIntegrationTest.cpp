/*
 * Copyright (c) 2015 - 2021, Intel Corporation
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
#include "record.hpp"
#include "Exception.hpp"
#include "PlatformTopo.hpp"
#include "MockPlatformTopo.hpp"
#include "MockApplicationSampler.hpp"
#include "geopm.h"

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
    HINT = geopm::EVENT_HINT,
};

TEST_F(EpochIOGroupIntegrationTest, read_batch_count)
{
    EXPECT_CALL(m_app, per_cpu_process());
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
