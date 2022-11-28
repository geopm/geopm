/*
 * Copyright (c) 2015 - 2022, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <memory>

#include "geopm/ServiceProxy.hpp"
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
        .WillOnce(Return("Maximum CPU frequency"))
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
                                               "Maximum CPU frequency",
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
    std::vector<geopm_request_s> signal_config = {geopm_request_s {1, 0, "CPU_FREQUENCY"},
                                                  geopm_request_s {2, 1, "TEMPERATURE"}};
    std::vector<geopm_request_s> control_config = {geopm_request_s {1, 0, "MAX_CPU_FREQUENCY"}};
    int server_pid_expect = 1234;
    std::string server_key_expect = "4321";
    EXPECT_CALL(*m_bus,
                make_call_message("PlatformStartBatch"))
        .WillOnce(Return(m_bus_message));
    EXPECT_CALL(*m_bus_message,
                open_container(SDBusMessage::M_MESSAGE_TYPE_ARRAY, "(iis)"))
        .Times(2);
    EXPECT_CALL(*m_bus_message, append_request(_))
        .Times(3);
    EXPECT_CALL(*m_bus_message,
                close_container())
        .Times(2);
    std::shared_ptr<SDBusMessage> bus_message_ptr = m_bus_message;
    EXPECT_CALL(*m_bus, call_method(bus_message_ptr))
        .WillOnce(Return(m_bus_reply));
    EXPECT_CALL(*m_bus_reply,
                enter_container(SDBusMessage::M_MESSAGE_TYPE_STRUCT, "is"));
    EXPECT_CALL(*m_bus_reply,
                read_integer())
        .WillOnce(Return(server_pid_expect));
    EXPECT_CALL(*m_bus_reply,
                read_string())
        .WillOnce(Return(server_key_expect));
    EXPECT_CALL(*m_bus_reply,
                exit_container());
    int server_pid = 0;
    std::string server_key;
    m_proxy->platform_start_batch(signal_config, control_config,
                                  server_pid, server_key);
    ASSERT_EQ(server_pid_expect, server_pid);
    ASSERT_EQ(server_key_expect, server_key);
}

TEST_F(ServiceProxyTest, platform_stop_batch)
{
    int server_pid = 4321;
    EXPECT_CALL(*m_bus, call_method("PlatformStopBatch", server_pid));
    m_proxy->platform_stop_batch(server_pid);
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
