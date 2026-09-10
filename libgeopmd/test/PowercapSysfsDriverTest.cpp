/*
 * Copyright (c) 2015 - 2025 Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "PowercapSysfsDriver.hpp"

#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

#include <algorithm>
#include <array>
#include <cstring>
#include <memory>

#include "gmock/gmock.h"
#include "gtest/gtest.h"

#include "geopm/Helper.hpp"
#include "geopm/IOGroup.hpp"
#include "geopm/PlatformTopo.hpp"

using geopm::PowercapSysfsDriver;
using geopm::IOGroup;
using geopm::SysfsDriver;

class PowercapFakeDirManager
{
    public:
        PowercapFakeDirManager(std::string base_path_template)
        {
            if (mkdtemp(&base_path_template[0]) == nullptr) {
                throw std::runtime_error("Could not create a temporary directory at " + base_path_template);
            }
            m_base_dir_path = std::string(base_path_template);
            m_created_dirs.push_back(m_base_dir_path);

            // Create fake powercap directories for a dual-socket Xeon system
            for (int socket = 0; socket < 2; ++socket) {
                std::string package_dir = m_base_dir_path + "/intel-rapl:" + std::to_string(socket);
                if (mkdir(package_dir.c_str(), 0755) == -1) {
                    cleanup();
                    throw std::runtime_error("Could not create directory at " + package_dir);
                }
                m_created_dirs.push_back(package_dir);

                // Create "name" file
                write_file(package_dir + "/name", "package-" + std::to_string(socket));
                write_file(package_dir + "/max_energy_range_uj", "262143328850");
                // Factory power ceilings for the long- and short-term constraints
                // (flat attribute files, as exposed by the kernel powercap driver).
                write_file(package_dir + "/constraint_0_max_power_uw", "400000000");
                write_file(package_dir + "/constraint_1_max_power_uw", "540000000");

                // Create constraint directory and files
                std::string constraint_dir = package_dir + "/constraint_0";
                if (mkdir(constraint_dir.c_str(), 0755) == -1) {
                    cleanup();
                    throw std::runtime_error("Could not create directory at " + constraint_dir);
                }
                m_created_dirs.push_back(constraint_dir);

                write_file(constraint_dir + "/power_limit_uw", "1500000");
                write_file(constraint_dir + "/time_window_us", "1000000");
            }
        }

        ~PowercapFakeDirManager()
        {
            cleanup();
        }

        std::string get_driver_dir() const
        {
            return m_base_dir_path;
        }

    private:
        void write_file(const std::string &file_path, const std::string &contents)
        {
            geopm::write_file(file_path, contents);
            m_created_powercap_files.insert(file_path);
        }

        void cleanup()
        {
            for (const auto &file_path : m_created_powercap_files) {
                unlink(file_path.c_str());
            }
            for (auto it = m_created_dirs.rbegin(); it != m_created_dirs.rend(); ++it) {
                rmdir(it->c_str());
            }
        }

        std::vector<std::string> m_created_dirs;
        std::set<std::string> m_created_powercap_files;
        std::string m_base_dir_path;
};

class PowercapSysfsDriverTest : public ::testing::Test
{
    protected:
        void SetUp();
        std::unique_ptr<PowercapFakeDirManager> m_dir_manager;
        std::unique_ptr<SysfsDriver> m_driver;
        std::map<std::string, SysfsDriver::properties_s> m_driver_properties;
};

void PowercapSysfsDriverTest::SetUp()
{
    m_dir_manager = std::make_unique<PowercapFakeDirManager>("/tmp/PowercapsysfsDriverTest_XXXXXX");
    m_driver = std::make_unique<PowercapSysfsDriver>(m_dir_manager->get_driver_dir());
    m_driver_properties = m_driver->properties();
}

TEST_F(PowercapSysfsDriverTest, driver_and_plugin_name_match)
{
    EXPECT_EQ("POWERCAP", m_driver->driver())
        << "Driver name should match the plugin name";
    EXPECT_EQ("POWERCAP", PowercapSysfsDriver::plugin_name())
        << "Plugin name should be POWERCAP";
}

TEST_F(PowercapSysfsDriverTest, domain_type_is_correct)
{
    EXPECT_EQ(GEOPM_DOMAIN_PACKAGE, m_driver->domain_type("POWERCAP::CPU_POWER_LIMIT"))
        << "Domain type for CPU_POWER_LIMIT should be PACKAGE";
    EXPECT_THROW(
        m_driver->domain_type("INVALID_SIGNAL"),
        geopm::Exception
    ) << "Should throw an exception for an invalid signal name";
}

TEST_F(PowercapSysfsDriverTest, attribute_path_valid_and_invalid_cases)
{
    std::string valid_path = m_driver->attribute_path("POWERCAP::CPU_POWER_LIMIT", 0);
    EXPECT_FALSE(valid_path.empty()) << "Attribute path for a valid signal should not be empty";

    EXPECT_THROW(
        m_driver->attribute_path("INVALID_SIGNAL", 0),
        geopm::Exception
    ) << "Should throw an exception for an invalid signal name";

    EXPECT_THROW(
        m_driver->attribute_path("POWERCAP::CPU_POWER_LIMIT", -1),
        geopm::Exception
    ) << "Should throw an exception for an invalid domain index";
}

TEST_F(PowercapSysfsDriverTest, signal_parse_valid_and_invalid_cases)
{
    auto parse_function = m_driver->signal_parse("POWERCAP::CPU_POWER_LIMIT");

    EXPECT_DOUBLE_EQ(1.5, parse_function("1500000"))
        << "Signal parse should correctly scale the value";

    EXPECT_TRUE(std::isnan(parse_function("INVALID_VALUE")))
        << "Signal parse should return NaN for invalid input";

    EXPECT_TRUE(std::isnan(parse_function("")))
        << "Signal parse should return NaN for empty input";

    EXPECT_THROW(
        m_driver->signal_parse("INVALID_SIGNAL"),
        geopm::Exception
    ) << "Should throw an exception for an invalid signal name";
}

TEST_F(PowercapSysfsDriverTest, control_gen_valid_and_invalid_cases)
{
    auto control_function = m_driver->control_gen("POWERCAP::CPU_POWER_LIMIT");

    EXPECT_EQ("1500000", control_function(1.5))
        << "Control generation should correctly scale the value";

    EXPECT_EQ("0", control_function(0.0))
        << "Control generation should handle zero value correctly";

    EXPECT_EQ("-1500000", control_function(-1.5))
        << "Control generation should handle negative values correctly";

    EXPECT_THROW(
        m_driver->control_gen("INVALID_CONTROL"),
        geopm::Exception
    ) << "Should throw an exception for an invalid control name";
}

TEST_F(PowercapSysfsDriverTest, properties_are_loaded_correctly)
{
    auto properties = m_driver->properties();
    EXPECT_FALSE(properties.empty()) << "Properties should not be empty";

    for (const auto &property : properties) {
        EXPECT_FALSE(property.first.empty()) << "Property name should not be empty";
        EXPECT_FALSE(property.second.attribute.empty()) << "Attribute should not be empty for property: " << property.first;
        EXPECT_GT(property.second.scaling_factor, 0.0) << "Scaling factor should be positive for property: " << property.first;
    }
}

TEST_F(PowercapSysfsDriverTest, max_power_signals_map_to_constraint_attributes)
{
    std::string pl1_path = m_driver->attribute_path("POWERCAP::CPU_CONSTRAINT_0_MAX_POWER", 0);
    EXPECT_THAT(pl1_path, ::testing::EndsWith("/intel-rapl:0/constraint_0_max_power_uw"))
        << "CPU_CONSTRAINT_0_MAX_POWER should map to the long-term max_power sysfs file";

    std::string pl2_path = m_driver->attribute_path("POWERCAP::CPU_CONSTRAINT_1_MAX_POWER", 1);
    EXPECT_THAT(pl2_path, ::testing::EndsWith("/intel-rapl:1/constraint_1_max_power_uw"))
        << "CPU_CONSTRAINT_1_MAX_POWER should map to the short-term max_power sysfs file";

    auto pl1_parse = m_driver->signal_parse("POWERCAP::CPU_CONSTRAINT_0_MAX_POWER");
    EXPECT_DOUBLE_EQ(400.0, pl1_parse("400000000"))
        << "Max-power signal should scale microwatts to watts";
}

TEST_F(PowercapSysfsDriverTest, constraint_0_max_power_aliases_cpu_power_max_avail)
{
    auto prop_it = m_driver_properties.find("POWERCAP::CPU_CONSTRAINT_0_MAX_POWER");
    ASSERT_NE(m_driver_properties.end(), prop_it)
        << "CPU_CONSTRAINT_0_MAX_POWER property should be loaded";
    EXPECT_EQ("CPU_POWER_MAX_AVAIL", prop_it->second.alias)
        << "Long-term max power should alias the high-level CPU_POWER_MAX_AVAIL signal";
    EXPECT_FALSE(prop_it->second.is_writable)
        << "Factory power ceiling is read-only";

    auto pl2_it = m_driver_properties.find("POWERCAP::CPU_CONSTRAINT_1_MAX_POWER");
    ASSERT_NE(m_driver_properties.end(), pl2_it)
        << "CPU_CONSTRAINT_1_MAX_POWER property should be loaded";
    EXPECT_EQ("", pl2_it->second.alias)
        << "Short-term max power should not carry a high-level alias";
}
