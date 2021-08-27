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
