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

#include "geopm/Helper.hpp"
#include "geopm/Agg.hpp"
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

TEST_F(ConstConfigIOGroupTest, input_empty_json)
{
    ConstConfigIOGroup iogroup("{}");
    EXPECT_EQ(iogroup.signal_names(), std::set<std::string>{});
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

TEST_F(ConstConfigIOGroupTest, input_duplicate_signal)
{
    /* When one signal is provided more than once,
       apparently the latter entry overwrites the data of the former. */
    std::string json_string = "{"
    "    \"GPU_CORE_FREQUENCY\": {"
    "        \"domain\": \"gpu\","
    "        \"description\": \"Provides GPU core frequency\","
    "        \"units\": \"hertz\","
    "        \"aggregation\": \"sum\","
    "        \"values\": [ 1500, 1600, 1700 ]"
    "    },"
    "    \"GPU_CORE_FREQUENCY\": {"
    "        \"domain\": \"cpu\","
    "        \"description\": \"Provides CPU core frequency\","
    "        \"units\": \"watts\","
    "        \"aggregation\": \"average\","
    "        \"values\": [ 1050, 1060, 1070 ]"
    "    }"
    "}";
    ConstConfigIOGroup iogroup(json_string);
    EXPECT_EQ(iogroup.signal_names(), (std::set<std::string>{"CONST_CONFIG::GPU_CORE_FREQUENCY"}));
    EXPECT_EQ(iogroup.signal_description("CONST_CONFIG::GPU_CORE_FREQUENCY"),
        "    description: " "Provides CPU core frequency" "\n"
        "    units: " "watts" "\n"
        "    aggregation: " "average" "\n"
        "    domain: " "cpu" "\n"
        "    iogroup: ConstConfigIOGroup");
}

TEST_F(ConstConfigIOGroupTest, input_missing_properties)
{
    std::string json_string = "{"
    "    \"GPU_CORE_FREQUENCY\": {"
    "        \"domain\": \"gpu\","
    "        \"units\": \"hertz\","
    "        \"values\": [ 1500, 1600, 1700 ]"
    "    }"
    "}";
    GEOPM_EXPECT_THROW_MESSAGE(
        ConstConfigIOGroup iogroup(json_string),
        GEOPM_ERROR_INVALID,
        "ConstConfigIOGroup::parse_config_json(): "
        "missing properties for signal \"" "GPU_CORE_FREQUENCY" "\": "
    );
}

TEST_F(ConstConfigIOGroupTest, input_unexpected_properties)
{
    std::string json_string = "{"
    "    \"GPU_CORE_FREQUENCY\": {"
    "        \"domain\": \"gpu\","
    "        \"magic\": \"fire\","
    "        \"description\": \"Provides GPU core frequency\","
    "        \"units\": \"hertz\","
    "        \"aggregation\": \"sum\","
    "        \"values\": [ 1500, 1600, 1700 ]"
    "    }"
    "}";
    GEOPM_EXPECT_THROW_MESSAGE(
        ConstConfigIOGroup iogroup(json_string),
        GEOPM_ERROR_INVALID,
        "ConstConfigIOGroup::parse_config_json():"
        " unexpected propety: \"" "magic" "\""
    );
}

TEST_F(ConstConfigIOGroupTest, input_capital_properties)
{
    std::string json_string = "{"
    "    \"GPU_CORE_FREQUENCY\": {"
    "        \"DOMAIN\": \"gpu\","
    "        \"description\": \"Provides GPU core frequency\","
    "        \"units\": \"hertz\","
    "        \"aggregation\": \"sum\","
    "        \"values\": [ 1500, 1600, 1700 ]"
    "    }"
    "}";
    GEOPM_EXPECT_THROW_MESSAGE(
        ConstConfigIOGroup iogroup(json_string),
        GEOPM_ERROR_INVALID,
        "ConstConfigIOGroup::parse_config_json():"
        " unexpected propety: \"" "DOMAIN" "\""
    );
}

TEST_F(ConstConfigIOGroupTest, input_duplicate_properties)
{
    /* Multiple apprearances of the same property overwrites the previous one. */
    std::string json_string = "{"
    "    \"GPU_CORE_FREQUENCY\": {"
    "        \"domain\": \"gpu\","
    "        \"description\": \"Provides GPU core frequency\","
    "        \"units\": \"hertz\","
    "        \"aggregation\": \"sum\","
    "        \"domain\": \"cpu\","
    "        \"values\": [ 1500, 1600, 1700 ],"
    "        \"description\": \"Scratches your feet\""
    "    }"
    "}";
    ConstConfigIOGroup iogroup(json_string);
    EXPECT_EQ(iogroup.signal_description("CONST_CONFIG::GPU_CORE_FREQUENCY"),
        "    description: " "Scratches your feet" "\n"
        "    units: " "hertz" "\n"
        "    aggregation: " "sum" "\n"
        "    domain: " "cpu" "\n"
        "    iogroup: ConstConfigIOGroup");
}

TEST_F(ConstConfigIOGroupTest, input_invalid_domain)
{
    std::string json_string = "{"
    "    \"GPU_CORE_FREQUENCY\": {"
    "        \"domain\": \"fpga\","
    "        \"description\": \"Provides GPU core frequency\","
    "        \"units\": \"hertz\","
    "        \"aggregation\": \"sum\","
    "        \"values\": [ 1500, 1600, 1700 ]"
    "    }"
    "}";
    GEOPM_EXPECT_THROW_MESSAGE(
        ConstConfigIOGroup iogroup(json_string),
        GEOPM_ERROR_INVALID,
        "PlatformTopo::domain_name_to_type(): unrecognized domain_name: " "fpga"
    );
}

TEST_F(ConstConfigIOGroupTest, input_invalid_units)
{
    std::string json_string = "{"
    "    \"GPU_CORE_FREQUENCY\": {"
    "        \"domain\": \"gpu\","
    "        \"description\": \"Provides GPU core frequency\","
    "        \"units\": \"kilograms\","
    "        \"aggregation\": \"sum\","
    "        \"values\": [ 1500, 1600, 1700 ]"
    "    }"
    "}";
    GEOPM_EXPECT_THROW_MESSAGE(
        ConstConfigIOGroup iogroup(json_string),
        GEOPM_ERROR_INVALID,
        "IOGroup::string_to_units(): invalid units string"
    );
}

TEST_F(ConstConfigIOGroupTest, input_invalid_aggregation)
{
    std::string json_string = "{"
    "    \"GPU_CORE_FREQUENCY\": {"
    "        \"domain\": \"gpu\","
    "        \"description\": \"Provides GPU core frequency\","
    "        \"units\": \"hertz\","
    "        \"aggregation\": \"bitwise_or\","
    "        \"values\": [ 1500, 1600, 1700 ]"
    "    }"
    "}";
    GEOPM_EXPECT_THROW_MESSAGE(
        ConstConfigIOGroup iogroup(json_string),
        GEOPM_ERROR_INVALID,
        "Agg::name_to_function(): unknown aggregation function: " "bitwise_or"
    );
}

TEST_F(ConstConfigIOGroupTest, input_incorrect_type)
{
    std::string json_string = "{"
    "    \"GPU_CORE_FREQUENCY\": {"
    "        \"domain\": \"gpu\","
    "        \"description\": \"Provides GPU core frequency\","
    "        \"units\": \"hertz\","
    "        \"aggregation\": 32,"
    "        \"values\": [ 1500, 1600, 1700 ]"
    "    }"
    "}";
    GEOPM_EXPECT_THROW_MESSAGE(
        ConstConfigIOGroup iogroup(json_string),
        GEOPM_ERROR_INVALID,
        "ConstConfigIOGroup::parse_config_json():"
        " incorrect type for property: \"" "aggregation" "\""
    );
}

TEST_F(ConstConfigIOGroupTest, input_array_value_type)
{
    std::string json_string = "{"
    "    \"GPU_CORE_FREQUENCY\": {"
    "        \"domain\": \"gpu\","
    "        \"description\": \"Provides GPU core frequency\","
    "        \"units\": \"hertz\","
    "        \"aggregation\": \"sum\","
    "        \"values\": [ 100, 200, \"threehundred\" ]"
    "    }"
    "}";
    GEOPM_EXPECT_THROW_MESSAGE(
        ConstConfigIOGroup iogroup(json_string),
        GEOPM_ERROR_INVALID,
        "ConstConfigIOGroup::parse_config_json():"
        " incorrect type for property: "
        "\"values\""
    );
}

TEST_F(ConstConfigIOGroupTest, input_array_value_empty)
{
    std::string json_string = "{"
    "    \"GPU_CORE_FREQUENCY\": {"
    "        \"domain\": \"gpu\","
    "        \"description\": \"Provides GPU core frequency\","
    "        \"units\": \"hertz\","
    "        \"aggregation\": \"sum\","
    "        \"values\": []"
    "    }"
    "}";
    ConstConfigIOGroup iogroup(json_string);
    GEOPM_EXPECT_THROW_MESSAGE(
        iogroup.read_signal("CONST_CONFIG::GPU_CORE_FREQUENCY", GEOPM_DOMAIN_GPU, 0),
        GEOPM_ERROR_INVALID,
        "ConstConfigIOGroup::read_signal(): domain_idx "
        "0" " out of range."
    );
}

TEST_F(ConstConfigIOGroupTest, valid_json_positive)
{
    std::string json_string = "{"
    "    \"GPU_CORE_FREQUENCY\": {"
    "        \"domain\": \"gpu\","
    "        \"description\": \"Provides GPU core frequency\","
    "        \"units\": \"hertz\","
    "        \"aggregation\": \"sum\","
    "        \"values\": [ 1500, 1600, 1700 ]"
    "    },"
    "    \"CPU_CORE_FREQUENCY\": {"
    "        \"domain\": \"cpu\","
    "        \"description\": \"Provides CPU core frequency\","
    "        \"units\": \"watts\","
    "        \"aggregation\": \"average\","
    "        \"values\": [ 1050, 1060, 1070 ]"
    "    }"
    "}";
    ConstConfigIOGroup iogroup(json_string);
    EXPECT_EQ(iogroup.signal_names(), (std::set<std::string>{
        "CONST_CONFIG::CPU_CORE_FREQUENCY", "CONST_CONFIG::GPU_CORE_FREQUENCY"}));
    EXPECT_EQ(iogroup.is_valid_signal("CONST_CONFIG::CPU_CORE_FREQUENCY"), true);
    EXPECT_EQ(iogroup.is_valid_signal("CONST_CONFIG::GPU_CORE_FREQUENCY"), true);
    EXPECT_EQ(iogroup.signal_domain_type("CONST_CONFIG::CPU_CORE_FREQUENCY"), GEOPM_DOMAIN_CPU);
    EXPECT_EQ(iogroup.signal_domain_type("CONST_CONFIG::GPU_CORE_FREQUENCY"), GEOPM_DOMAIN_GPU);
    EXPECT_EQ(iogroup.push_signal("CONST_CONFIG::CPU_CORE_FREQUENCY", GEOPM_DOMAIN_CPU, 1), 0);
    EXPECT_EQ(iogroup.push_signal("CONST_CONFIG::GPU_CORE_FREQUENCY", GEOPM_DOMAIN_GPU, 2), 1);
    EXPECT_EQ(iogroup.sample(0), 1060);
    EXPECT_EQ(iogroup.sample(1), 1700);
    EXPECT_EQ(iogroup.read_signal("CONST_CONFIG::CPU_CORE_FREQUENCY", GEOPM_DOMAIN_CPU, 2), 1070);
    EXPECT_EQ(iogroup.read_signal("CONST_CONFIG::GPU_CORE_FREQUENCY", GEOPM_DOMAIN_GPU, 0), 1500);
    {
        auto expected_function = geopm::Agg::average;
        auto actual_function = *iogroup.agg_function("CONST_CONFIG::CPU_CORE_FREQUENCY")
            .target<double (*) (const std::vector<double>&)>();
        EXPECT_EQ(expected_function, actual_function);
    }
    {
        auto expected_function = geopm::Agg::sum;
        auto actual_function = *iogroup.agg_function("CONST_CONFIG::GPU_CORE_FREQUENCY")
            .target<double (*) (const std::vector<double>&)>();
        EXPECT_EQ(expected_function, actual_function);
    }
    {
        auto expected_function = geopm::string_format_double;
        auto actual_function = *iogroup.format_function("CONST_CONFIG::CPU_CORE_FREQUENCY")
            .target<std::string (*) (double)>();
        EXPECT_EQ(expected_function, actual_function);
    }
    {
        auto expected_function = geopm::string_format_double;
        auto actual_function = *iogroup.format_function("CONST_CONFIG::GPU_CORE_FREQUENCY")
            .target<std::string (*) (double)>();
        EXPECT_EQ(expected_function, actual_function);
    }
    EXPECT_EQ(iogroup.signal_description("CONST_CONFIG::CPU_CORE_FREQUENCY"),
        "    description: " "Provides CPU core frequency" "\n"
        "    units: " "watts" "\n"
        "    aggregation: " "average" "\n"
        "    domain: " "cpu" "\n"
        "    iogroup: ConstConfigIOGroup");
    EXPECT_EQ(iogroup.signal_description("CONST_CONFIG::GPU_CORE_FREQUENCY"),
        "    description: " "Provides GPU core frequency" "\n"
        "    units: " "hertz" "\n"
        "    aggregation: " "sum" "\n"
        "    domain: " "gpu" "\n"
        "    iogroup: ConstConfigIOGroup");
    EXPECT_EQ(iogroup.signal_behavior("CONST_CONFIG::CPU_CORE_FREQUENCY"), geopm::IOGroup::M_SIGNAL_BEHAVIOR_CONSTANT);
    EXPECT_EQ(iogroup.signal_behavior("CONST_CONFIG::GPU_CORE_FREQUENCY"), geopm::IOGroup::M_SIGNAL_BEHAVIOR_CONSTANT);
    EXPECT_EQ(iogroup.name(), "CONST_CONFIG");
    EXPECT_EQ(iogroup.plugin_name(), "CONST_CONFIG");
}

TEST_F(ConstConfigIOGroupTest, valid_json_negative)
{
    std::string json_string = "{"
    "    \"GPU_CORE_FREQUENCY\": {"
    "        \"domain\": \"gpu\","
    "        \"description\": \"Provides GPU core frequency\","
    "        \"units\": \"hertz\","
    "        \"aggregation\": \"sum\","
    "        \"values\": [ 1500, 1600, 1700 ]"
    "    }"
    "}";
    ConstConfigIOGroup iogroup(json_string);

    EXPECT_EQ(iogroup.control_names(), std::set<std::string>{});
    EXPECT_EQ(iogroup.is_valid_signal("CONST_CONFIG::UNCORE_RATIO_LIMIT:MIN_RATIO"), false);
    EXPECT_EQ(iogroup.is_valid_control("CONST_CONFIG::UNCORE_RATIO_LIMIT:MIN_RATIO"), false);
    EXPECT_EQ(iogroup.signal_domain_type("CONST_CONFIG::UNCORE_RATIO_LIMIT:MIN_RATIO"), GEOPM_DOMAIN_INVALID);
    EXPECT_EQ(iogroup.control_domain_type("CONST_CONFIG::UNCORE_RATIO_LIMIT:MIN_RATIO"), GEOPM_DOMAIN_INVALID);
    GEOPM_EXPECT_THROW_MESSAGE(
        iogroup.push_signal("CONST_CONFIG::UNCORE_RATIO_LIMIT:MIN_RATIO", 0, 0),
        GEOPM_ERROR_INVALID,
        "ConstConfigIOGroup::push_signal(): "
        "CONST_CONFIG::UNCORE_RATIO_LIMIT:MIN_RATIO" " not valid for ConstConfigIOGroup"
    );
    GEOPM_EXPECT_THROW_MESSAGE(
        iogroup.push_signal("CONST_CONFIG::GPU_CORE_FREQUENCY", GEOPM_DOMAIN_CPU, 0),
        GEOPM_ERROR_INVALID,
        "ConstConfigIOGroup::push_signal(): domain_type "
        "3" " not valid for ConstConfigIOGroup"
    );
    GEOPM_EXPECT_THROW_MESSAGE(
        iogroup.push_signal("CONST_CONFIG::GPU_CORE_FREQUENCY", GEOPM_DOMAIN_GPU, -1),
        GEOPM_ERROR_INVALID,
        "ConstConfigIOGroup::push_signal(): domain_idx "
        "-1" " out of range."
    );
    GEOPM_EXPECT_THROW_MESSAGE(
        iogroup.push_signal("CONST_CONFIG::GPU_CORE_FREQUENCY", GEOPM_DOMAIN_GPU, 3),
        GEOPM_ERROR_INVALID,
        "ConstConfigIOGroup::push_signal(): domain_idx "
        "3" " out of range."
    );
    GEOPM_EXPECT_THROW_MESSAGE(
        iogroup.push_control("CONST_CONFIG::UNCORE_RATIO_LIMIT:MIN_RATIO", 0, 0),
        GEOPM_ERROR_INVALID,
        "ConstConfigIOGroup::push_control(): there are no "
        "controls supported by the ConstConfigIOGroup"
    );
    GEOPM_EXPECT_THROW_MESSAGE(
        iogroup.sample(3),
        GEOPM_ERROR_INVALID,
        "ConstConfigIOGroup::sample(): batch_idx "
        "3" " out of range."
    );
    GEOPM_EXPECT_THROW_MESSAGE(
        iogroup.adjust(0, 3.14),
        GEOPM_ERROR_INVALID,
        "ConstConfigIOGroup::adjust(): there are no controls "
        "supported by the ConstConfigIOGroup"
    );
    GEOPM_EXPECT_THROW_MESSAGE(
        iogroup.read_signal("CONST_CONFIG::UNCORE_RATIO_LIMIT:MIN_RATIO", 0, 0),
        GEOPM_ERROR_INVALID,
        "ConstConfigIOGroup::read_signal(): "
        "CONST_CONFIG::UNCORE_RATIO_LIMIT:MIN_RATIO" " not valid for ConstConfigIOGroup"
    );
    GEOPM_EXPECT_THROW_MESSAGE(
        iogroup.read_signal("CONST_CONFIG::GPU_CORE_FREQUENCY", GEOPM_DOMAIN_CPU, 0),
        GEOPM_ERROR_INVALID,
        "ConstConfigIOGroup::read_signal(): domain_type "
        "3"
        " not valid for ConstConfigIOGroup"
    );
    GEOPM_EXPECT_THROW_MESSAGE(
        iogroup.read_signal("CONST_CONFIG::GPU_CORE_FREQUENCY", GEOPM_DOMAIN_GPU, -1),
        GEOPM_ERROR_INVALID,
        "ConstConfigIOGroup::read_signal(): domain_idx "
        "-1" " out of range."
    );
    GEOPM_EXPECT_THROW_MESSAGE(
        iogroup.read_signal("CONST_CONFIG::GPU_CORE_FREQUENCY", GEOPM_DOMAIN_GPU, 3),
        GEOPM_ERROR_INVALID,
        "ConstConfigIOGroup::read_signal(): domain_idx "
        "3" " out of range."
    );
    GEOPM_EXPECT_THROW_MESSAGE(
        iogroup.write_control("CONST_CONFIG::UNCORE_RATIO_LIMIT:MIN_RATIO", 0, 0, 3.14),
        GEOPM_ERROR_INVALID,
        "ConstConfigIOGroup::write_control(): there are no "
        "controls supported by the ConstConfigIOGroup"
    );
    GEOPM_EXPECT_THROW_MESSAGE(
        iogroup.agg_function("CONST_CONFIG::UNCORE_RATIO_LIMIT:MIN_RATIO"),
        GEOPM_ERROR_INVALID,
        "ConstConfigIOGroup::agg_function(): unknown how "
        "to aggregate \"" "CONST_CONFIG::UNCORE_RATIO_LIMIT:MIN_RATIO" "\""
    );
    GEOPM_EXPECT_THROW_MESSAGE(
        iogroup.format_function("CONST_CONFIG::UNCORE_RATIO_LIMIT:MIN_RATIO"),
        GEOPM_ERROR_INVALID,
        "ConstConfigIOGroup::format_function(): unknown "
        "how to format \"" "CONST_CONFIG::UNCORE_RATIO_LIMIT:MIN_RATIO" "\""
    );
    GEOPM_EXPECT_THROW_MESSAGE(
        iogroup.signal_description("CONST_CONFIG::UNCORE_RATIO_LIMIT:MIN_RATIO"),
        GEOPM_ERROR_INVALID,
        "ConstConfigIOGroup::signal_description(): "
        "signal_name " "CONST_CONFIG::UNCORE_RATIO_LIMIT:MIN_RATIO"
        " not valid for ConstConfigIOGroup"
    );
    GEOPM_EXPECT_THROW_MESSAGE(
        iogroup.control_description("CONST_CONFIG::UNCORE_RATIO_LIMIT:MIN_RATIO"),
        GEOPM_ERROR_INVALID,
        "ConstConfigIOGroup::control_description: there are "
        "no controls supported by the ConstConfigIOGroup"
    );
    GEOPM_EXPECT_THROW_MESSAGE(
        iogroup.signal_behavior("CONST_CONFIG::UNCORE_RATIO_LIMIT:MIN_RATIO"),
        GEOPM_ERROR_INVALID,
        "ConstConfigIOGroup::signal_behavior(): "
        "signal_name " "CONST_CONFIG::UNCORE_RATIO_LIMIT:MIN_RATIO"
        " not valid for ConstConfigIOGroup"
    );
}

void ConstConfigIOGroupTest::TearDown()
{
}