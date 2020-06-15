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

#include "gtest/gtest.h"
#include "gmock/gmock.h"


#include "ProcessEpochImp.hpp"
#include "ApplicationSampler.hpp"
#include "Exception.hpp"
#include "geopm.h"
#include "geopm_test.hpp"

using geopm::ProcessEpochImp;
using geopm::record_s;


class ProcessEpochImpTest : public ::testing::Test
{
    protected:
        void update_all(void);

        ProcessEpochImp m_process;
        std::vector<record_s> m_records;
};

void ProcessEpochImpTest::update_all(void)
{
    for (const auto &rec : m_records) {
        m_process.update(rec);
    }
}

// shorter names for the enum event types
enum {
    REGION_ENTRY = geopm::EVENT_REGION_ENTRY,
    REGION_EXIT = geopm::EVENT_REGION_EXIT,
    EPOCH_COUNT = geopm::EVENT_EPOCH_COUNT,
    HINT = geopm::EVENT_HINT,
};

TEST_F(ProcessEpochImpTest, epoch_count)
{
    // default value
    EXPECT_DOUBLE_EQ(0, m_process.epoch_count());

    // pre-epoch regions
    m_records = {
        {0.1, 0, REGION_ENTRY, 0xCAFE},
        {0.2, 0, REGION_EXIT, 0xCAFE}
    };
    update_all();
    EXPECT_DOUBLE_EQ(0, m_process.epoch_count());

    m_records = {
        {0.3, 0, EPOCH_COUNT, 0x1},
    };
    update_all();
    EXPECT_DOUBLE_EQ(1, m_process.epoch_count());
    m_records = {
        {0.4, 0, EPOCH_COUNT, 0x2},
    };
    update_all();
    EXPECT_DOUBLE_EQ(2, m_process.epoch_count());

    m_records = {
        {0.5, 0, EPOCH_COUNT, 0x4},
    };
    update_all();
    EXPECT_DOUBLE_EQ(4, m_process.epoch_count());
}

TEST_F(ProcessEpochImpTest, epoch_runtime)
{
    // default value
    EXPECT_TRUE(std::isnan(m_process.last_epoch_runtime()));

    // pre-epoch regions
    m_records = {
        {0.1, 0, REGION_ENTRY, 0xCAFE},
        {0.2, 0, REGION_EXIT, 0xCAFE}
    };
    update_all();
    EXPECT_TRUE(std::isnan(m_process.last_epoch_runtime()));

    // first epoch
    m_records = {
        {0.3, 0, EPOCH_COUNT, 0x0},
    };
    update_all();
    EXPECT_TRUE(std::isnan(m_process.last_epoch_runtime()));

    // second epoch
    m_records = {
        {0.8, 0, EPOCH_COUNT, 0x1},
    };
    update_all();
    EXPECT_DOUBLE_EQ(0.5, m_process.last_epoch_runtime());
}

TEST_F(ProcessEpochImpTest, hint_time)
{
    // default values
    EXPECT_TRUE(std::isnan(m_process.last_epoch_runtime_network()));
    EXPECT_TRUE(std::isnan(m_process.last_epoch_runtime_ignore()));

    // pre-epoch regions
    m_records = {
        {0.1, 0, REGION_ENTRY, 0xCAFE},
        {0.2, 0, REGION_EXIT, 0xCAFE}
    };
    update_all();
    EXPECT_TRUE(std::isnan(m_process.last_epoch_runtime_network()));
    EXPECT_TRUE(std::isnan(m_process.last_epoch_runtime_ignore()));

    // first epoch
    m_records = {
        {0.3, 0, EPOCH_COUNT, 0x0},
    };
    update_all();
    EXPECT_TRUE(std::isnan(m_process.last_epoch_runtime_network()));
    EXPECT_TRUE(std::isnan(m_process.last_epoch_runtime_ignore()));

    // second epoch, no hint
    m_records = {
        {0.6, 0, REGION_ENTRY, 0xBABA},
        {0.7, 0, REGION_EXIT, 0xBABA},
        {0.8, 0, EPOCH_COUNT, 0x1},
    };
    update_all();
    EXPECT_NEAR(0.0, m_process.last_epoch_runtime_network(), 0.0001);
    EXPECT_NEAR(0.0, m_process.last_epoch_runtime_ignore(), 0.0001);

    // ignore region
    m_records = {
        {0.9, 0, REGION_ENTRY, 0xBABA},
        {0.9, 0, HINT, GEOPM_REGION_HINT_IGNORE},
        {1.1, 0, REGION_EXIT, 0xBABA},
        {1.1, 0, HINT, GEOPM_REGION_HINT_UNKNOWN},
        {1.2, 0, EPOCH_COUNT, 0x2},
    };
    update_all();
    EXPECT_NEAR(0.0, m_process.last_epoch_runtime_network(), 0.0001);
    EXPECT_NEAR(0.2, m_process.last_epoch_runtime_ignore(), 0.0001);

    // network time
    m_records = {
        {1.6, 0, REGION_ENTRY, 0xBABA},
        {1.6, 0, HINT, GEOPM_REGION_HINT_NETWORK},
        {1.8, 0, HINT, GEOPM_REGION_HINT_UNKNOWN},
        {2.0, 0, REGION_EXIT, 0xBABA},
        {2.0, 0, REGION_ENTRY, 0xDADA},
        {2.1, 0, REGION_EXIT, 0xDADA},
        {2.2, 0, EPOCH_COUNT, 0x3},
    };
    update_all();
    EXPECT_NEAR(0.2, m_process.last_epoch_runtime_network(), 0.0001);
    EXPECT_NEAR(0.0, m_process.last_epoch_runtime_ignore(), 0.0001);

    // hint changes within region
    m_records = {
        {2.3, 0, REGION_ENTRY, 0xFACE},
        {2.3, 0, HINT, GEOPM_REGION_HINT_IGNORE},
        {2.4, 0, HINT, GEOPM_REGION_HINT_COMPUTE},
        {2.5, 0, HINT, GEOPM_REGION_HINT_NETWORK},
        {2.6, 0, HINT, GEOPM_REGION_HINT_IGNORE},
        {2.7, 0, HINT, GEOPM_REGION_HINT_NETWORK},
        {2.8, 0, HINT, GEOPM_REGION_HINT_NETWORK},
        {2.9, 0, HINT, GEOPM_REGION_HINT_MEMORY},
        {3.0, 0, HINT, GEOPM_REGION_HINT_IGNORE},
        {3.1, 0, REGION_EXIT, 0xFACE},
        {3.1, 0, HINT, GEOPM_REGION_HINT_UNKNOWN},
        {3.2, 0, EPOCH_COUNT, 0x4},
    };
    update_all();
    EXPECT_NEAR(0.3, m_process.last_epoch_runtime_network(), 0.0001);
    EXPECT_NEAR(0.3, m_process.last_epoch_runtime_ignore(), 0.0001);

    // hint across epochs
    m_records = {
        {3.3, 0, HINT, GEOPM_REGION_HINT_IGNORE},
        {3.4, 0, EPOCH_COUNT, 0x5},
    };
    update_all();
    EXPECT_NEAR(0.0, m_process.last_epoch_runtime_network(), 0.0001);
    EXPECT_NEAR(0.1, m_process.last_epoch_runtime_ignore(), 0.0001);
    m_records = {
        {3.6, 0, EPOCH_COUNT, 0x6},
    };
    update_all();
    EXPECT_NEAR(0.0, m_process.last_epoch_runtime_network(), 0.0001);
    EXPECT_NEAR(0.2, m_process.last_epoch_runtime_ignore(), 0.0001);
    m_records = {
        {3.9, 0, HINT, GEOPM_REGION_HINT_NETWORK},
        {4.0, 0, HINT, GEOPM_REGION_HINT_IGNORE},
        {4.1, 0, EPOCH_COUNT, 0x7},
    };
    update_all();
    EXPECT_NEAR(0.1, m_process.last_epoch_runtime_network(), 0.0001);
    EXPECT_NEAR(0.4, m_process.last_epoch_runtime_ignore(), 0.0001);

    EXPECT_THROW(m_process.last_epoch_runtime_hint(99), geopm::Exception);
    EXPECT_THROW(m_process.last_epoch_runtime_hint(-1), geopm::Exception);
}
