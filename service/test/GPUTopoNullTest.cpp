/*
 * Copyright (c) 2015 - 2023, Intel Corporation
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
#include "GPUTopoNull.hpp"
#include "geopm_topo.h"

using geopm::GPUTopo;
using geopm::GPUTopoNull;
using geopm::Exception;
using testing::Return;

TEST(GPUTopoNullTest, default_config)
{
    std::unique_ptr<GPUTopoNull> topo;
    topo = geopm::make_unique<GPUTopoNull>();
    EXPECT_EQ(0, topo->num_gpu());
    EXPECT_EQ(0, topo->num_gpu(GEOPM_DOMAIN_GPU_CHIP));
    EXPECT_EQ(topo->cpu_affinity_ideal(0), std::set<int>{});
    EXPECT_EQ(topo->cpu_affinity_ideal(GEOPM_DOMAIN_GPU_CHIP, 0), std::set<int>{});
}
