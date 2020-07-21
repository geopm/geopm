/*
 * Copyright (c) 2015, 2016, 2017, 2018, 2019, 2020, Intel Corporation
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

#include "config.h"

#include <utility>

#include "gtest/gtest.h"
#include "gmock/gmock.h"

#include "DomainControl.hpp"
#include "MockControl.hpp"
#include "geopm_test.hpp"

using geopm::Control;
using geopm::DomainControl;

class DomainControlTest : public ::testing::Test
{
    protected:
        void SetUp();
        std::shared_ptr<MockControl> m_cpu_0;
        std::shared_ptr<MockControl> m_cpu_1;
        std::shared_ptr<DomainControl> m_ctl;
};

void DomainControlTest::SetUp()
{
    m_cpu_0 = std::make_shared<MockControl>();
    m_cpu_1 = std::make_shared<MockControl>();
    std::vector<std::shared_ptr<Control> > controls = {m_cpu_0, m_cpu_1};
    m_ctl = std::make_shared<DomainControl>(controls);
}

TEST_F(DomainControlTest, write)
{
    double value = 5.432;
    EXPECT_CALL(*m_cpu_0, write(value));
    EXPECT_CALL(*m_cpu_1, write(value));
    m_ctl->write(value);
}

TEST_F(DomainControlTest, write_batch)
{
    double value = 8.765;
    EXPECT_CALL(*m_cpu_0, setup_batch());
    EXPECT_CALL(*m_cpu_1, setup_batch());
    m_ctl->setup_batch();
    EXPECT_CALL(*m_cpu_0, adjust(value));
    EXPECT_CALL(*m_cpu_1, adjust(value));
    m_ctl->adjust(value);
}

TEST_F(DomainControlTest, setup_batch)
{
    // setup batch can be called multiple times without further side effects
    EXPECT_CALL(*m_cpu_0, setup_batch()).Times(1);
    EXPECT_CALL(*m_cpu_1, setup_batch()).Times(1);
    m_ctl->setup_batch();
    m_ctl->setup_batch();
}

TEST_F(DomainControlTest, errors)
{
    // cannot construct if any cpu controls are null
    GEOPM_EXPECT_THROW_MESSAGE(DomainControl({m_cpu_0, nullptr}), GEOPM_ERROR_INVALID,
                               "internal controls cannot be null");
    // cannot call adjust before setup_batch
    GEOPM_EXPECT_THROW_MESSAGE(m_ctl->adjust(123), GEOPM_ERROR_RUNTIME,
                               "cannot call adjust() before setup_batch()");

}

TEST_F(DomainControlTest, save_restore)
{
    EXPECT_CALL(*m_cpu_0, save());
    EXPECT_CALL(*m_cpu_1, save());
    m_ctl->save();
    EXPECT_CALL(*m_cpu_0, restore());
    EXPECT_CALL(*m_cpu_1, restore());
    m_ctl->restore();
}
