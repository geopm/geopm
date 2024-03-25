/*
 * Copyright (c) 2015 - 2024, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <unistd.h>
#include <limits.h>

#include <string>
#include <set>
#include <vector>

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
using geopm::make_cpu_set;
using testing::Return;

class NVMLGPUTopoTest : public :: testing :: Test
{
    protected:
        void SetUp();
        void TearDown();
        void set_up_device_pool_expectations(int num_cpu,
                                             const std::vector<std::set<int>> &cpus_allowed);

        std::shared_ptr<MockNVMLDevicePool> m_device_pool;
};

void NVMLGPUTopoTest::SetUp()
{
    m_device_pool = std::make_shared<MockNVMLDevicePool>();
}

void NVMLGPUTopoTest::TearDown()
{
}

void NVMLGPUTopoTest::set_up_device_pool_expectations(int num_cpu,
                                                      const std::vector<std::set<int>> &cpus)
{
    for (size_t idx = 0; idx < cpus.size(); ++idx) {
        EXPECT_CALL(*m_device_pool, cpu_affinity_ideal_mask(idx))
            .WillOnce(Return(make_cpu_set(num_cpu, cpus[idx])));
    }
    EXPECT_CALL(*m_device_pool, num_gpu()).WillOnce(Return(cpus.size()));
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
    const std::vector<std::set<int>> gpu_bitmask = {
        {0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19},
        {0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19},
        {20,21,22,23,24,25,26,27,28,29,30,31,32,33,34,35,36,37,38,39},
        {20,21,22,23,24,25,26,27,28,29,30,31,32,33,34,35,36,37,38,39}};
    const int num_gpu = gpu_bitmask.size();
    const int num_cpu = 40;

    set_up_device_pool_expectations(num_cpu, gpu_bitmask);

    NVMLGPUTopo topo(*m_device_pool, num_cpu);
    EXPECT_EQ(num_gpu, topo.num_gpu());
    EXPECT_EQ(num_gpu, topo.num_gpu(GEOPM_DOMAIN_GPU_CHIP));

    std::vector<std::set<int>> cpus_allowed_set = {
        {0,1,2,3,4,5,6,7,8,9},
        {10,11,12,13,14,15,16,17,18,19},
        {20,21,22,23,24,25,26,27,28,29},
        {30,31,32,33,34,35,36,37,38,39}};
    for (int gpu_idx = 0; gpu_idx < num_gpu; ++gpu_idx) {
        ASSERT_THAT(topo.cpu_affinity_ideal(gpu_idx), cpus_allowed_set[gpu_idx]);
        ASSERT_THAT(topo.cpu_affinity_ideal(GEOPM_DOMAIN_GPU_CHIP, gpu_idx), cpus_allowed_set[gpu_idx]);
    }
}

//Test case: All CPUs are associated with one and only one GPUs
TEST_F(NVMLGPUTopoTest, mutex_affinitization_config)
{
    const std::vector<std::set<int>> gpu_bitmask = {
        {0,1,2,3,4,5,6,7,8,9},
        {10,11,12,13,14,15,16,17,18,19},
        {20,21,22,23,24,25,26,27,28,29},
        {30,31,32,33,34,35,36,37,38,39}};
    const int num_gpu = gpu_bitmask.size();
    const int num_cpu = 40;

    set_up_device_pool_expectations(num_cpu, gpu_bitmask);

    NVMLGPUTopo topo(*m_device_pool, num_cpu);
    EXPECT_EQ(num_gpu, topo.num_gpu());

    std::vector<std::set<int>> cpus_allowed_set = {
        {0,1,2,3,4,5,6,7,8,9},
        {10,11,12,13,14,15,16,17,18,19},
        {20,21,22,23,24,25,26,27,28,29},
        {30,31,32,33,34,35,36,37,38,39}};
    for (int gpu_idx = 0; gpu_idx < num_gpu; ++gpu_idx) {
        ASSERT_THAT(topo.cpu_affinity_ideal(gpu_idx), cpus_allowed_set[gpu_idx]);
    }
}

//Test case: All CPUs are associated with all GPUs
TEST_F(NVMLGPUTopoTest, equidistant_affinitization_config)
{
    const int num_gpu = 4;
    const int num_cpu = 40;
    const std::set<int> all_40_cpus =
        {0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32,
            33,34,35,36,37,38,39};
    const std::vector<std::set<int>> gpu_bitmask(num_gpu, all_40_cpus);

    set_up_device_pool_expectations(num_cpu, gpu_bitmask);

    NVMLGPUTopo topo(*m_device_pool, num_cpu);

    EXPECT_EQ(num_gpu, topo.num_gpu());

    std::vector<std::set<int>> cpus_allowed_set = {
        {0,1,2,3,4,5,6,7,8,9},
        {10,11,12,13,14,15,16,17,18,19},
        {20,21,22,23,24,25,26,27,28,29},
        {30,31,32,33,34,35,36,37,38,39}};
    for (int gpu_idx = 0; gpu_idx < num_gpu; ++gpu_idx) {
        ASSERT_THAT(topo.cpu_affinity_ideal(gpu_idx), cpus_allowed_set[gpu_idx]);
    }
}

//Test case:  GPU N+1 associates with all CPUs of GPU N, but not vice versa
TEST_F(NVMLGPUTopoTest, n1_superset_n_affinitization_config)
{
    const std::vector<std::set<int>> gpu_bitmask = {
        {12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32, 33,34,35,36,37,38,39},
        {8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32, 33,34,35,36,
            37,38,39},
        {4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32,33,34,
            35,36,37,38,39},
        {0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32,
            33,34,35,36,37,38,39}};
    const int num_gpu = gpu_bitmask.size();
    const int num_cpu = 40;

    set_up_device_pool_expectations(num_cpu, gpu_bitmask);

    NVMLGPUTopo topo(*m_device_pool, num_cpu);

    EXPECT_EQ(num_gpu, topo.num_gpu());

    std::vector<std::set<int>> cpus_allowed_set = {
        {12,13,14,15,16,17,18,19,20,21},
        {8 ,9 ,10,11,22,23,24,25,26,27},
        {4 ,5 ,6 ,7 ,28,29,30,31,32,33},
        {0 ,1 ,2 ,3 ,34,35,36,37,38,39}};
    for (int gpu_idx = 0; gpu_idx < num_gpu; ++gpu_idx) {
        ASSERT_THAT(topo.cpu_affinity_ideal(gpu_idx), cpus_allowed_set[gpu_idx]);
    }
}

//Test case:  Last GPU has the smallest map, and the entire map will be 'stolen' to cause starvation
TEST_F(NVMLGPUTopoTest, greedbuster_affinitization_config)
{
    const std::vector<std::set<int>> gpu_bitmask = {
        {0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32,
            33,34,35,36,37,38,39},
        {4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32,33,34,
            35,36,37,38,39},
        {8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32, 33,34,35,36,
            37,38,39},
        {0,1,2,3,4,5,6,7,8,9}};
    const int num_cpu = 40;
    
    set_up_device_pool_expectations(num_cpu, gpu_bitmask);

    GEOPM_EXPECT_THROW_MESSAGE(NVMLGPUTopo topo(*m_device_pool, num_cpu), GEOPM_ERROR_INVALID, "Failed to affinitize all valid CPUs to GPUs");
}

//Test case: Different GPU/CPU count, namely an approximation of the HPE Apollo 6500
//           system with 8 GPUs and 28 cores per socket.
TEST_F(NVMLGPUTopoTest, hpe_6500_affinitization_config)
{
    const std::set<int> set1 =
        {0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27};
    const std::set<int> set2 =
        {24,25,26,27,28,29,30,31,32,33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,48,49,50,51,52,
            53,54,55};
    const std::vector<std::set<int>> gpu_bitmask = {
        set1, set1, set1, set1,
        set2, set2, set2, set2};
    const int num_gpu = gpu_bitmask.size();
    const int num_cpu = 56;

    set_up_device_pool_expectations(num_cpu, gpu_bitmask);

    NVMLGPUTopo topo(*m_device_pool, num_cpu);

    EXPECT_EQ(num_gpu, topo.num_gpu());
    std::vector<std::set<int>> cpus_allowed_set = {
        {0 ,1 ,2 ,3 ,4 ,5 ,6 },
        {7 ,8 ,9 ,10,11,12,13},
        {14,15,16,17,18,19,20},
        {21,22,23,24,25,26,27},
        {28,29,30,31,32,33,34},
        {35,36,37,38,39,40,41},
        {42,43,44,45,46,47,48},
        {49,50,51,52,53,54,55}};

    for (int gpu_idx = 0; gpu_idx < num_gpu; ++gpu_idx) {
        ASSERT_THAT(topo.cpu_affinity_ideal(gpu_idx), cpus_allowed_set[gpu_idx]);
    }
}

//Test case: CPU count that is not evenly divisible by the GPU count
TEST_F(NVMLGPUTopoTest, uneven_affinitization_config)
{
    const std::vector<std::set<int>> gpu_bitmask = {
        {0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19},
        {0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19},
        {0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19}};
    const int num_gpu = gpu_bitmask.size();
    const int num_cpu = 20;

    set_up_device_pool_expectations(num_cpu, gpu_bitmask);

    NVMLGPUTopo topo(*m_device_pool, num_cpu);

    EXPECT_EQ(num_gpu, topo.num_gpu());
    std::vector<std::set<int>> cpus_allowed_set = {
        {0 ,1 ,2 ,3 ,4 ,5 ,18,19},
        {6 ,7 ,8 ,9 ,10,11},
        {12,13,14,15,16,17}};

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

    const std::set<int> all_128_cpus =
        {0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32,
            33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,48,49,50,51,52,53,54,55,56,57,58,59,60,
            61,62,63,64,65,66,67,68,69,70,71,72,73,74,75,76,77,78,79,80,81,82,83,84,85,86,87,88,
            89,90,91,92,93,94,95,96,97,98,99,100,101,102,103,104,105,106,107,108,109,110,111,112,
            113,114,115,116,117,118,119,120,121,122,123,124,125,126,127};
    const std::vector<std::set<int>> gpu_bitmask(num_gpu, all_128_cpus);

    set_up_device_pool_expectations(num_cpu, gpu_bitmask);

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
    const std::set<int> set1 =
        {0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,64,65,66,67};
    const std::set<int> set2 =
        {24,25,26,27,28,29,30,31,32,33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,48,49,50,51,52,
            53,54,55,123,124,125,126,127};
    const std::vector<std::set<int>> gpu_bitmask = {
        set1, set1, set1, set1,
        set2, set2, set2, set2};
    const int num_gpu = gpu_bitmask.size();
    const int num_cpu = 128;

    set_up_device_pool_expectations(num_cpu, gpu_bitmask);

    NVMLGPUTopo topo(*m_device_pool, num_cpu);

    EXPECT_EQ(num_gpu, topo.num_gpu());
    std::set<int> cpus_allowed_set[num_gpu];
    cpus_allowed_set[0] = {0 ,1 ,2 ,3 ,4 ,5 ,6 ,7};
    cpus_allowed_set[1] = {8 ,9 ,10,11,12,13,14,15};
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
