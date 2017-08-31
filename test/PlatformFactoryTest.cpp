/*
 * Copyright (c) 2015, 2016, 2017, Intel Corporation
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

#include <iostream>

#include "gtest/gtest.h"
#include "gmock/gmock.h"
#include "geopm_error.h"
#include "Exception.hpp"
#include "PlatformFactory.hpp"
#include "MockPlatformImp.hpp"
#include "MockPlatform.hpp"
#include "Policy.hpp"

using ::testing::Return;
using ::testing::Pointee;
using ::testing::_;

class PlatformFactoryTest: public :: testing :: Test
{

};

TEST_F(PlatformFactoryTest, platform_register)
{
    MockPlatform *m_platform = new MockPlatform();
    MockPlatformImp *m_platform_imp = new MockPlatformImp();
    std::unique_ptr<geopm::Platform> m_ap = std::unique_ptr<geopm::Platform>(m_platform);
    std::unique_ptr<geopm::PlatformImp> m_ap_imp = std::unique_ptr<geopm::PlatformImp>(m_platform_imp);
    geopm::PlatformFactory m_platform_fact(std::move(m_ap), std::move(m_ap_imp));
    std::string pname = "Haswell";
    std::string ans;
    geopm::Platform* p = NULL;

    EXPECT_CALL(*m_platform_imp, msr_offset(_))
    .WillRepeatedly(Return(500));

    EXPECT_CALL(*m_platform_imp, initialize());
    EXPECT_CALL(*m_platform, model_supported(_,_))
    .Times(1)
    .WillOnce(Return(true));

    EXPECT_CALL(*m_platform_imp, model_supported(_))
    .Times(1)
    .WillOnce(Return(true));

    EXPECT_CALL(*m_platform_imp, platform_name())
    .Times(1)
    .WillOnce(Return(pname));

    p = m_platform_fact.platform("rapl", true);
    ASSERT_FALSE(p == NULL);

    p->name(ans);
    ASSERT_FALSE(ans.empty());

    EXPECT_FALSE(ans.compare(pname));
}

TEST_F(PlatformFactoryTest, no_supported_platform)
{
    MockPlatform *m_platform = new MockPlatform();
    MockPlatformImp *m_platform_imp = new MockPlatformImp();
    std::unique_ptr<geopm::Platform> m_ap = std::unique_ptr<geopm::Platform>(m_platform);
    std::unique_ptr<geopm::PlatformImp> m_ap_imp = std::unique_ptr<geopm::PlatformImp>(m_platform_imp);
    geopm::PlatformFactory m_platform_fact(std::move(m_ap), std::move(m_ap_imp));
    geopm::Platform* p = NULL;
    int thrown = 0;

    EXPECT_CALL(*m_platform_imp, msr_offset(_))
    .WillRepeatedly(Return(500));

    EXPECT_CALL(*m_platform, model_supported(_,_))
    .Times(1)
    .WillOnce(Return(false));

    m_platform_fact.register_platform(std::move(m_ap_imp));
    m_platform_fact.register_platform(std::move(m_ap));

    try {
        p = m_platform_fact.platform("rapl", true);
    }
    catch (geopm::Exception e) {
        thrown = e.err_value();
    }
    ASSERT_TRUE(p == NULL);
    EXPECT_TRUE(thrown == GEOPM_ERROR_PLATFORM_UNSUPPORTED);
}
