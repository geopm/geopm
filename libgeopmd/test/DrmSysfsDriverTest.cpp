/*
 * Copyright (c) 2015 - 2024 Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "DrmSysfsDriver.hpp"

#include <fcntl.h>
#include <sstream>
#include <sys/stat.h>
#include <system_error>
#include <unistd.h>

#include <algorithm>
#include <array>
#include <cstring>
#include <memory>

#include "gmock/gmock.h"
#include "gtest/gtest.h"

#include "geopm/Helper.hpp"
#include "geopm/IOGroup.hpp"
#include "geopm_topo.h"

#include "test/MockPlatformTopo.hpp"

using geopm::DrmSysfsDriver;
using geopm::IOGroup;
using geopm::SysfsDriver;
using testing::Return;
using testing::Matches;
using testing::StartsWith;
using testing::EndsWith;
using testing::Not;
using testing::ResultOf;
using testing::AllOf;
using testing::AnyOf;
using testing::Eq;

class DrmFakeDirManager
{
    public:
        DrmFakeDirManager(std::string base_path_template)
        {
            if (mkdtemp(&base_path_template[0]) == nullptr) {
                throw std::runtime_error("Could not create a temporary directory at " + base_path_template);
            }
            m_base_dir_path = std::string(base_path_template);
            m_created_dirs.push_back(m_base_dir_path);

            auto meaningless_dir_path = m_base_dir_path + "/something_else";

            if (mkdir(meaningless_dir_path.c_str(), 0755) == -1) {
                rmdir(m_base_dir_path.c_str());
                throw std::runtime_error("Could not create directory at " + meaningless_dir_path);
            }
            m_created_dirs.push_back(meaningless_dir_path);
        }

        ~DrmFakeDirManager()
        {
            for (const auto &file_path : m_created_files) {
                unlink(file_path.c_str());
            }
            // Clean up directories we created in reverse order so
            // any attempted directory removals are on empty directories
            for (auto it = m_created_dirs.rbegin(); it != m_created_dirs.rend(); ++it) {
                rmdir(it->c_str());
            }
        }

        void create_card(int card_idx) {
            std::ostringstream oss;
            oss << m_base_dir_path << "/card" << card_idx;
            auto new_path = oss.str();
            if (mkdir(new_path.c_str(), 0755) == -1) {
                throw std::system_error(errno, std::generic_category(), "Could not create directory at " + new_path);
            }
            m_created_dirs.push_back(new_path);
            oss << "/gt";
            new_path = oss.str();
            if (mkdir(new_path.c_str(), 0755) == -1) {
                throw std::system_error(errno, std::generic_category(), "Could not create directory at " + new_path);
            }
            m_created_dirs.push_back(new_path);
        }

        void create_card_hwmon(int card_idx, int hwmon_idx) {
            std::ostringstream oss;
            oss << m_base_dir_path << "/card" << card_idx << "/device";
            auto new_path = oss.str();
            errno = 0;
            if (mkdir(new_path.c_str(), 0755) == -1 && errno != EEXIST) {
                throw std::system_error(errno, std::generic_category(), "Could not create directory at " + new_path);
            }
            if (errno != EEXIST) {
                m_created_dirs.push_back(new_path);
            }
            oss << "/hwmon";
            new_path = oss.str();
            errno = 0;
            if (mkdir(new_path.c_str(), 0755) == -1 && errno != EEXIST) {
                throw std::system_error(errno, std::generic_category(), "Could not create directory at " + new_path);
            }
            if (errno != EEXIST) {
                m_created_dirs.push_back(new_path);
            }
            oss << "/hwmon" << hwmon_idx;
            new_path = oss.str();
            if (mkdir(new_path.c_str(), 0755) == -1) {
                throw std::system_error(errno, std::generic_category(), "Could not create directory at " + new_path);
            }
            m_created_dirs.push_back(new_path);
        }

        void create_tile_in_card(int card_idx, int tile_idx) {
            std::ostringstream oss;
            oss << m_base_dir_path << "/card" << card_idx << "/gt/gt" << tile_idx;
            auto tile_path = oss.str();
            if (mkdir(tile_path.c_str(), 0755) == -1) {
                throw std::system_error(errno, std::generic_category(), "Could not create directory at " + tile_path);
            }
            m_created_dirs.push_back(tile_path);
        }

        void write_file_in_card_tile(int card_idx, int tile_idx, const std::string &file_name, const std::string &contents)
        {
            std::ostringstream oss;
            oss << m_base_dir_path << "/card" << card_idx << "/gt/gt" << tile_idx << "/" << file_name;
            auto file_path = oss.str();
            geopm::write_file(file_path, contents);
            m_created_files.insert(file_path);
        }

        void write_hwmon_name_and_attribute(int card_index, int hwmon_index, const std::string &name,
                                            const std::string &attribute, const std::string &contents)
        {
            std::ostringstream oss;
            oss << m_base_dir_path << "/card" << card_index << "/device/hwmon/hwmon" << hwmon_index;
            auto hwmon_dir_path = oss.str();
            geopm::write_file(hwmon_dir_path + "/name", name);
            m_created_files.insert(hwmon_dir_path + "/name");
            geopm::write_file(hwmon_dir_path + "/" + attribute, contents);
            m_created_files.insert(hwmon_dir_path + "/" + attribute);
        }

        std::string get_driver_dir() const
        {
            return m_base_dir_path;
        }

    private:
        std::vector<std::string> m_created_dirs;
        std::set<std::string> m_created_files;
        std::string m_base_dir_path;
};

class DrmSysfsDriverTest : public ::testing::Test
{
    protected:
        void SetUp();
        std::shared_ptr<MockPlatformTopo> m_topo;
        std::unique_ptr<DrmFakeDirManager> m_dir_manager;
        std::unique_ptr<SysfsDriver> m_driver;
        std::map<std::string, SysfsDriver::properties_s> m_driver_properties;
};

void DrmSysfsDriverTest::SetUp()
{
    m_topo = make_topo(1 /*packages*/, 2 /*cores*/, 4 /*cpus*/);
    ON_CALL(*m_topo, num_domain(GEOPM_DOMAIN_GPU)).WillByDefault(Return(1));
    ON_CALL(*m_topo, num_domain(GEOPM_DOMAIN_GPU_CHIP)).WillByDefault(Return(2));

    m_dir_manager = std::make_unique<DrmFakeDirManager>("/tmp/DrmsysfsDriverTest_XXXXXX");
    m_dir_manager->create_card(0);
    m_dir_manager->create_tile_in_card(0, 0);
    m_dir_manager->create_tile_in_card(0, 1);
    m_dir_manager->write_file_in_card_tile(0, 0, "rps_cur_freq_mhz", "1234");
    m_dir_manager->write_file_in_card_tile(0, 1, "rps_cur_freq_mhz", "2345");
    m_dir_manager->write_file_in_card_tile(0, 0, "rps_act_freq_mhz", "1230");
    m_dir_manager->write_file_in_card_tile(0, 1, "rps_act_freq_mhz", "2340");
    m_driver = std::make_unique<DrmSysfsDriver>(*m_topo, m_dir_manager->get_driver_dir(), "TEST_DRIVER_PREFIX");
    m_driver_properties = m_driver->properties();
}

TEST_F(DrmSysfsDriverTest, iogroup_plugin_name_matches_driver_name)
{
    EXPECT_EQ("TEST_DRIVER_PREFIX", m_driver->driver());
    EXPECT_EQ("DRM", DrmSysfsDriver::plugin_name_drm());
    EXPECT_EQ("ACCEL", DrmSysfsDriver::plugin_name_accel());
}

TEST_F(DrmSysfsDriverTest, domain_type)
{
    m_driver = std::make_unique<DrmSysfsDriver>(*m_topo, m_dir_manager->get_driver_dir(), "TEST_DRIVER_PREFIX");
    for (const auto &attribute_properties : m_driver->properties()) {
        auto domain_type = [this](const std::string &s) { return m_driver->domain_type(s); };
        EXPECT_THAT(attribute_properties.first,
                    AnyOf(AllOf(StartsWith("TEST_DRIVER_PREFIX::HWMON::"), Not(EndsWith("::GPU_CHIP")), Not(EndsWith("::GPU")), ResultOf("domain type", domain_type, Eq(GEOPM_DOMAIN_GPU))),
                          AllOf(Not(StartsWith("TEST_DRIVER_PREFIX::HWMON::")), Not(EndsWith("::GPU_CHIP")), Not(EndsWith("::GPU")), ResultOf("domain type", domain_type, Eq(GEOPM_DOMAIN_GPU_CHIP))),
                          AllOf(EndsWith("::GPU_CHIP"), ResultOf("domain type", domain_type, Eq(GEOPM_DOMAIN_GPU_CHIP))),
                          AllOf(EndsWith("::GPU"), ResultOf("domain type", domain_type, Eq(GEOPM_DOMAIN_GPU)))));
    }
}

TEST_F(DrmSysfsDriverTest, attribute_path)
{
    EXPECT_EQ(m_dir_manager->get_driver_dir() + "/card0/gt/gt0/rps_cur_freq_mhz",
              m_driver->attribute_path("TEST_DRIVER_PREFIX::RPS_CUR_FREQ", 0))
        << "Should successfully get a path for an attribute that exists";
    EXPECT_THROW(m_driver->attribute_path("TEST_DRIVER_PREFIX::A_MADE_UP_ATTRIBUTE_NAME", 0), geopm::Exception)
        << "Should fail to get a path for an attribute that does not exist";
    EXPECT_THROW(m_driver->attribute_path("TEST_DRIVER_PREFIX::RPS_CUR_FREQ", 12345), geopm::Exception)
        << "Should fail to get a path for an attribute at a domain that does not exist";
}

TEST_F(DrmSysfsDriverTest, hwmon_attribute_paths)
{
    m_dir_manager->create_card_hwmon(0, 123);
    m_dir_manager->write_hwmon_name_and_attribute(0, 123, "i915\n", "curr1_crit", "12125");

    // Try a few on card 1 for more coverage of multi-card/multi-tile enumeration
    ON_CALL(*m_topo, num_domain(GEOPM_DOMAIN_GPU)).WillByDefault(Return(2));
    ON_CALL(*m_topo, num_domain(GEOPM_DOMAIN_GPU_CHIP)).WillByDefault(Return(4));
    m_dir_manager->create_card(1);
    m_dir_manager->create_tile_in_card(1, 0);
    m_dir_manager->create_tile_in_card(1, 1);
    m_dir_manager->create_card_hwmon(1, 45);
    m_dir_manager->create_card_hwmon(1, 6);
    m_dir_manager->create_card_hwmon(1, 7);
    m_dir_manager->write_hwmon_name_and_attribute(1, 45, "i915_gt0\n", "energy1_input", "123456");
    m_dir_manager->write_hwmon_name_and_attribute(1, 6, "i915_gt1\n", "energy1_input", "234567");
    m_dir_manager->write_hwmon_name_and_attribute(1, 7, "i915\n", "energy1_input", "345678");

    m_driver = std::make_unique<DrmSysfsDriver>(*m_topo, m_dir_manager->get_driver_dir(), "TEST_DRIVER_PREFIX");

    EXPECT_EQ(m_dir_manager->get_driver_dir() + "/card0/device/hwmon/hwmon123/curr1_crit",
              m_driver->attribute_path("TEST_DRIVER_PREFIX::HWMON::CURR1_CRIT", 0))
        << "Should successfully get a TEST_DRIVER_PREFIX->HWMON path for a card-scoped hwmon";

    // Card1/GT0: gpu_chip 2
    EXPECT_EQ(m_dir_manager->get_driver_dir() + "/card1/device/hwmon/hwmon45/energy1_input",
              m_driver->attribute_path("TEST_DRIVER_PREFIX::HWMON::ENERGY1_INPUT::GPU_CHIP", 2))
        << "Should successfully get a TEST_DRIVER_PREFIX->HWMON path for a tile-scoped hwmon";
    // Card1/GT1: gpu_chip 3
    EXPECT_EQ(m_dir_manager->get_driver_dir() + "/card1/device/hwmon/hwmon6/energy1_input",
              m_driver->attribute_path("TEST_DRIVER_PREFIX::HWMON::ENERGY1_INPUT::GPU_CHIP", 3))
        << "Should successfully get a TEST_DRIVER_PREFIX->HWMON path for a tile-scoped hwmon";
    // Card1: gpu 1
    EXPECT_EQ(m_dir_manager->get_driver_dir() + "/card1/device/hwmon/hwmon7/energy1_input",
              m_driver->attribute_path("TEST_DRIVER_PREFIX::HWMON::ENERGY1_INPUT::GPU", 1))
        << "Should successfully get a TEST_DRIVER_PREFIX->HWMON path for a card hwmon";
}

TEST_F(DrmSysfsDriverTest, signal_parse)
{
    EXPECT_THROW(m_driver->signal_parse("TEST_DRIVER_PREFIX::A_MADE_UP_ATTRIBUTE_NAME"), geopm::Exception)
        << "Should fail to parse a signal that does not exist";
    EXPECT_DOUBLE_EQ(1.234e9, m_driver->signal_parse("TEST_DRIVER_PREFIX::RPS_CUR_FREQ")("1234" /* in MHz */));
    EXPECT_DOUBLE_EQ(2.345e9, m_driver->signal_parse("TEST_DRIVER_PREFIX::RPS_ACT_FREQ")("2345" /* in MHz */));
}

TEST_F(DrmSysfsDriverTest, control_gen)
{
    EXPECT_THROW(m_driver->control_gen("TEST_DRIVER_PREFIX::A_MADE_UP_ATTRIBUTE_NAME"), geopm::Exception)
        << "Should fail to generate a control that does not exist";
    EXPECT_EQ("1100", m_driver->control_gen("TEST_DRIVER_PREFIX::RPS_MIN_FREQ")(1.1e9));
    EXPECT_EQ("1200", m_driver->control_gen("TEST_DRIVER_PREFIX::RPS_MAX_FREQ")(1.2e9));
}
