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
    m_driver = std::make_unique<DrmSysfsDriver>(*m_topo, m_dir_manager->get_driver_dir());
    m_driver_properties = m_driver->properties();
}

TEST_F(DrmSysfsDriverTest, iogroup_plugin_name_matches_driver_name)
{
    EXPECT_EQ("DRM", m_driver->driver());
    EXPECT_EQ("DRM", DrmSysfsDriver::plugin_name());
}

TEST_F(DrmSysfsDriverTest, domain_type_is_gpu_chip)
{
    m_driver = std::make_unique<DrmSysfsDriver>(*m_topo, m_dir_manager->get_driver_dir());
    for (const auto &attribute_properties : m_driver->properties()) {
        EXPECT_EQ(GEOPM_DOMAIN_GPU_CHIP, m_driver->domain_type(attribute_properties.first));
    }
}

TEST_F(DrmSysfsDriverTest, attribute_path)
{
    EXPECT_EQ(m_dir_manager->get_driver_dir() + "/card0/gt/gt0/rps_cur_freq_mhz",
              m_driver->attribute_path("DRM::RPS_CUR_FREQ", 0))
        << "Should successfully get a path for an attribute that exists";
    EXPECT_THROW(m_driver->attribute_path("DRM::A_MADE_UP_ATTRIBUTE_NAME", 0), geopm::Exception)
        << "Should fail to get a path for an attribute that does not exist";
    EXPECT_THROW(m_driver->attribute_path("DRM::RPS_CUR_FREQ", 12345), geopm::Exception)
        << "Should fail to get a path for an attribute at a domain that does not exist";
}

TEST_F(DrmSysfsDriverTest, signal_parse)
{
    EXPECT_THROW(m_driver->signal_parse("DRM::A_MADE_UP_ATTRIBUTE_NAME"), geopm::Exception)
        << "Should fail to parse a signal that does not exist";
    EXPECT_DOUBLE_EQ(1.234e9, m_driver->signal_parse("DRM::RPS_CUR_FREQ")("1234" /* in MHz */));
    EXPECT_DOUBLE_EQ(2.345e9, m_driver->signal_parse("DRM::RPS_ACT_FREQ")("2345" /* in MHz */));
}

TEST_F(DrmSysfsDriverTest, control_gen)
{
    EXPECT_THROW(m_driver->control_gen("DRM::A_MADE_UP_ATTRIBUTE_NAME"), geopm::Exception)
        << "Should fail to generate a control that does not exist";
    EXPECT_EQ("1100", m_driver->control_gen("DRM::RPS_MIN_FREQ")(1.1e9));
    EXPECT_EQ("1200", m_driver->control_gen("DRM::RPS_MAX_FREQ")(1.2e9));
}
