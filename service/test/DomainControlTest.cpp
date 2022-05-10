/*
 * Copyright (c) 2015 - 2022, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
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
    // cannot construct if any CPU controls are null
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
