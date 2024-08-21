/*
 * Copyright (c) 2015 - 2024 Intel Corporation
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
    {
        DrmGpuTopo topo(m_dir_manager->get_driver_dir());
        EXPECT_EQ(0, topo.num_gpu());
        EXPECT_EQ(0, topo.num_gpu(GEOPM_DOMAIN_GPU_CHIP));
    }

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

TEST_F(DrmGpuTopoTest, cpu_masks)
{
    m_dir_manager->create_card(0);
    m_dir_manager->write_local_cpus(0, "00000001");
    {
        DrmGpuTopo topo(m_dir_manager->get_driver_dir());
        EXPECT_THAT(topo.cpu_affinity_ideal(0), UnorderedElementsAre(0));
    }

    m_dir_manager->write_local_cpus(0, "00000000");
    {
        DrmGpuTopo topo(m_dir_manager->get_driver_dir());
        EXPECT_THAT(topo.cpu_affinity_ideal(0), UnorderedElementsAre());
    }

    m_dir_manager->write_local_cpus(0, "800000f0");
    {
        DrmGpuTopo topo(m_dir_manager->get_driver_dir());
        EXPECT_THAT(topo.cpu_affinity_ideal(0), UnorderedElementsAre(4, 5, 6, 7, 31));
    }

    m_dir_manager->write_local_cpus(0, "00000001,00000002");
    {
        DrmGpuTopo topo(m_dir_manager->get_driver_dir());
        EXPECT_THAT(topo.cpu_affinity_ideal(0), UnorderedElementsAre(1, 32));
    }

    m_dir_manager->write_local_cpus(0, "1,00000002");
    {
        DrmGpuTopo topo(m_dir_manager->get_driver_dir());
        EXPECT_THAT(topo.cpu_affinity_ideal(0), UnorderedElementsAre(1, 32));
    }

    m_dir_manager->write_local_cpus(0, "100000002");
    {
        EXPECT_THROW(DrmGpuTopo(m_dir_manager->get_driver_dir()), geopm::Exception);
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

TEST_F(DrmGpuTopoTest, non_zero_card)
{
    // Scenario: GEOPM GPU 0 is not DRM card 0
    m_dir_manager->create_card(5);
    m_dir_manager->create_card(7);
    m_dir_manager->create_tile_in_card(5, 3);
    m_dir_manager->create_tile_in_card(5, 9);
    m_dir_manager->create_tile_in_card(7, 123);
    m_dir_manager->create_tile_in_card(7, 456);

    DrmGpuTopo topo(m_dir_manager->get_driver_dir());
    EXPECT_EQ(2, topo.num_gpu());
    EXPECT_EQ(4, topo.num_gpu(GEOPM_DOMAIN_GPU_CHIP));
    EXPECT_THAT(topo.card_path(0), EndsWith("/card5"));
    EXPECT_THAT(topo.card_path(1), EndsWith("/card7"));
    EXPECT_THROW(topo.card_path(2), geopm::Exception);
    EXPECT_THAT(topo.gt_path(0), EndsWith("/gt3"));
    EXPECT_THAT(topo.gt_path(1), EndsWith("/gt9"));
    EXPECT_THAT(topo.gt_path(2), EndsWith("/gt123"));
    EXPECT_THAT(topo.gt_path(3), EndsWith("/gt456"));
    EXPECT_THROW(topo.gt_path(4), geopm::Exception);
    EXPECT_THAT(topo.cpu_affinity_ideal(0), UnorderedElementsAre(0));
    EXPECT_THAT(topo.cpu_affinity_ideal(1), UnorderedElementsAre(0));
    EXPECT_THAT(topo.cpu_affinity_ideal(GEOPM_DOMAIN_GPU_CHIP, 0), UnorderedElementsAre(0));
    EXPECT_THAT(topo.cpu_affinity_ideal(GEOPM_DOMAIN_GPU_CHIP, 1), UnorderedElementsAre(0));
    EXPECT_THAT(topo.cpu_affinity_ideal(GEOPM_DOMAIN_GPU_CHIP, 2), UnorderedElementsAre(0));
    EXPECT_THAT(topo.cpu_affinity_ideal(GEOPM_DOMAIN_GPU_CHIP, 3), UnorderedElementsAre(0));
    EXPECT_THROW(topo.cpu_affinity_ideal(2), geopm::Exception);
    EXPECT_THROW(topo.cpu_affinity_ideal(GEOPM_DOMAIN_GPU_CHIP, 4), geopm::Exception);
}

TEST_F(DrmGpuTopoTest, driver_name)
{
    {
        // No GPUs --> no GPU drivers
        DrmGpuTopo topo(m_dir_manager->get_driver_dir());
        EXPECT_THROW(topo.driver_name(), geopm::Exception);
    }

    {
        m_dir_manager->create_card(0);
        DrmGpuTopo topo(m_dir_manager->get_driver_dir());
        EXPECT_EQ("test_driver", topo.driver_name());
    }
}
