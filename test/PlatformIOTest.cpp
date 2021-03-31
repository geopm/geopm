/*
 * Copyright (c) 2015 - 2021, Intel Corporation
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
#include "PlatformIOImp.hpp"
#include "IOGroup.hpp"
#include "MockIOGroup.hpp"
#include "MockPlatformTopo.hpp"
#include "PlatformTopo.hpp"
#include "Exception.hpp"
#include "Agg.hpp"
#include "CombinedSignal.hpp"
#include "geopm_test.hpp"

using geopm::IOGroup;
using geopm::PlatformIO;
using geopm::PlatformIOImp;
using geopm::PlatformTopo;
using geopm::Agg;

using ::testing::_;
using ::testing::Return;
using ::testing::SetArgReferee;
using ::testing::AtLeast;

class PlatformIOTestMockIOGroup : public MockIOGroup
{
    public:
        PlatformIOTestMockIOGroup()
        {
            // Suppress warnings; these methods are called by
            //  PlatformIO::find_signal_iogroup() and
            //  PlatformIO::find_control_iogroup() to determine the
            //  IOGroup that provides a signal or control.  The number
            //  of times depends on an IOGroup's order in the list of
            //  registered plugins
            EXPECT_CALL(*this, is_valid_signal(_)).Times(AtLeast(0));
            EXPECT_CALL(*this, is_valid_control(_)).Times(AtLeast(0));
        }

        // Set up mock behavior for the IOGroup to provide a set of signals for specific domains
        void set_valid_signals(const std::vector<std::pair<std::string, int> > &signals)
        {
            // All signals are invalid unless in provided list
            ON_CALL(*this, is_valid_signal(_)).WillByDefault(Return(false));
            ON_CALL(*this, signal_domain_type(_)).WillByDefault(Return(GEOPM_DOMAIN_INVALID));
            std::set<std::string> names;
            int index = 0;
            for (const auto &sig : signals) {
                names.insert(sig.first);
                ON_CALL(*this, is_valid_signal(sig.first))
                    .WillByDefault(Return(true));
                ON_CALL(*this, signal_domain_type(sig.first))
                    .WillByDefault(Return(sig.second));
                ON_CALL(*this, push_signal(sig.first, _, _))
                    .WillByDefault(Return(index));
                ++index;
            }
            ON_CALL(*this, signal_names()).WillByDefault(Return(names));
        }

        // Set up mock behavior for the IOGroup to provide a set of controls for specific domains
        void set_valid_controls(const std::vector<std::pair<std::string, int> > &controls)
        {
            // All controls are invalid unless in provided list
            ON_CALL(*this, is_valid_control(_)).WillByDefault(Return(false));
            ON_CALL(*this, control_domain_type(_)).WillByDefault(Return(GEOPM_DOMAIN_INVALID));
            std::set<std::string> names;
            int index = 0;
            for (const auto &con : controls) {
                names.insert(con.first);
                ON_CALL(*this, is_valid_control(con.first))
                    .WillByDefault(Return(true));
                ON_CALL(*this, control_domain_type(con.first))
                    .WillByDefault(Return(con.second));
                ON_CALL(*this, push_control(con.first, _, _))
                    .WillByDefault(Return(index));
                ++index;
            }
            ON_CALL(*this, control_names()).WillByDefault(Return(names));
        }
};

class PlatformIOTest : public ::testing::Test
{
    protected:
        void SetUp();
        std::list<std::shared_ptr<PlatformIOTestMockIOGroup> > m_iogroup_ptr;
        std::unique_ptr<PlatformIOImp> m_platio;
        std::shared_ptr<MockPlatformTopo> m_topo;
        std::set<int> m_cpu_set_board;
        std::set<int> m_cpu_set0;
        std::set<int> m_cpu_set1;

        std::shared_ptr<PlatformIOTestMockIOGroup> m_time_iogroup;
        std::shared_ptr<PlatformIOTestMockIOGroup> m_control_iogroup;
        std::shared_ptr<PlatformIOTestMockIOGroup> m_override_iogroup;
};

void PlatformIOTest::SetUp()
{
    // Basic IOGroup
    m_time_iogroup = std::make_shared<PlatformIOTestMockIOGroup>();
    m_time_iogroup->set_valid_signals({{"TIME", GEOPM_DOMAIN_BOARD}});

    // IOGroup with signals and controls with the same name
    m_control_iogroup = std::make_shared<PlatformIOTestMockIOGroup>();
    m_control_iogroup->set_valid_signals({
            {"FREQ", GEOPM_DOMAIN_CPU},
            {"MODE", GEOPM_DOMAIN_PACKAGE}});
    m_control_iogroup->set_valid_controls({
            {"FREQ", GEOPM_DOMAIN_CPU},
            {"MODE", GEOPM_DOMAIN_PACKAGE}});

    // IOGroup that overrides previously registered signals and controls
    m_override_iogroup = std::make_shared<PlatformIOTestMockIOGroup>();
    m_override_iogroup->set_valid_signals({{"MODE", GEOPM_DOMAIN_BOARD}});
    m_override_iogroup->set_valid_controls({{"MODE", GEOPM_DOMAIN_BOARD}});

    // Settings for PlatformTopo: 2 socket, 4 cpus each
    m_topo = make_topo(2, 4, 8);
    m_cpu_set_board = {0, 1, 2, 3, 4, 5, 6, 7};
    m_cpu_set0 = {0, 1, 4, 5};
    m_cpu_set1 = {2, 3, 6, 7};
    // suppress warnings about num_domain calls
    EXPECT_CALL(*m_topo, num_domain(_)).Times(AtLeast(0));

    m_iogroup_ptr = {
        m_time_iogroup,
        m_control_iogroup,
        m_override_iogroup
    };
    // construct list of base pointers for contructor
    std::list<std::shared_ptr<IOGroup> > iogroup_list;
    for (auto ptr : m_iogroup_ptr) {
        iogroup_list.emplace_back(ptr);
    }

    m_platio.reset(new PlatformIOImp(iogroup_list, *m_topo));
}

TEST_F(PlatformIOTest, signal_control_names)
{
    for (auto iog : m_iogroup_ptr) {
        EXPECT_CALL(*iog, signal_names());
        EXPECT_CALL(*iog, control_names());
    }

    // IOGroup signals and PlatformIO signals
    std::set<std::string> expected_signals = {
        "TIME",
        "FREQ", "MODE",
    };
    std::set<std::string> result = m_platio->signal_names();
    EXPECT_EQ(expected_signals.size(), result.size());
    EXPECT_EQ(expected_signals, result);

    std::set<std::string> expected_controls {"FREQ", "MODE"};
    result =  m_platio->control_names();
    EXPECT_EQ(expected_controls.size(), result.size());
    EXPECT_EQ(expected_controls, result);
}

TEST_F(PlatformIOTest, signal_control_description)
{
    std::string time_description = "time signal";
    std::string freq_signal_desc = "freq signal";
    std::string freq_control_desc = "freq control";
    EXPECT_CALL(*m_time_iogroup, signal_description("TIME"))
        .WillOnce(Return(time_description));
    EXPECT_CALL(*m_control_iogroup, signal_description("FREQ"))
        .WillOnce(Return(freq_signal_desc));
    EXPECT_CALL(*m_control_iogroup, control_description("FREQ"))
        .WillOnce(Return(freq_control_desc));
    EXPECT_EQ(time_description, m_platio->signal_description("TIME"));
    EXPECT_EQ(freq_signal_desc, m_platio->signal_description("FREQ"));
    EXPECT_EQ(freq_control_desc, m_platio->control_description("FREQ"));
}

TEST_F(PlatformIOTest, domain_type)
{
    int domain_type = GEOPM_DOMAIN_INVALID;

    EXPECT_CALL(*m_time_iogroup, signal_domain_type("TIME"));
    domain_type = m_platio->signal_domain_type("TIME");
    EXPECT_EQ(GEOPM_DOMAIN_BOARD, domain_type);

    EXPECT_CALL(*m_control_iogroup, signal_domain_type("FREQ"));
    EXPECT_CALL(*m_control_iogroup, control_domain_type("FREQ"));
    domain_type = m_platio->signal_domain_type("FREQ");
    EXPECT_EQ(GEOPM_DOMAIN_CPU, domain_type);
    domain_type = m_platio->control_domain_type("FREQ");
    EXPECT_EQ(GEOPM_DOMAIN_CPU, domain_type);

    GEOPM_EXPECT_THROW_MESSAGE(m_platio->signal_domain_type("INVALID"),
                               GEOPM_ERROR_INVALID, "signal name \"INVALID\" not found");
    GEOPM_EXPECT_THROW_MESSAGE(m_platio->control_domain_type("INVALID"),
                               GEOPM_ERROR_INVALID, "control name \"INVALID\" not found");
}

TEST_F(PlatformIOTest, push_signal)
{
    int idx = -1;
    EXPECT_EQ(0, m_platio->num_signal_pushed());
    EXPECT_CALL(*m_control_iogroup, signal_domain_type("FREQ"));
    EXPECT_CALL(*m_control_iogroup, push_signal("FREQ", GEOPM_DOMAIN_CPU, 0));
    idx = m_platio->push_signal("FREQ", GEOPM_DOMAIN_CPU, 0);
    EXPECT_EQ(0, idx);
    EXPECT_CALL(*m_time_iogroup, signal_domain_type("TIME"));
    EXPECT_CALL(*m_time_iogroup, push_signal("TIME", GEOPM_DOMAIN_BOARD, 0));
    idx = m_platio->push_signal("TIME", GEOPM_DOMAIN_BOARD, 0);
    EXPECT_EQ(1, idx);
    EXPECT_EQ(idx, m_platio->push_signal("TIME", GEOPM_DOMAIN_BOARD, 0));

    GEOPM_EXPECT_THROW_MESSAGE(m_platio->push_signal("INVALID", GEOPM_DOMAIN_CPU, 0),
                               GEOPM_ERROR_INVALID, "no support for signal name \"INVALID\"");

    EXPECT_EQ(2, m_platio->num_signal_pushed());

    for (auto iog : m_iogroup_ptr) {
        EXPECT_CALL(*iog, read_batch());
    }
    m_platio->read_batch();
    GEOPM_EXPECT_THROW_MESSAGE(m_platio->push_signal("TIME", GEOPM_DOMAIN_BOARD, 0),
                               GEOPM_ERROR_INVALID, "pushing signals after");
}

TEST_F(PlatformIOTest, push_signal_agg)
{
    EXPECT_CALL(*m_topo, is_nested_domain(GEOPM_DOMAIN_CPU,
                                         GEOPM_DOMAIN_PACKAGE));
    EXPECT_CALL(*m_topo, domain_nested(GEOPM_DOMAIN_CPU, GEOPM_DOMAIN_PACKAGE, 0));

    EXPECT_CALL(*m_control_iogroup, signal_domain_type("FREQ")).Times(AtLeast(1));
    for (auto cpu : m_cpu_set0) {
        EXPECT_CALL(*m_control_iogroup, push_signal("FREQ", GEOPM_DOMAIN_CPU, cpu));
    }
    EXPECT_CALL(*m_control_iogroup, agg_function("FREQ"))
        .WillOnce(Return(geopm::Agg::average));
    EXPECT_EQ(0, m_platio->num_signal_pushed());
    // Domain of FREQ is CPU
    m_platio->push_signal("FREQ", GEOPM_DOMAIN_PACKAGE, 0);
    EXPECT_EQ(1 + m_cpu_set0.size(), (unsigned int)m_platio->num_signal_pushed());
}

TEST_F(PlatformIOTest, push_control)
{
    EXPECT_EQ(0, m_platio->num_control_pushed());

    EXPECT_CALL(*m_control_iogroup, control_domain_type("FREQ"));
    EXPECT_CALL(*m_control_iogroup, push_control("FREQ", GEOPM_DOMAIN_CPU, 0));
    EXPECT_CALL(*m_control_iogroup, save_control());
    m_platio->save_control();
    int idx = m_platio->push_control("FREQ", GEOPM_DOMAIN_CPU, 0);
    EXPECT_EQ(0, idx);
    EXPECT_EQ(idx, m_platio->push_control("FREQ", GEOPM_DOMAIN_CPU, 0));
    GEOPM_EXPECT_THROW_MESSAGE(m_platio->push_control("INVALID", GEOPM_DOMAIN_CPU, 0),
                               GEOPM_ERROR_INVALID, "control name \"INVALID\" not found");

    EXPECT_EQ(1, m_platio->num_control_pushed());
}

TEST_F(PlatformIOTest, push_control_agg)
{
    EXPECT_CALL(*m_topo, is_nested_domain(GEOPM_DOMAIN_CPU,
                                         GEOPM_DOMAIN_PACKAGE));
    EXPECT_CALL(*m_topo, domain_nested(GEOPM_DOMAIN_CPU, GEOPM_DOMAIN_PACKAGE, 0));
    EXPECT_EQ(0, m_platio->num_control_pushed());
    EXPECT_CALL(*m_control_iogroup, control_domain_type("FREQ")).Times(AtLeast(1));
    EXPECT_CALL(*m_control_iogroup, save_control());
    m_platio->save_control();
    for (auto cpu : m_cpu_set0) {
        EXPECT_CALL(*m_control_iogroup, push_control("FREQ", GEOPM_DOMAIN_CPU, cpu));
    }
    m_platio->push_control("FREQ", GEOPM_DOMAIN_PACKAGE, 0);
    EXPECT_EQ(1 + m_cpu_set0.size(), (unsigned int)m_platio->num_control_pushed());
}

TEST_F(PlatformIOTest, sample)
{
    EXPECT_CALL(*m_control_iogroup, signal_domain_type("FREQ"));
    EXPECT_CALL(*m_control_iogroup, push_signal("FREQ", _, _));
    EXPECT_CALL(*m_time_iogroup, signal_domain_type("TIME"));
    EXPECT_CALL(*m_time_iogroup, push_signal("TIME", _, _));
    int freq_idx = m_platio->push_signal("FREQ", GEOPM_DOMAIN_CPU, 0);
    int time_idx = m_platio->push_signal("TIME", GEOPM_DOMAIN_BOARD, 0);

    for (auto iog : m_iogroup_ptr) {
        EXPECT_CALL(*iog, read_batch());
    }
    m_platio->read_batch();
    EXPECT_EQ(0, freq_idx);
    EXPECT_EQ(1, time_idx);

    EXPECT_CALL(*m_control_iogroup, sample(0))
        .WillOnce(Return(2e9));
    EXPECT_CALL(*m_time_iogroup, sample(0))
        .WillOnce(Return(1.0));

    double freq = m_platio->sample(freq_idx);
    EXPECT_DOUBLE_EQ(2e9, freq);
    double time = m_platio->sample(time_idx);
    EXPECT_DOUBLE_EQ(1.0, time);

    GEOPM_EXPECT_THROW_MESSAGE(m_platio->sample(-1), GEOPM_ERROR_INVALID, "signal_idx out of range");
    GEOPM_EXPECT_THROW_MESSAGE(m_platio->sample(10), GEOPM_ERROR_INVALID, "signal_idx out of range");
}

TEST_F(PlatformIOTest, sample_agg)
{
    EXPECT_CALL(*m_topo, is_nested_domain(GEOPM_DOMAIN_CPU,
                                         GEOPM_DOMAIN_PACKAGE));
    EXPECT_CALL(*m_topo, domain_nested(GEOPM_DOMAIN_CPU, GEOPM_DOMAIN_PACKAGE, 0));
    EXPECT_CALL(*m_control_iogroup, signal_domain_type("FREQ")).Times(AtLeast(1));
    EXPECT_CALL(*m_control_iogroup, agg_function("FREQ"))
        .WillOnce(Return(geopm::Agg::average));
    for (auto cpu : m_cpu_set0) {
        EXPECT_CALL(*m_control_iogroup, push_signal("FREQ", GEOPM_DOMAIN_CPU, cpu))
            .WillOnce(Return(cpu));
    }
    int freq_idx = m_platio->push_signal("FREQ", GEOPM_DOMAIN_PACKAGE, 0);

    for (auto iog : m_iogroup_ptr) {
        EXPECT_CALL(*iog, read_batch());
    }
    m_platio->read_batch();

    for (auto cpu : m_cpu_set0) {
        EXPECT_CALL(*m_control_iogroup, sample(cpu)).WillOnce(Return(cpu));
    }
    double freq = m_platio->sample(freq_idx);

    double sum = 0;
    for (auto cpu : m_cpu_set0) {
        sum += cpu;
    }
    EXPECT_DOUBLE_EQ(sum / m_cpu_set0.size(), freq);
}

TEST_F(PlatformIOTest, adjust)
{
    EXPECT_CALL(*m_control_iogroup, control_domain_type("FREQ"));
    EXPECT_CALL(*m_control_iogroup, push_control("FREQ", _, _));
    EXPECT_CALL(*m_control_iogroup, save_control());
    m_platio->save_control();
    int freq_idx = m_platio->push_control("FREQ", GEOPM_DOMAIN_CPU, 0);
    EXPECT_EQ(0, freq_idx);

    EXPECT_CALL(*m_control_iogroup, adjust(0, 3e9));
    m_platio->adjust(freq_idx, 3e9);

    for (auto iog : m_iogroup_ptr) {
        EXPECT_CALL(*iog, write_batch());
    }
    m_platio->write_batch();
    GEOPM_EXPECT_THROW_MESSAGE(m_platio->adjust(-1, 0.0), GEOPM_ERROR_INVALID, "control_idx out of range");
    GEOPM_EXPECT_THROW_MESSAGE(m_platio->adjust(10, 0.0), GEOPM_ERROR_INVALID, "control_idx out of range");
}

TEST_F(PlatformIOTest, adjust_agg)
{
    EXPECT_CALL(*m_topo, is_nested_domain(GEOPM_DOMAIN_CPU,
                                         GEOPM_DOMAIN_PACKAGE));
    EXPECT_CALL(*m_topo, domain_nested(GEOPM_DOMAIN_CPU, GEOPM_DOMAIN_PACKAGE, 0));
    double value = 1.23e9;
    EXPECT_CALL(*m_control_iogroup, control_domain_type("FREQ")).Times(AtLeast(1));
    EXPECT_CALL(*m_control_iogroup, save_control());
    m_platio->save_control();
    for (auto cpu : m_cpu_set0) {
        EXPECT_CALL(*m_control_iogroup, push_control("FREQ", GEOPM_DOMAIN_CPU, cpu))
            .WillOnce(Return(cpu));
    }
    int freq_idx = m_platio->push_control("FREQ", GEOPM_DOMAIN_PACKAGE, 0);

    for (auto cpu : m_cpu_set0) {
        EXPECT_CALL(*m_control_iogroup, adjust(cpu, value));
    }
    m_platio->adjust(freq_idx, value);

    for (auto iog : m_iogroup_ptr) {
        EXPECT_CALL(*iog, write_batch());
    }
    m_platio->write_batch();
}

TEST_F(PlatformIOTest, read_signal)
{
    EXPECT_CALL(*m_topo, is_nested_domain(_, _));
    EXPECT_CALL(*m_control_iogroup, signal_domain_type("FREQ"));
    EXPECT_CALL(*m_control_iogroup, read_signal("FREQ", GEOPM_DOMAIN_CPU, 0))
        .WillOnce(Return(4e9));
    double freq = m_platio->read_signal("FREQ", GEOPM_DOMAIN_CPU, 0);
    EXPECT_DOUBLE_EQ(4e9, freq);

    EXPECT_CALL(*m_time_iogroup, signal_domain_type("TIME")).Times(AtLeast(1));
    EXPECT_CALL(*m_time_iogroup, read_signal("TIME", GEOPM_DOMAIN_BOARD, 0))
        .WillOnce(Return(2.0));
    double time = m_platio->read_signal("TIME", GEOPM_DOMAIN_BOARD, 0);
    EXPECT_DOUBLE_EQ(2.0, time);

    GEOPM_EXPECT_THROW_MESSAGE(m_platio->read_signal("INVALID", GEOPM_DOMAIN_CPU, 0),
                               GEOPM_ERROR_INVALID, "signal name \"INVALID\" not found");
    GEOPM_EXPECT_THROW_MESSAGE(m_platio->read_signal("TIME", GEOPM_DOMAIN_BOARD_MEMORY, 0),
                               GEOPM_ERROR_INVALID, "domain 4 is not valid for signal \"TIME\"");
}

TEST_F(PlatformIOTest, read_signal_agg)
{
    EXPECT_CALL(*m_topo, is_nested_domain(_, _));
    EXPECT_CALL(*m_topo, domain_nested(_, _, _));
    EXPECT_CALL(*m_control_iogroup, signal_domain_type("FREQ")).Times(AtLeast(1));
    EXPECT_CALL(*m_control_iogroup, agg_function("FREQ")).WillOnce(Return(Agg::average));
    for (auto cpu : m_cpu_set0) {
        EXPECT_CALL(*m_control_iogroup, read_signal("FREQ", GEOPM_DOMAIN_CPU, cpu))
            .WillOnce(Return(1e9 * (cpu)));
    }
    // CPU from IOGroup is used, not package
    EXPECT_CALL(*m_control_iogroup, read_signal("FREQ", GEOPM_DOMAIN_PACKAGE, _)).Times(0);
    double freq = m_platio->read_signal("FREQ", GEOPM_DOMAIN_PACKAGE, 0);
    double expected = (0 + 1 + 4 + 5) * 1e9 / 4.0;
    EXPECT_DOUBLE_EQ(expected, freq);
}

TEST_F(PlatformIOTest, write_control)
{
    // write_control will not affect pushed controls
    EXPECT_CALL(*m_override_iogroup, write_batch()).Times(0);

    double value = 3e9;
    EXPECT_CALL(*m_topo, is_nested_domain(_, _));
    EXPECT_CALL(*m_override_iogroup, control_domain_type("MODE")).Times(AtLeast(1));
    EXPECT_CALL(*m_override_iogroup, write_control("MODE", GEOPM_DOMAIN_BOARD, 0, value));
    m_platio->write_control("MODE", GEOPM_DOMAIN_BOARD, 0, value);
    GEOPM_EXPECT_THROW_MESSAGE(m_platio->write_control("INVALID", GEOPM_DOMAIN_CPU, 0, 0.0),
                               GEOPM_ERROR_INVALID, "control name \"INVALID\" not found");
    GEOPM_EXPECT_THROW_MESSAGE(m_platio->write_control("MODE", GEOPM_DOMAIN_BOARD_MEMORY, 0, 4e9),
                               GEOPM_ERROR_INVALID, "domain 4 is not valid for control \"MODE\"");
}

TEST_F(PlatformIOTest, write_control_agg)
{
    // write_control will not affect pushed controls
    EXPECT_CALL(*m_override_iogroup, write_batch()).Times(0);

    double value = 3e9;
    EXPECT_CALL(*m_topo, is_nested_domain(_, _));
    EXPECT_CALL(*m_topo, domain_nested(_, _, _));
    EXPECT_CALL(*m_control_iogroup, control_domain_type("FREQ")).Times(AtLeast(1));
    for (auto cpu : m_cpu_set0) {
        EXPECT_CALL(*m_control_iogroup, write_control("FREQ", GEOPM_DOMAIN_CPU, cpu, value));
    }
    // package domain should not be used directly
    EXPECT_CALL(*m_control_iogroup, write_control("FREQ", GEOPM_DOMAIN_PACKAGE, _, _)).Times(0);
    m_platio->write_control("FREQ", GEOPM_DOMAIN_PACKAGE, 0, value);
}

TEST_F(PlatformIOTest, read_signal_override)
{
    // overridden IOGroup will not be used
    EXPECT_CALL(*m_control_iogroup, signal_domain_type("MODE")).Times(0);
    EXPECT_CALL(*m_control_iogroup, read_signal(_, _, _)).Times(0);

    EXPECT_CALL(*m_topo, is_nested_domain(_, _));
    EXPECT_CALL(*m_override_iogroup, signal_domain_type("MODE")).Times(AtLeast(1));
    EXPECT_CALL(*m_override_iogroup, read_signal("MODE", GEOPM_DOMAIN_BOARD, 0))
        .WillOnce(Return(5e9));

    double freq = m_platio->read_signal("MODE", GEOPM_DOMAIN_BOARD, 0);
    EXPECT_DOUBLE_EQ(5e9, freq);

    EXPECT_THROW(m_platio->read_signal("MODE", GEOPM_DOMAIN_PACKAGE, 0),
                 geopm::Exception);
}

TEST_F(PlatformIOTest, write_control_override)
{
    // overridden IOGroup will not be used
    EXPECT_CALL(*m_control_iogroup, control_domain_type("MODE")).Times(0);
    EXPECT_CALL(*m_control_iogroup, write_control(_, _, _, _)).Times(0);

    double value = 10;
    EXPECT_CALL(*m_topo, is_nested_domain(_, _));
    EXPECT_CALL(*m_override_iogroup, control_domain_type("MODE")).Times(AtLeast(1));
    EXPECT_CALL(*m_override_iogroup, write_control("MODE", GEOPM_DOMAIN_BOARD, 0, value));
    m_platio->write_control("MODE", GEOPM_DOMAIN_BOARD, 0, value);

    EXPECT_THROW(m_platio->write_control("MODE", GEOPM_DOMAIN_PACKAGE, 0, value),
                 geopm::Exception);
}

TEST_F(PlatformIOTest, agg_function)
{
    double value = 12.3456;
    EXPECT_CALL(*m_override_iogroup, agg_function("MODE"))
        .WillOnce(Return(
            [value](const std::vector<double> &x) -> double {
                return value;
            }));
    auto mode_func = m_platio->agg_function("MODE");
    EXPECT_EQ(value, mode_func({5, 6, 7}));

    GEOPM_EXPECT_THROW_MESSAGE(m_platio->agg_function("INVALID"),
                               GEOPM_ERROR_INVALID, "unknown how to aggregate");
}

TEST_F(PlatformIOTest, signal_behavior)
{
    int expected_behavior = IOGroup::M_SIGNAL_BEHAVIOR_MONOTONE;
    EXPECT_CALL(*m_time_iogroup, signal_behavior("TIME"))
        .WillOnce(Return(expected_behavior));
    EXPECT_EQ(expected_behavior, m_platio->signal_behavior("TIME"));
    GEOPM_EXPECT_THROW_MESSAGE(m_platio->signal_behavior("INVALID"),
                               GEOPM_ERROR_INVALID, "unknown signal \"INVALID\"");
}
