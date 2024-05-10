/*
 * Copyright (c) 2015 - 2024 Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "CpufreqSysfsDriver.hpp"

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
#include "geopm_topo.h"

#include "test/MockPlatformTopo.hpp"

using geopm::CpufreqSysfsDriver;
using geopm::IOGroup;
using geopm::SysfsDriver;

class CpufreqFakeDirManager
{
    public:
        CpufreqFakeDirManager(std::string base_path_template)
        {
            if (mkdtemp(&base_path_template[0]) == nullptr) {
                throw std::runtime_error("Could not create a temporary directory at " + base_path_template);
            }
            m_base_dir_path = std::string(base_path_template);
            m_created_dirs.push_back(m_base_dir_path);

            m_policy_dir_path = m_base_dir_path + "/policy0";
            auto meaningless_dir_path = m_base_dir_path + "/something_else";

            if (mkdir(meaningless_dir_path.c_str(), 0755) == -1) {
                rmdir(m_base_dir_path.c_str());
                throw std::runtime_error("Could not create directory at " + meaningless_dir_path);
            }
            m_created_dirs.push_back(meaningless_dir_path);
            if (mkdir(m_policy_dir_path.c_str(), 0755) == -1) {
                rmdir(meaningless_dir_path.c_str());
                rmdir(m_base_dir_path.c_str());
                throw std::runtime_error("Could not create directory at " + m_policy_dir_path);
            }
            m_created_dirs.push_back(m_policy_dir_path);
        }

        ~CpufreqFakeDirManager()
        {
            for (const auto &file_path : m_created_policy_files) {
                unlink(file_path.c_str());
            }
            // Clean up directories we created in reverse order so
            // any attempted directory removals are on empty directories
            for (auto it = m_created_dirs.rbegin(); it != m_created_dirs.rend(); ++it) {
                rmdir(it->c_str());
            }
        }

        void write_file_in_policy(const std::string &file_name, const std::string &contents)
        {
            std::string file_path = m_policy_dir_path + "/" + file_name;
            geopm::write_file(file_path, contents);
            m_created_policy_files.insert(file_path);
        }

        std::string get_driver_dir() const
        {
            return m_base_dir_path;
        }

        std::string get_policy_dir() const
        {
            return m_policy_dir_path;
        }

    private:
        std::vector<std::string> m_created_dirs;
        std::set<std::string> m_created_policy_files;
        std::string m_base_dir_path;
        std::string m_policy_dir_path;
};

class CpufreqSysfsDriverTest : public ::testing::Test
{
    protected:
        void SetUp();
        std::shared_ptr<MockPlatformTopo> m_topo;
        std::unique_ptr<CpufreqFakeDirManager> m_dir_manager;
        std::unique_ptr<SysfsDriver> m_driver;
        std::map<std::string, SysfsDriver::properties_s> m_driver_properties;
        int m_exposed_cpu;
};

void CpufreqSysfsDriverTest::SetUp()
{
    m_topo = make_topo(2 /*packages*/, 4 /*cores*/, 8 /*cpus*/);
    m_exposed_cpu = 2;
    m_dir_manager = std::make_unique<CpufreqFakeDirManager>("/tmp/CpufreqsysfsDriverTest_XXXXXX");
    m_dir_manager->write_file_in_policy("affected_cpus", std::to_string(m_exposed_cpu));
    m_driver = std::make_unique<CpufreqSysfsDriver>(*m_topo, m_dir_manager->get_driver_dir());
    m_driver_properties = m_driver->properties();
}

TEST_F(CpufreqSysfsDriverTest, iogroup_plugin_name_matches_driver_name)
{
    EXPECT_EQ("CPUFREQ", m_driver->driver());
    EXPECT_EQ("CPUFREQ", CpufreqSysfsDriver::plugin_name());
}

TEST_F(CpufreqSysfsDriverTest, domain_type_is_detected_from_driver)
{
    m_dir_manager->write_file_in_policy("affected_cpus", "0");
    m_driver = std::make_unique<CpufreqSysfsDriver>(*m_topo, m_dir_manager->get_driver_dir());
    m_driver_properties = m_driver->properties();
    for (const auto &attribute_properties : m_driver->properties()) {
        EXPECT_EQ(GEOPM_DOMAIN_CPU, m_driver->domain_type(attribute_properties.first));
    }

    m_dir_manager->write_file_in_policy("affected_cpus", "0 4" /* both in core 0 */);
    m_driver = std::make_unique<CpufreqSysfsDriver>(*m_topo, m_dir_manager->get_driver_dir());
    for (const auto &attribute_properties : m_driver->properties()) {
        EXPECT_EQ(GEOPM_DOMAIN_CORE, m_driver->domain_type(attribute_properties.first));
    }

    m_dir_manager->write_file_in_policy("affected_cpus", "0 1 4 5" /* all in package 0 */);
    m_driver = std::make_unique<CpufreqSysfsDriver>(*m_topo, m_dir_manager->get_driver_dir());
    for (const auto &attribute_properties : m_driver->properties()) {
        EXPECT_EQ(GEOPM_DOMAIN_PACKAGE, m_driver->domain_type(attribute_properties.first));
    }

    m_dir_manager->write_file_in_policy("affected_cpus", "0 1 2 3 4 5 6 7");
    m_driver = std::make_unique<CpufreqSysfsDriver>(*m_topo, m_dir_manager->get_driver_dir());
    for (const auto &attribute_properties : m_driver->properties()) {
        EXPECT_EQ(GEOPM_DOMAIN_BOARD, m_driver->domain_type(attribute_properties.first));
    }
}

TEST_F(CpufreqSysfsDriverTest, attribute_path)
{
    EXPECT_EQ(m_dir_manager->get_policy_dir() + "/scaling_cur_freq",
              m_driver->attribute_path("CPUFREQ::SCALING_CUR_FREQ", m_exposed_cpu))
        << "Should successfully get a path for an attribute that exists";
    EXPECT_THROW(m_driver->attribute_path("CPUFREQ::A_MADE_UP_ATTRIBUTE_NAME", m_exposed_cpu), geopm::Exception)
        << "Should fail to get a path for an attribute that does not exist";
    EXPECT_THROW(m_driver->attribute_path("CPUFREQ::SCALING_CUR_FREQ", 12345), geopm::Exception)
        << "Should fail to get a path for an attribute at a domain that does not exist";
}

TEST_F(CpufreqSysfsDriverTest, signal_parse)
{
    EXPECT_THROW(m_driver->signal_parse("CPUFREQ::A_MADE_UP_ATTRIBUTE_NAME"), geopm::Exception)
        << "Should fail to parse a signal that does not exist";
    EXPECT_DOUBLE_EQ(1.1e9, m_driver->signal_parse("CPUFREQ::SCALING_CUR_FREQ")("1100000" /* in kHz */));
    EXPECT_DOUBLE_EQ(100e-9, m_driver->signal_parse("CPUFREQ::CPUINFO_TRANSITION_LATENCY")("100" /* in ns */));
    EXPECT_TRUE(std::isnan(m_driver->signal_parse("CPUFREQ::SCALING_SETSPEED")("<unsupported>")));
    EXPECT_TRUE(std::isnan(m_driver->signal_parse("CPUFREQ::SCALING_SETSPEED")("")));
    EXPECT_TRUE(std::isnan(m_driver->signal_parse("CPUFREQ::SCALING_SETSPEED")("BADDAD")));
}

TEST_F(CpufreqSysfsDriverTest, control_gen)
{
    EXPECT_THROW(m_driver->control_gen("CPUFREQ::A_MADE_UP_ATTRIBUTE_NAME"), geopm::Exception)
        << "Should fail to generate a control that does not exist";
    EXPECT_EQ("1100000", m_driver->control_gen("CPUFREQ::SCALING_CUR_FREQ")(1.1e9));
    EXPECT_EQ("100", m_driver->control_gen("CPUFREQ::CPUINFO_TRANSITION_LATENCY")(100e-9));
}
