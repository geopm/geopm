/*
 * Copyright (c) 2015 - 2023, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <memory>

#include "gtest/gtest.h"

#include "ControlMessage.hpp"
#include "geopm/Helper.hpp"

using geopm::ControlMessage;
using geopm::ControlMessageImp;

enum control_message_test_e {
    M_STATUS_UNDEFINED = ControlMessageImp::M_STATUS_UNDEFINED,
    M_STATUS_MAP_BEGIN = ControlMessageImp::M_STATUS_MAP_BEGIN,
    M_STATUS_MAP_END = ControlMessageImp::M_STATUS_MAP_END,
    M_STATUS_SAMPLE_BEGIN = ControlMessageImp::M_STATUS_SAMPLE_BEGIN,
    M_STATUS_SAMPLE_END = ControlMessageImp::M_STATUS_SAMPLE_END,
    M_STATUS_NAME_BEGIN = ControlMessageImp::M_STATUS_NAME_BEGIN,
    M_STATUS_NAME_LOOP_BEGIN = ControlMessageImp::M_STATUS_NAME_LOOP_BEGIN,
    M_STATUS_NAME_LOOP_END = ControlMessageImp::M_STATUS_NAME_LOOP_END,
    M_STATUS_NAME_END = ControlMessageImp::M_STATUS_NAME_END,
    M_STATUS_SHUTDOWN = ControlMessageImp::M_STATUS_SHUTDOWN,
    M_STATUS_ABORT = ControlMessageImp::M_STATUS_ABORT,
};


class ControlMessageTest: public testing::Test
{
    public:
        ControlMessageTest() = default;
        virtual ~ControlMessageTest() = default;
    protected:
        void SetUp();
        const int m_timeout = 60;
        struct geopm_ctl_message_s m_test_ctl_msg_buffer;
        std::unique_ptr<ControlMessage> m_test_ctl_msg;
        std::unique_ptr<ControlMessage> m_test_app_msg;
        std::unique_ptr<ControlMessage> m_test_app_noop_msg;
};


void ControlMessageTest::SetUp()
{
    // Application control message must be constructed first to avoid
    // hang when controller message is constructed.
    m_test_app_msg = geopm::make_unique<ControlMessageImp>(m_test_ctl_msg_buffer, false, true, m_timeout);
    m_test_ctl_msg = geopm::make_unique<ControlMessageImp>(m_test_ctl_msg_buffer, true, true, m_timeout);
    m_test_app_noop_msg = geopm::make_unique<ControlMessageImp>(m_test_ctl_msg_buffer, false, false, m_timeout);
}

TEST_F(ControlMessageTest, step)
{
    m_test_ctl_msg->step();
    ASSERT_EQ(M_STATUS_MAP_BEGIN, m_test_ctl_msg_buffer.ctl_status);
    m_test_ctl_msg->step();
    ASSERT_EQ(M_STATUS_MAP_END, m_test_ctl_msg_buffer.ctl_status);
    m_test_ctl_msg->step();
    ASSERT_EQ(M_STATUS_SAMPLE_BEGIN, m_test_ctl_msg_buffer.ctl_status);
    m_test_ctl_msg->step();
    ASSERT_EQ(M_STATUS_SAMPLE_END, m_test_ctl_msg_buffer.ctl_status);
    m_test_ctl_msg->step();
    ASSERT_EQ(M_STATUS_NAME_BEGIN, m_test_ctl_msg_buffer.ctl_status);
    m_test_ctl_msg->step();
    ASSERT_EQ(M_STATUS_NAME_LOOP_BEGIN, m_test_ctl_msg_buffer.ctl_status);
    m_test_ctl_msg->step();
    ASSERT_EQ(M_STATUS_NAME_LOOP_END, m_test_ctl_msg_buffer.ctl_status);
    m_test_ctl_msg->step();
    ASSERT_EQ(M_STATUS_NAME_END, m_test_ctl_msg_buffer.ctl_status);
    m_test_ctl_msg->step();
    ASSERT_EQ(M_STATUS_SHUTDOWN, m_test_ctl_msg_buffer.ctl_status);
    m_test_ctl_msg->step();
    ASSERT_EQ(M_STATUS_SHUTDOWN, m_test_ctl_msg_buffer.ctl_status);
    m_test_ctl_msg->step();
    ASSERT_EQ(M_STATUS_SHUTDOWN, m_test_ctl_msg_buffer.ctl_status);
}


TEST_F(ControlMessageTest, wait)
{
    // Step all three controller messages
    m_test_ctl_msg->step();
    ASSERT_EQ(M_STATUS_MAP_BEGIN, m_test_ctl_msg_buffer.ctl_status);
    ASSERT_EQ(M_STATUS_UNDEFINED, m_test_ctl_msg_buffer.app_status);
    m_test_app_msg->step();
    ASSERT_EQ(M_STATUS_MAP_BEGIN, m_test_ctl_msg_buffer.ctl_status);
    ASSERT_EQ(M_STATUS_MAP_BEGIN, m_test_ctl_msg_buffer.app_status);
    m_test_app_noop_msg->step();
    ASSERT_EQ(M_STATUS_MAP_BEGIN, m_test_ctl_msg_buffer.ctl_status);
    ASSERT_EQ(M_STATUS_MAP_BEGIN, m_test_ctl_msg_buffer.app_status);
    // Wait all three controller messages
    m_test_ctl_msg->wait();
    ASSERT_EQ(M_STATUS_MAP_BEGIN, m_test_ctl_msg_buffer.ctl_status);
    ASSERT_EQ(M_STATUS_MAP_BEGIN, m_test_ctl_msg_buffer.app_status);
    m_test_app_msg->wait();
    ASSERT_EQ(M_STATUS_MAP_BEGIN, m_test_ctl_msg_buffer.ctl_status);
    ASSERT_EQ(M_STATUS_MAP_BEGIN, m_test_ctl_msg_buffer.app_status);
    m_test_app_noop_msg->wait();
    ASSERT_EQ(M_STATUS_MAP_BEGIN, m_test_ctl_msg_buffer.ctl_status);
    ASSERT_EQ(M_STATUS_MAP_BEGIN, m_test_ctl_msg_buffer.app_status);
    // Step again
    m_test_ctl_msg->step();
    ASSERT_EQ(M_STATUS_MAP_END, m_test_ctl_msg_buffer.ctl_status);
    ASSERT_EQ(M_STATUS_MAP_BEGIN, m_test_ctl_msg_buffer.app_status);
    m_test_app_msg->step();
    ASSERT_EQ(M_STATUS_MAP_END, m_test_ctl_msg_buffer.ctl_status);
    ASSERT_EQ(M_STATUS_MAP_END, m_test_ctl_msg_buffer.app_status);
    m_test_app_noop_msg->step();
    ASSERT_EQ(M_STATUS_MAP_END, m_test_ctl_msg_buffer.ctl_status);
    ASSERT_EQ(M_STATUS_MAP_END, m_test_ctl_msg_buffer.app_status);
    // Wait again
    m_test_ctl_msg->wait();
    ASSERT_EQ(M_STATUS_MAP_END, m_test_ctl_msg_buffer.ctl_status);
    ASSERT_EQ(M_STATUS_MAP_END, m_test_ctl_msg_buffer.app_status);
    m_test_app_msg->wait();
    ASSERT_EQ(M_STATUS_MAP_END, m_test_ctl_msg_buffer.ctl_status);
    ASSERT_EQ(M_STATUS_MAP_END, m_test_ctl_msg_buffer.app_status);
    m_test_app_noop_msg->wait();
    ASSERT_EQ(M_STATUS_MAP_END, m_test_ctl_msg_buffer.ctl_status);
    ASSERT_EQ(M_STATUS_MAP_END, m_test_ctl_msg_buffer.app_status);
}

TEST_F(ControlMessageTest, cpu_rank)
{
    int num_cpu = 256;
    for (int cpu = 0; cpu < num_cpu; ++cpu) {
        m_test_app_msg->cpu_rank(cpu, num_cpu - cpu - 1);
    }
    for (int cpu = 0; cpu < num_cpu; ++cpu) {
        ASSERT_EQ(num_cpu - cpu - 1, m_test_ctl_msg->cpu_rank(cpu));
    }
}

TEST_F(ControlMessageTest, is_sample_begin)
{
    for (int i = 1; i <= M_STATUS_SHUTDOWN; ++i) {
        m_test_app_msg->step();
        if (i == M_STATUS_SAMPLE_BEGIN) {
           ASSERT_TRUE(m_test_ctl_msg->is_sample_begin());
        }
        else {
           ASSERT_FALSE(m_test_ctl_msg->is_sample_begin());
        }
    }
}

TEST_F(ControlMessageTest, is_sample_end)
{
    for (int i = 1; i <= M_STATUS_SHUTDOWN; ++i) {
        m_test_app_msg->step();
        if (i == M_STATUS_SAMPLE_END) {
           ASSERT_TRUE(m_test_ctl_msg->is_sample_end());
        }
        else {
           ASSERT_FALSE(m_test_ctl_msg->is_sample_end());
        }
    }
}

TEST_F(ControlMessageTest, is_name_begin)
{
    for (int i = 1; i <= M_STATUS_SHUTDOWN; ++i) {
        m_test_app_msg->step();
        if (i == M_STATUS_NAME_BEGIN) {
           ASSERT_TRUE(m_test_ctl_msg->is_name_begin());
        }
        else {
           ASSERT_FALSE(m_test_ctl_msg->is_name_begin());
        }
    }
}

TEST_F(ControlMessageTest, is_shutdown)
{
    for (int i = 1; i <= M_STATUS_SHUTDOWN + 2; ++i) {
        m_test_app_msg->step();
        if (i >= M_STATUS_SHUTDOWN) {
           ASSERT_TRUE(m_test_ctl_msg->is_shutdown());
        }
        else {
           ASSERT_FALSE(m_test_ctl_msg->is_shutdown());
        }
    }

}

TEST_F(ControlMessageTest, loop_begin_0)
{
    for (int i = 1; i < M_STATUS_NAME_LOOP_BEGIN; ++i) {
        m_test_app_msg->step();
        m_test_ctl_msg->step();
    }
    for (int i = 0; i < 10; ++i) {
        m_test_ctl_msg_buffer.ctl_status = M_STATUS_NAME_LOOP_BEGIN;
        m_test_app_msg->loop_begin();
        m_test_ctl_msg->loop_begin();

        ASSERT_EQ(M_STATUS_NAME_LOOP_BEGIN, m_test_ctl_msg_buffer.ctl_status);
        ASSERT_EQ(M_STATUS_NAME_LOOP_BEGIN, m_test_ctl_msg_buffer.app_status);
        m_test_app_msg->step();
        m_test_ctl_msg->step();
        ASSERT_EQ(M_STATUS_NAME_LOOP_END, m_test_ctl_msg_buffer.ctl_status);
        ASSERT_EQ(M_STATUS_NAME_LOOP_END, m_test_ctl_msg_buffer.app_status);
    }
    m_test_app_msg->step();
    m_test_ctl_msg->step();
    ASSERT_EQ(M_STATUS_NAME_END, m_test_ctl_msg_buffer.ctl_status);
    ASSERT_EQ(M_STATUS_NAME_END, m_test_ctl_msg_buffer.app_status);
    m_test_app_msg->step();
    m_test_ctl_msg->step();
    ASSERT_EQ(M_STATUS_SHUTDOWN, m_test_ctl_msg_buffer.ctl_status);
    ASSERT_EQ(M_STATUS_SHUTDOWN, m_test_ctl_msg_buffer.app_status);

}

TEST_F(ControlMessageTest, loop_begin_1)
{
    for (int i = 1; i < M_STATUS_NAME_LOOP_BEGIN; ++i) {
        m_test_app_msg->step();
        m_test_ctl_msg->step();
    }
    for (int i = 0; i < 10; ++i) {
        m_test_ctl_msg_buffer.app_status = M_STATUS_NAME_LOOP_BEGIN;
        m_test_ctl_msg->loop_begin();
        m_test_app_msg->loop_begin();
        ASSERT_EQ(M_STATUS_NAME_LOOP_BEGIN, m_test_ctl_msg_buffer.ctl_status);
        ASSERT_EQ(M_STATUS_NAME_LOOP_BEGIN, m_test_ctl_msg_buffer.app_status);
        m_test_app_msg->step();
        m_test_ctl_msg->step();
        ASSERT_EQ(M_STATUS_NAME_LOOP_END, m_test_ctl_msg_buffer.ctl_status);
        ASSERT_EQ(M_STATUS_NAME_LOOP_END, m_test_ctl_msg_buffer.app_status);
    }
    m_test_app_msg->step();
    m_test_ctl_msg->step();
    ASSERT_EQ(M_STATUS_NAME_END, m_test_ctl_msg_buffer.ctl_status);
    ASSERT_EQ(M_STATUS_NAME_END, m_test_ctl_msg_buffer.app_status);
    m_test_app_msg->step();
    m_test_ctl_msg->step();
    ASSERT_EQ(M_STATUS_SHUTDOWN, m_test_ctl_msg_buffer.ctl_status);
    ASSERT_EQ(M_STATUS_SHUTDOWN, m_test_ctl_msg_buffer.app_status);

}
