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
#include <memory>
#include <string>
#include <algorithm>

#include "gtest/gtest.h"
#include "gmock/gmock.h"

#include "IOGroup.hpp"
#include "PlatformIOInternal.hpp"
#include "PlatformTopo.hpp"
#include "Exception.hpp"
#include "geopm_test.hpp"

using geopm::IOGroup;
using geopm::PlatformIO;
using geopm::PlatformTopo;

using ::testing::_;
using ::testing::Return;

class _MockIOGroup : public geopm::IOGroup
{
    public:

        void set_valid_signal_names(std::list<std::string> names) {
            m_valid_signals = names;
        }
        void set_valid_control_names(std::list<std::string> names) {
            m_valid_controls = names;
        }
        bool is_valid_signal(const std::string &signal_name) override
        {
            return std::find(m_valid_signals.begin(),
                             m_valid_signals.end(),
                             signal_name) != m_valid_signals.end();
        }
        bool is_valid_control(const std::string &control_name) override
        {
            return std::find(m_valid_controls.begin(),
                             m_valid_controls.end(),
                             control_name) != m_valid_controls.end();
        }
        MOCK_METHOD1(signal_domain_type,
                     int(const std::string &signal_name));
        MOCK_METHOD1(control_domain_type,
                     int(const std::string &control_name));
        MOCK_METHOD3(push_signal,
                     int(const std::string &signal_name, int domain_type, int domain_idx));
        MOCK_METHOD3(push_control,
                     int(const std::string &control_name, int domain_type, int domain_idx));
        MOCK_METHOD0(read_batch,
                     void(void));
        MOCK_METHOD0(write_batch,
                     void(void));
        MOCK_METHOD1(sample,
                     double(int batch_idx));
        MOCK_METHOD2(adjust,
                     void(int batch_idx, double setting));
        MOCK_METHOD3(read_signal,
                     double(const std::string &signal_name,
                            int domain_type, int domain_idx));
        MOCK_METHOD4(write_control,
            void(const std::string &control_name, int domain_type,
                 int domain_idx, double setting));

    protected:
        std::list<std::string> m_valid_signals;
        std::list<std::string> m_valid_controls;
};

class PlatformIOTest : public ::testing::Test
{
    protected:
        void SetUp();
        std::list<_MockIOGroup *> m_iogroup_ptr;
        std::unique_ptr<PlatformIO> m_platio;
};

void PlatformIOTest::SetUp()
{
    std::list<std::unique_ptr<IOGroup>> iogroup_list;
    auto tmp = new _MockIOGroup;
    iogroup_list.emplace_back(tmp);
    m_iogroup_ptr.push_back(tmp);
    tmp->set_valid_signal_names({"TIME"});
    ON_CALL(*tmp, signal_domain_type("TIME"))
        .WillByDefault(Return(PlatformTopo::M_DOMAIN_BOARD));

    tmp = new _MockIOGroup;
    iogroup_list.emplace_back(tmp);
    m_iogroup_ptr.push_back(tmp);
    tmp->set_valid_signal_names({"FREQ", "POWER"});
    tmp->set_valid_control_names({"FREQ", "POWER"});
    ON_CALL(*tmp, signal_domain_type("FREQ"))
        .WillByDefault(Return(PlatformTopo::M_DOMAIN_CPU));
    ON_CALL(*tmp, control_domain_type("FREQ"))
        .WillByDefault(Return(PlatformTopo::M_DOMAIN_CPU));
    ON_CALL(*tmp, signal_domain_type("POWER"))
        .WillByDefault(Return(PlatformTopo::M_DOMAIN_PACKAGE));
    ON_CALL(*tmp, control_domain_type("POWER"))
        .WillByDefault(Return(PlatformTopo::M_DOMAIN_PACKAGE));

    tmp = new _MockIOGroup;
    iogroup_list.emplace_back(tmp);
    m_iogroup_ptr.push_back(tmp);
    tmp->set_valid_signal_names({"POWER"});
    tmp->set_valid_control_names({"POWER"});
    ON_CALL(*tmp, signal_domain_type("POWER"))
        .WillByDefault(Return(PlatformTopo::M_DOMAIN_BOARD));
    ON_CALL(*tmp, control_domain_type("POWER"))
        .WillByDefault(Return(PlatformTopo::M_DOMAIN_BOARD));

    m_platio.reset(new PlatformIO(std::move(iogroup_list)));
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
    EXPECT_EQ(PlatformTopo::M_DOMAIN_BOARD, domain_type);
    domain_type = m_platio->signal_domain_type("FREQ");
    EXPECT_EQ(PlatformTopo::M_DOMAIN_CPU, domain_type);
    EXPECT_THROW_MESSAGE(m_platio->signal_domain_type("INVALID"),
                         GEOPM_ERROR_INVALID, "signal name \"INVALID\" not found");

    domain_type = m_platio->control_domain_type("FREQ");
    EXPECT_EQ(PlatformTopo::M_DOMAIN_CPU, domain_type);
    EXPECT_THROW_MESSAGE(m_platio->control_domain_type("INVALID"),
                         GEOPM_ERROR_INVALID, "control name \"INVALID\" not found");
}

TEST_F(PlatformIOTest, push_signal)
{
    for (auto &it : m_iogroup_ptr) {
        if (it->is_valid_signal("TIME")) {
            EXPECT_CALL(*it, push_signal("TIME", _, _));
        }
        if (it->is_valid_signal("FREQ")) {
            EXPECT_CALL(*it, push_signal("FREQ", _, _));
        }
        EXPECT_CALL(*it, read_batch());
    }

    EXPECT_EQ(0, m_platio->num_signal());

    int idx = m_platio->push_signal("FREQ", PlatformTopo::M_DOMAIN_CPU, 0);
    EXPECT_EQ(0, idx);
    idx = m_platio->push_signal("TIME", PlatformTopo::M_DOMAIN_CPU, 0);
    EXPECT_EQ(1, idx);
    EXPECT_THROW_MESSAGE(m_platio->push_signal("INVALID", PlatformTopo::M_DOMAIN_CPU, 0),
                         GEOPM_ERROR_INVALID, "signal name \"INVALID\" not found");


    EXPECT_EQ(2, m_platio->num_signal());

    m_platio->read_batch();

    EXPECT_THROW_MESSAGE(m_platio->push_signal("TIME", PlatformTopo::M_DOMAIN_CPU, 0),
                         GEOPM_ERROR_INVALID, "pushing signals after");
}

TEST_F(PlatformIOTest, push_control)
{
    for (auto &it : m_iogroup_ptr) {
        if (it->is_valid_control("FREQ")) {
            EXPECT_CALL(*it, push_control("FREQ", PlatformTopo::M_DOMAIN_CPU, 0));
        }
    }

    EXPECT_EQ(0, m_platio->num_control());

    int idx = m_platio->push_control("FREQ", PlatformTopo::M_DOMAIN_CPU, 0);
    EXPECT_EQ(0, idx);
    EXPECT_THROW_MESSAGE(m_platio->push_control("INVALID", PlatformTopo::M_DOMAIN_CPU, 0),
                         GEOPM_ERROR_INVALID, "control name \"INVALID\" not found");

    EXPECT_EQ(1, m_platio->num_control());
}

TEST_F(PlatformIOTest, sample)
{
    for (auto &it : m_iogroup_ptr) {
        if (it->is_valid_signal("FREQ")) {
            EXPECT_CALL(*it, sample(0)).WillOnce(Return(2e9));
            EXPECT_CALL(*it, push_signal(_, _, _));
        }
        if (it->is_valid_signal("TIME")) {
            EXPECT_CALL(*it, sample(0)).WillOnce(Return(1.0));
            EXPECT_CALL(*it, push_signal(_, _, _));
        }
        EXPECT_CALL(*it, read_batch());
    }
    int freq_idx = m_platio->push_signal("FREQ", PlatformTopo::M_DOMAIN_CPU, 0);
    int time_idx = m_platio->push_signal("TIME", PlatformTopo::M_DOMAIN_CPU, 0);
    m_platio->read_batch();
    EXPECT_EQ(0, freq_idx);
    EXPECT_EQ(1, time_idx);
    double freq = m_platio->sample(freq_idx);
    EXPECT_DOUBLE_EQ(2e9, freq);
    double time = m_platio->sample(time_idx);
    EXPECT_DOUBLE_EQ(1.0, time);
    EXPECT_THROW_MESSAGE(m_platio->sample(-1), GEOPM_ERROR_INVALID, "signal_idx out of range");
    EXPECT_THROW_MESSAGE(m_platio->sample(10), GEOPM_ERROR_INVALID, "signal_idx out of range");
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
    int freq_idx = m_platio->push_control("FREQ", PlatformTopo::M_DOMAIN_CPU, 0);
    EXPECT_EQ(0, freq_idx);
    m_platio->adjust(freq_idx, 3e9);
    m_platio->write_batch();
    EXPECT_THROW_MESSAGE(m_platio->adjust(-1, 0.0), GEOPM_ERROR_INVALID, "control_idx out of range");
    EXPECT_THROW_MESSAGE(m_platio->adjust(10, 0.0), GEOPM_ERROR_INVALID, "control_idx out of range");
}

TEST_F(PlatformIOTest, read_signal)
{
    for (auto &it : m_iogroup_ptr) {
        if (it->is_valid_signal("FREQ")) {
            EXPECT_CALL(*it, read_signal("FREQ", PlatformTopo::M_DOMAIN_CPU, 0)).WillOnce(Return(4e9));
        }
        if (it->is_valid_signal("TIME")) {
            EXPECT_CALL(*it, read_signal("TIME", PlatformTopo::M_DOMAIN_CPU, 0)).WillOnce(Return(2.0));
        }
        EXPECT_CALL(*it, read_batch()).Times(0);
    }
    double freq = m_platio->read_signal("FREQ", PlatformTopo::M_DOMAIN_CPU, 0);
    double time = m_platio->read_signal("TIME", PlatformTopo::M_DOMAIN_CPU, 0);
    EXPECT_DOUBLE_EQ(4e9, freq);
    EXPECT_DOUBLE_EQ(2.0, time);
    EXPECT_THROW_MESSAGE(m_platio->read_signal("INVALID", PlatformTopo::M_DOMAIN_CPU, 0),
                         GEOPM_ERROR_INVALID, "signal name \"INVALID\" not found");
}

TEST_F(PlatformIOTest, write_control)
{
    for (auto &it : m_iogroup_ptr) {
        if (it->is_valid_control("FREQ")) {
            EXPECT_CALL(*it, write_control("FREQ", PlatformTopo::M_DOMAIN_CPU, 0, 3e9));
        }
        EXPECT_CALL(*it, write_batch()).Times(0);
    }
    m_platio->write_control("FREQ", PlatformTopo::M_DOMAIN_CPU, 0, 3e9);
    EXPECT_THROW_MESSAGE(m_platio->write_control("INVALID", PlatformTopo::M_DOMAIN_CPU, 0, 0.0),
                         GEOPM_ERROR_INVALID, "control name \"INVALID\" not found");
}

TEST_F(PlatformIOTest, read_signal_override)
{
    for (auto &it : m_iogroup_ptr) {
        EXPECT_CALL(*it, signal_domain_type("POWER"));
        if (it->signal_domain_type("POWER") == PlatformTopo::M_DOMAIN_BOARD) {
            EXPECT_CALL(*it, read_signal("POWER", PlatformTopo::M_DOMAIN_CPU, 0)).WillOnce(Return(5e9));
        }
        else {
            EXPECT_CALL(*it, read_signal(_, _, _)).Times(0);
        }
    }
    double freq = m_platio->read_signal("POWER", PlatformTopo::M_DOMAIN_CPU, 0);
    EXPECT_DOUBLE_EQ(5e9, freq);
}
