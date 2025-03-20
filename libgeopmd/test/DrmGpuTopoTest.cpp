/*
 * Copyright (c) 2015 - 2025 Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "DrmGpuTopo.hpp"

#include <set>
#include <algorithm>
#include "gmock/gmock.h"
#include "gtest/gtest.h"

#include "DrmFakeDirManager.hpp"
#include "geopm/Exception.hpp"
#include "geopm_topo.h"

using geopm::DrmGpuTopo;
using ::testing::UnorderedElementsAre;
using ::testing::EndsWith;

class DrmGpuTopoTest : public :: testing :: Test
{
    protected:
        void SetUp();
        void TearDown();
        std::unique_ptr<DrmFakeDirManager> m_dir_manager;
};

void DrmGpuTopoTest::SetUp()
{
    m_dir_manager = std::make_unique<DrmFakeDirManager>("/tmp/DrmsysfsDriverTest_XXXXXX");
}

void DrmGpuTopoTest::TearDown()
{
}

TEST_F(DrmGpuTopoTest, num_gpu)
{
    EXPECT_THROW(DrmGpuTopo(m_dir_manager->get_driver_dir()), geopm::Exception);

    m_dir_manager->create_card(0, 0);
    {
        DrmGpuTopo topo(m_dir_manager->get_driver_dir());
        EXPECT_EQ(1, topo.num_gpu());
        EXPECT_EQ(0, topo.num_gpu(GEOPM_DOMAIN_GPU_CHIP));
    }

    m_dir_manager->create_card(99, 0);
    {
        DrmGpuTopo topo(m_dir_manager->get_driver_dir());
        EXPECT_EQ(2, topo.num_gpu());
        EXPECT_EQ(0, topo.num_gpu(GEOPM_DOMAIN_GPU_CHIP));
    }

    m_dir_manager->create_tile_in_card(0, 0);
    m_dir_manager->create_tile_in_card(99, 0);
    {
        DrmGpuTopo topo(m_dir_manager->get_driver_dir());
        EXPECT_EQ(2, topo.num_gpu());
        EXPECT_EQ(2, topo.num_gpu(GEOPM_DOMAIN_GPU_CHIP));
    }
}

TEST_F(DrmGpuTopoTest, unbalanced_gpu_chips)
{
    m_dir_manager->create_card(0, 0);
    m_dir_manager->create_tile_in_card(0, 0);
    m_dir_manager->create_tile_in_card(0, 1);
    m_dir_manager->create_card(1, 0);
    m_dir_manager->create_tile_in_card(1, 0);
    EXPECT_THROW(DrmGpuTopo(m_dir_manager->get_driver_dir()), geopm::Exception);
}

TEST_F(DrmGpuTopoTest, driver_name)
{
    m_dir_manager->create_card(0, 0);
    DrmGpuTopo topo(m_dir_manager->get_driver_dir());
    EXPECT_EQ("test_driver", topo.driver_name());
}

TEST_F(DrmGpuTopoTest, gpu_affinity_ideal)
{
    m_dir_manager->create_card(0, 0);
    m_dir_manager->create_card(1, 0);
    m_dir_manager->create_card(2, 0);
    m_dir_manager->create_card(3, 1);
    m_dir_manager->create_card(4, 1);
    m_dir_manager->create_card(5, 1);
    m_dir_manager->create_tile_in_card(0, 0);
    m_dir_manager->create_tile_in_card(0, 1);
    m_dir_manager->create_tile_in_card(1, 0);
    m_dir_manager->create_tile_in_card(1, 1);
    m_dir_manager->create_tile_in_card(2, 0);
    m_dir_manager->create_tile_in_card(2, 1);
    m_dir_manager->create_tile_in_card(3, 0);
    m_dir_manager->create_tile_in_card(3, 1);
    m_dir_manager->create_tile_in_card(4, 0);
    m_dir_manager->create_tile_in_card(4, 1);
    m_dir_manager->create_tile_in_card(5, 0);
    m_dir_manager->create_tile_in_card(5, 1);
    m_dir_manager->write_local_cpus(0, "0000,00000000,0fffffff,ffffff00,00000000,000fffff,ffffffff");
    m_dir_manager->write_local_cpus(1, "0000,00000000,0fffffff,ffffff00,00000000,000fffff,ffffffff");
    m_dir_manager->write_local_cpus(2, "0000,00000000,0fffffff,ffffff00,00000000,000fffff,ffffffff");
    m_dir_manager->write_local_cpus(3, "ffff,ffffffff,f0000000,000000ff,ffffffff,fff00000,00000000");
    m_dir_manager->write_local_cpus(4, "ffff,ffffffff,f0000000,000000ff,ffffffff,fff00000,00000000");
    m_dir_manager->write_local_cpus(5, "ffff,ffffffff,f0000000,000000ff,ffffffff,fff00000,00000000");

    DrmGpuTopo topo(m_dir_manager->get_driver_dir());

    std::set<int> all_cpus;
    for (int chip_idx = 0; chip_idx < 12; ++chip_idx) {
        auto aff = topo.cpu_affinity_ideal(GEOPM_DOMAIN_GPU_CHIP, chip_idx);
        std::vector<int> chip_cpu_overlap;
        std::set_intersection(all_cpus.begin(), all_cpus.end(), aff.begin(), aff.end(),
                              std::back_inserter(chip_cpu_overlap));
        // Assert that no two chips share CPUs
        EXPECT_TRUE(chip_cpu_overlap.empty());
        all_cpus.insert(aff.begin(), aff.end());
    }
    // Assert that all 208 CPUs are assigned to some GPU Chip
    ASSERT_EQ(208ULL, all_cpus.size());
    EXPECT_EQ(0, *all_cpus.begin());
    EXPECT_EQ(207, *all_cpus.rbegin());

    // Check chip hierarchy
    for (int gpu_idx = 0; gpu_idx < 6; ++gpu_idx) {
        auto aff_chip_0 = topo.cpu_affinity_ideal(GEOPM_DOMAIN_GPU_CHIP, 2 * gpu_idx);
        auto aff_chip_1 = topo.cpu_affinity_ideal(GEOPM_DOMAIN_GPU_CHIP, 2 * gpu_idx + 1);
        auto aff_gpu = topo.cpu_affinity_ideal(GEOPM_DOMAIN_GPU, gpu_idx);
        std::set<int> aff_chip_comb(aff_chip_0.begin(), aff_chip_0.end());
        aff_chip_comb.insert(aff_chip_1.begin(), aff_chip_1.end());
        EXPECT_EQ(aff_gpu, aff_chip_comb);
    }

}

TEST_F(DrmGpuTopoTest, gpu_affinity_invalid_index)
{
    m_dir_manager->create_card(0, 0);
    m_dir_manager->create_tile_in_card(0, 0);

    m_dir_manager->write_local_cpus(0, "00000001");

    DrmGpuTopo topo(m_dir_manager->get_driver_dir());

    // Verify invalid GPU index
    EXPECT_THROW(topo.cpu_affinity_ideal(1), geopm::Exception);

    // Verify invalid GPU chip index
    EXPECT_THROW(topo.cpu_affinity_ideal(GEOPM_DOMAIN_GPU_CHIP, 1), geopm::Exception);
}
