/*
 * Copyright (c) 2015 - 2022, Intel Corporation
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
#include "MockNVMLDevicePool.hpp"
#include "NVMLGPUTopo.hpp"
#include "geopm_test.hpp"
#include "geopm_topo.h"

using geopm::NVMLGPUTopo;
using geopm::Exception;
using testing::Return;

class NVMLGPUTopoTest : public :: testing :: Test
{
    protected:
        void SetUp();
        void TearDown();

        std::shared_ptr<MockNVMLDevicePool> m_device_pool;
};

void NVMLGPUTopoTest::SetUp()
{
    m_device_pool = std::make_shared<MockNVMLDevicePool>();
}

void NVMLGPUTopoTest::TearDown()
{
}

//Test case: Mock num_gpu = 0 so we hit the appropriate warning and throw on affinitization requests.
TEST_F(NVMLGPUTopoTest, no_gpu_config)
{
    const int num_gpu = 0;
    const int num_cpu = 40;

    EXPECT_CALL(*m_device_pool, num_gpu()).WillOnce(Return(num_gpu));

    NVMLGPUTopo topo(*m_device_pool, num_cpu);
    EXPECT_EQ(num_gpu, topo.num_gpu());

    GEOPM_EXPECT_THROW_MESSAGE(topo.cpu_affinity_ideal(num_gpu), GEOPM_ERROR_INVALID, "gpu_idx 0 is out of range");
    GEOPM_EXPECT_THROW_MESSAGE(topo.cpu_affinity_ideal(GEOPM_DOMAIN_GPU, num_gpu), GEOPM_ERROR_INVALID, "gpu_idx 0 is out of range");
    GEOPM_EXPECT_THROW_MESSAGE(topo.cpu_affinity_ideal(GEOPM_DOMAIN_GPU_CHIP, num_gpu), GEOPM_ERROR_INVALID, "gpu_idx 0 is out of range");
}

//Test case: The HPE SX40 default system configuration
TEST_F(NVMLGPUTopoTest, hpe_sx40_default_config)
{
    const int num_gpu = 4;
    const int num_cpu = 40;

    unsigned long gpu_bitmask[num_gpu];
    gpu_bitmask[0] = 0x00000fffff;
    gpu_bitmask[1] = 0x00000fffff;
    gpu_bitmask[2] = 0xfffff00000;
    gpu_bitmask[3] = 0xfffff00000;

    for (int gpu_idx = 0; gpu_idx < num_gpu; ++gpu_idx) {
        EXPECT_CALL(*m_device_pool, cpu_affinity_ideal_mask(gpu_idx)).WillOnce(Return((cpu_set_t *) &gpu_bitmask[gpu_idx]));
    }

    EXPECT_CALL(*m_device_pool, num_gpu()).WillOnce(Return(num_gpu));

    NVMLGPUTopo topo(*m_device_pool, num_cpu);
    EXPECT_EQ(num_gpu, topo.num_gpu());
    EXPECT_EQ(num_gpu, topo.num_gpu(GEOPM_DOMAIN_GPU_CHIP));
    std::set<int> cpus_allowed_set[num_gpu];
    cpus_allowed_set[0] = {0,1,2,3,4,5,6,7,8,9};
    cpus_allowed_set[1] = {10,11,12,13,14,15,16,17,18,19};
    cpus_allowed_set[2] = {20,21,22,23,24,25,26,27,28,29};
    cpus_allowed_set[3] = {30,31,32,33,34,35,36,37,38,39};

    for (int gpu_idx = 0; gpu_idx < num_gpu; ++gpu_idx) {
        ASSERT_THAT(topo.cpu_affinity_ideal(gpu_idx), cpus_allowed_set[gpu_idx]);
        ASSERT_THAT(topo.cpu_affinity_ideal(GEOPM_DOMAIN_GPU_CHIP, gpu_idx), cpus_allowed_set[gpu_idx]);
    }
}

//Test case: All CPUs are associated with one and only one GPUs
TEST_F(NVMLGPUTopoTest, mutex_affinitization_config)
{
    const int num_gpu = 4;
    const int num_cpu = 40;

    unsigned long gpu_bitmask[num_gpu];
    gpu_bitmask[0] = 0x00000003ff;
    gpu_bitmask[1] = 0x00000ffc00;
    gpu_bitmask[2] = 0x003ff00000;
    gpu_bitmask[3] = 0xffc0000000;

    for (int gpu_idx = 0; gpu_idx < num_gpu; ++gpu_idx) {
        EXPECT_CALL(*m_device_pool, cpu_affinity_ideal_mask(gpu_idx)).WillOnce(Return((cpu_set_t *)&gpu_bitmask[gpu_idx]));
    }
    EXPECT_CALL(*m_device_pool, num_gpu()).WillOnce(Return(num_gpu));

    NVMLGPUTopo topo(*m_device_pool, num_cpu);
    EXPECT_EQ(num_gpu, topo.num_gpu());
    std::set<int> cpus_allowed_set[num_gpu];
    cpus_allowed_set[0] = {0,1,2,3,4,5,6,7,8,9};
    cpus_allowed_set[1] = {10,11,12,13,14,15,16,17,18,19};
    cpus_allowed_set[2] = {20,21,22,23,24,25,26,27,28,29};
    cpus_allowed_set[3] = {30,31,32,33,34,35,36,37,38,39};

    for (int gpu_idx = 0; gpu_idx < num_gpu; ++gpu_idx) {
        ASSERT_THAT(topo.cpu_affinity_ideal(gpu_idx), cpus_allowed_set[gpu_idx]);
    }
}

//Test case: All CPUs are associated with all GPUs
TEST_F(NVMLGPUTopoTest, equidistant_affinitization_config)
{
    const int num_gpu = 4;
    const int num_cpu = 40;

    unsigned long gpu_bitmask[num_gpu];
    gpu_bitmask[0] = 0xffffffffff;
    gpu_bitmask[1] = 0xffffffffff;
    gpu_bitmask[2] = 0xffffffffff;
    gpu_bitmask[3] = 0xffffffffff;

    for (int gpu_idx = 0; gpu_idx < num_gpu; ++gpu_idx) {
        EXPECT_CALL(*m_device_pool, cpu_affinity_ideal_mask(gpu_idx)).WillOnce(Return((cpu_set_t *)&gpu_bitmask[gpu_idx]));
    }
    EXPECT_CALL(*m_device_pool, num_gpu()).WillOnce(Return(num_gpu));

    NVMLGPUTopo topo(*m_device_pool, num_cpu);

    EXPECT_EQ(num_gpu, topo.num_gpu());
    std::set<int> cpus_allowed_set[num_gpu];
    cpus_allowed_set[0] = {0,1,2,3,4,5,6,7,8,9};
    cpus_allowed_set[1] = {10,11,12,13,14,15,16,17,18,19};
    cpus_allowed_set[2] = {20,21,22,23,24,25,26,27,28,29};
    cpus_allowed_set[3] = {30,31,32,33,34,35,36,37,38,39};

    for (int gpu_idx = 0; gpu_idx < num_gpu; ++gpu_idx) {
        ASSERT_THAT(topo.cpu_affinity_ideal(gpu_idx), cpus_allowed_set[gpu_idx]);
    }
}

//Test case:  GPU N+1 associates with all CPUs of GPU N, but not vice versa
TEST_F(NVMLGPUTopoTest, n1_superset_n_affinitization_config)
{
    const int num_gpu = 4;
    const int num_cpu = 40;

    unsigned long gpu_bitmask[num_gpu];
    gpu_bitmask[0] = 0xfffffff000;
    gpu_bitmask[1] = 0xffffffff00;
    gpu_bitmask[2] = 0xfffffffff0;
    gpu_bitmask[3] = 0xffffffffff;

    for (int gpu_idx = 0; gpu_idx < num_gpu; ++gpu_idx) {
        EXPECT_CALL(*m_device_pool, cpu_affinity_ideal_mask(gpu_idx)).WillOnce(Return((cpu_set_t *)&gpu_bitmask[gpu_idx]));
    }
    EXPECT_CALL(*m_device_pool, num_gpu()).WillOnce(Return(num_gpu));

    NVMLGPUTopo topo(*m_device_pool, num_cpu);

    EXPECT_EQ(num_gpu, topo.num_gpu());
    std::set<int> cpus_allowed_set[num_gpu];
    cpus_allowed_set[0] = {12,13,14,15,16,17,18,19,20,21};
    cpus_allowed_set[1] = {8,9,10,11,22,23,24,25,26,27};
    cpus_allowed_set[2] = {4,5,6,7,28,29,30,31,32,33};
    cpus_allowed_set[3] = {0,1,2,3,34,35,36,37,38,39};

    for (int gpu_idx = 0; gpu_idx < num_gpu; ++gpu_idx) {
        ASSERT_THAT(topo.cpu_affinity_ideal(gpu_idx), cpus_allowed_set[gpu_idx]);
    }
}

//Test case:  Last GPU has the smallest map, and the entire map will be 'stolen' to cause starvation
TEST_F(NVMLGPUTopoTest, greedbuster_affinitization_config)
{
    const int num_gpu = 4;
    const int num_cpu = 40;

    unsigned long gpu_bitmask[num_gpu];
    gpu_bitmask[0] = 0xffffffffff;
    gpu_bitmask[1] = 0xfffffffff0;
    gpu_bitmask[2] = 0x0fffffff00;
    gpu_bitmask[3] = 0x00000003ff;

    for (int gpu_idx = 0; gpu_idx < num_gpu; ++gpu_idx) {
        EXPECT_CALL(*m_device_pool, cpu_affinity_ideal_mask(gpu_idx)).WillOnce(Return((cpu_set_t *)&gpu_bitmask[gpu_idx]));
    }
    EXPECT_CALL(*m_device_pool, num_gpu()).WillOnce(Return(num_gpu));

    GEOPM_EXPECT_THROW_MESSAGE(NVMLGPUTopo topo(*m_device_pool, num_cpu), GEOPM_ERROR_INVALID, "Failed to affinitize all valid CPUs to GPUs");
}

//Test case: Different GPU/CPU count, namely an approximation of the HPE Apollo 6500
//           system with 8 GPUs and 28 cores per socket.
TEST_F(NVMLGPUTopoTest, hpe_6500_affinitization_config)
{
    const int num_gpu = 8;
    const int num_cpu = 56;

    unsigned long gpu_bitmask[num_gpu];
    gpu_bitmask[0] = 0x0000000fffffff;
    gpu_bitmask[1] = 0x0000000fffffff;
    gpu_bitmask[2] = 0x0000000fffffff;
    gpu_bitmask[3] = 0x0000000fffffff;
    gpu_bitmask[4] = 0xffffffff000000;
    gpu_bitmask[5] = 0xffffffff000000;
    gpu_bitmask[6] = 0xffffffff000000;
    gpu_bitmask[7] = 0xffffffff000000;

    for (int gpu_idx = 0; gpu_idx < num_gpu; ++gpu_idx) {
        EXPECT_CALL(*m_device_pool, cpu_affinity_ideal_mask(gpu_idx)).WillOnce(Return((cpu_set_t *)&gpu_bitmask[gpu_idx]));
    }
    EXPECT_CALL(*m_device_pool, num_gpu()).WillOnce(Return(num_gpu));

    NVMLGPUTopo topo(*m_device_pool, num_cpu);

    EXPECT_EQ(num_gpu, topo.num_gpu());
    std::set<int> cpus_allowed_set[num_gpu];
    cpus_allowed_set[0] = {0,1,2,3,4,5,6 };
    cpus_allowed_set[1] = {7,8,9,10,11,12,13};
    cpus_allowed_set[2] = {14,15,16,17,18,19,20};
    cpus_allowed_set[3] = {21,22,23,24,25,26,27};
    cpus_allowed_set[4] = {28,29,30,31,32,33,34};
    cpus_allowed_set[5] = {35,36,37,38,39,40,41};
    cpus_allowed_set[6] = {42,43,44,45,46,47,48};
    cpus_allowed_set[7] = {49,50,51,52,53,54,55};

    for (int gpu_idx = 0; gpu_idx < num_gpu; ++gpu_idx) {
        ASSERT_THAT(topo.cpu_affinity_ideal(gpu_idx), cpus_allowed_set[gpu_idx]);
    }
}

//Test case: CPU count that is not evenly divisible by the GPU count
TEST_F(NVMLGPUTopoTest, uneven_affinitization_config)
{
    const int num_gpu = 3;
    const int num_cpu =20;

    unsigned long gpu_bitmask[num_gpu];
    gpu_bitmask[0] = 0xfffff;
    gpu_bitmask[1] = 0xfffff;
    gpu_bitmask[2] = 0xfffff;

    for (int gpu_idx = 0; gpu_idx < num_gpu; ++gpu_idx) {
        EXPECT_CALL(*m_device_pool, cpu_affinity_ideal_mask(gpu_idx)).WillOnce(Return((cpu_set_t *)&gpu_bitmask[gpu_idx]));
    }
    EXPECT_CALL(*m_device_pool, num_gpu()).WillOnce(Return(num_gpu));

    NVMLGPUTopo topo(*m_device_pool, num_cpu);

    EXPECT_EQ(num_gpu, topo.num_gpu());
    std::set<int> cpus_allowed_set[num_gpu];
    cpus_allowed_set[0] = {0,1,2,3,4,5,18,19};
    cpus_allowed_set[1] = {6,7,8,9,10,11};
    cpus_allowed_set[2] = {12,13,14,15,16,17};

    for (int gpu_idx = 0; gpu_idx < num_gpu; ++gpu_idx) {
        ASSERT_THAT(topo.cpu_affinity_ideal(gpu_idx), cpus_allowed_set[gpu_idx]);
    }
}

//Test case: High Core count, theoretical system to test large CPU SETS.
//           This represents a system with 64 cores and 8 GPUs
TEST_F(NVMLGPUTopoTest, high_cpu_count_config)
{
    const int num_gpu = 8;
    const int num_cpu = 128;

    unsigned long gpu_bitmask[num_gpu][2];
    gpu_bitmask[0][0] = 0xffffffffffffffff;
    gpu_bitmask[0][1] = 0xffffffffffffffff;
    gpu_bitmask[1][0] = 0xffffffffffffffff;
    gpu_bitmask[1][1] = 0xffffffffffffffff;
    gpu_bitmask[2][0] = 0xffffffffffffffff;
    gpu_bitmask[2][1] = 0xffffffffffffffff;
    gpu_bitmask[3][0] = 0xffffffffffffffff;
    gpu_bitmask[3][1] = 0xffffffffffffffff;
    gpu_bitmask[4][0] = 0xffffffffffffffff;
    gpu_bitmask[4][1] = 0xffffffffffffffff;
    gpu_bitmask[5][0] = 0xffffffffffffffff;
    gpu_bitmask[5][1] = 0xffffffffffffffff;
    gpu_bitmask[6][0] = 0xffffffffffffffff;
    gpu_bitmask[6][1] = 0xffffffffffffffff;
    gpu_bitmask[7][0] = 0xffffffffffffffff;
    gpu_bitmask[7][1] = 0xffffffffffffffff;

    for (int gpu_idx = 0; gpu_idx < num_gpu; ++gpu_idx) {
        EXPECT_CALL(*m_device_pool, cpu_affinity_ideal_mask(gpu_idx)).WillOnce(Return((cpu_set_t *)&gpu_bitmask[gpu_idx]));
    }
    EXPECT_CALL(*m_device_pool, num_gpu()).WillOnce(Return(num_gpu));

    NVMLGPUTopo topo(*m_device_pool, num_cpu);

    EXPECT_EQ(num_gpu, topo.num_gpu());
    std::set<int> cpus_allowed_set[num_gpu];

    for (int gpu_idx = 0; gpu_idx < num_gpu; ++gpu_idx) {
        for (int cpu_idx = 0; cpu_idx < num_cpu/num_gpu; ++cpu_idx) {
            cpus_allowed_set[gpu_idx].insert(cpu_idx+(gpu_idx*16));
        }

        ASSERT_THAT(topo.cpu_affinity_ideal(gpu_idx), cpus_allowed_set[gpu_idx]);
    }
}

//Test case: High Core count system with sparse affinitization, to test uneven distribution with gaps.
TEST_F(NVMLGPUTopoTest, high_cpu_count_gaps_config)
{
    const int num_gpu = 8;
    const int num_cpu = 128;

    unsigned long gpu_bitmask[num_gpu][2];
    gpu_bitmask[0][0] = 0x000000000fffffff;
    gpu_bitmask[0][1] = 0x000000000000000f;
    gpu_bitmask[1][0] = 0x000000000fffffff;
    gpu_bitmask[1][1] = 0x000000000000000f;
    gpu_bitmask[2][0] = 0x000000000fffffff;
    gpu_bitmask[2][1] = 0x000000000000000f;
    gpu_bitmask[3][0] = 0x000000000fffffff;
    gpu_bitmask[3][1] = 0x000000000000000f;
    gpu_bitmask[4][0] = 0x00ffffffff000000;
    gpu_bitmask[4][1] = 0xf800000000000000;
    gpu_bitmask[5][0] = 0x00ffffffff000000;
    gpu_bitmask[5][1] = 0xf800000000000000;
    gpu_bitmask[6][0] = 0x00ffffffff000000;
    gpu_bitmask[6][1] = 0xf800000000000000;
    gpu_bitmask[7][0] = 0x00ffffffff000000;
    gpu_bitmask[7][1] = 0xf800000000000000;

    for (int gpu_idx = 0; gpu_idx < num_gpu; ++gpu_idx) {
        EXPECT_CALL(*m_device_pool, cpu_affinity_ideal_mask(gpu_idx)).WillOnce(Return((cpu_set_t *)&gpu_bitmask[gpu_idx]));
    }
    EXPECT_CALL(*m_device_pool, num_gpu()).WillOnce(Return(num_gpu));

    NVMLGPUTopo topo(*m_device_pool, num_cpu);

    EXPECT_EQ(num_gpu, topo.num_gpu());
    std::set<int> cpus_allowed_set[num_gpu];
    cpus_allowed_set[0] = {0,1,2,3,4,5,6,7};
    cpus_allowed_set[1] = {8,9,10,11,12,13,14,15};
    cpus_allowed_set[2] = {16,17,18,19,20,21,22,23};
    cpus_allowed_set[3] = {24,25,26,27,64,65,66,67};
    cpus_allowed_set[4] = {28,29,30,31,32,33,34,35,127};
    cpus_allowed_set[5] = {36,37,38,39,40,41,42,43};
    cpus_allowed_set[6] = {44,45,46,47,48,49,50,51};
    cpus_allowed_set[7] = {52,53,54,55,123,124,125,126};

    for (int gpu_idx = 0; gpu_idx < num_gpu; ++gpu_idx) {
        ASSERT_THAT(topo.cpu_affinity_ideal(gpu_idx), cpus_allowed_set[gpu_idx]);
    }
}
