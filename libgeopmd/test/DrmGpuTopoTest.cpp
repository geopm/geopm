/*
 * Copyright (c) 2015 - 2025 Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "DrmGpuTopo.hpp"

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

    m_dir_manager->create_card(0);
    {
        DrmGpuTopo topo(m_dir_manager->get_driver_dir());
        EXPECT_EQ(1, topo.num_gpu());
        EXPECT_EQ(0, topo.num_gpu(GEOPM_DOMAIN_GPU_CHIP));
    }

    m_dir_manager->create_card(99);
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
    m_dir_manager->create_card(0);
    m_dir_manager->create_tile_in_card(0, 0);
    m_dir_manager->create_tile_in_card(0, 1);
    m_dir_manager->create_card(1);
    m_dir_manager->create_tile_in_card(1, 0);
    EXPECT_THROW(DrmGpuTopo(m_dir_manager->get_driver_dir()), geopm::Exception);
}

TEST_F(DrmGpuTopoTest, driver_name)
{
    m_dir_manager->create_card(0);
    DrmGpuTopo topo(m_dir_manager->get_driver_dir());
    EXPECT_EQ("test_driver", topo.driver_name());
}

TEST_F(DrmGpuTopoTest, gpu_affinity_ideal)
{
    m_dir_manager->create_card(0);
    m_dir_manager->create_card(1);
    m_dir_manager->create_tile_in_card(0, 0);
    m_dir_manager->create_tile_in_card(0, 1);
    m_dir_manager->create_tile_in_card(1, 0);
    m_dir_manager->create_tile_in_card(1, 1);

    m_dir_manager->write_local_cpus(0, "00000000,00000003");
    m_dir_manager->write_local_cpus(1, "00000000,0000000c");

    DrmGpuTopo topo(m_dir_manager->get_driver_dir());

    // Verify CPU affinity assignments for GPUs
    EXPECT_THAT(topo.cpu_affinity_ideal(0), UnorderedElementsAre(0, 1));
    EXPECT_THAT(topo.cpu_affinity_ideal(1), UnorderedElementsAre(2, 3));

    // Verify CPU affinity assignments for GPU chips
    EXPECT_THAT(topo.cpu_affinity_ideal(GEOPM_DOMAIN_GPU_CHIP, 0), UnorderedElementsAre(0));
    EXPECT_THAT(topo.cpu_affinity_ideal(GEOPM_DOMAIN_GPU_CHIP, 1), UnorderedElementsAre(1));
    EXPECT_THAT(topo.cpu_affinity_ideal(GEOPM_DOMAIN_GPU_CHIP, 2), UnorderedElementsAre(2));
    EXPECT_THAT(topo.cpu_affinity_ideal(GEOPM_DOMAIN_GPU_CHIP, 3), UnorderedElementsAre(3));
}

TEST_F(DrmGpuTopoTest, gpu_affinity_invalid_index)
{
    m_dir_manager->create_card(0);
    m_dir_manager->create_tile_in_card(0, 0);

    m_dir_manager->write_local_cpus(0, "00000001");

    DrmGpuTopo topo(m_dir_manager->get_driver_dir());

    // Verify invalid GPU index
    EXPECT_THROW(topo.cpu_affinity_ideal(1), geopm::Exception);

    // Verify invalid GPU chip index
    EXPECT_THROW(topo.cpu_affinity_ideal(GEOPM_DOMAIN_GPU_CHIP, 1), geopm::Exception);
}
