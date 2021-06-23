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

#include <unistd.h>
#include <limits.h>

#include <fstream>
#include <string>

#include "gtest/gtest.h"
#include "gmock/gmock.h"

#include "config.h"
#include "Helper.hpp"
#include "Exception.hpp"
#include "MockLevelZeroDevicePool.hpp"
#include "LevelZeroAcceleratorTopo.hpp"
#include "geopm_test.hpp"

using geopm::LevelZeroAcceleratorTopo;
using geopm::Exception;
using testing::Return;

class LevelZeroAcceleratorTopoTest : public :: testing :: Test
{
    protected:
        void SetUp();
        void TearDown();

        std::shared_ptr<MockLevelZeroDevicePool> m_device_pool;
};

void LevelZeroAcceleratorTopoTest::SetUp()
{
    m_device_pool = std::make_shared<MockLevelZeroDevicePool>();
}

void LevelZeroAcceleratorTopoTest::TearDown()
{
}

//Test case: Mock num_accelerator = 0 so we hit the appropriate warning and throw on affinitization requests.
TEST_F(LevelZeroAcceleratorTopoTest, no_gpu_config)
{
    const int num_accelerator = 0;
    const int num_cpu = 40;

    EXPECT_CALL(*m_device_pool, num_accelerator()).WillOnce(Return(num_accelerator));

    LevelZeroAcceleratorTopo topo(*m_device_pool, num_cpu);
    EXPECT_EQ(num_accelerator, topo.num_accelerator());

    GEOPM_EXPECT_THROW_MESSAGE(topo.cpu_affinity_ideal(num_accelerator), GEOPM_ERROR_INVALID, "accel_idx 0 is out of range");
}

TEST_F(LevelZeroAcceleratorTopoTest, four_forty_config)
{
    const int num_accelerator = 4;
    const int num_cpu = 40;

    EXPECT_CALL(*m_device_pool, num_accelerator()).WillOnce(Return(num_accelerator));

    LevelZeroAcceleratorTopo topo(*m_device_pool, num_cpu);
    EXPECT_EQ(num_accelerator, topo.num_accelerator());
    std::set<int> cpus_allowed_set[num_accelerator];
    cpus_allowed_set[0] = {0,1,2,3,4,5,6,7,8,9};
    cpus_allowed_set[1] = {10,11,12,13,14,15,16,17,18,19};
    cpus_allowed_set[2] = {20,21,22,23,24,25,26,27,28,29};
    cpus_allowed_set[3] = {30,31,32,33,34,35,36,37,38,39};

    for (int accel_idx = 0; accel_idx < num_accelerator; ++accel_idx) {
        ASSERT_THAT(topo.cpu_affinity_ideal(accel_idx), cpus_allowed_set[accel_idx]);
    }
}

//Test case: Different GPU/CPU count, with 8 GPUs and 28 cores per socket.
TEST_F(LevelZeroAcceleratorTopoTest, eight_fiftysix_affinitization_config)
{
    const int num_accelerator = 8;
    const int num_cpu = 56;

    EXPECT_CALL(*m_device_pool, num_accelerator()).WillOnce(Return(num_accelerator));

    LevelZeroAcceleratorTopo topo(*m_device_pool, num_cpu);

    EXPECT_EQ(num_accelerator, topo.num_accelerator());
    std::set<int> cpus_allowed_set[num_accelerator];
    cpus_allowed_set[0] = {0 ,1 ,2 ,3 ,4 ,5 ,6 };
    cpus_allowed_set[1] = {7 ,8 ,9 ,10,11,12,13};
    cpus_allowed_set[2] = {14,15,16,17,18,19,20};
    cpus_allowed_set[3] = {21,22,23,24,25,26,27};
    cpus_allowed_set[4] = {28,29,30,31,32,33,34};
    cpus_allowed_set[5] = {35,36,37,38,39,40,41};
    cpus_allowed_set[6] = {42,43,44,45,46,47,48};
    cpus_allowed_set[7] = {49,50,51,52,53,54,55};

    for (int accel_idx = 0; accel_idx < num_accelerator; ++accel_idx) {
        ASSERT_THAT(topo.cpu_affinity_ideal(accel_idx), cpus_allowed_set[accel_idx]);
    }
}

//Test case: CPU count that is not evenly divisible by the accelerator count
TEST_F(LevelZeroAcceleratorTopoTest, uneven_affinitization_config)
{
    const int num_accelerator = 3;
    const int num_cpu =20;

    EXPECT_CALL(*m_device_pool, num_accelerator()).WillOnce(Return(num_accelerator));

    LevelZeroAcceleratorTopo topo(*m_device_pool, num_cpu);

    EXPECT_EQ(num_accelerator, topo.num_accelerator());
    std::set<int> cpus_allowed_set[num_accelerator];
    cpus_allowed_set[0] = {0 ,1 ,2 ,3 ,4 ,5 ,18};
    cpus_allowed_set[1] = {6 ,7 ,8 ,9 ,10,11,19};
    cpus_allowed_set[2] = {12,13,14,15,16,17};

    for (int accel_idx = 0; accel_idx < num_accelerator; ++accel_idx) {
        ASSERT_THAT(topo.cpu_affinity_ideal(accel_idx), cpus_allowed_set[accel_idx]);
    }
}

//Test case: High Core count, theoretical system to test large CPU SETS.
//           This represents a system with 64 cores and 8 GPUs
TEST_F(LevelZeroAcceleratorTopoTest, high_cpu_count_config)
{
    const int num_accelerator = 8;
    const int num_cpu = 128;

    EXPECT_CALL(*m_device_pool, num_accelerator()).WillOnce(Return(num_accelerator));

    LevelZeroAcceleratorTopo topo(*m_device_pool, num_cpu);

    EXPECT_EQ(num_accelerator, topo.num_accelerator());
    std::set<int> cpus_allowed_set[num_accelerator];

    for (int accel_idx = 0; accel_idx < num_accelerator; ++accel_idx) {
        for (int cpu_idx = 0; cpu_idx < num_cpu/num_accelerator; ++cpu_idx) {
            cpus_allowed_set[accel_idx].insert(cpu_idx+(accel_idx*16));
        }

        ASSERT_THAT(topo.cpu_affinity_ideal(accel_idx), cpus_allowed_set[accel_idx]);
    }
}
