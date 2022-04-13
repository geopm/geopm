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
#include "NVMLAcceleratorTopo.hpp"
#include "geopm_test.hpp"
#include "geopm_topo.h"

using geopm::NVMLAcceleratorTopo;
using geopm::Exception;
using testing::Return;

class NVMLAcceleratorTopoTest : public :: testing :: Test
{
    protected:
        void SetUp();
        void TearDown();

        std::shared_ptr<MockNVMLDevicePool> m_device_pool;
};

void NVMLAcceleratorTopoTest::SetUp()
{
    m_device_pool = std::make_shared<MockNVMLDevicePool>();
}

void NVMLAcceleratorTopoTest::TearDown()
{
}

//Test case: Mock num_accelerator = 0 so we hit the appropriate warning and throw on affinitization requests.
TEST_F(NVMLAcceleratorTopoTest, no_gpu_config)
{
    const int num_accelerator = 0;
    const int num_cpu = 40;

    EXPECT_CALL(*m_device_pool, num_accelerator()).WillOnce(Return(num_accelerator));

    NVMLAcceleratorTopo topo(*m_device_pool, num_cpu);
    EXPECT_EQ(num_accelerator, topo.num_accelerator());

    GEOPM_EXPECT_THROW_MESSAGE(topo.cpu_affinity_ideal(num_accelerator), GEOPM_ERROR_INVALID, "accel_idx 0 is out of range");
    GEOPM_EXPECT_THROW_MESSAGE(topo.cpu_affinity_ideal(GEOPM_DOMAIN_BOARD_ACCELERATOR, num_accelerator), GEOPM_ERROR_INVALID, "accel_idx 0 is out of range");
    GEOPM_EXPECT_THROW_MESSAGE(topo.cpu_affinity_ideal(GEOPM_DOMAIN_BOARD_ACCELERATOR_CHIP, num_accelerator), GEOPM_ERROR_INVALID, "accel_idx 0 is out of range");
}

//Test case: The HPE SX40 default system configuration
TEST_F(NVMLAcceleratorTopoTest, hpe_sx40_default_config)
{
    const int num_accelerator = 4;
    const int num_cpu = 40;

    unsigned long accel_bitmask[num_accelerator];
    accel_bitmask[0] = 0x00000fffff;
    accel_bitmask[1] = 0x00000fffff;
    accel_bitmask[2] = 0xfffff00000;
    accel_bitmask[3] = 0xfffff00000;

    for (int accel_idx = 0; accel_idx < num_accelerator; ++accel_idx) {
        EXPECT_CALL(*m_device_pool, cpu_affinity_ideal_mask(accel_idx)).WillOnce(Return((cpu_set_t *) &accel_bitmask[accel_idx]));
    }

    EXPECT_CALL(*m_device_pool, num_accelerator()).WillOnce(Return(num_accelerator));

    NVMLAcceleratorTopo topo(*m_device_pool, num_cpu);
    EXPECT_EQ(num_accelerator, topo.num_accelerator());
    EXPECT_EQ(num_accelerator, topo.num_accelerator(GEOPM_DOMAIN_BOARD_ACCELERATOR_CHIP));
    std::set<int> cpus_allowed_set[num_accelerator];
    cpus_allowed_set[0] = {0,1,2,3,4,5,6,7,8,9};
    cpus_allowed_set[1] = {10,11,12,13,14,15,16,17,18,19};
    cpus_allowed_set[2] = {20,21,22,23,24,25,26,27,28,29};
    cpus_allowed_set[3] = {30,31,32,33,34,35,36,37,38,39};

    for (int accel_idx = 0; accel_idx < num_accelerator; ++accel_idx) {
        ASSERT_THAT(topo.cpu_affinity_ideal(accel_idx), cpus_allowed_set[accel_idx]);
        ASSERT_THAT(topo.cpu_affinity_ideal(GEOPM_DOMAIN_BOARD_ACCELERATOR_CHIP, accel_idx), cpus_allowed_set[accel_idx]);
    }
}

//Test case: All cpus are associated with one and only one GPUs
TEST_F(NVMLAcceleratorTopoTest, mutex_affinitization_config)
{
    const int num_accelerator = 4;
    const int num_cpu = 40;

    unsigned long accel_bitmask[num_accelerator];
    accel_bitmask[0] = 0x00000003ff;
    accel_bitmask[1] = 0x00000ffc00;
    accel_bitmask[2] = 0x003ff00000;
    accel_bitmask[3] = 0xffc0000000;

    for (int accel_idx = 0; accel_idx < num_accelerator; ++accel_idx) {
        EXPECT_CALL(*m_device_pool, cpu_affinity_ideal_mask(accel_idx)).WillOnce(Return((cpu_set_t *)&accel_bitmask[accel_idx]));
    }
    EXPECT_CALL(*m_device_pool, num_accelerator()).WillOnce(Return(num_accelerator));

    NVMLAcceleratorTopo topo(*m_device_pool, num_cpu);
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

//Test case: All cpus are associated with all GPUs
TEST_F(NVMLAcceleratorTopoTest, equidistant_affinitization_config)
{
    const int num_accelerator = 4;
    const int num_cpu = 40;

    unsigned long accel_bitmask[num_accelerator];
    accel_bitmask[0] = 0xffffffffff;
    accel_bitmask[1] = 0xffffffffff;
    accel_bitmask[2] = 0xffffffffff;
    accel_bitmask[3] = 0xffffffffff;

    for (int accel_idx = 0; accel_idx < num_accelerator; ++accel_idx) {
        EXPECT_CALL(*m_device_pool, cpu_affinity_ideal_mask(accel_idx)).WillOnce(Return((cpu_set_t *)&accel_bitmask[accel_idx]));
    }
    EXPECT_CALL(*m_device_pool, num_accelerator()).WillOnce(Return(num_accelerator));

    NVMLAcceleratorTopo topo(*m_device_pool, num_cpu);

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

//Test case:  Accel N+1 associates with all CPUs of Accel N, but not vice versa
TEST_F(NVMLAcceleratorTopoTest, n1_superset_n_affinitization_config)
{
    const int num_accelerator = 4;
    const int num_cpu = 40;

    unsigned long accel_bitmask[num_accelerator];
    accel_bitmask[0] = 0xfffffff000;
    accel_bitmask[1] = 0xffffffff00;
    accel_bitmask[2] = 0xfffffffff0;
    accel_bitmask[3] = 0xffffffffff;

    for (int accel_idx = 0; accel_idx < num_accelerator; ++accel_idx) {
        EXPECT_CALL(*m_device_pool, cpu_affinity_ideal_mask(accel_idx)).WillOnce(Return((cpu_set_t *)&accel_bitmask[accel_idx]));
    }
    EXPECT_CALL(*m_device_pool, num_accelerator()).WillOnce(Return(num_accelerator));

    NVMLAcceleratorTopo topo(*m_device_pool, num_cpu);

    EXPECT_EQ(num_accelerator, topo.num_accelerator());
    std::set<int> cpus_allowed_set[num_accelerator];
    cpus_allowed_set[0] = {12,13,14,15,16,17,18,19,20,21};
    cpus_allowed_set[1] = {8 ,9 ,10,11,22,23,24,25,26,27};
    cpus_allowed_set[2] = {4 ,5 ,6 ,7 ,28,29,30,31,32,33};
    cpus_allowed_set[3] = {0 ,1 ,2 ,3 ,34,35,36,37,38,39};

    for (int accel_idx = 0; accel_idx < num_accelerator; ++accel_idx) {
        ASSERT_THAT(topo.cpu_affinity_ideal(accel_idx), cpus_allowed_set[accel_idx]);
    }
}

//Test case:  Last accelerator has the smallest map, and the entire map will be 'stolen' to cause starvation
TEST_F(NVMLAcceleratorTopoTest, greedbuster_affinitization_config)
{
    const int num_accelerator = 4;
    const int num_cpu = 40;

    unsigned long accel_bitmask[num_accelerator];
    accel_bitmask[0] = 0xffffffffff;
    accel_bitmask[1] = 0xfffffffff0;
    accel_bitmask[2] = 0x0fffffff00;
    accel_bitmask[3] = 0x00000003ff;

    for (int accel_idx = 0; accel_idx < num_accelerator; ++accel_idx) {
        EXPECT_CALL(*m_device_pool, cpu_affinity_ideal_mask(accel_idx)).WillOnce(Return((cpu_set_t *)&accel_bitmask[accel_idx]));
    }
    EXPECT_CALL(*m_device_pool, num_accelerator()).WillOnce(Return(num_accelerator));

    GEOPM_EXPECT_THROW_MESSAGE(NVMLAcceleratorTopo topo(*m_device_pool, num_cpu), GEOPM_ERROR_INVALID, "Failed to affinitize all valid CPUs to Accelerators");
}

//Test case: Different GPU/CPU count, namely an approximation of the HPE Apollo 6500
//           system with 8 GPUs and 28 cores per socket.
TEST_F(NVMLAcceleratorTopoTest, hpe_6500_affinitization_config)
{
    const int num_accelerator = 8;
    const int num_cpu = 56;

    unsigned long accel_bitmask[num_accelerator];
    accel_bitmask[0] = 0x0000000fffffff;
    accel_bitmask[1] = 0x0000000fffffff;
    accel_bitmask[2] = 0x0000000fffffff;
    accel_bitmask[3] = 0x0000000fffffff;
    accel_bitmask[4] = 0xffffffff000000;
    accel_bitmask[5] = 0xffffffff000000;
    accel_bitmask[6] = 0xffffffff000000;
    accel_bitmask[7] = 0xffffffff000000;

    for (int accel_idx = 0; accel_idx < num_accelerator; ++accel_idx) {
        EXPECT_CALL(*m_device_pool, cpu_affinity_ideal_mask(accel_idx)).WillOnce(Return((cpu_set_t *)&accel_bitmask[accel_idx]));
    }
    EXPECT_CALL(*m_device_pool, num_accelerator()).WillOnce(Return(num_accelerator));

    NVMLAcceleratorTopo topo(*m_device_pool, num_cpu);

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
TEST_F(NVMLAcceleratorTopoTest, uneven_affinitization_config)
{
    const int num_accelerator = 3;
    const int num_cpu =20;

    unsigned long accel_bitmask[num_accelerator];
    accel_bitmask[0] = 0xfffff;
    accel_bitmask[1] = 0xfffff;
    accel_bitmask[2] = 0xfffff;

    for (int accel_idx = 0; accel_idx < num_accelerator; ++accel_idx) {
        EXPECT_CALL(*m_device_pool, cpu_affinity_ideal_mask(accel_idx)).WillOnce(Return((cpu_set_t *)&accel_bitmask[accel_idx]));
    }
    EXPECT_CALL(*m_device_pool, num_accelerator()).WillOnce(Return(num_accelerator));

    NVMLAcceleratorTopo topo(*m_device_pool, num_cpu);

    EXPECT_EQ(num_accelerator, topo.num_accelerator());
    std::set<int> cpus_allowed_set[num_accelerator];
    cpus_allowed_set[0] = {0 ,1 ,2 ,3 ,4 ,5 ,18,19};
    cpus_allowed_set[1] = {6 ,7 ,8 ,9 ,10,11};
    cpus_allowed_set[2] = {12,13,14,15,16,17};

    for (int accel_idx = 0; accel_idx < num_accelerator; ++accel_idx) {
        ASSERT_THAT(topo.cpu_affinity_ideal(accel_idx), cpus_allowed_set[accel_idx]);
    }
}

//Test case: High Core count, theoretical system to test large CPU SETS.
//           This represents a system with 64 cores and 8 GPUs
TEST_F(NVMLAcceleratorTopoTest, high_cpu_count_config)
{
    const int num_accelerator = 8;
    const int num_cpu = 128;

    unsigned long accel_bitmask[num_accelerator][2];
    accel_bitmask[0][0] = 0xffffffffffffffff;
    accel_bitmask[0][1] = 0xffffffffffffffff;
    accel_bitmask[1][0] = 0xffffffffffffffff;
    accel_bitmask[1][1] = 0xffffffffffffffff;
    accel_bitmask[2][0] = 0xffffffffffffffff;
    accel_bitmask[2][1] = 0xffffffffffffffff;
    accel_bitmask[3][0] = 0xffffffffffffffff;
    accel_bitmask[3][1] = 0xffffffffffffffff;
    accel_bitmask[4][0] = 0xffffffffffffffff;
    accel_bitmask[4][1] = 0xffffffffffffffff;
    accel_bitmask[5][0] = 0xffffffffffffffff;
    accel_bitmask[5][1] = 0xffffffffffffffff;
    accel_bitmask[6][0] = 0xffffffffffffffff;
    accel_bitmask[6][1] = 0xffffffffffffffff;
    accel_bitmask[7][0] = 0xffffffffffffffff;
    accel_bitmask[7][1] = 0xffffffffffffffff;

    for (int accel_idx = 0; accel_idx < num_accelerator; ++accel_idx) {
        EXPECT_CALL(*m_device_pool, cpu_affinity_ideal_mask(accel_idx)).WillOnce(Return((cpu_set_t *)&accel_bitmask[accel_idx]));
    }
    EXPECT_CALL(*m_device_pool, num_accelerator()).WillOnce(Return(num_accelerator));

    NVMLAcceleratorTopo topo(*m_device_pool, num_cpu);

    EXPECT_EQ(num_accelerator, topo.num_accelerator());
    std::set<int> cpus_allowed_set[num_accelerator];

    for (int accel_idx = 0; accel_idx < num_accelerator; ++accel_idx) {
        for (int cpu_idx = 0; cpu_idx < num_cpu/num_accelerator; ++cpu_idx) {
            cpus_allowed_set[accel_idx].insert(cpu_idx+(accel_idx*16));
        }

        ASSERT_THAT(topo.cpu_affinity_ideal(accel_idx), cpus_allowed_set[accel_idx]);
    }
}

//Test case: High Core count system with sparse affinitization, to test uneven distribution with gaps.
TEST_F(NVMLAcceleratorTopoTest, high_cpu_count_gaps_config)
{
    const int num_accelerator = 8;
    const int num_cpu = 128;

    unsigned long accel_bitmask[num_accelerator][2];
    accel_bitmask[0][0] = 0x000000000fffffff;
    accel_bitmask[0][1] = 0x000000000000000f;
    accel_bitmask[1][0] = 0x000000000fffffff;
    accel_bitmask[1][1] = 0x000000000000000f;
    accel_bitmask[2][0] = 0x000000000fffffff;
    accel_bitmask[2][1] = 0x000000000000000f;
    accel_bitmask[3][0] = 0x000000000fffffff;
    accel_bitmask[3][1] = 0x000000000000000f;
    accel_bitmask[4][0] = 0x00ffffffff000000;
    accel_bitmask[4][1] = 0xf800000000000000;
    accel_bitmask[5][0] = 0x00ffffffff000000;
    accel_bitmask[5][1] = 0xf800000000000000;
    accel_bitmask[6][0] = 0x00ffffffff000000;
    accel_bitmask[6][1] = 0xf800000000000000;
    accel_bitmask[7][0] = 0x00ffffffff000000;
    accel_bitmask[7][1] = 0xf800000000000000;

    for (int accel_idx = 0; accel_idx < num_accelerator; ++accel_idx) {
        EXPECT_CALL(*m_device_pool, cpu_affinity_ideal_mask(accel_idx)).WillOnce(Return((cpu_set_t *)&accel_bitmask[accel_idx]));
    }
    EXPECT_CALL(*m_device_pool, num_accelerator()).WillOnce(Return(num_accelerator));

    NVMLAcceleratorTopo topo(*m_device_pool, num_cpu);

    EXPECT_EQ(num_accelerator, topo.num_accelerator());
    std::set<int> cpus_allowed_set[num_accelerator];
    cpus_allowed_set[0] = {0 ,1 ,2 ,3 ,4 ,5 ,6 ,7};
    cpus_allowed_set[1] = {8 ,9 ,10,11,12,13,14,15};
    cpus_allowed_set[2] = {16,17,18,19,20,21,22,23};
    cpus_allowed_set[3] = {24,25,26,27,64,65,66,67};
    cpus_allowed_set[4] = {28,29,30,31,32,33,34,35,127};
    cpus_allowed_set[5] = {36,37,38,39,40,41,42,43};
    cpus_allowed_set[6] = {44,45,46,47,48,49,50,51};
    cpus_allowed_set[7] = {52,53,54,55,123,124,125,126};

    for (int accel_idx = 0; accel_idx < num_accelerator; ++accel_idx) {
        ASSERT_THAT(topo.cpu_affinity_ideal(accel_idx), cpus_allowed_set[accel_idx]);
    }
}
