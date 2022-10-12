/*
 * Copyright (c) 2015 - 2022, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "ConstConfigIOGroup.hpp"

#include <set>
#include <string>
#include <fstream>
#include <iostream>
#include <regex>
#include <unistd.h>

#include "gmock/gmock.h"
#include "gtest/gtest.h"

#include "geopm_topo.h"  // for geopm_domain_e
#include "geopm_test.hpp" // for GEOPM_EXPECT_THROW_MESSAGE

#include "geopm/Exception.hpp"
#include "geopm/PlatformIO.hpp"
#include "geopm/PlatformTopo.hpp"

using geopm::ConstConfigIOGroup;
using geopm::Exception;
using geopm::PlatformIO;
using geopm::PlatformTopo;

class ConstConfigIOGroupTest : public::testing::Test
{
    protected:
        void SetUp() override;
        void TearDown() override;
};

void ConstConfigIOGroupTest::SetUp()
{
}

TEST_F(ConstConfigIOGroupTest, input_empty_string)
{
    GEOPM_EXPECT_THROW_MESSAGE(
        ConstConfigIOGroup iogroup(""),
        GEOPM_ERROR_INVALID,
        "ConstConfigIOGroup::parse_config_json(): "
        "detected a malformed JSON string"
    );
}

TEST_F(ConstConfigIOGroupTest, input_gibberish)
{
    GEOPM_EXPECT_THROW_MESSAGE(
        ConstConfigIOGroup iogroup("asdfklfj234890fnjklsd"),
        GEOPM_ERROR_INVALID,
        "ConstConfigIOGroup::parse_config_json(): "
        "detected a malformed JSON string"
    );
}

TEST_F(ConstConfigIOGroupTest, valid_json)
{
    std::string json_string = "{"
    "    \"GPU_CORE_FREQUENCY\": {"
    "        \"domain\": \"gpu\","
    "        \"description\": \"Provides GPU core frequency\","
    "        \"units\": \"hertz\","
    "        \"aggregation\": \"sum\","
    "        \"values\": { 1500, 1600, 1700 }"
    "    }"
    "}";
    ConstConfigIOGroup iogroup(json_string);
    // EXPECT_EQ(iogroup.control_names(), std::set<std::string>{});
}

TEST_F(ConstConfigIOGroupTest, control_names)
{
    ConstConfigIOGroup iogroup("");
    EXPECT_EQ(iogroup.control_names(), std::set<std::string>{});
}

TEST_F(ConstConfigIOGroupTest, is_valid_control)
{
    ConstConfigIOGroup iogroup("");
    EXPECT_EQ(iogroup.is_valid_control("CONST_CONFIG::UNCORE_RATIO_LIMIT:MIN_RATIO"), false);
}

void ConstConfigIOGroupTest::TearDown()
{
}