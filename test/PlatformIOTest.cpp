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

#include <list>
#include <set>
#include <memory>
#include <string>
#include <algorithm>
#include <cmath>
#include "gtest/gtest.h"
#include "gmock/gmock.h"

#include "geopm_internal.h"
#include "geopm_hash.h"
#include "IOGroup.hpp"
#include "MockIOGroup.hpp"
#include "MockPlatformTopo.hpp"
#include "PlatformIOInternal.hpp"
#include "PlatformTopo.hpp"
#include "Exception.hpp"
#include "Agg.hpp"
#include "CombinedSignal.hpp"
#include "geopm_test.hpp"

using geopm::IOGroup;
using geopm::PlatformIO;
using geopm::IPlatformIO;
using geopm::IPlatformTopo;

using ::testing::_;
using ::testing::Return;
using ::testing::SetArgReferee;

class PlatformIOTestMockIOGroup : public MockIOGroup
{
    public:
        void set_valid_signal_names(const std::set<std::string> &names) {
            m_valid_signals = names;
        }
        void set_valid_control_names(const std::set<std::string> &names) {
            m_valid_controls = names;
        }
        bool is_valid_signal(const std::string &signal_name) const override
        {
            return std::find(m_valid_signals.begin(),
                             m_valid_signals.end(),
                             signal_name) != m_valid_signals.end();
        }
        bool is_valid_control(const std::string &control_name) const override
        {
            return std::find(m_valid_controls.begin(),
                             m_valid_controls.end(),
                             control_name) != m_valid_controls.end();
        }
        std::set<std::string> signal_names(void) const override
        {
            return m_valid_signals;
        }
        std::set<std::string> control_names(void) const override
        {
            return m_valid_controls;
        }
    protected:
        std::set<std::string> m_valid_signals;
        std::set<std::string> m_valid_controls;
};

class PlatformIOTest : public ::testing::Test
{
    protected:
        void SetUp();
        std::list<std::shared_ptr<PlatformIOTestMockIOGroup> > m_iogroup_ptr;
        std::unique_ptr<PlatformIO> m_platio;
        MockPlatformTopo m_topo;
        const unsigned int M_NUM_CPU = 4;
};

void PlatformIOTest::SetUp()
{
    std::list<std::shared_ptr<IOGroup> > iogroup_list;

    // IOGroups for specific signals
    // Not realistic, but easier to set expectations for testing
    auto tmp = std::make_shared<PlatformIOTestMockIOGroup>();
    iogroup_list.emplace_back(tmp);
    m_iogroup_ptr.push_back(tmp);
    tmp->set_valid_signal_names({"TIME"});
    ON_CALL(*tmp, signal_domain_type("TIME"))
        .WillByDefault(Return(IPlatformTopo::M_DOMAIN_BOARD));

    tmp = std::make_shared<PlatformIOTestMockIOGroup>();
    iogroup_list.emplace_back(tmp);
    m_iogroup_ptr.push_back(tmp);
    tmp->set_valid_signal_names({"ENERGY_PACKAGE"});
    ON_CALL(*tmp, signal_domain_type("ENERGY_PACKAGE"))
        .WillByDefault(Return(IPlatformTopo::M_DOMAIN_PACKAGE));

    tmp = std::make_shared<PlatformIOTestMockIOGroup>();
    iogroup_list.emplace_back(tmp);
    m_iogroup_ptr.push_back(tmp);
    tmp->set_valid_signal_names({"ENERGY_DRAM"});
    ON_CALL(*tmp, signal_domain_type("ENERGY_DRAM"))
        .WillByDefault(Return(IPlatformTopo::M_DOMAIN_BOARD_MEMORY));

    tmp = std::make_shared<PlatformIOTestMockIOGroup>();
    iogroup_list.emplace_back(tmp);
    m_iogroup_ptr.push_back(tmp);
    tmp->set_valid_signal_names({"REGION_HASH"});
    ON_CALL(*tmp, signal_domain_type("REGION_HASH"))
        .WillByDefault(Return(IPlatformTopo::M_DOMAIN_CPU));
    ON_CALL(*tmp, agg_function("REGION_HASH"))
        .WillByDefault(Return(geopm::Agg::region_hash));

    // IOGroups with signals and controls
    tmp = std::make_shared<PlatformIOTestMockIOGroup>();
    iogroup_list.emplace_back(tmp);
    m_iogroup_ptr.push_back(tmp);
    tmp->set_valid_signal_names({"FREQ", "MODE"});
    tmp->set_valid_control_names({"FREQ", "MODE"});
    ON_CALL(*tmp, signal_domain_type("FREQ"))
        .WillByDefault(Return(IPlatformTopo::M_DOMAIN_CPU));
    ON_CALL(*tmp, control_domain_type("FREQ"))
        .WillByDefault(Return(IPlatformTopo::M_DOMAIN_CPU));
    ON_CALL(*tmp, signal_domain_type("MODE"))
        .WillByDefault(Return(IPlatformTopo::M_DOMAIN_PACKAGE));
    ON_CALL(*tmp, control_domain_type("MODE"))
        .WillByDefault(Return(IPlatformTopo::M_DOMAIN_PACKAGE));

    // Group that overrides previous signals and controls
    tmp = std::make_shared<PlatformIOTestMockIOGroup>();
    iogroup_list.emplace_back(tmp);
    m_iogroup_ptr.push_back(tmp);
    tmp->set_valid_signal_names({"MODE"});
    tmp->set_valid_control_names({"MODE"});
    ON_CALL(*tmp, signal_domain_type("MODE"))
        .WillByDefault(Return(IPlatformTopo::M_DOMAIN_BOARD));
    ON_CALL(*tmp, control_domain_type("MODE"))
        .WillByDefault(Return(IPlatformTopo::M_DOMAIN_BOARD));

    // Settings for PlatformTopo: 1 socket 4 cpus
    std::set<int> cpu_set = {0, 1, 2, 3};
    ON_CALL(m_topo, is_domain_within(IPlatformTopo::M_DOMAIN_CPU, IPlatformTopo::M_DOMAIN_BOARD))
        .WillByDefault(Return(true));
    ON_CALL(m_topo, is_domain_within(IPlatformTopo::M_DOMAIN_CPU, IPlatformTopo::M_DOMAIN_BOARD_MEMORY))
        .WillByDefault(Return(true));
    ON_CALL(m_topo, is_domain_within(IPlatformTopo::M_DOMAIN_CPU, IPlatformTopo::M_DOMAIN_PACKAGE))
        .WillByDefault(Return(true));
    ON_CALL(m_topo, domain_cpus(IPlatformTopo::M_DOMAIN_BOARD, _))
        .WillByDefault(Return(cpu_set));
    ON_CALL(m_topo, domain_cpus(IPlatformTopo::M_DOMAIN_BOARD_MEMORY, _))
        .WillByDefault(Return(cpu_set));
    ON_CALL(m_topo, domain_cpus(IPlatformTopo::M_DOMAIN_PACKAGE, _))
        .WillByDefault(Return(cpu_set));
    ON_CALL(m_topo, domain_idx(IPlatformTopo::M_DOMAIN_CPU, _))
        .WillByDefault(testing::ReturnArg<1>());

    m_platio.reset(new PlatformIO(iogroup_list, m_topo));
}

TEST_F(PlatformIOTest, signal_control_names)
{
    // IOGroup signals and PlatformIO signals
    //@todo finish REGION_HINT
    std::set<std::string> expected_signals {"TIME", "ENERGY_PACKAGE", "ENERGY_DRAM",
            "REGION_HASH", "REGION_HINT", "FREQ", "MODE", "POWER_PACKAGE", "POWER_DRAM"};
    EXPECT_EQ(expected_signals.size(), m_platio->signal_names().size());
    EXPECT_EQ(expected_signals, m_platio->signal_names());

    std::set<std::string> expected_controls {"FREQ", "MODE"};
    EXPECT_EQ(expected_controls.size(), m_platio->control_names().size());
    EXPECT_EQ(expected_controls, m_platio->control_names());
}


TEST_F(PlatformIOTest, signal_control_description)
{
    std::string time_description = "time signal";
    std::string freq_signal_desc = "freq signal";
    std::string freq_control_desc = "freq control";
    for (auto &it : m_iogroup_ptr) {
        if (it->is_valid_signal("TIME")) {
            EXPECT_CALL(*it, signal_description("TIME"))
                .WillOnce(Return(time_description));
        }
        if (it->is_valid_signal("FREQ")) {
            EXPECT_CALL(*it, signal_description("FREQ"))
                .WillOnce(Return(freq_signal_desc));
        }
        if (it->is_valid_control("FREQ")) {
            EXPECT_CALL(*it, control_description("FREQ"))
                .WillOnce(Return(freq_control_desc));
        }
    }
    EXPECT_EQ(time_description, m_platio->signal_description("TIME"));
    EXPECT_EQ(freq_signal_desc, m_platio->signal_description("FREQ"));
    EXPECT_EQ(freq_control_desc, m_platio->control_description("FREQ"));
}

TEST_F(PlatformIOTest, domain_type)
{
    for (auto &it : m_iogroup_ptr) {
        if (it->is_valid_signal("TIME")) {
            EXPECT_CALL(*it, signal_domain_type("TIME"));
        }
        if (it->is_valid_signal("FREQ")) {
            EXPECT_CALL(*it, signal_domain_type("FREQ"));
        }
        if (it->is_valid_control("FREQ")) {
            EXPECT_CALL(*it, control_domain_type("FREQ"));
        }
        if (it->is_valid_signal("ENERGY_PACKAGE")) {
            EXPECT_CALL(*it, signal_domain_type("ENERGY_PACKAGE"));
        }
        if (it->is_valid_signal("ENERGY_DRAM")) {
            EXPECT_CALL(*it, signal_domain_type("ENERGY_DRAM"));
        }
    }

    int domain_type = m_platio->signal_domain_type("TIME");
    EXPECT_EQ(IPlatformTopo::M_DOMAIN_BOARD, domain_type);
    domain_type = m_platio->signal_domain_type("FREQ");
    EXPECT_EQ(IPlatformTopo::M_DOMAIN_CPU, domain_type);
    domain_type = m_platio->signal_domain_type("POWER_PACKAGE");
    EXPECT_EQ(IPlatformTopo::M_DOMAIN_PACKAGE, domain_type);
    domain_type = m_platio->signal_domain_type("POWER_DRAM");
    EXPECT_EQ(IPlatformTopo::M_DOMAIN_BOARD_MEMORY, domain_type);
    GEOPM_EXPECT_THROW_MESSAGE(m_platio->signal_domain_type("INVALID"),
                               GEOPM_ERROR_INVALID, "signal name \"INVALID\" not found");

    domain_type = m_platio->control_domain_type("FREQ");
    EXPECT_EQ(IPlatformTopo::M_DOMAIN_CPU, domain_type);
    GEOPM_EXPECT_THROW_MESSAGE(m_platio->control_domain_type("INVALID"),
                               GEOPM_ERROR_INVALID, "control name \"INVALID\" not found");
}

TEST_F(PlatformIOTest, push_signal)
{
    for (auto &it : m_iogroup_ptr) {
        if (it->is_valid_signal("TIME")) {
            EXPECT_CALL(*it, push_signal("TIME", _, _));
            EXPECT_CALL(*it, signal_domain_type("TIME"));
        }
        if (it->is_valid_signal("FREQ")) {
            EXPECT_CALL(*it, push_signal("FREQ", _, _));
            EXPECT_CALL(*it, signal_domain_type("FREQ"));
        }
        EXPECT_CALL(*it, read_batch());
    }

    EXPECT_EQ(0, m_platio->num_signal());

    int idx = m_platio->push_signal("FREQ", IPlatformTopo::M_DOMAIN_CPU, 0);
    EXPECT_EQ(0, idx);
    idx = m_platio->push_signal("TIME", IPlatformTopo::M_DOMAIN_BOARD, 0);
    EXPECT_EQ(1, idx);
    EXPECT_EQ(idx, m_platio->push_signal("TIME", IPlatformTopo::M_DOMAIN_BOARD, 0));
    GEOPM_EXPECT_THROW_MESSAGE(m_platio->push_signal("INVALID", IPlatformTopo::M_DOMAIN_CPU, 0),
                               GEOPM_ERROR_INVALID, "signal name \"INVALID\" not found");

    EXPECT_EQ(2, m_platio->num_signal());

    m_platio->read_batch();
    GEOPM_EXPECT_THROW_MESSAGE(m_platio->push_signal("TIME", IPlatformTopo::M_DOMAIN_BOARD, 0),
                               GEOPM_ERROR_INVALID, "pushing signals after");
}

TEST_F(PlatformIOTest, push_signal_agg)
{
    EXPECT_CALL(m_topo, is_domain_within(IPlatformTopo::M_DOMAIN_CPU,
                                         IPlatformTopo::M_DOMAIN_PACKAGE));
    std::set<int> package_cpus {0, 1, 2, 3};
    ASSERT_EQ(M_NUM_CPU, package_cpus.size());
    EXPECT_CALL(m_topo, domain_cpus(IPlatformTopo::M_DOMAIN_PACKAGE, 0))
        .WillOnce(Return(package_cpus));
    EXPECT_CALL(m_topo, domain_idx(IPlatformTopo::M_DOMAIN_CPU, _))
        .Times(M_NUM_CPU);

    for (auto &it : m_iogroup_ptr) {
        if (it->is_valid_signal("FREQ")) {
            EXPECT_CALL(*it, push_signal("FREQ", _, _)).Times(M_NUM_CPU);
            // called once for initial push, once for converting domain, once for each cpu,
            EXPECT_CALL(*it, signal_domain_type("FREQ")).Times(2 + M_NUM_CPU);
            EXPECT_CALL(*it, agg_function("FREQ"))
                .WillOnce(Return(geopm::Agg::average));
        }
    }

    EXPECT_EQ(0, m_platio->num_signal());
    // Domain of FREQ is CPU
    m_platio->push_signal("FREQ", IPlatformTopo::M_DOMAIN_PACKAGE, 0);
    EXPECT_EQ(1 + M_NUM_CPU, (unsigned int)m_platio->num_signal());
}

TEST_F(PlatformIOTest, signal_power)
{
    for (auto &it : m_iogroup_ptr) {
        if (it->is_valid_signal("TIME")) {
            EXPECT_CALL(*it, push_signal("TIME", _, _))
                .Times(1);
            EXPECT_CALL(*it, signal_domain_type("TIME"))
                .Times(1);
        }
        if (it->is_valid_signal("ENERGY_PACKAGE")) {
            EXPECT_CALL(*it, push_signal("ENERGY_PACKAGE", _, _))
                .Times(1);
            EXPECT_CALL(*it, signal_domain_type("ENERGY_PACKAGE"))
                .Times(1);
        }
        if (it->is_valid_signal("ENERGY_DRAM")) {
            EXPECT_CALL(*it, push_signal("ENERGY_DRAM", _, _))
                .Times(1);
            EXPECT_CALL(*it, signal_domain_type("ENERGY_DRAM"))
                .Times(1);
        }
    }

    int pkg_idx = m_platio->push_signal("POWER_PACKAGE", IPlatformTopo::M_DOMAIN_PACKAGE, 0);
    int pkg_energy_idx = m_platio->push_signal("ENERGY_PACKAGE", IPlatformTopo::M_DOMAIN_PACKAGE, 0);
    EXPECT_NE(pkg_energy_idx, pkg_idx);
    int dram_idx = m_platio->push_signal("POWER_DRAM", IPlatformTopo::M_DOMAIN_BOARD_MEMORY, 0);
    int dram_energy_idx = m_platio->push_signal("ENERGY_DRAM", IPlatformTopo::M_DOMAIN_BOARD_MEMORY, 0);
    EXPECT_NE(dram_energy_idx, dram_idx);

    for (auto &it : m_iogroup_ptr) {
        EXPECT_CALL(*it, read_batch()).Times(3);
        if (it->is_valid_signal("TIME")) {
            EXPECT_CALL(*it, sample(0))
                .WillOnce(Return(2.0)).WillOnce(Return(2.0))
                .WillOnce(Return(3.0)).WillOnce(Return(3.0))
                .WillOnce(Return(4.0)).WillOnce(Return(4.0));
        }
        if (it->is_valid_signal("ENERGY_PACKAGE")) {
            EXPECT_CALL(*it, sample(0))
                .WillOnce(Return(777.77))
                .WillOnce(Return(888.88))
                .WillOnce(Return(999.99));
        }
        if (it->is_valid_signal("ENERGY_DRAM")) {
            EXPECT_CALL(*it, sample(0))
                .WillOnce(Return(333.33))
                .WillOnce(Return(555.55))
                .WillOnce(Return(777.77));
        }
    }

    m_platio->read_batch();
    double result = m_platio->sample(pkg_idx);
    EXPECT_TRUE(std::isnan(result)); // only one sample so far
    result = m_platio->sample(dram_idx);
    EXPECT_TRUE(std::isnan(result));

    m_platio->read_batch();
    result = m_platio->sample(pkg_idx);
    EXPECT_DOUBLE_EQ(111.11, result);
    result = m_platio->sample(dram_idx);
    EXPECT_DOUBLE_EQ(222.22, result);

    m_platio->read_batch();
    result = m_platio->sample(pkg_idx);
    EXPECT_DOUBLE_EQ(111.11, result);
    result = m_platio->sample(dram_idx);
    EXPECT_DOUBLE_EQ(222.22, result);
}

TEST_F(PlatformIOTest, push_control)
{
    for (auto &it : m_iogroup_ptr) {
        if (it->is_valid_control("FREQ")) {
            EXPECT_CALL(*it, push_control("FREQ", IPlatformTopo::M_DOMAIN_CPU, 0));
            EXPECT_CALL(*it, control_domain_type("FREQ"));
        }
    }

    EXPECT_EQ(0, m_platio->num_control());

    int idx = m_platio->push_control("FREQ", IPlatformTopo::M_DOMAIN_CPU, 0);
    EXPECT_EQ(0, idx);
    EXPECT_EQ(idx, m_platio->push_control("FREQ", IPlatformTopo::M_DOMAIN_CPU, 0));
    GEOPM_EXPECT_THROW_MESSAGE(m_platio->push_control("INVALID", IPlatformTopo::M_DOMAIN_CPU, 0),
                               GEOPM_ERROR_INVALID, "control name \"INVALID\" not found");

    EXPECT_EQ(1, m_platio->num_control());
}

TEST_F(PlatformIOTest, push_control_agg)
{
    EXPECT_CALL(m_topo, is_domain_within(IPlatformTopo::M_DOMAIN_CPU,
                                         IPlatformTopo::M_DOMAIN_PACKAGE));
    std::set<int> package_cpus {0, 1, 2, 3};
    ASSERT_EQ(M_NUM_CPU, package_cpus.size());
    EXPECT_CALL(m_topo, domain_cpus(IPlatformTopo::M_DOMAIN_PACKAGE, 0))
        .WillOnce(Return(package_cpus));
    EXPECT_CALL(m_topo, domain_idx(IPlatformTopo::M_DOMAIN_CPU, _))
        .Times(M_NUM_CPU);

    for (auto &it : m_iogroup_ptr) {
        if (it->is_valid_control("FREQ")) {
            EXPECT_CALL(*it, push_control("FREQ", IPlatformTopo::M_DOMAIN_CPU, _))
                .Times(M_NUM_CPU);
            // called once for initial push, once for converting domain, once for each cpu,
            EXPECT_CALL(*it, control_domain_type("FREQ")).Times(2 + M_NUM_CPU);
        }
    }

    EXPECT_EQ(0, m_platio->num_control());

    m_platio->push_control("FREQ", IPlatformTopo::M_DOMAIN_PACKAGE, 0);
    EXPECT_EQ(1 + M_NUM_CPU, (unsigned int)m_platio->num_control());
}

TEST_F(PlatformIOTest, sample)
{
    for (auto &it : m_iogroup_ptr) {
        if (it->is_valid_signal("FREQ")) {
            EXPECT_CALL(*it, sample(0)).WillOnce(Return(2e9));
            EXPECT_CALL(*it, push_signal(_, _, _));
            EXPECT_CALL(*it, signal_domain_type("FREQ"));
        }
        if (it->is_valid_signal("TIME")) {
            EXPECT_CALL(*it, sample(0)).WillOnce(Return(1.0));
            EXPECT_CALL(*it, push_signal(_, _, _));
            EXPECT_CALL(*it, signal_domain_type("TIME"));
        }
        EXPECT_CALL(*it, read_batch());
    }
    int freq_idx = m_platio->push_signal("FREQ", IPlatformTopo::M_DOMAIN_CPU, 0);
    int time_idx = m_platio->push_signal("TIME", IPlatformTopo::M_DOMAIN_BOARD, 0);
    m_platio->read_batch();
    EXPECT_EQ(0, freq_idx);
    EXPECT_EQ(1, time_idx);
    double freq = m_platio->sample(freq_idx);
    EXPECT_DOUBLE_EQ(2e9, freq);
    double time = m_platio->sample(time_idx);
    EXPECT_DOUBLE_EQ(1.0, time);
    GEOPM_EXPECT_THROW_MESSAGE(m_platio->sample(-1), GEOPM_ERROR_INVALID, "signal_idx out of range");
    GEOPM_EXPECT_THROW_MESSAGE(m_platio->sample(10), GEOPM_ERROR_INVALID, "signal_idx out of range");
}

TEST_F(PlatformIOTest, sample_agg)
{
    EXPECT_CALL(m_topo, is_domain_within(IPlatformTopo::M_DOMAIN_CPU,
                                         IPlatformTopo::M_DOMAIN_PACKAGE));
    std::set<int> package_cpus {0, 1, 2, 3};
    ASSERT_EQ(M_NUM_CPU, package_cpus.size());
    EXPECT_CALL(m_topo, domain_cpus(IPlatformTopo::M_DOMAIN_PACKAGE, 0))
        .WillOnce(Return(package_cpus));
    EXPECT_CALL(m_topo, domain_idx(IPlatformTopo::M_DOMAIN_CPU, _))
        .Times(M_NUM_CPU);

    for (auto &it : m_iogroup_ptr) {
        if (it->is_valid_signal("FREQ")) {
            // called once for initial push, once for converting domain, once for each cpu,
            EXPECT_CALL(*it, signal_domain_type("FREQ")).Times(2 + M_NUM_CPU);
            EXPECT_CALL(*it, agg_function("FREQ"))
                .WillOnce(Return(geopm::Agg::average));
            for (unsigned int ii = 0; ii < M_NUM_CPU; ++ii) {
                EXPECT_CALL(*it, push_signal("FREQ", IPlatformTopo::M_DOMAIN_CPU, ii))
                    .WillOnce(Return(ii));
                EXPECT_CALL(*it, sample(ii)).WillOnce(Return(ii));
            }
        }
        EXPECT_CALL(*it, read_batch());
    }
    double sum = 0;
    for (unsigned int ii = 0; ii < M_NUM_CPU; ++ii) {
        sum += ii;
    }
    int freq_idx = m_platio->push_signal("FREQ", IPlatformTopo::M_DOMAIN_PACKAGE, 0);
    m_platio->read_batch();
    double freq = m_platio->sample(freq_idx);
    EXPECT_DOUBLE_EQ(sum / M_NUM_CPU, freq);
}

TEST_F(PlatformIOTest, adjust)
{
    for (auto &it : m_iogroup_ptr) {
        if (it->is_valid_control("FREQ")) {
            EXPECT_CALL(*it, push_control("FREQ", _, _));
            EXPECT_CALL(*it, adjust(0, 3e9));
            EXPECT_CALL(*it, control_domain_type("FREQ"));
        }
        EXPECT_CALL(*it, write_batch());
    }
    int freq_idx = m_platio->push_control("FREQ", IPlatformTopo::M_DOMAIN_CPU, 0);
    EXPECT_EQ(0, freq_idx);
    m_platio->adjust(freq_idx, 3e9);
    m_platio->write_batch();
    GEOPM_EXPECT_THROW_MESSAGE(m_platio->adjust(-1, 0.0), GEOPM_ERROR_INVALID, "control_idx out of range");
    GEOPM_EXPECT_THROW_MESSAGE(m_platio->adjust(10, 0.0), GEOPM_ERROR_INVALID, "control_idx out of range");
}

TEST_F(PlatformIOTest, adjust_agg)
{
    EXPECT_CALL(m_topo, is_domain_within(IPlatformTopo::M_DOMAIN_CPU,
                                         IPlatformTopo::M_DOMAIN_PACKAGE));
    std::set<int> package_cpus {0, 1, 2, 3};
    ASSERT_EQ(M_NUM_CPU, package_cpus.size());
    EXPECT_CALL(m_topo, domain_cpus(IPlatformTopo::M_DOMAIN_PACKAGE, 0))
        .WillOnce(Return(package_cpus));
    EXPECT_CALL(m_topo, domain_idx(IPlatformTopo::M_DOMAIN_CPU, _))
        .Times(M_NUM_CPU);

    double value = 1.23e9;
    for (auto &it : m_iogroup_ptr) {
        if (it->is_valid_control("FREQ")) {
            // called once for initial push, once for converting domain, once for each cpu,
            EXPECT_CALL(*it, control_domain_type("FREQ")).Times(2 + M_NUM_CPU);
            // adjusting package will adjust every cpu on the package
            for (unsigned int ii = 0; ii < M_NUM_CPU; ++ii) {
                EXPECT_CALL(*it, push_control("FREQ", IPlatformTopo::M_DOMAIN_CPU, ii))
                    .WillOnce(Return(ii));
                EXPECT_CALL(*it, adjust(ii, value));
            }
        }
        EXPECT_CALL(*it, write_batch());
    }
    int freq_idx = m_platio->push_control("FREQ", IPlatformTopo::M_DOMAIN_PACKAGE, 0);
    m_platio->adjust(freq_idx, value);
    m_platio->write_batch();
}

TEST_F(PlatformIOTest, read_signal)
{
    for (auto &it : m_iogroup_ptr) {
        if (it->is_valid_signal("FREQ")) {
            EXPECT_CALL(*it, read_signal("FREQ", IPlatformTopo::M_DOMAIN_CPU, 0)).WillOnce(Return(4e9));
        }
        if (it->is_valid_signal("TIME")) {
            EXPECT_CALL(*it, read_signal("TIME", IPlatformTopo::M_DOMAIN_BOARD, 0)).WillOnce(Return(2.0));
        }
        EXPECT_CALL(*it, read_batch()).Times(0);
    }
    double freq = m_platio->read_signal("FREQ", IPlatformTopo::M_DOMAIN_CPU, 0);
    double time = m_platio->read_signal("TIME", IPlatformTopo::M_DOMAIN_BOARD, 0);
    EXPECT_DOUBLE_EQ(4e9, freq);
    EXPECT_DOUBLE_EQ(2.0, time);
    GEOPM_EXPECT_THROW_MESSAGE(m_platio->read_signal("INVALID", IPlatformTopo::M_DOMAIN_CPU, 0),
                               GEOPM_ERROR_INVALID, "signal name \"INVALID\" not found");
}

TEST_F(PlatformIOTest, write_control)
{
    for (auto &it : m_iogroup_ptr) {
        if (it->is_valid_control("FREQ")) {
            EXPECT_CALL(*it, write_control("FREQ", IPlatformTopo::M_DOMAIN_CPU, 0, 3e9));
        }
        EXPECT_CALL(*it, write_batch()).Times(0);
    }
    m_platio->write_control("FREQ", IPlatformTopo::M_DOMAIN_CPU, 0, 3e9);
    GEOPM_EXPECT_THROW_MESSAGE(m_platio->write_control("INVALID", IPlatformTopo::M_DOMAIN_CPU, 0, 0.0),
                               GEOPM_ERROR_INVALID, "control name \"INVALID\" not found");
}

TEST_F(PlatformIOTest, read_signal_override)
{
    for (auto &it : m_iogroup_ptr) {
        EXPECT_CALL(*it, signal_domain_type("MODE"));
        if (it->signal_domain_type("MODE") == IPlatformTopo::M_DOMAIN_BOARD) {
            EXPECT_CALL(*it, read_signal("MODE", IPlatformTopo::M_DOMAIN_CPU, 0)).WillOnce(Return(5e9));
        }
        else {
            EXPECT_CALL(*it, read_signal(_, _, _)).Times(0);
        }
    }
    double freq = m_platio->read_signal("MODE", IPlatformTopo::M_DOMAIN_CPU, 0);
    EXPECT_DOUBLE_EQ(5e9, freq);
}

TEST_F(PlatformIOTest, agg_function)
{
    for (auto &it : m_iogroup_ptr) {
        if (it->is_valid_signal("REGION_HASH")) {
            EXPECT_CALL(*it, agg_function("REGION_HASH"));
        }
    }

    auto region_id_func = m_platio->agg_function("REGION_HASH");
    EXPECT_EQ(geopm_field_to_signal(GEOPM_REGION_ID_UNMARKED),
              region_id_func({5, 6, 7}));

    GEOPM_EXPECT_THROW_MESSAGE(m_platio->agg_function("INVALID"),
                               GEOPM_ERROR_INVALID, "unknown how to aggregate");
}
