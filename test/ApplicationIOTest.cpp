/*
 * Copyright (c) 2015 - 2023, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <memory>
#include <set>
#include <list>
#include <string>

#include "gtest/gtest.h"
#include "gmock/gmock.h"

#include "ApplicationIO.hpp"
#include "ApplicationSampler.hpp"
#include "geopm/Helper.hpp"
#include "MockProfileSampler.hpp"
#include "MockPlatformIO.hpp"
#include "MockPlatformTopo.hpp"
#include "MockServiceProxy.hpp"

using geopm::ApplicationIO;
using geopm::ApplicationIOImp;
using geopm::PlatformTopo;
using testing::_;
using testing::Return;

class ApplicationIOTest : public ::testing::Test
{
    protected:
        void SetUp();
        std::string m_shm_key = "test_shm";
        std::shared_ptr<MockProfileSampler> m_sampler;
        MockPlatformIO m_platform_io;
        MockPlatformTopo m_platform_topo;
        std::unique_ptr<ApplicationIO> m_app_io;
        std::shared_ptr<geopm::ServiceProxy> m_service_proxy;
        std::string m_profile_name;
        std::string m_report_name;
};

void ApplicationIOTest::SetUp()
{
    auto &tmp_app_sampler = geopm::ApplicationSampler::application_sampler();

    m_sampler = std::make_shared<MockProfileSampler>();
    tmp_app_sampler.set_sampler(m_sampler);
    m_service_proxy = std::make_shared<MockServiceProxy>();
    m_profile_name = "test_profile_name";
    m_report_name = "test_geopm.report";

    EXPECT_CALL(*m_sampler, initialize());
    m_app_io = geopm::make_unique<ApplicationIOImp>(tmp_app_sampler,
                                                    m_service_proxy,
                                                    m_profile_name,
                                                    m_report_name,
                                                    5);
    m_app_io->connect();
}

TEST_F(ApplicationIOTest, passthrough)
{
    EXPECT_CALL(*m_sampler, do_shutdown()).WillOnce(Return(false));
    EXPECT_FALSE(m_app_io->do_shutdown());

    EXPECT_CALL(*m_sampler, report_name()).WillOnce(Return("my_report"));
    EXPECT_EQ("my_report", m_app_io->report_name());

    EXPECT_CALL(*m_sampler, profile_name()).WillOnce(Return("my_profile"));
    EXPECT_EQ("my_profile", m_app_io->profile_name());

    std::set<std::string> regions = {"region A", "region B"};
    EXPECT_CALL(*m_sampler, name_set()).WillOnce(Return(regions));
    EXPECT_EQ(regions, m_app_io->region_name_set());
}
