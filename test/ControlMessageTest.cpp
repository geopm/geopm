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

#include "gtest/gtest.h"
#include "ControlMessage.hpp"

class ControlMessageTest: public geopm::ControlMessage, public testing::Test
{
    public:
        ControlMessageTest();
        virtual ~ControlMessageTest();
    protected:
        void SetUp();
        void TearDown();
        struct geopm_ctl_message_s m_test_ctl_msg_buffer;
        geopm::ControlMessage *m_test_ctl_msg;
        geopm::ControlMessage *m_test_app_msg;
};

ControlMessageTest::ControlMessageTest()
    : geopm::ControlMessage(&m_test_ctl_msg_buffer, false, false)
{

}

ControlMessageTest::~ControlMessageTest()
{

}

void ControlMessageTest::SetUp()
{
    m_test_ctl_msg = new geopm::ControlMessage(&m_test_ctl_msg_buffer, true, true);
    m_test_app_msg = new geopm::ControlMessage(&m_test_ctl_msg_buffer, false, true);
}

void ControlMessageTest::TearDown()
{
    delete m_test_app_msg;
    delete m_test_ctl_msg;
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
    step();
    ASSERT_EQ(M_STATUS_MAP_BEGIN, m_test_ctl_msg_buffer.ctl_status);
    ASSERT_EQ(M_STATUS_MAP_BEGIN, m_test_ctl_msg_buffer.app_status);
    // Wait all three controller messages
    m_test_ctl_msg->wait();
    ASSERT_EQ(M_STATUS_MAP_BEGIN, m_test_ctl_msg_buffer.ctl_status);
    ASSERT_EQ(M_STATUS_MAP_BEGIN, m_test_ctl_msg_buffer.app_status);
    m_test_app_msg->wait();
    ASSERT_EQ(M_STATUS_MAP_BEGIN, m_test_ctl_msg_buffer.ctl_status);
    ASSERT_EQ(M_STATUS_MAP_BEGIN, m_test_ctl_msg_buffer.app_status);
    wait();
    ASSERT_EQ(M_STATUS_MAP_BEGIN, m_test_ctl_msg_buffer.ctl_status);
    ASSERT_EQ(M_STATUS_MAP_BEGIN, m_test_ctl_msg_buffer.app_status);
    // Step again
    m_test_ctl_msg->step();
    ASSERT_EQ(M_STATUS_MAP_END, m_test_ctl_msg_buffer.ctl_status);
    ASSERT_EQ(M_STATUS_MAP_BEGIN, m_test_ctl_msg_buffer.app_status);
    m_test_app_msg->step();
    ASSERT_EQ(M_STATUS_MAP_END, m_test_ctl_msg_buffer.ctl_status);
    ASSERT_EQ(M_STATUS_MAP_END, m_test_ctl_msg_buffer.app_status);
    step();
    ASSERT_EQ(M_STATUS_MAP_END, m_test_ctl_msg_buffer.ctl_status);
    ASSERT_EQ(M_STATUS_MAP_END, m_test_ctl_msg_buffer.app_status);
    // Wait again
    m_test_ctl_msg->wait();
    ASSERT_EQ(M_STATUS_MAP_END, m_test_ctl_msg_buffer.ctl_status);
    ASSERT_EQ(M_STATUS_MAP_END, m_test_ctl_msg_buffer.app_status);
    m_test_app_msg->wait();
    ASSERT_EQ(M_STATUS_MAP_END, m_test_ctl_msg_buffer.ctl_status);
    ASSERT_EQ(M_STATUS_MAP_END, m_test_ctl_msg_buffer.app_status);
    wait();
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
