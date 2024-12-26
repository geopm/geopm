/*
 * Copyright (c) 2015 - 2024 Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <unistd.h>

#include <fstream>
#include <string>

#include "gtest/gtest.h"
#include "gmock/gmock.h"

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
        void add_device(const std::string& device_address, const std::string& local_cpus);

        std::shared_ptr<MockLevelZero> m_levelzero;
        std::string m_test_dir;
        std::string m_test_devices_dir;
        std::vector<std::string> m_test_device_addresses;
};

void LevelZeroGPUTopoTest::SetUp()
{
    char test_dir_template[] = "/tmp/LevelZeroGPUTopoTest_XXXXXX";
    if (mkdtemp(test_dir_template) == nullptr) {
        throw std::runtime_error(std::string("Could not create a temporary directory at ") +
                                 test_dir_template);
    }
    m_test_dir = test_dir_template;
    m_test_devices_dir = m_test_dir + "/sys_bus_devices";
    if (mkdir(m_test_devices_dir.c_str(), 0755) == -1) {
        rmdir(m_test_dir.c_str());
        throw std::runtime_error("Could not create directory at " + m_test_devices_dir);
    }
    m_levelzero = std::make_shared<MockLevelZero>();
}

void LevelZeroGPUTopoTest::TearDown()
{
    for (const auto& device_address : m_test_device_addresses) {
        std::string device_path = m_test_devices_dir + "/" + device_address;
        std::string cpumask_path = device_path + "/local_cpus";
        unlink(cpumask_path.c_str());
        rmdir(device_path.c_str());
    }
    rmdir(m_test_devices_dir.c_str());
    rmdir(m_test_dir.c_str());
}

void LevelZeroGPUTopoTest::add_device(const std::string& device_address, const std::string& local_cpus)
{
    std::string device_path = m_test_devices_dir + "/" + device_address;
    if (mkdir(device_path.c_str(), 0755) == -1) {
        throw std::runtime_error("Could not create directory at " + device_path);
    }
    geopm::write_file(device_path + "/local_cpus", local_cpus);
    m_test_device_addresses.push_back(device_address);
    ON_CALL(*m_levelzero, pci_dbdf_address(m_test_device_addresses.size() - 1)).WillByDefault(Return(device_address));
}

//Test case: Mock num_gpu = 0 so we hit the appropriate warning and throw on affinitization requests.
TEST_F(LevelZeroGPUTopoTest, no_gpu_config)
{
    const int num_gpu = 0;
    LevelZeroDevicePoolImp m_device_pool(*m_levelzero);

    EXPECT_CALL(*m_levelzero, num_gpu(GEOPM_DOMAIN_GPU)).WillRepeatedly(Return(num_gpu));
    EXPECT_CALL(*m_levelzero, num_gpu(GEOPM_DOMAIN_GPU_CHIP)).WillRepeatedly(Return(num_gpu));

    LevelZeroGPUTopo topo(m_device_pool, m_test_devices_dir);
    EXPECT_EQ(num_gpu, topo.num_gpu());
    EXPECT_EQ(num_gpu, topo.num_gpu(GEOPM_DOMAIN_GPU_CHIP));

    GEOPM_EXPECT_THROW_MESSAGE(topo.cpu_affinity_ideal(num_gpu), GEOPM_ERROR_INVALID, "gpu_idx 0 is out of range");
}

TEST_F(LevelZeroGPUTopoTest, four_forty_config)
{
    const int num_gpu = 4;
    int num_gpu_subdevice = 4;
    add_device("gpu0",    "000003ff");
    add_device("gpu1",    "000ffc00");
    add_device("gpu2",    "3ff00000");
    add_device("gpu3", "ff,c0000000");
    LevelZeroDevicePoolImp m_device_pool(*m_levelzero);

    EXPECT_CALL(*m_levelzero, num_gpu(GEOPM_DOMAIN_GPU)).WillRepeatedly(Return(num_gpu));
    EXPECT_CALL(*m_levelzero, num_gpu(GEOPM_DOMAIN_GPU_CHIP)).WillRepeatedly(Return(num_gpu_subdevice));

    LevelZeroGPUTopo topo(m_device_pool, m_test_devices_dir);
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
    EXPECT_CALL(*m_levelzero, num_gpu(GEOPM_DOMAIN_GPU)).WillRepeatedly(Return(num_gpu));
    EXPECT_CALL(*m_levelzero, num_gpu(GEOPM_DOMAIN_GPU_CHIP)).WillRepeatedly(Return(num_gpu_subdevice));

    LevelZeroGPUTopo topo_sub(m_device_pool, m_test_devices_dir);
    EXPECT_EQ(num_gpu, topo_sub.num_gpu());
    EXPECT_EQ(num_gpu_subdevice, topo_sub.num_gpu(GEOPM_DOMAIN_GPU_CHIP));

    std::vector<std::set<int>> cpus_allowed_set_subdevice = {
        {0,1,2,3,4,5,6,7,8,9},
        {0,1,2,3,4,5,6,7,8,9},
        {10,11,12,13,14,15,16,17,18,19},
        {10,11,12,13,14,15,16,17,18,19},
        {20,21,22,23,24,25,26,27,28,29},
        {20,21,22,23,24,25,26,27,28,29},
        {30,31,32,33,34,35,36,37,38,39},
        {30,31,32,33,34,35,36,37,38,39},
    };

    for (int gpu_idx = 0; gpu_idx < num_gpu; ++gpu_idx) {
        ASSERT_THAT(topo_sub.cpu_affinity_ideal(gpu_idx), cpus_allowed_set[gpu_idx]);
    }
    for (int gpu_idx = 0; gpu_idx < static_cast<int>(cpus_allowed_set_subdevice.size()); ++gpu_idx) {
        ASSERT_THAT(topo_sub.cpu_affinity_ideal(GEOPM_DOMAIN_GPU_CHIP, gpu_idx), cpus_allowed_set_subdevice[gpu_idx]);
    }
}

//Test case: Different GPU/CPU count, with 8 GPUs and 28 cores per socket.
TEST_F(LevelZeroGPUTopoTest, eight_fiftysix_affinitization_config)
{
    const int num_gpu = 8;
    const int num_gpu_subdevice = 8;
    add_device("gpu0",        "0000007f");
    add_device("gpu1",        "00003f80");
    add_device("gpu2",        "001fc000");
    add_device("gpu3",        "0fe00000");
    add_device("gpu4",      "7,f0000000");
    add_device("gpu5",    "3f8,00000000");
    add_device("gpu6",  "1fc00,00000000");
    add_device("gpu7", "fe0000,00000000");
    LevelZeroDevicePoolImp m_device_pool(*m_levelzero);

    EXPECT_CALL(*m_levelzero, num_gpu(GEOPM_DOMAIN_GPU)).WillRepeatedly(Return(num_gpu));
    EXPECT_CALL(*m_levelzero, num_gpu(GEOPM_DOMAIN_GPU_CHIP)).WillRepeatedly(Return(num_gpu_subdevice));

    LevelZeroGPUTopo topo(m_device_pool, m_test_devices_dir);

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
    add_device("gpu0",        "0004003f");
    add_device("gpu1",        "00080fc0");
    add_device("gpu2",        "0003f000");
    LevelZeroDevicePoolImp m_device_pool(*m_levelzero);

    EXPECT_CALL(*m_levelzero, num_gpu(GEOPM_DOMAIN_GPU)).WillRepeatedly(Return(num_gpu));
    EXPECT_CALL(*m_levelzero, num_gpu(GEOPM_DOMAIN_GPU_CHIP)).WillRepeatedly(Return(num_gpu_subdevice));

    LevelZeroGPUTopo topo(m_device_pool, m_test_devices_dir);

    EXPECT_EQ(num_gpu, topo.num_gpu());
    std::set<int> cpus_allowed_set[num_gpu];
    cpus_allowed_set[0] = {0 ,1 ,2 ,3 ,4 ,5 ,18};
    cpus_allowed_set[1] = {6 ,7 ,8 ,9 ,10,11,19};
    cpus_allowed_set[2] = {12,13,14,15,16,17};

    for (int gpu_idx = 0; gpu_idx < num_gpu; ++gpu_idx) {
        ASSERT_THAT(topo.cpu_affinity_ideal(gpu_idx), cpus_allowed_set[gpu_idx]);
    }

    std::set<int> cpus_allowed_set_subdevice[num_gpu_subdevice];
    cpus_allowed_set_subdevice[0] = {0 ,1 ,2 ,3 ,4 ,5 ,18};
    cpus_allowed_set_subdevice[1] = {0 ,1 ,2 ,3 ,4 ,5 ,18};

    cpus_allowed_set_subdevice[2] = {6 ,7 ,8 ,9 ,10,11,19};
    cpus_allowed_set_subdevice[3] = {6 ,7 ,8 ,9 ,10,11,19};

    cpus_allowed_set_subdevice[4] = {12,13,14,15,16,17};
    cpus_allowed_set_subdevice[5] = {12,13,14,15,16,17};

    for (int sub_idx = 0; sub_idx < num_gpu; ++sub_idx) {
        ASSERT_THAT(topo.cpu_affinity_ideal(GEOPM_DOMAIN_GPU_CHIP, sub_idx), cpus_allowed_set_subdevice[sub_idx]);
    }
}

//Test case: High Core count, theoretical system to test large CPU SETS.
TEST_F(LevelZeroGPUTopoTest, high_cpu_count_config)
{
    LevelZeroDevicePoolImp m_device_pool(*m_levelzero);
    add_device("gpu0", "80000000,00000000,00000000,00000000");

    EXPECT_CALL(*m_levelzero, num_gpu(GEOPM_DOMAIN_GPU)).WillRepeatedly(Return(1));
    EXPECT_CALL(*m_levelzero, num_gpu(GEOPM_DOMAIN_GPU_CHIP)).WillRepeatedly(Return(1));

    LevelZeroGPUTopo topo(m_device_pool, m_test_devices_dir);

    EXPECT_EQ(1, topo.num_gpu());
    ASSERT_EQ(topo.cpu_affinity_ideal(0), std::set<int>{127});
    ASSERT_EQ(topo.cpu_affinity_ideal(GEOPM_DOMAIN_GPU_CHIP, 0), std::set<int>{127});
}
