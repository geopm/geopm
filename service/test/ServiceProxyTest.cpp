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

#include <memory>

#include "ServiceProxy.hpp"
#include "MockSDBus.hpp"
#include "MockSDBusMessage.hpp"
#include "geopm_test.hpp"
#include "geopm/PlatformIO.hpp"

using geopm::signal_info_s;
using geopm::control_info_s;
using geopm::ServiceProxyImp;
using geopm::SDBus;
using geopm::SDBusMessage;
using testing::Return;
using testing::_;

class ServiceProxyTest : public ::testing::Test
{
    protected:
        void SetUp();
        void check_signal_info(const signal_info_s &actual,
                               const signal_info_s &expect) const;
        void check_control_info(const control_info_s &actual,
                                const control_info_s &expect) const;
        std::shared_ptr<MockSDBus> m_bus;
        std::shared_ptr<MockSDBusMessage> m_bus_message;
        std::shared_ptr<MockSDBusMessage> m_bus_reply;
        std::shared_ptr<ServiceProxyImp> m_proxy;
};

void ServiceProxyTest::SetUp()
{
    m_bus = std::make_shared<MockSDBus>();
    m_bus_message = std::make_shared<MockSDBusMessage>();
    m_bus_reply = std::make_shared<MockSDBusMessage>();
    m_proxy = std::make_shared<ServiceProxyImp>(m_bus);
}

TEST_F(ServiceProxyTest, platform_get_user_access)
{
    int num_container = 3; // One struct and two arrays of strings
    EXPECT_CALL(*m_bus, call_method("PlatformGetUserAccess"))
        .WillOnce(Return(m_bus_reply));
    EXPECT_CALL(*m_bus_reply, enter_container(_,_))
        .Times(num_container);
    EXPECT_CALL(*m_bus_reply, exit_container())
        .Times(num_container);
    EXPECT_CALL(*m_bus_reply, read_string())
        .WillOnce(Return("instructions"))
        .WillOnce(Return("misses"))
        .WillOnce(Return("ops"))
        .WillOnce(Return(""))
        .WillOnce(Return("frequency"))
        .WillOnce(Return("power"))
        .WillOnce(Return(""));
    EXPECT_CALL(*m_bus_reply, was_success())
        .WillOnce(Return(true))
        .WillOnce(Return(true))
        .WillOnce(Return(true))
        .WillOnce(Return(false))
        .WillOnce(Return(true))
        .WillOnce(Return(true))
        .WillOnce(Return(false));
    std::vector<std::string> signal_actual;
    std::vector<std::string> control_actual;
    m_proxy->platform_get_user_access(signal_actual, control_actual);
    std::vector<std::string> signal_expect = {"instructions", "misses", "ops"};
    std::vector<std::string> control_expect = {"frequency", "power"};
    EXPECT_EQ(signal_expect, signal_actual);
    EXPECT_EQ(control_expect, control_actual);
}

TEST_F(ServiceProxyTest, platform_get_signal_info)
{
    int num_container = 3; // One array with two structures in it
    EXPECT_CALL(*m_bus_reply, enter_container(_,_))
        .Times(num_container + 1); // Extra enter array of struct termination
    EXPECT_CALL(*m_bus_reply, exit_container())
        .Times(num_container);
    EXPECT_CALL(*m_bus_reply, was_success())
        .WillOnce(Return(true))
        .WillOnce(Return(true))
        .WillOnce(Return(false));
    EXPECT_CALL(*m_bus_reply, read_string())
        .WillOnce(Return("instructions"))
        .WillOnce(Return("Number of instructions retired"))
        .WillOnce(Return("misses"))
        .WillOnce(Return("Number of cache misses"));
    EXPECT_CALL(*m_bus_reply, read_integer())
        .WillOnce(Return(1))
        .WillOnce(Return(2))
        .WillOnce(Return(3))
        .WillOnce(Return(4))
        .WillOnce(Return(5))
        .WillOnce(Return(6))
        .WillOnce(Return(7))
        .WillOnce(Return(8));
    EXPECT_CALL(*m_bus, make_call_message("PlatformGetSignalInfo"))
        .WillOnce(Return(m_bus_message));
    std::vector<std::string> input_names = {"instructions", "misses"};
    EXPECT_CALL(*m_bus_message, append_strings(input_names))
        .Times(1);
    std::shared_ptr<SDBusMessage> bus_message_ptr = m_bus_message;
    EXPECT_CALL(*m_bus, call_method(bus_message_ptr))
        .WillOnce(Return(m_bus_reply));
    std::vector<signal_info_s> info_actual = m_proxy->platform_get_signal_info(input_names);

    std::vector<signal_info_s> info_expect = {{"instructions",
                                               "Number of instructions retired",
                                                1, 2, 3, 4},
                                              {"misses",
                                               "Number of cache misses",
                                                5, 6, 7, 8}};
    ASSERT_EQ(info_actual.size(), info_expect.size());
    for (unsigned info_idx = 0; info_idx != info_actual.size(); ++info_idx) {
        check_signal_info(info_expect.at(info_idx), info_actual.at(info_idx));
    }
}

void ServiceProxyTest::check_signal_info(const signal_info_s &actual,
                                         const signal_info_s &expect) const
{
    EXPECT_EQ(actual.name, expect.name);
    EXPECT_EQ(actual.description, expect.description);
    EXPECT_EQ(actual.domain, expect.domain);
    EXPECT_EQ(actual.aggregation, expect.aggregation);
    EXPECT_EQ(actual.string_format, expect.string_format);
    EXPECT_EQ(actual.behavior, expect.behavior);
}

TEST_F(ServiceProxyTest, platform_get_control_info)
{
    int num_container = 3; // One array with two structures in it
    EXPECT_CALL(*m_bus_reply, enter_container(_,_))
        .Times(num_container + 1); // Extra enter array of struct termination
    EXPECT_CALL(*m_bus_reply, exit_container())
        .Times(num_container);
    EXPECT_CALL(*m_bus_reply, was_success())
        .WillOnce(Return(true))
        .WillOnce(Return(true))
        .WillOnce(Return(false));
    EXPECT_CALL(*m_bus_reply, read_string())
        .WillOnce(Return("frequency"))
        .WillOnce(Return("Maximum cpu frequency"))
        .WillOnce(Return("power"))
        .WillOnce(Return("Maximum power cap"));
    EXPECT_CALL(*m_bus_reply, read_integer())
        .WillOnce(Return(1))
        .WillOnce(Return(2));
    EXPECT_CALL(*m_bus, make_call_message("PlatformGetControlInfo"))
        .WillOnce(Return(m_bus_message));
    std::vector<std::string> input_names = {"frequency", "power"};
    EXPECT_CALL(*m_bus_message, append_strings(input_names))
        .Times(1);
    std::shared_ptr<SDBusMessage> bus_message_ptr = m_bus_message;
    EXPECT_CALL(*m_bus, call_method(bus_message_ptr))
        .WillOnce(Return(m_bus_reply));
    std::vector<control_info_s> info_actual = m_proxy->platform_get_control_info(input_names);

    std::vector<control_info_s> info_expect = {{"frequency",
                                               "Maximum cpu frequency",
                                                1},
                                              {"power",
                                               "Maximum power cap",
                                                2}};
    ASSERT_EQ(info_actual.size(), info_expect.size());
    for (unsigned info_idx = 0; info_idx != info_actual.size(); ++info_idx) {
        check_control_info(info_expect.at(info_idx), info_actual.at(info_idx));
    }
}

void ServiceProxyTest::check_control_info(const control_info_s &actual,
                                          const control_info_s &expect) const
{
    EXPECT_EQ(actual.name, expect.name);
    EXPECT_EQ(actual.description, expect.description);
    EXPECT_EQ(actual.domain, expect.domain);
}

TEST_F(ServiceProxyTest, platform_open_session)
{
    EXPECT_CALL(*m_bus, call_method("PlatformOpenSession"));
    m_proxy->platform_open_session();
}

TEST_F(ServiceProxyTest, platform_close_session)
{
    EXPECT_CALL(*m_bus, call_method("PlatformCloseSession"));
    m_proxy->platform_close_session();
}

TEST_F(ServiceProxyTest, platform_start_batch)
{
    std::vector<struct geopm_request_s> signal_config;
    std::vector<struct geopm_request_s> control_config;
    int server_pid = -1;
    std::string server_key;
    GEOPM_EXPECT_THROW_MESSAGE(m_proxy->platform_start_batch(signal_config,
                                                             control_config,
                                                             server_pid,
                                                             server_key),
                               GEOPM_ERROR_NOT_IMPLEMENTED,
                               "platform_start_batch");
}

TEST_F(ServiceProxyTest, platform_stop_batch)
{
    GEOPM_EXPECT_THROW_MESSAGE(m_proxy->platform_stop_batch(100),
                               GEOPM_ERROR_NOT_IMPLEMENTED,
                               "platform_stop_batch");
}

TEST_F(ServiceProxyTest, platform_read_signal)
{
    double expect_read = 42.24;
    EXPECT_CALL(*m_bus, call_method("PlatformReadSignal", "instructions", 1, 2))
        .WillOnce(Return(m_bus_reply));
    EXPECT_CALL(*m_bus_reply, read_double())
        .WillOnce(Return(expect_read));
    double actual_read = m_proxy->platform_read_signal("instructions", 1, 2);
    EXPECT_EQ(expect_read, actual_read);
}

TEST_F(ServiceProxyTest, platform_write_control)
{
    EXPECT_CALL(*m_bus, call_method("PlatformWriteControl", "frequency", 1, 2, 1.0e9));
    m_proxy->platform_write_control("frequency", 1, 2, 1.0e9);
}
