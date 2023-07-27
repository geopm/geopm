/*
 * Copyright (c) 2015 - 2023, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "ConstConfigIOGroup.hpp"

#include <cerrno>
#include <set>
#include <string>
#include <fstream>
#include <iostream>
#include <regex>
#include <list>
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

#include "MockPlatformTopo.hpp"

using ::testing::Return;
using ::testing::_;
using ::testing::AtMost;

using geopm::ConstConfigIOGroup;
using geopm::Exception;
using geopm::PlatformIO;
using geopm::PlatformTopo;

class ConstConfigIOGroupTest : public::testing::Test
{
    protected:
        void SetUp();
        void set_up_topo_expect_exactly(const std::list<int> domains);
        void set_up_topo_expect_at_most(const std::list<int> domains);
        static void create_config_file(const std::string &config);

        static const std::string M_CONFIG_FILE_PATH;
        static constexpr int M_NUM_PACKAGE = 1;
        static constexpr int M_NUM_CORE = 1;
        static constexpr int M_NUM_CPU = 3;
        static constexpr int M_NUM_GPU = 3;

        std::shared_ptr<MockPlatformTopo> m_default_topo;
};

const std::string ConstConfigIOGroupTest::M_CONFIG_FILE_PATH =
        "const_config_test.json";

void ConstConfigIOGroupTest::SetUp()
{
    m_default_topo = make_topo(M_NUM_PACKAGE, M_NUM_CORE, M_NUM_CPU);
    ON_CALL(*m_default_topo, num_domain(GEOPM_DOMAIN_GPU))
        .WillByDefault(Return(M_NUM_GPU));
}

void ConstConfigIOGroupTest::set_up_topo_expect_exactly(const std::list<int> domains)
{
    EXPECT_CALL(*m_default_topo, num_domain(_)).Times(0);
    for (const auto d : domains) {
        EXPECT_CALL(*m_default_topo, num_domain(d))
            .Times(1)
            .RetiresOnSaturation();
    }
}

void ConstConfigIOGroupTest::set_up_topo_expect_at_most(const std::list<int> domains)
{
    EXPECT_CALL(*m_default_topo, num_domain(_)).Times(0);
    for (const auto d : domains) {
        EXPECT_CALL(*m_default_topo, num_domain(d))
            .Times(AtMost(1))
            .RetiresOnSaturation();
    }
}

void ConstConfigIOGroupTest::create_config_file(const std::string &config)
{
    ASSERT_NO_THROW(geopm::write_file(M_CONFIG_FILE_PATH, config));
}

TEST_F(ConstConfigIOGroupTest, input_empty_string)
{
    set_up_topo_expect_exactly({});
    create_config_file("  ");

    GEOPM_EXPECT_THROW_MESSAGE(
        ConstConfigIOGroup iogroup(*m_default_topo, M_CONFIG_FILE_PATH, ""),
        GEOPM_ERROR_INVALID,
        "ConstConfigIOGroup::parse_config_json(): "
        "detected a malformed JSON string"
    );
}

TEST_F(ConstConfigIOGroupTest, input_empty_json)
{
    set_up_topo_expect_exactly({});
    create_config_file("{}");

    ConstConfigIOGroup iogroup(*m_default_topo, M_CONFIG_FILE_PATH, "");
    EXPECT_EQ(iogroup.signal_names(), std::set<std::string>{});
    EXPECT_EQ(iogroup.control_names(), std::set<std::string>{});
}

TEST_F(ConstConfigIOGroupTest, input_gibberish)
{
    set_up_topo_expect_exactly({});
    create_config_file("asdfklfj234890fnjklsd");

    GEOPM_EXPECT_THROW_MESSAGE(
        ConstConfigIOGroup iogroup(*m_default_topo, M_CONFIG_FILE_PATH, ""),
        GEOPM_ERROR_INVALID,
        "ConstConfigIOGroup::parse_config_json(): "
        "detected a malformed JSON string"
    );
}

TEST_F(ConstConfigIOGroupTest, input_duplicate_signal)
{
    set_up_topo_expect_exactly({GEOPM_DOMAIN_GPU});
    std::string json_string = "{"
    "    \"GPU_CORE_FREQUENCY\": {"
    "        \"domain\": \"cpu\","
    "        \"description\": \"Provides CPU core frequency\","
    "        \"units\": \"watts\","
    "        \"aggregation\": \"average\","
    "        \"values\": [ 1050, 1060, 1070 ]"
    "    },"
    "    \"GPU_CORE_FREQUENCY\": {"
    "        \"domain\": \"gpu\","
    "        \"description\": \"Provides GPU core frequency\","
    "        \"units\": \"hertz\","
    "        \"aggregation\": \"sum\","
    "        \"values\": [ 1500, 1600, 1700 ]"
    "    }"
    "}";
    create_config_file(json_string);
    ConstConfigIOGroup iogroup(*m_default_topo, M_CONFIG_FILE_PATH, "");

    EXPECT_EQ(iogroup.signal_names(), (std::set<std::string>{"CONST_CONFIG::GPU_CORE_FREQUENCY"}));
    EXPECT_EQ(iogroup.control_names(), std::set<std::string>{});
    EXPECT_EQ(iogroup.signal_description("CONST_CONFIG::GPU_CORE_FREQUENCY"),
        "    description: " "Provides GPU core frequency" "\n"
        "    units: " "hertz" "\n"
        "    aggregation: " "sum" "\n"
        "    domain: " "gpu" "\n"
        "    iogroup: ConstConfigIOGroup");
}

TEST_F(ConstConfigIOGroupTest, input_missing_properties)
{
    set_up_topo_expect_exactly({});
    std::string json_string = "{"
    "    \"GPU_CORE_FREQUENCY\": {"
    "        \"domain\": \"gpu\","
    "        \"units\": \"hertz\","
    "        \"values\": [ 1500, 1600, 1700 ]"
    "    }"
    "}";
    create_config_file(json_string);

    GEOPM_EXPECT_THROW_MESSAGE(
        ConstConfigIOGroup iogroup(*m_default_topo, M_CONFIG_FILE_PATH, ""),
        GEOPM_ERROR_INVALID,
        "ConstConfigIOGroup::parse_config_json(): "
        "missing properties for signal \"" "GPU_CORE_FREQUENCY" "\": "
        "aggregation" ", " "description"
    );
}

TEST_F(ConstConfigIOGroupTest, input_values_and_common_value)
{
    set_up_topo_expect_exactly({});
    std::string json_string = "{"
    "    \"GPU_CORE_FREQUENCY\": {"
    "        \"domain\": \"gpu\","
    "        \"description\": \"Provides GPU core frequency\","
    "        \"units\": \"hertz\","
    "        \"aggregation\": \"sum\","
    "        \"values\": [ 1500, 1600, 1700 ],"
    "        \"common_value\": 1500"
    "    }"
    "}";
    create_config_file(json_string);

    GEOPM_EXPECT_THROW_MESSAGE(
        ConstConfigIOGroup iogroup(*m_default_topo, M_CONFIG_FILE_PATH, ""),
        GEOPM_ERROR_INVALID,
        "ConstConfigIOGroup::parse_config_json(): "
        "\"values\" and \"common_value\" provided for signal \"GPU_CORE_FREQUENCY\""
    );
}

TEST_F(ConstConfigIOGroupTest, missing_values_and_common_value)
{
    set_up_topo_expect_exactly({});
    std::string json_string = "{"
    "    \"GPU_CORE_FREQUENCY\": {"
    "        \"domain\": \"gpu\","
    "        \"description\": \"Provides GPU core frequency\","
    "        \"units\": \"hertz\","
    "        \"aggregation\": \"sum\""
    "    }"
    "}";
    create_config_file(json_string);

    GEOPM_EXPECT_THROW_MESSAGE(
        ConstConfigIOGroup iogroup(*m_default_topo, M_CONFIG_FILE_PATH, ""),
        GEOPM_ERROR_INVALID,
        "ConstConfigIOGroup::parse_config_json(): "
        "missing \"values\" and \"common_value\" for signal \"GPU_CORE_FREQUENCY\""
    );
}

TEST_F(ConstConfigIOGroupTest, input_unexpected_properties)
{
    set_up_topo_expect_exactly({});
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
    create_config_file(json_string);

    GEOPM_EXPECT_THROW_MESSAGE(
        ConstConfigIOGroup iogroup(*m_default_topo, M_CONFIG_FILE_PATH, ""),
        GEOPM_ERROR_INVALID,
        "ConstConfigIOGroup::parse_config_json(): "
        "for signal \"GPU_CORE_FREQUENCY\", unexpected property: \"" "magic" "\""
    );
}

TEST_F(ConstConfigIOGroupTest, input_capital_properties)
{
    set_up_topo_expect_exactly({});
    std::string json_string = "{"
    "    \"GPU_CORE_FREQUENCY\": {"
    "        \"DOMAIN\": \"gpu\","
    "        \"description\": \"Provides GPU core frequency\","
    "        \"units\": \"hertz\","
    "        \"aggregation\": \"sum\","
    "        \"values\": [ 1500, 1600, 1700 ]"
    "    }"
    "}";
    create_config_file(json_string);

    GEOPM_EXPECT_THROW_MESSAGE(
        ConstConfigIOGroup iogroup(*m_default_topo, M_CONFIG_FILE_PATH, ""),
        GEOPM_ERROR_INVALID,
        "ConstConfigIOGroup::parse_config_json(): "
        "for signal \"GPU_CORE_FREQUENCY\", unexpected property: \"" "DOMAIN" "\""
    );
}

TEST_F(ConstConfigIOGroupTest, input_duplicate_properties)
{
    /* Multiple apprearances of the same property overwrites the previous one. */
    set_up_topo_expect_exactly({GEOPM_DOMAIN_GPU});
    std::string json_string = "{"
    "    \"GPU_CORE_FREQUENCY\": {"
    "        \"domain\": \"cpu\","
    "        \"description\": \"Provides GPU core frequency\","
    "        \"units\": \"hertz\","
    "        \"aggregation\": \"sum\","
    "        \"domain\": \"gpu\","
    "        \"values\": [ 1500, 1600, 1700 ],"
    "        \"description\": \"Scratches your feet\""
    "    }"
    "}";
    create_config_file(json_string);
    ConstConfigIOGroup iogroup(*m_default_topo, M_CONFIG_FILE_PATH, "");

    EXPECT_EQ(iogroup.signal_names(), (std::set<std::string>{"CONST_CONFIG::GPU_CORE_FREQUENCY"}));
    EXPECT_EQ(iogroup.control_names(), std::set<std::string>{});
    EXPECT_EQ(iogroup.signal_description("CONST_CONFIG::GPU_CORE_FREQUENCY"),
        "    description: " "Scratches your feet" "\n"
        "    units: " "hertz" "\n"
        "    aggregation: " "sum" "\n"
        "    domain: " "gpu" "\n"
        "    iogroup: ConstConfigIOGroup");
}

TEST_F(ConstConfigIOGroupTest, input_empty_domain)
{
    set_up_topo_expect_exactly({});
    std::string json_string = "{"
    "    \"GPU_CORE_FREQUENCY\": {"
    "        \"domain\": \"\","
    "        \"description\": \"Provides GPU core frequency\","
    "        \"units\": \"hertz\","
    "        \"aggregation\": \"sum\","
    "        \"values\": [ 1500, 1600, 1700 ]"
    "    }"
    "}";
    create_config_file(json_string);

    GEOPM_EXPECT_THROW_MESSAGE(
        ConstConfigIOGroup iogroup(*m_default_topo, M_CONFIG_FILE_PATH, ""),
        GEOPM_ERROR_INVALID,
        "PlatformTopo::domain_name_to_type(): unrecognized domain_name: "
    );
}

TEST_F(ConstConfigIOGroupTest, input_empty_description)
{
    set_up_topo_expect_at_most({GEOPM_DOMAIN_GPU});
    std::string json_string = "{"
    "    \"GPU_CORE_FREQUENCY\": {"
    "        \"domain\": \"gpu\","
    "        \"description\": \"\","
    "        \"units\": \"hertz\","
    "        \"aggregation\": \"sum\","
    "        \"values\": [ 1500, 1600, 1700 ]"
    "    }"
    "}";
    create_config_file(json_string);
    GEOPM_EXPECT_THROW_MESSAGE(
        ConstConfigIOGroup iogroup(*m_default_topo, M_CONFIG_FILE_PATH, ""),
        GEOPM_ERROR_INVALID,
        "ConstConfigIOGroup::parse_config_json(): "
        "empty description provided for signal \"GPU_CORE_FREQUENCY\""
    );
}

TEST_F(ConstConfigIOGroupTest, input_empty_units)
{
    set_up_topo_expect_at_most({GEOPM_DOMAIN_GPU});
    std::string json_string = "{"
    "    \"GPU_CORE_FREQUENCY\": {"
    "        \"domain\": \"gpu\","
    "        \"description\": \"Provides GPU core frequency\","
    "        \"units\": \"\","
    "        \"aggregation\": \"sum\","
    "        \"values\": [ 1500, 1600, 1700 ]"
    "    }"
    "}";
    create_config_file(json_string);

    GEOPM_EXPECT_THROW_MESSAGE(
        ConstConfigIOGroup iogroup(*m_default_topo, M_CONFIG_FILE_PATH, ""),
        GEOPM_ERROR_INVALID,
        "IOGroup::string_to_units(): invalid units string"
    );
}

TEST_F(ConstConfigIOGroupTest, input_empty_aggregation)
{
    set_up_topo_expect_at_most({GEOPM_DOMAIN_GPU});
    std::string json_string = "{"
    "    \"GPU_CORE_FREQUENCY\": {"
    "        \"domain\": \"gpu\","
    "        \"description\": \"Provides GPU core frequency\","
    "        \"units\": \"hertz\","
    "        \"aggregation\": \"\","
    "        \"values\": [ 1500, 1600, 1700 ]"
    "    }"
    "}";
    create_config_file(json_string);

    GEOPM_EXPECT_THROW_MESSAGE(
        ConstConfigIOGroup iogroup(*m_default_topo, M_CONFIG_FILE_PATH, ""),
        GEOPM_ERROR_INVALID,
        "Agg::name_to_function(): unknown aggregation function: "
    );
}

TEST_F(ConstConfigIOGroupTest, input_invalid_domain)
{
    set_up_topo_expect_exactly({});
    std::string json_string = "{"
    "    \"GPU_CORE_FREQUENCY\": {"
    "        \"domain\": \"fpga\","
    "        \"description\": \"Provides GPU core frequency\","
    "        \"units\": \"hertz\","
    "        \"aggregation\": \"sum\","
    "        \"values\": [ 1500, 1600, 1700 ]"
    "    }"
    "}";
    create_config_file(json_string);

    GEOPM_EXPECT_THROW_MESSAGE(
        ConstConfigIOGroup iogroup(*m_default_topo, M_CONFIG_FILE_PATH, ""),
        GEOPM_ERROR_INVALID,
        "PlatformTopo::domain_name_to_type(): unrecognized domain_name: " "fpga"
    );
}

TEST_F(ConstConfigIOGroupTest, input_invalid_units)
{
    set_up_topo_expect_at_most({GEOPM_DOMAIN_GPU});
    std::string json_string = "{"
    "    \"GPU_CORE_FREQUENCY\": {"
    "        \"domain\": \"gpu\","
    "        \"description\": \"Provides GPU core frequency\","
    "        \"units\": \"kilograms\","
    "        \"aggregation\": \"sum\","
    "        \"values\": [ 1500, 1600, 1700 ]"
    "    }"
    "}";
    create_config_file(json_string);

    GEOPM_EXPECT_THROW_MESSAGE(
        ConstConfigIOGroup iogroup(*m_default_topo, M_CONFIG_FILE_PATH, ""),
        GEOPM_ERROR_INVALID,
        "IOGroup::string_to_units(): invalid units string"
    );
}

TEST_F(ConstConfigIOGroupTest, input_invalid_aggregation)
{
    set_up_topo_expect_at_most({GEOPM_DOMAIN_GPU});
    std::string json_string = "{"
    "    \"GPU_CORE_FREQUENCY\": {"
    "        \"domain\": \"gpu\","
    "        \"description\": \"Provides GPU core frequency\","
    "        \"units\": \"hertz\","
    "        \"aggregation\": \"bitwise_or\","
    "        \"values\": [ 1500, 1600, 1700 ]"
    "    }"
    "}";
    create_config_file(json_string);

    GEOPM_EXPECT_THROW_MESSAGE(
        ConstConfigIOGroup iogroup(*m_default_topo, M_CONFIG_FILE_PATH, ""),
        GEOPM_ERROR_INVALID,
        "Agg::name_to_function(): unknown aggregation function: " "bitwise_or"
    );
}

TEST_F(ConstConfigIOGroupTest, input_incorrect_type)
{
    set_up_topo_expect_exactly({});
    std::string json_string = "{"
    "    \"GPU_CORE_FREQUENCY\": {"
    "        \"domain\": \"gpu\","
    "        \"description\": \"Provides GPU core frequency\","
    "        \"units\": \"hertz\","
    "        \"aggregation\": 32,"
    "        \"values\": [ 1500, 1600, 1700 ]"
    "    }"
    "}";
    create_config_file(json_string);

    GEOPM_EXPECT_THROW_MESSAGE(
        ConstConfigIOGroup iogroup(*m_default_topo, M_CONFIG_FILE_PATH, ""),
        GEOPM_ERROR_INVALID,
        "ConstConfigIOGroup::parse_config_json(): "
        "for signal \"GPU_CORE_FREQUENCY\", incorrect type for property: \"" "aggregation" "\""
    );
}

TEST_F(ConstConfigIOGroupTest, input_array_value_type)
{
    set_up_topo_expect_at_most({GEOPM_DOMAIN_GPU});
    std::string json_string = "{"
    "    \"GPU_CORE_FREQUENCY\": {"
    "        \"domain\": \"gpu\","
    "        \"description\": \"Provides GPU core frequency\","
    "        \"units\": \"hertz\","
    "        \"aggregation\": \"sum\","
    "        \"values\": [ 100, 200, \"threehundred\" ]"
    "    }"
    "}";
    create_config_file(json_string);

    GEOPM_EXPECT_THROW_MESSAGE(
        ConstConfigIOGroup iogroup(*m_default_topo, M_CONFIG_FILE_PATH, ""),
        GEOPM_ERROR_INVALID,
        "ConstConfigIOGroup::parse_config_json(): "
        "for signal \"GPU_CORE_FREQUENCY\", "
        "incorrect type for property: \"values\""
    );
}

TEST_F(ConstConfigIOGroupTest, input_common_value_type)
{
    set_up_topo_expect_at_most({GEOPM_DOMAIN_GPU});
    std::string json_string = "{"
    "    \"GPU_CORE_FREQUENCY\": {"
    "        \"domain\": \"gpu\","
    "        \"description\": \"Provides GPU core frequency\","
    "        \"units\": \"hertz\","
    "        \"aggregation\": \"sum\","
    "        \"common_value\": \"threehundred\""
    "    }"
    "}";
    create_config_file(json_string);

    GEOPM_EXPECT_THROW_MESSAGE(
        ConstConfigIOGroup iogroup(*m_default_topo, M_CONFIG_FILE_PATH, ""),
        GEOPM_ERROR_INVALID,
        "ConstConfigIOGroup::parse_config_json(): "
        "for signal \"GPU_CORE_FREQUENCY\", "
        "incorrect type for property: \"common_value\""
    );
}

TEST_F(ConstConfigIOGroupTest, input_array_value_num_domain)
{
    set_up_topo_expect_exactly({GEOPM_DOMAIN_GPU});
    std::string json_string = "{"
    "    \"GPU_CORE_FREQUENCY\": {"
    "        \"domain\": \"gpu\","
    "        \"description\": \"Provides GPU core frequency\","
    "        \"units\": \"hertz\","
    "        \"aggregation\": \"sum\","
    "        \"values\": [ 100, 200 ]"
    "    }"
    "}";
    create_config_file(json_string);

    GEOPM_EXPECT_THROW_MESSAGE(
        ConstConfigIOGroup iogroup(*m_default_topo, M_CONFIG_FILE_PATH, ""),
        GEOPM_ERROR_INVALID,
        "ConstConfigIOGroup::parse_config_json(): "
        "number of values for signal \"GPU_CORE_FREQUENCY\" "
        "does not match domain size"
    );
}

TEST_F(ConstConfigIOGroupTest, input_array_value_empty)
{
    set_up_topo_expect_exactly({});
    std::string json_string = "{"
    "    \"GPU_CORE_FREQUENCY\": {"
    "        \"domain\": \"gpu\","
    "        \"description\": \"Provides GPU core frequency\","
    "        \"units\": \"hertz\","
    "        \"aggregation\": \"sum\","
    "        \"values\": []"
    "    }"
    "}";
    create_config_file(json_string);

    GEOPM_EXPECT_THROW_MESSAGE(
        ConstConfigIOGroup iogroup(*m_default_topo, M_CONFIG_FILE_PATH, ""),
        GEOPM_ERROR_INVALID,
        "ConstConfigIOGroup::parse_config_json(): "
        "empty array of values provided for signal \"GPU_CORE_FREQUENCY\""
    );
}

TEST_F(ConstConfigIOGroupTest, valid_json_positive)
{
    set_up_topo_expect_exactly({GEOPM_DOMAIN_GPU, GEOPM_DOMAIN_CPU});
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
    create_config_file(json_string);
    ConstConfigIOGroup iogroup(*m_default_topo, M_CONFIG_FILE_PATH, "");

    EXPECT_EQ(iogroup.signal_names(), (std::set<std::string>{
        "CONST_CONFIG::CPU_CORE_FREQUENCY", "CONST_CONFIG::GPU_CORE_FREQUENCY"}));
    EXPECT_EQ(iogroup.is_valid_signal("CONST_CONFIG::CPU_CORE_FREQUENCY"), true);
    EXPECT_EQ(iogroup.is_valid_signal("CONST_CONFIG::GPU_CORE_FREQUENCY"), true);
    EXPECT_EQ(iogroup.signal_domain_type("CONST_CONFIG::CPU_CORE_FREQUENCY"), GEOPM_DOMAIN_CPU);
    EXPECT_EQ(iogroup.signal_domain_type("CONST_CONFIG::GPU_CORE_FREQUENCY"), GEOPM_DOMAIN_GPU);

    EXPECT_CALL(*m_default_topo, num_domain(GEOPM_DOMAIN_CPU))
        .Times(1)
        .RetiresOnSaturation();
    EXPECT_EQ(iogroup.push_signal("CONST_CONFIG::CPU_CORE_FREQUENCY", GEOPM_DOMAIN_CPU, 1), 0);
    EXPECT_CALL(*m_default_topo, num_domain(GEOPM_DOMAIN_GPU))
        .Times(1)
        .RetiresOnSaturation();
    EXPECT_EQ(iogroup.push_signal("CONST_CONFIG::GPU_CORE_FREQUENCY", GEOPM_DOMAIN_GPU, 2), 1);
    EXPECT_EQ(iogroup.sample(0), 1060);
    EXPECT_EQ(iogroup.sample(1), 1700);

    EXPECT_CALL(*m_default_topo, num_domain(GEOPM_DOMAIN_CPU))
        .Times(1)
        .RetiresOnSaturation();
    EXPECT_EQ(iogroup.read_signal("CONST_CONFIG::CPU_CORE_FREQUENCY", GEOPM_DOMAIN_CPU, 2), 1070);
    EXPECT_CALL(*m_default_topo, num_domain(GEOPM_DOMAIN_GPU))
        .Times(1)
        .RetiresOnSaturation();
    EXPECT_EQ(iogroup.read_signal("CONST_CONFIG::GPU_CORE_FREQUENCY", GEOPM_DOMAIN_GPU, 0), 1500);

    {
        std::function<double(const std::vector<double> &)> func =
            iogroup.agg_function("CONST_CONFIG::CPU_CORE_FREQUENCY");
        EXPECT_TRUE(is_agg_average(func));
    }
    {
        std::function<double(const std::vector<double> &)> func =
            iogroup.agg_function("CONST_CONFIG::GPU_CORE_FREQUENCY");
        EXPECT_TRUE(is_agg_sum(func));
    }
    {
        std::function<std::string(double)> func =
            iogroup.format_function("CONST_CONFIG::CPU_CORE_FREQUENCY");
        EXPECT_TRUE(is_format_double(func));
    }
    {
        std::function<std::string(double)> func =
            iogroup.format_function("CONST_CONFIG::GPU_CORE_FREQUENCY");
        EXPECT_TRUE(is_format_double(func));
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

TEST_F(ConstConfigIOGroupTest, valid_json_with_common_value_positive)
{
    std::string json_string = "{"
    "    \"GPU_CORE_FREQUENCY\": {"
    "        \"domain\": \"gpu\","
    "        \"description\": \"Provides GPU core frequency\","
    "        \"units\": \"hertz\","
    "        \"aggregation\": \"sum\","
    "        \"common_value\": 1500"
    "    }"
    "}";
    create_config_file(json_string);
    ConstConfigIOGroup iogroup(*m_default_topo, M_CONFIG_FILE_PATH, "");

    EXPECT_CALL(*m_default_topo, num_domain(GEOPM_DOMAIN_GPU))
        .Times(1)
        .RetiresOnSaturation();
    EXPECT_EQ(iogroup.read_signal("CONST_CONFIG::GPU_CORE_FREQUENCY", GEOPM_DOMAIN_GPU, 0), 1500);
    EXPECT_CALL(*m_default_topo, num_domain(GEOPM_DOMAIN_GPU))
        .Times(1)
        .RetiresOnSaturation();
    EXPECT_EQ(iogroup.read_signal("CONST_CONFIG::GPU_CORE_FREQUENCY", GEOPM_DOMAIN_GPU, 2), 1500);

    EXPECT_CALL(*m_default_topo, num_domain(GEOPM_DOMAIN_GPU))
        .Times(1)
        .RetiresOnSaturation();
    EXPECT_EQ(iogroup.push_signal("CONST_CONFIG::GPU_CORE_FREQUENCY", GEOPM_DOMAIN_GPU, 0), 0);
    EXPECT_CALL(*m_default_topo, num_domain(GEOPM_DOMAIN_GPU))
        .Times(1)
        .RetiresOnSaturation();
    EXPECT_EQ(iogroup.push_signal("CONST_CONFIG::GPU_CORE_FREQUENCY", GEOPM_DOMAIN_GPU, 2), 1);
    EXPECT_EQ(iogroup.sample(0), 1500);
    EXPECT_EQ(iogroup.sample(1), 1500);
}

TEST_F(ConstConfigIOGroupTest, valid_json_negative)
{
    set_up_topo_expect_exactly({GEOPM_DOMAIN_GPU});
    std::string json_string = "{"
    "    \"GPU_CORE_FREQUENCY\": {"
    "        \"domain\": \"gpu\","
    "        \"description\": \"Provides GPU core frequency\","
    "        \"units\": \"hertz\","
    "        \"aggregation\": \"sum\","
    "        \"values\": [ 1500, 1600, 1700 ]"
    "    }"
    "}";
    create_config_file(json_string);
    ConstConfigIOGroup iogroup(*m_default_topo, M_CONFIG_FILE_PATH, "");

    EXPECT_EQ(iogroup.control_names(), std::set<std::string>{});
    EXPECT_EQ(iogroup.is_valid_signal("CONST_CONFIG::UNCORE_RATIO_LIMIT:MIN_RATIO"), false);
    EXPECT_EQ(iogroup.is_valid_control("CONST_CONFIG::UNCORE_RATIO_LIMIT:MIN_RATIO"), false);
    EXPECT_EQ(iogroup.is_valid_control("CONST_CONFIG::GPU_CORE_FREQUENCY"), false);
    EXPECT_EQ(iogroup.signal_domain_type("CONST_CONFIG::UNCORE_RATIO_LIMIT:MIN_RATIO"), GEOPM_DOMAIN_INVALID);
    EXPECT_EQ(iogroup.control_domain_type("CONST_CONFIG::UNCORE_RATIO_LIMIT:MIN_RATIO"), GEOPM_DOMAIN_INVALID);
    EXPECT_EQ(iogroup.control_domain_type("CONST_CONFIG::GPU_CORE_FREQUENCY"), GEOPM_DOMAIN_INVALID);
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

    EXPECT_CALL(*m_default_topo, num_domain(GEOPM_DOMAIN_GPU))
        .Times(AtMost(1))
        .RetiresOnSaturation();
    GEOPM_EXPECT_THROW_MESSAGE(
        iogroup.push_signal("CONST_CONFIG::GPU_CORE_FREQUENCY", GEOPM_DOMAIN_GPU, -1),
        GEOPM_ERROR_INVALID,
        "ConstConfigIOGroup::push_signal(): domain_idx "
        "-1" " out of range."
    );
    EXPECT_CALL(*m_default_topo, num_domain(GEOPM_DOMAIN_GPU))
        .Times(AtMost(1))
        .RetiresOnSaturation();
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
        iogroup.push_control("CONST_CONFIG::GPU_CORE_FREQUENCY", 0, 0),
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

    EXPECT_CALL(*m_default_topo, num_domain(GEOPM_DOMAIN_GPU))
        .Times(AtMost(1))
        .RetiresOnSaturation();
    GEOPM_EXPECT_THROW_MESSAGE(
        iogroup.read_signal("CONST_CONFIG::GPU_CORE_FREQUENCY", GEOPM_DOMAIN_GPU, -1),
        GEOPM_ERROR_INVALID,
        "ConstConfigIOGroup::read_signal(): domain_idx "
        "-1" " out of range."
    );
    EXPECT_CALL(*m_default_topo, num_domain(GEOPM_DOMAIN_GPU))
        .Times(AtMost(1))
        .RetiresOnSaturation();
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
        iogroup.write_control("CONST_CONFIG::GPU_CORE_FREQUENCY", 0, 0, 3.14),
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
        iogroup.control_description("CONST_CONFIG::GPU_CORE_FREQUENCY"),
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

TEST_F(ConstConfigIOGroupTest, valid_json_with_common_value_negative)
{
    std::string json_string = "{"
    "    \"GPU_CORE_FREQUENCY\": {"
    "        \"domain\": \"gpu\","
    "        \"description\": \"Provides GPU core frequency\","
    "        \"units\": \"hertz\","
    "        \"aggregation\": \"sum\","
    "        \"common_value\": 1500"
    "    }"
    "}";
    create_config_file(json_string);
    ConstConfigIOGroup iogroup(*m_default_topo, M_CONFIG_FILE_PATH, "");

    EXPECT_CALL(*m_default_topo, num_domain(GEOPM_DOMAIN_GPU))
        .Times(AtMost(1))
        .RetiresOnSaturation();
    GEOPM_EXPECT_THROW_MESSAGE(
        iogroup.read_signal("CONST_CONFIG::GPU_CORE_FREQUENCY", GEOPM_DOMAIN_GPU, -1),
        GEOPM_ERROR_INVALID,
        "ConstConfigIOGroup::read_signal(): domain_idx "
        "-1" " out of range."
    );
    EXPECT_CALL(*m_default_topo, num_domain(GEOPM_DOMAIN_GPU))
        .Times(AtMost(1))
        .RetiresOnSaturation();
    GEOPM_EXPECT_THROW_MESSAGE(
        iogroup.read_signal("CONST_CONFIG::GPU_CORE_FREQUENCY", GEOPM_DOMAIN_GPU, 3),
        GEOPM_ERROR_INVALID,
        "ConstConfigIOGroup::read_signal(): domain_idx "
        "3" " out of range."
    );
    
    EXPECT_CALL(*m_default_topo, num_domain(GEOPM_DOMAIN_GPU))
        .Times(AtMost(1))
        .RetiresOnSaturation();
    GEOPM_EXPECT_THROW_MESSAGE(
        iogroup.push_signal("CONST_CONFIG::GPU_CORE_FREQUENCY", GEOPM_DOMAIN_GPU, -1),
        GEOPM_ERROR_INVALID,
        "ConstConfigIOGroup::push_signal(): domain_idx "
        "-1" " out of range."
    );
    EXPECT_CALL(*m_default_topo, num_domain(GEOPM_DOMAIN_GPU))
        .Times(AtMost(1))
        .RetiresOnSaturation();
    GEOPM_EXPECT_THROW_MESSAGE(
        iogroup.push_signal("CONST_CONFIG::GPU_CORE_FREQUENCY", GEOPM_DOMAIN_GPU, 3),
        GEOPM_ERROR_INVALID,
        "ConstConfigIOGroup::push_signal(): domain_idx "
        "3" " out of range."
    );
}

TEST_F(ConstConfigIOGroupTest, loads_default_config)
{
    EXPECT_CALL(*m_default_topo, num_domain(_)).Times(0);
    EXPECT_CALL(*m_default_topo, num_domain(GEOPM_DOMAIN_GPU))
        .Times(2)
        .RetiresOnSaturation();
    std::string json_string = "{"
    "    \"GPU_CORE_FREQUENCY\": {"
    "        \"domain\": \"gpu\","
    "        \"description\": \"Provides GPU core frequency\","
    "        \"units\": \"hertz\","
    "        \"aggregation\": \"sum\","
    "        \"values\": [ 1500, 1600, 1700 ]"
    "    }"
    "}";
    create_config_file(json_string);

    ConstConfigIOGroup iogroup1(*m_default_topo, "", M_CONFIG_FILE_PATH);
    EXPECT_EQ(iogroup1.signal_names(), std::set<std::string>{"CONST_CONFIG::GPU_CORE_FREQUENCY"});
    
    ConstConfigIOGroup iogroup2(*m_default_topo, "/fake_dir/fake_config.json", M_CONFIG_FILE_PATH);
    EXPECT_EQ(iogroup2.signal_names(), std::set<std::string>{"CONST_CONFIG::GPU_CORE_FREQUENCY"});
}

TEST_F(ConstConfigIOGroupTest, no_default_config)
{
    set_up_topo_expect_exactly({});
    std::string file_path = "/fake_dir/fake_config.json";
    GEOPM_EXPECT_THROW_MESSAGE(ConstConfigIOGroup iogroup(*m_default_topo, "", file_path),
                               ENOENT,
                               "file \"" + file_path + "\" could not be opened");
}
