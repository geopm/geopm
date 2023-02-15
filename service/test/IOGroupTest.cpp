/*
 * Copyright (c) 2015 - 2023, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

/// A sanity check that all registered IOGroups are internally consistent
/// with respect to the list of signals and controls they provide.  This
/// test can be used to check a new IOGroup plugin by loading it with
/// GEOPM_PLUGIN_PATH.

#include <vector>
#include <set>
#include <string>
#include <memory>

#include "gtest/gtest.h"
#include "gmock/gmock.h"

#include "geopm/PluginFactory.hpp"
#include "geopm/IOGroup.hpp"
#include "geopm/PlatformTopo.hpp"
#include "geopm/Helper.hpp"
#include "geopm_test.hpp"

using geopm::PluginFactory;
using geopm::IOGroup;
using geopm::PlatformTopo;
using testing::Return;

class IOGroupTest : public ::testing::Test
{
    protected:
        IOGroupTest();
        void SetUp(void);
        PluginFactory<IOGroup> &m_factory;
        std::vector<std::string> m_plugin_names;
        std::vector<std::unique_ptr<IOGroup> > m_plugins;
};

IOGroupTest::IOGroupTest()
    : m_factory(geopm::iogroup_factory())
{

}

void IOGroupTest::SetUp()
{
    // Default plugins
    m_plugin_names = m_factory.plugin_names();
    for (const auto &name : m_plugin_names) {
        try {
            m_plugins.emplace_back(m_factory.make_plugin(name));
        }
        catch(const std::exception &ex) {
            std::cerr << "Warning: failed to load " << name << " IOGroup. "
                      << "The error was: " << ex.what() << std::endl;
        }
    }
}

TEST_F(IOGroupTest, signal_names_are_valid)
{
    for (const auto &group : m_plugins) {
        auto signal_names = group->signal_names();
        for (auto name : signal_names) {
            EXPECT_TRUE(group->is_valid_signal(name)) << name;
            EXPECT_NE(GEOPM_DOMAIN_INVALID, group->signal_domain_type(name)) << name;
            EXPECT_LT(-1, group->signal_behavior(name));
        }
    }
}

TEST_F(IOGroupTest, control_names_are_valid)
{
    for (const auto &group : m_plugins) {
        auto control_names = group->control_names();
        for (auto name : control_names) {
            EXPECT_TRUE(group->is_valid_control(name)) << name;
            EXPECT_NE(GEOPM_DOMAIN_INVALID, group->control_domain_type(name)) << name;
        }
    }
}

TEST_F(IOGroupTest, signals_have_agg_functions)
{
    std::vector<double> data {5.5, 6.6, 7.8, 9.0};
    for (const auto &group : m_plugins) {
        auto signal_names = group->signal_names();
        for (auto name : signal_names) {
            std::function<double(std::vector<double>)> func;
            EXPECT_NO_THROW(func = group->agg_function(name)) << name;
            EXPECT_NO_THROW(func(data)) << name;
        }
    }
}

TEST_F(IOGroupTest, signals_have_format_functions)
{
    double signal = 1.0;
    for (const auto &group : m_plugins) {
        auto signal_names = group->signal_names();
        for (auto name : signal_names) {
            std::function<std::string(double)> func;
            EXPECT_NO_THROW(func = group->format_function(name)) << name;
            EXPECT_NO_THROW(func(signal)) << name;
        }
    }
}

TEST_F(IOGroupTest, signals_have_descriptions)
{
    for (const auto &group : m_plugins) {
        auto signal_names = group->signal_names();
        for (auto name : signal_names) {
            EXPECT_NO_THROW(group->signal_description(name)) << name;
        }
    }
}

TEST_F(IOGroupTest, controls_have_descriptions)
{
    for (const auto &group : m_plugins) {
        auto control_names = group->control_names();
        for (auto name : control_names) {
            EXPECT_NO_THROW(group->control_description(name)) << name;
        }
    }
}

TEST_F(IOGroupTest, string_to_behavior)
{
    EXPECT_EQ(IOGroup::M_SIGNAL_BEHAVIOR_CONSTANT,
              IOGroup::string_to_behavior("constant"));
    EXPECT_EQ(IOGroup::M_SIGNAL_BEHAVIOR_MONOTONE,
              IOGroup::string_to_behavior("monotone"));
    EXPECT_EQ(IOGroup::M_SIGNAL_BEHAVIOR_VARIABLE,
              IOGroup::string_to_behavior("variable"));
    EXPECT_EQ(IOGroup::M_SIGNAL_BEHAVIOR_LABEL,
              IOGroup::string_to_behavior("label"));

    GEOPM_EXPECT_THROW_MESSAGE(IOGroup::string_to_behavior("invalid"),
                               GEOPM_ERROR_INVALID, "invalid behavior string");
}
