/*
 * Copyright (c) 2015, 2016, 2017, 2018, Intel Corporation
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in
 *       the documentation and/or other materials provided with the
 *       distribution.
 *
 *     * Neither the name of Intel Corporation nor the names of its
 *       contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY LOG OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/// A sanity check that all registered IOGroups are internally consistent
/// with respect to the list of signals and controls they provide.  This
/// test can be used to check a new IOGroup plugin by loading it with
/// GEOPM_PLUGIN.

#include <vector>
#include <set>
#include <string>
#include <memory>

#include "gtest/gtest.h"
#include "gmock/gmock.h"

#include "PluginFactory.hpp"
#include "IOGroup.hpp"
#include "PlatformTopo.hpp"
#include "ProfileIOGroup.hpp"
#include "Helper.hpp"
#include "MockEpochRuntimeRegulator.hpp"
#include "MockProfileIOSample.hpp"

using geopm::PluginFactory;
using geopm::IOGroup;
using geopm::IPlatformTopo;
using geopm::ProfileIOGroup;
using testing::Return;

class IOGroupTest : public ::testing::Test
{
    protected:
        IOGroupTest();
        void SetUp(void);
        PluginFactory<IOGroup> &m_factory;
        std::vector<std::string> m_plugin_names;
        std::vector<std::unique_ptr<IOGroup> > m_plugins;

        // Used by ProfileIOGroup
        MockEpochRuntimeRegulator m_epoch_reg;
        std::shared_ptr<MockProfileIOSample> m_profile_sample;
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

    // Add IOGroups that are not registered by default
    m_profile_sample = std::make_shared<MockProfileIOSample>();
    std::vector<int> rank;
    EXPECT_CALL(*m_profile_sample, cpu_rank())
        .WillOnce(Return(rank));

    m_plugins.emplace_back(geopm::make_unique<ProfileIOGroup>(m_profile_sample, m_epoch_reg));
}

TEST_F(IOGroupTest, signal_names_are_valid)
{
    for (const auto &group : m_plugins) {
        auto signal_names = group->signal_names();
        for (auto name : signal_names) {
            EXPECT_TRUE(group->is_valid_signal(name)) << name;
            EXPECT_NE(IPlatformTopo::M_DOMAIN_INVALID, group->signal_domain_type(name)) << name;
        }
    }
}

TEST_F(IOGroupTest, control_names_are_valid)
{
    for (const auto &group : m_plugins) {
        auto control_names = group->control_names();
        for (auto name : control_names) {
            EXPECT_TRUE(group->is_valid_control(name)) << name;
            EXPECT_NE(IPlatformTopo::M_DOMAIN_INVALID, group->control_domain_type(name)) << name;
        }
    }
}

TEST_F(IOGroupTest, signals_have_agg_functions)
{
    std::vector<double> data {5.5, 6.6, 7.8, 9.0};
    for (const auto &group : m_plugins) {
        auto signal_names = group->signal_names();
        for (auto name : signal_names) {
            EXPECT_NO_THROW(group->agg_function(name)) << name;
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
