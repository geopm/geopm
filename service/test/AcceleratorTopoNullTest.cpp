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
#include "AcceleratorTopoNull.hpp"
#include "geopm_topo.h"

using geopm::AcceleratorTopo;
using geopm::AcceleratorTopoNull;
using geopm::Exception;
using testing::Return;

TEST(AcceleratorTopoNullTest, default_config)
{
    std::unique_ptr<AcceleratorTopoNull> topo;
    topo = geopm::make_unique<AcceleratorTopoNull>();
    EXPECT_EQ(0, topo->num_accelerator());
    EXPECT_EQ(0, topo->num_accelerator(GEOPM_DOMAIN_BOARD_ACCELERATOR_CHIP));
    EXPECT_EQ(topo->cpu_affinity_ideal(0), std::set<int>{});
    EXPECT_EQ(topo->cpu_affinity_ideal(GEOPM_DOMAIN_BOARD_ACCELERATOR_CHIP, 0), std::set<int>{});
}
