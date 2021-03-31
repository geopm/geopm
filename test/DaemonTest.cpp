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

#include "gtest/gtest.h"
#include "gmock/gmock.h"

#include "geopm_time.h"
#include "DaemonImp.hpp"
#include "MockEndpoint.hpp"
#include "MockPolicyStore.hpp"
#include "geopm_test.hpp"

using geopm::Daemon;
using geopm::DaemonImp;
using testing::Return;
using testing::_;

class DaemonTest : public testing::Test
{
    protected:
        void SetUp();
        void TearDown();

        double m_timeout = 2;
        std::shared_ptr<MockEndpoint> m_endpoint;
        std::shared_ptr<MockPolicyStore> m_policystore;
        std::shared_ptr<Daemon> m_daemon;

        const std::string M_NO_AGENT = "";
        const std::string M_AGENT = "myagent";
};

void DaemonTest::SetUp()
{
    m_endpoint = std::make_shared<MockEndpoint>();
    m_policystore = std::make_shared<MockPolicyStore>();

    EXPECT_CALL(*m_endpoint, open());

    m_daemon = std::make_shared<DaemonImp>(m_endpoint, m_policystore);
}

void DaemonTest::TearDown()
{
    EXPECT_CALL(*m_endpoint, close());

    m_daemon.reset();
}

TEST_F(DaemonTest, get_default_policy)
{
    std::vector<double> policy {1.1, 2.2, 3.4};
    // first call to get_agent() after wait_for_agent_attach() should have a value
    EXPECT_CALL(*m_endpoint, get_agent())
        .WillOnce(Return(M_AGENT));
    EXPECT_CALL(*m_endpoint, get_profile_name())
        .WillOnce(Return(""));
    EXPECT_CALL(*m_policystore, get_best(M_AGENT, ""))
        .WillOnce(Return(policy));
    EXPECT_CALL(*m_endpoint, write_policy(policy));

    m_daemon->update_endpoint_from_policystore(m_timeout);
}

TEST_F(DaemonTest, get_profile_policy)
{
    std::vector<double> policy {1.1, 2.2, 3.4};
    std::string profile_name = "myprofile";
    // first call to get_agent() after wait_for_agent_attach() should have a value
    EXPECT_CALL(*m_endpoint, get_agent())
        .WillOnce(Return(M_AGENT));
    EXPECT_CALL(*m_endpoint, get_profile_name())
        .WillOnce(Return(profile_name));
    EXPECT_CALL(*m_policystore, get_best(M_AGENT, profile_name))
        .WillOnce(Return(policy));
    EXPECT_CALL(*m_endpoint, write_policy(policy));

    m_daemon->update_endpoint_from_policystore(m_timeout);
}
