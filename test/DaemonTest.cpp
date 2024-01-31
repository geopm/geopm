/*
 * Copyright (c) 2015 - 2024, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
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
