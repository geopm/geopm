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
#include "gtest/gtest.h"
#include "gmock/gmock.h"

#include "geopm_message.h"
#include "geopm_hash.h"
#include "IOGroup.hpp"
#include "MockIOGroup.hpp"
#include "MockPlatformTopo.hpp"
#include "PlatformIOInternal.hpp"
#include "PlatformTopo.hpp"
#include "Exception.hpp"
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
        const int M_NUM_CPU = 4;
};

void PlatformIOTest::SetUp()
{
    std::list<std::shared_ptr<IOGroup>> iogroup_list;

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
    tmp->set_valid_signal_names({"REGION_ID#"});
    ON_CALL(*tmp, signal_domain_type("REGION_ID#"))
        .WillByDefault(Return(IPlatformTopo::M_DOMAIN_CPU));

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
    ON_CALL(m_topo, domain_cpus(IPlatformTopo::M_DOMAIN_BOARD, _, _))
        .WillByDefault(SetArgReferee<2>(cpu_set));
    ON_CALL(m_topo, domain_cpus(IPlatformTopo::M_DOMAIN_BOARD_MEMORY, _, _))
        .WillByDefault(SetArgReferee<2>(cpu_set));
    ON_CALL(m_topo, domain_cpus(IPlatformTopo::M_DOMAIN_PACKAGE, _, _))
        .WillByDefault(SetArgReferee<2>(cpu_set));
    ON_CALL(m_topo, domain_idx(IPlatformTopo::M_DOMAIN_CPU, _))
        .WillByDefault(testing::ReturnArg<1>());

    m_platio.reset(new PlatformIO(iogroup_list, m_topo));
}

TEST_F(PlatformIOTest, signal_control_names)
{
    // IOGroup signals and PlatformIO signals
    std::set<std::string> expected_signals {"TIME", "ENERGY_PACKAGE", "ENERGY_DRAM",
            "REGION_ID#", "FREQ", "MODE", "POWER_PACKAGE", "POWER_DRAM"};
    EXPECT_EQ(expected_signals.size(), m_platio->signal_names().size());
    EXPECT_EQ(expected_signals, m_platio->signal_names());

    std::set<std::string> expected_controls {"FREQ", "MODE"};
    EXPECT_EQ(expected_controls.size(), m_platio->control_names().size());
    EXPECT_EQ(expected_controls, m_platio->control_names());
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
    }

    int domain_type = m_platio->signal_domain_type("TIME");
    EXPECT_EQ(IPlatformTopo::M_DOMAIN_BOARD, domain_type);
    domain_type = m_platio->signal_domain_type("FREQ");
    EXPECT_EQ(IPlatformTopo::M_DOMAIN_CPU, domain_type);
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
    GEOPM_EXPECT_THROW_MESSAGE(m_platio->push_signal("INVALID", IPlatformTopo::M_DOMAIN_CPU, 0),
                               GEOPM_ERROR_INVALID, "signal name \"INVALID\" not found");

    EXPECT_EQ(2, m_platio->num_signal());

    // TODO: test of push_region_signal_total
    // TODO: test of push_combined_signal

    m_platio->read_batch();
    GEOPM_EXPECT_THROW_MESSAGE(m_platio->push_signal("TIME", IPlatformTopo::M_DOMAIN_BOARD, 0),
                               GEOPM_ERROR_INVALID, "pushing signals after");
}

TEST_F(PlatformIOTest, signal_power)
{
    EXPECT_CALL(m_topo, is_domain_within(_, _)).Times(2);
    EXPECT_CALL(m_topo, domain_cpus(_, _, _)).Times(2);
    EXPECT_CALL(m_topo, domain_idx(_, _)).Times(2 * M_NUM_CPU);
    for (auto &it : m_iogroup_ptr) {
        if (it->is_valid_signal("TIME")) {
            EXPECT_CALL(*it, push_signal("TIME", _, _))
                .Times(2);
            EXPECT_CALL(*it, signal_domain_type("TIME"))
                .Times(2);
        }
        if (it->is_valid_signal("ENERGY_PACKAGE")) {
            EXPECT_CALL(*it, push_signal("ENERGY_PACKAGE", _, _))
                .Times(2);
            EXPECT_CALL(*it, signal_domain_type("ENERGY_PACKAGE"))
                .Times(2);
        }
        if (it->is_valid_signal("ENERGY_DRAM")) {
            EXPECT_CALL(*it, push_signal("ENERGY_DRAM", _, _))
                .Times(2);
            EXPECT_CALL(*it, signal_domain_type("ENERGY_DRAM"))
                .Times(2);
        }
        if (it->is_valid_signal("REGION_ID#")) {
            EXPECT_CALL(*it, push_signal("REGION_ID#", _, _))
                .Times(2 *  M_NUM_CPU);
            EXPECT_CALL(*it, signal_domain_type("REGION_ID#"))
                .Times(3 *  M_NUM_CPU);
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
        if (it->is_valid_signal("REGION_ID#")) {
            EXPECT_CALL(*it, sample(0)).Times(6 * M_NUM_CPU)
                .WillRepeatedly(Return(42));
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
        }
    }

    EXPECT_EQ(0, m_platio->num_control());

    int idx = m_platio->push_control("FREQ", IPlatformTopo::M_DOMAIN_CPU, 0);
    EXPECT_EQ(0, idx);
    GEOPM_EXPECT_THROW_MESSAGE(m_platio->push_control("INVALID", IPlatformTopo::M_DOMAIN_CPU, 0),
                               GEOPM_ERROR_INVALID, "control name \"INVALID\" not found");

    EXPECT_EQ(1, m_platio->num_control());
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

TEST_F(PlatformIOTest, sample_region_total)
{
    // expectations for push
    for (auto &it : m_iogroup_ptr) {
        if (it->is_valid_signal("ENERGY_PACKAGE")) {
            EXPECT_CALL(*it, push_signal(_, _, _));
            EXPECT_CALL(*it, signal_domain_type("ENERGY_PACKAGE"));
        }
        if (it->is_valid_signal("TIME")) {
            EXPECT_CALL(*it, push_signal(_, _, _));
            EXPECT_CALL(*it, signal_domain_type("TIME"));
        }
        if (it->is_valid_signal("REGION_ID#")) {
            EXPECT_CALL(*it, push_signal(_, _, _)).Times(4 + 4);
            EXPECT_CALL(*it, signal_domain_type("REGION_ID#")).Times(4 + 4 + 4);
        }
    }
    // region id signal is per cpu, so needs to be aggregated for both package and board
    EXPECT_CALL(m_topo, is_domain_within(IPlatformTopo::M_DOMAIN_CPU, IPlatformTopo::M_DOMAIN_PACKAGE));
    EXPECT_CALL(m_topo, is_domain_within(IPlatformTopo::M_DOMAIN_CPU, IPlatformTopo::M_DOMAIN_BOARD));
    EXPECT_CALL(m_topo, domain_cpus(IPlatformTopo::M_DOMAIN_PACKAGE, 0, _));
    EXPECT_CALL(m_topo, domain_cpus(IPlatformTopo::M_DOMAIN_BOARD, 0, _));
    EXPECT_CALL(m_topo, domain_idx(IPlatformTopo::M_DOMAIN_CPU, _)).Times(4 + 4);

    int nrg_idx = m_platio->push_signal("ENERGY_PACKAGE", IPlatformTopo::M_DOMAIN_PACKAGE, 0);
    int time_idx = m_platio->push_signal("TIME", IPlatformTopo::M_DOMAIN_BOARD, 0);

    m_platio->push_region_signal_total(nrg_idx, IPlatformTopo::M_DOMAIN_PACKAGE, 0);
    m_platio->push_region_signal_total(time_idx, IPlatformTopo::M_DOMAIN_BOARD, 0);

    uint64_t rid1 = 0x444;
    uint64_t rid2 = 0x555;
    double rid1sig = geopm_field_to_signal(rid1);
    double rid2sig = geopm_field_to_signal(rid2);

    // expected return values and counts for each iteration
    // sample count: 4 times for each signal in sample_total, 4 times in read_batch for each signal = 24
    // note that these counts will change if number of CPUs per domain changes
    int rid_sample_count = 24;
    // rid 1 enters at batch 0 and exits at batch 2
    // rid 2 enters at batch 2 and exits at batch 4
    // rid 1 enters at batch 4 and is still running at batch 5
    std::vector<double> energy = {100, 200, 300, 400, 500, 600};
    std::vector<double> time = {1.0, 2.0, 3.0, 4.0, 5.0, 6.0};
    std::vector<double> region = {rid1sig, rid1sig, rid2sig, rid2sig, rid1sig};
    std::vector<double> exp_rid1_nrg  = {0, 100, 200, 200, 200, 300};
    std::vector<double> exp_rid1_time = {0, 1.0, 2.0, 2.0, 2.0, 3.0};
    std::vector<double> exp_rid2_nrg  = {0, 0, 0, 100, 200, 200};
    std::vector<double> exp_rid2_time = {0, 0, 0, 1.0, 2.0, 2.0};
    int num_batch = region.size();

    for (int batch = 0; batch < num_batch; ++batch) {
        for (auto &it : m_iogroup_ptr) {
            if (it->is_valid_signal("ENERGY_PACKAGE")) {
                EXPECT_CALL(*it, sample(0)).Times(2)
                    .WillRepeatedly(Return(energy[batch]));
            }
            if (it->is_valid_signal("TIME")) {
                EXPECT_CALL(*it, sample(0)).Times(2)
                    .WillRepeatedly(Return(time[batch]));
            }
            if (it->is_valid_signal("REGION_ID#")) {
                EXPECT_CALL(*it, sample(0)).Times(rid_sample_count)
                    .WillRepeatedly(Return(region[batch]));
            }
            EXPECT_CALL(*it, read_batch());
        }
        m_platio->read_batch();

        EXPECT_EQ(exp_rid1_nrg[batch], m_platio->sample_region_total(nrg_idx, rid1));
        EXPECT_EQ(exp_rid1_time[batch], m_platio->sample_region_total(time_idx, rid1));
        EXPECT_EQ(exp_rid2_nrg[batch], m_platio->sample_region_total(nrg_idx, rid2));
        EXPECT_EQ(exp_rid2_time[batch], m_platio->sample_region_total(time_idx, rid2));
    }
}

TEST_F(PlatformIOTest, adjust)
{
    for (auto &it : m_iogroup_ptr) {
        if (it->is_valid_control("FREQ")) {
            EXPECT_CALL(*it, push_control("FREQ", _, _));
            EXPECT_CALL(*it, adjust(0, 3e9));
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
    std::vector<double> data {16, 2, 4, 9, 128, 32, 4, 64};
    double sum = 259;
    double average = 32.375;
    double median = 12.5;
    double min = 2;
    double max = 128;
    double stddev = 43.902;
    EXPECT_DOUBLE_EQ(sum, IPlatformIO::agg_sum(data));
    EXPECT_DOUBLE_EQ(average, IPlatformIO::agg_average(data));
    EXPECT_DOUBLE_EQ(median, IPlatformIO::agg_median(data));
    EXPECT_DOUBLE_EQ(4, IPlatformIO::agg_median({4}));
    EXPECT_DOUBLE_EQ(4, IPlatformIO::agg_median({2, 4, 6}));
    EXPECT_DOUBLE_EQ(min, IPlatformIO::agg_min(data));
    EXPECT_DOUBLE_EQ(max, IPlatformIO::agg_max(data));
    EXPECT_NEAR(stddev, IPlatformIO::agg_stddev(data), 0.001);

    EXPECT_EQ(1.0, IPlatformIO::agg_and({1.0, 1.0}));
    EXPECT_EQ(0.0, IPlatformIO::agg_and({1.0, 1.0, 0.0}));
    EXPECT_EQ(1.0, IPlatformIO::agg_or({1.0, 1.0}));
    EXPECT_EQ(1.0, IPlatformIO::agg_or({1.0, 1.0, 0.0}));
    EXPECT_EQ(0.0, IPlatformIO::agg_or({0.0, 0.0}));

    EXPECT_EQ(geopm_field_to_signal(GEOPM_REGION_ID_UNMARKED),
              IPlatformIO::agg_region_id({5, 6, 7}));
    EXPECT_EQ(77,
              IPlatformIO::agg_region_id({77, 77, geopm_field_to_signal(GEOPM_REGION_ID_UNDEFINED), 77}));

    auto region_id_func = m_platio->agg_function("REGION_ID#");
    EXPECT_EQ(geopm_field_to_signal(GEOPM_REGION_ID_UNMARKED),
              region_id_func({5, 6, 7}));
    EXPECT_EQ(77,
              region_id_func({77, 77, geopm_field_to_signal(GEOPM_REGION_ID_UNDEFINED), 77}));

    GEOPM_EXPECT_THROW_MESSAGE(m_platio->agg_function("INVALID"),
                               GEOPM_ERROR_INVALID, "unknown how to aggregate");

}
