/*
 * Copyright (c) 2015 - 2024, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <unistd.h>
#include <limits.h>

#include <fstream>
#include <string>

#include "gtest/gtest.h"
#include "gmock/gmock.h"

#include "config.h"
#include "geopm/Helper.hpp"
#include "geopm/Exception.hpp"
#include "MockLevelZero.hpp"
#include "LevelZeroDevicePoolImp.hpp"
#include "LevelZeroGPUTopo.hpp"
#include "geopm_test.hpp"
#include "geopm_topo.h"

using geopm::LevelZeroGPUTopo;
using geopm::LevelZeroDevicePoolImp;
using geopm::Exception;
using testing::Return;

class LevelZeroGPUTopoTest : public :: testing :: Test
{
    protected:
        void SetUp();
        void TearDown();

        std::shared_ptr<MockLevelZero> m_levelzero;
};

void LevelZeroGPUTopoTest::SetUp()
{
    m_levelzero = std::make_shared<MockLevelZero>();
}

void LevelZeroGPUTopoTest::TearDown()
{
}

//Test case: Mock num_gpu = 0 so we hit the appropriate warning and throw on affinitization requests.
TEST_F(LevelZeroGPUTopoTest, no_gpu_config)
{
    const int num_gpu = 0;
    const int num_cpu = 40;
    LevelZeroDevicePoolImp m_device_pool(*m_levelzero);

    EXPECT_CALL(*m_levelzero, num_gpu(GEOPM_DOMAIN_GPU)).WillOnce(Return(num_gpu));
    EXPECT_CALL(*m_levelzero, num_gpu(GEOPM_DOMAIN_GPU_CHIP)).WillOnce(Return(num_gpu));

    LevelZeroGPUTopo topo(m_device_pool, num_cpu);
    EXPECT_EQ(num_gpu, topo.num_gpu());
    EXPECT_EQ(num_gpu, topo.num_gpu(GEOPM_DOMAIN_GPU_CHIP));

    GEOPM_EXPECT_THROW_MESSAGE(topo.cpu_affinity_ideal(num_gpu), GEOPM_ERROR_INVALID, "gpu_idx 0 is out of range");
}

TEST_F(LevelZeroGPUTopoTest, four_forty_config)
{
    const int num_gpu = 4;
    int num_gpu_subdevice = 4;
    const int num_cpu = 40;
    LevelZeroDevicePoolImp m_device_pool(*m_levelzero);

    EXPECT_CALL(*m_levelzero, num_gpu(GEOPM_DOMAIN_GPU)).WillOnce(Return(num_gpu));
    EXPECT_CALL(*m_levelzero, num_gpu(GEOPM_DOMAIN_GPU_CHIP)).WillOnce(Return(num_gpu_subdevice));

    LevelZeroGPUTopo topo(m_device_pool, num_cpu);
    EXPECT_EQ(num_gpu, topo.num_gpu());
    EXPECT_EQ(num_gpu_subdevice, topo.num_gpu(GEOPM_DOMAIN_GPU_CHIP));

    std::set<int> cpus_allowed_set[num_gpu];
    cpus_allowed_set[0] = {0,1,2,3,4,5,6,7,8,9};
    cpus_allowed_set[1] = {10,11,12,13,14,15,16,17,18,19};
    cpus_allowed_set[2] = {20,21,22,23,24,25,26,27,28,29};
    cpus_allowed_set[3] = {30,31,32,33,34,35,36,37,38,39};

    for (int gpu_idx = 0; gpu_idx < num_gpu; ++gpu_idx) {
        ASSERT_THAT(topo.cpu_affinity_ideal(gpu_idx), cpus_allowed_set[gpu_idx]);
        ASSERT_THAT(topo.cpu_affinity_ideal(GEOPM_DOMAIN_GPU_CHIP, gpu_idx), cpus_allowed_set[gpu_idx]);
    }

    num_gpu_subdevice = 8;
    EXPECT_CALL(*m_levelzero, num_gpu(GEOPM_DOMAIN_GPU)).WillOnce(Return(num_gpu));
    EXPECT_CALL(*m_levelzero, num_gpu(GEOPM_DOMAIN_GPU_CHIP)).WillOnce(Return(num_gpu_subdevice));

    LevelZeroGPUTopo topo_sub(m_device_pool, num_cpu);
    EXPECT_EQ(num_gpu, topo_sub.num_gpu());
    EXPECT_EQ(num_gpu_subdevice, topo_sub.num_gpu(GEOPM_DOMAIN_GPU_CHIP));

    std::set<int> cpus_allowed_set_subdevice[num_gpu_subdevice];
    cpus_allowed_set_subdevice[0] = {0,2,4,6,8};
    cpus_allowed_set_subdevice[1] = {1,3,5,7,9};
    cpus_allowed_set_subdevice[2] = {10,12,14,16,18};
    cpus_allowed_set_subdevice[3] = {11,13,15,17,19};
    cpus_allowed_set_subdevice[4] = {20,22,24,26,28};
    cpus_allowed_set_subdevice[5] = {21,23,25,27,29};
    cpus_allowed_set_subdevice[6] = {30,32,34,36,38};
    cpus_allowed_set_subdevice[7] = {31,33,35,37,39};

    for (int gpu_idx = 0; gpu_idx < num_gpu; ++gpu_idx) {
        ASSERT_THAT(topo_sub.cpu_affinity_ideal(gpu_idx), cpus_allowed_set[gpu_idx]);
    }
    for (int gpu_idx = 0; gpu_idx < num_gpu_subdevice; ++gpu_idx) {
        ASSERT_THAT(topo_sub.cpu_affinity_ideal(GEOPM_DOMAIN_GPU_CHIP, gpu_idx), cpus_allowed_set_subdevice[gpu_idx]);
    }
}

//Test case: Different GPU/CPU count, with 8 GPUs and 28 cores per socket.
TEST_F(LevelZeroGPUTopoTest, eight_fiftysix_affinitization_config)
{
    const int num_gpu = 8;
    const int num_gpu_subdevice = 8;
    const int num_cpu = 56;
    LevelZeroDevicePoolImp m_device_pool(*m_levelzero);

    EXPECT_CALL(*m_levelzero, num_gpu(GEOPM_DOMAIN_GPU)).WillOnce(Return(num_gpu));
    EXPECT_CALL(*m_levelzero, num_gpu(GEOPM_DOMAIN_GPU_CHIP)).WillOnce(Return(num_gpu_subdevice));

    LevelZeroGPUTopo topo(m_device_pool, num_cpu);

    EXPECT_EQ(num_gpu, topo.num_gpu());
    EXPECT_EQ(num_gpu_subdevice, topo.num_gpu(GEOPM_DOMAIN_GPU_CHIP));
    std::set<int> cpus_allowed_set[num_gpu];
    cpus_allowed_set[0] = {0 ,1 ,2 ,3 ,4 ,5 ,6 };
    cpus_allowed_set[1] = {7 ,8 ,9 ,10,11,12,13};
    cpus_allowed_set[2] = {14,15,16,17,18,19,20};
    cpus_allowed_set[3] = {21,22,23,24,25,26,27};
    cpus_allowed_set[4] = {28,29,30,31,32,33,34};
    cpus_allowed_set[5] = {35,36,37,38,39,40,41};
    cpus_allowed_set[6] = {42,43,44,45,46,47,48};
    cpus_allowed_set[7] = {49,50,51,52,53,54,55};

    for (int gpu_idx = 0; gpu_idx < num_gpu; ++gpu_idx) {
        ASSERT_THAT(topo.cpu_affinity_ideal(gpu_idx), cpus_allowed_set[gpu_idx]);
        ASSERT_THAT(topo.cpu_affinity_ideal(GEOPM_DOMAIN_GPU_CHIP, gpu_idx), cpus_allowed_set[gpu_idx]);
    }
}

//Test case: CPU count that is not evenly divisible by the GPU count
TEST_F(LevelZeroGPUTopoTest, uneven_affinitization_config)
{
    const int num_gpu = 3;
    const int num_gpu_subdevice = 6;
    const int num_cpu =20;
    LevelZeroDevicePoolImp m_device_pool(*m_levelzero);

    EXPECT_CALL(*m_levelzero, num_gpu(GEOPM_DOMAIN_GPU)).WillOnce(Return(num_gpu));
    EXPECT_CALL(*m_levelzero, num_gpu(GEOPM_DOMAIN_GPU_CHIP)).WillOnce(Return(num_gpu_subdevice));

    LevelZeroGPUTopo topo(m_device_pool, num_cpu);

    EXPECT_EQ(num_gpu, topo.num_gpu());
    std::set<int> cpus_allowed_set[num_gpu];
    cpus_allowed_set[0] = {0 ,1 ,2 ,3 ,4 ,5 ,18};
    cpus_allowed_set[1] = {6 ,7 ,8 ,9 ,10,11,19};
    cpus_allowed_set[2] = {12,13,14,15,16,17};

    for (int gpu_idx = 0; gpu_idx < num_gpu; ++gpu_idx) {
        ASSERT_THAT(topo.cpu_affinity_ideal(gpu_idx), cpus_allowed_set[gpu_idx]);
    }

    std::set<int> cpus_allowed_set_subdevice[num_gpu_subdevice];
    cpus_allowed_set_subdevice[0] = {0, 2, 4,18};
    cpus_allowed_set_subdevice[1] = {1, 3, 5};

    cpus_allowed_set_subdevice[2] = {6, 8,10,19};
    cpus_allowed_set_subdevice[3] = {7, 9,11};

    cpus_allowed_set_subdevice[4] = {12,14,16};
    cpus_allowed_set_subdevice[5] = {13,15,17};

    for (int sub_idx = 0; sub_idx < num_gpu; ++sub_idx) {
        ASSERT_THAT(topo.cpu_affinity_ideal(GEOPM_DOMAIN_GPU_CHIP, sub_idx), cpus_allowed_set_subdevice[sub_idx]);
    }
}

//Test case: High Core count, theoretical system to test large CPU SETS.
//           This represents a system with 64 cores and 8 GPUs
TEST_F(LevelZeroGPUTopoTest, high_cpu_count_config)
{
    const int num_gpu = 8;
    const int num_gpu_subdevice = 32;
    const int num_cpu = 128;
    LevelZeroDevicePoolImp m_device_pool(*m_levelzero);

    EXPECT_CALL(*m_levelzero, num_gpu(GEOPM_DOMAIN_GPU)).WillOnce(Return(num_gpu));
    EXPECT_CALL(*m_levelzero, num_gpu(GEOPM_DOMAIN_GPU_CHIP)).WillOnce(Return(num_gpu_subdevice));

    LevelZeroGPUTopo topo(m_device_pool, num_cpu);

    EXPECT_EQ(num_gpu, topo.num_gpu());
    std::set<int> cpus_allowed_set[num_gpu];

    for (int gpu_idx = 0; gpu_idx < num_gpu; ++gpu_idx) {
        for (int cpu_idx = 0; cpu_idx < num_cpu/num_gpu; ++cpu_idx) {
            cpus_allowed_set[gpu_idx].insert(cpu_idx+(gpu_idx*16));
        }
        ASSERT_THAT(topo.cpu_affinity_ideal(gpu_idx), cpus_allowed_set[gpu_idx]);
    }

    std::set<int> cpus_allowed_set_subdevice[num_gpu_subdevice];
    for (int sub_idx = 0; sub_idx < num_gpu_subdevice; ++sub_idx) {
        for (int cpu_idx = 0; cpu_idx < num_cpu/num_gpu_subdevice; ++cpu_idx) {
            int gpu_idx = sub_idx/(num_gpu_subdevice/num_gpu);
            cpus_allowed_set_subdevice[sub_idx].insert((cpu_idx)*4 + sub_idx + (gpu_idx)*12);
        }
        ASSERT_THAT(topo.cpu_affinity_ideal(GEOPM_DOMAIN_GPU_CHIP, sub_idx), cpus_allowed_set_subdevice[sub_idx]);
    }
}
