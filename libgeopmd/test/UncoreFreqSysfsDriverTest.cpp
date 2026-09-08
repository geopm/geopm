/*
 * Copyright (c) 2015 - 2025 Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "UncoreFreqSysfsDriver.hpp"

#include <sys/stat.h>
#include <unistd.h>

#include <cmath>
#include <memory>
#include <set>
#include <stdexcept>
#include <string>
#include <vector>

#include "gmock/gmock.h"
#include "gtest/gtest.h"

#include "geopm/Helper.hpp"
#include "geopm/PlatformTopo.hpp"
#include "geopm_topo.h"

using geopm::UncoreFreqSysfsDriver;
using geopm::SysfsDriver;

class UncoreFreqFakeDirManager
{
    public:
        UncoreFreqFakeDirManager(std::string base_path_template)
        {
            if (mkdtemp(&base_path_template[0]) == nullptr) {
                throw std::runtime_error("Could not create a temporary directory at " + base_path_template);
            }
            m_base_dir_path = std::string(base_path_template);
            m_created_dirs.push_back(m_base_dir_path);

            // Single-package CWF-like tree: one package_*_die_* fan-out
            // control directory plus two uncore* domain directories.
            std::string package_dir = m_base_dir_path + "/package_00_die_00";
            make_dir(package_dir);
            write_file(package_dir + "/max_freq_khz", "2200000");
            write_file(package_dir + "/min_freq_khz", "800000");
            write_file(package_dir + "/initial_max_freq_khz", "2400000");
            write_file(package_dir + "/initial_min_freq_khz", "800000");

            // uncore00 has the lower domain_id, so it is the
            // representative status directory for package 0.
            std::string uncore0_dir = m_base_dir_path + "/uncore00";
            make_dir(uncore0_dir);
            write_file(uncore0_dir + "/package_id", "0");
            write_file(uncore0_dir + "/domain_id", "0");
            write_file(uncore0_dir + "/current_freq_khz", "2100000");

            std::string uncore1_dir = m_base_dir_path + "/uncore01";
            make_dir(uncore1_dir);
            write_file(uncore1_dir + "/package_id", "0");
            write_file(uncore1_dir + "/domain_id", "1");
            write_file(uncore1_dir + "/current_freq_khz", "2000000");
        }

        ~UncoreFreqFakeDirManager()
        {
            cleanup();
        }

        std::string get_driver_dir() const
        {
            return m_base_dir_path;
        }

    private:
        void make_dir(const std::string &dir_path)
        {
            if (mkdir(dir_path.c_str(), 0755) == -1) {
                cleanup();
                throw std::runtime_error("Could not create directory at " + dir_path);
            }
            m_created_dirs.push_back(dir_path);
        }

        void write_file(const std::string &file_path, const std::string &contents)
        {
            geopm::write_file(file_path, contents);
            m_created_files.insert(file_path);
        }

        void cleanup()
        {
            for (const auto &file_path : m_created_files) {
                unlink(file_path.c_str());
            }
            for (auto it = m_created_dirs.rbegin(); it != m_created_dirs.rend(); ++it) {
                rmdir(it->c_str());
            }
        }

        std::vector<std::string> m_created_dirs;
        std::set<std::string> m_created_files;
        std::string m_base_dir_path;
};

class UncoreFreqSysfsDriverTest : public ::testing::Test
{
    protected:
        void SetUp();
        std::unique_ptr<UncoreFreqFakeDirManager> m_dir_manager;
        std::unique_ptr<SysfsDriver> m_driver;
        std::map<std::string, SysfsDriver::properties_s> m_driver_properties;
};

void UncoreFreqSysfsDriverTest::SetUp()
{
    m_dir_manager = std::make_unique<UncoreFreqFakeDirManager>("/tmp/UncoreFreqSysfsDriverTest_XXXXXX");
    m_driver = std::make_unique<UncoreFreqSysfsDriver>(m_dir_manager->get_driver_dir());
    m_driver_properties = m_driver->properties();
}

TEST_F(UncoreFreqSysfsDriverTest, driver_and_plugin_name_match)
{
    EXPECT_EQ("UNCORE", m_driver->driver())
        << "Driver name should match the plugin name";
    EXPECT_EQ("UNCORE", UncoreFreqSysfsDriver::plugin_name())
        << "Plugin name should be UNCORE";
}

TEST_F(UncoreFreqSysfsDriverTest, domain_type_is_package)
{
    EXPECT_EQ(GEOPM_DOMAIN_PACKAGE, m_driver->domain_type("UNCORE::MAX_FREQ_KHZ"))
        << "Domain type for MAX_FREQ_KHZ should be PACKAGE";
    EXPECT_EQ(GEOPM_DOMAIN_PACKAGE, m_driver->domain_type("UNCORE::CURRENT_FREQ_KHZ"))
        << "Domain type for CURRENT_FREQ_KHZ should be PACKAGE";
    EXPECT_THROW(
        m_driver->domain_type("INVALID_SIGNAL"),
        geopm::Exception
    ) << "Should throw an exception for an invalid signal name";
}

TEST_F(UncoreFreqSysfsDriverTest, control_attribute_path_uses_package_die_dir)
{
    std::string max_path = m_driver->attribute_path("UNCORE::MAX_FREQ_KHZ", 0);
    EXPECT_EQ(m_dir_manager->get_driver_dir() + "/package_00_die_00/max_freq_khz", max_path)
        << "Control attributes should map to the package_*_die_* fan-out directory";

    std::string min_path = m_driver->attribute_path("UNCORE::MIN_FREQ_KHZ", 0);
    EXPECT_EQ(m_dir_manager->get_driver_dir() + "/package_00_die_00/min_freq_khz", min_path);

    std::string initial_path = m_driver->attribute_path("UNCORE::INITIAL_MAX_FREQ_KHZ", 0);
    EXPECT_EQ(m_dir_manager->get_driver_dir() + "/package_00_die_00/initial_max_freq_khz", initial_path);
}

TEST_F(UncoreFreqSysfsDriverTest, status_attribute_path_uses_lowest_domain_uncore_dir)
{
    std::string status_path = m_driver->attribute_path("UNCORE::CURRENT_FREQ_KHZ", 0);
    EXPECT_EQ(m_dir_manager->get_driver_dir() + "/uncore00/current_freq_khz", status_path)
        << "Status attribute should map to the uncore* domain with the lowest domain_id";
}

TEST_F(UncoreFreqSysfsDriverTest, attribute_path_invalid_cases)
{
    EXPECT_THROW(
        m_driver->attribute_path("INVALID_SIGNAL", 0),
        geopm::Exception
    ) << "Should throw an exception for an invalid signal name";

    EXPECT_THROW(
        m_driver->attribute_path("UNCORE::MAX_FREQ_KHZ", 1),
        geopm::Exception
    ) << "Should throw an exception for a domain index that does not exist";
}

TEST_F(UncoreFreqSysfsDriverTest, signal_parse_scales_khz_to_hertz)
{
    auto parse_function = m_driver->signal_parse("UNCORE::MAX_FREQ_KHZ");

    EXPECT_DOUBLE_EQ(2.2e9, parse_function("2200000"))
        << "Signal parse should scale kHz to Hz";

    EXPECT_TRUE(std::isnan(parse_function("INVALID_VALUE")))
        << "Signal parse should return NaN for invalid input";

    EXPECT_TRUE(std::isnan(parse_function("")))
        << "Signal parse should return NaN for empty input";

    EXPECT_THROW(
        m_driver->signal_parse("INVALID_SIGNAL"),
        geopm::Exception
    ) << "Should throw an exception for an invalid signal name";
}

TEST_F(UncoreFreqSysfsDriverTest, control_gen_scales_hertz_to_khz)
{
    auto control_function = m_driver->control_gen("UNCORE::MAX_FREQ_KHZ");

    EXPECT_EQ("2200000", control_function(2.2e9))
        << "Control generation should scale Hz to kHz";

    EXPECT_EQ("0", control_function(0.0))
        << "Control generation should handle zero value correctly";

    EXPECT_THROW(
        m_driver->control_gen("INVALID_CONTROL"),
        geopm::Exception
    ) << "Should throw an exception for an invalid control name";
}

TEST_F(UncoreFreqSysfsDriverTest, properties_have_expected_aliases_and_writeability)
{
    ASSERT_FALSE(m_driver_properties.empty()) << "Properties should not be empty";

    const auto &max_prop = m_driver_properties.at("UNCORE::MAX_FREQ_KHZ");
    EXPECT_TRUE(max_prop.is_writable) << "MAX_FREQ_KHZ should be writeable";
    EXPECT_EQ("CPU_UNCORE_FREQUENCY_MAX_CONTROL", max_prop.alias);

    const auto &min_prop = m_driver_properties.at("UNCORE::MIN_FREQ_KHZ");
    EXPECT_TRUE(min_prop.is_writable) << "MIN_FREQ_KHZ should be writeable";
    EXPECT_EQ("CPU_UNCORE_FREQUENCY_MIN_CONTROL", min_prop.alias);

    const auto &current_prop = m_driver_properties.at("UNCORE::CURRENT_FREQ_KHZ");
    EXPECT_FALSE(current_prop.is_writable) << "CURRENT_FREQ_KHZ should be read-only";
    EXPECT_EQ("CPU_UNCORE_FREQUENCY_STATUS", current_prop.alias);

    const auto &initial_max_prop = m_driver_properties.at("UNCORE::INITIAL_MAX_FREQ_KHZ");
    EXPECT_FALSE(initial_max_prop.is_writable) << "INITIAL_MAX_FREQ_KHZ should be read-only";

    for (const auto &property : m_driver_properties) {
        EXPECT_FALSE(property.second.attribute.empty())
            << "Attribute should not be empty for property: " << property.first;
        EXPECT_GT(property.second.scaling_factor, 0.0)
            << "Scaling factor should be positive for property: " << property.first;
    }
}

TEST_F(UncoreFreqSysfsDriverTest, missing_directory_throws)
{
    EXPECT_THROW(
        UncoreFreqSysfsDriver("/tmp/UncoreFreqSysfsDriverTest_does_not_exist_XXXXXX"),
        geopm::Exception
    ) << "Constructing against an absent sysfs tree should throw (pre-TPMI hardware)";
}
