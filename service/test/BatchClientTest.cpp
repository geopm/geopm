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

#include "config.h"

#include <cerrno>

#include "BatchClient.hpp"
#include "BatchStatus.hpp"
#include "MockBatchStatus.hpp"
#include "MockSharedMemory.hpp"
#include "geopm/Helper.hpp"
#include "geopm_test.hpp"

using testing::InSequence;
using testing::Return;
using testing::_;
using geopm::BatchClient;
using geopm::BatchClientImp;
using geopm::BatchStatus;


class BatchClientTest : public ::testing::Test
{
    protected:
        void SetUp();
        std::shared_ptr<MockBatchStatus> m_batch_status;
        std::shared_ptr<MockSharedMemory> m_signal_shmem;
        std::shared_ptr<MockSharedMemory> m_control_shmem;
        std::shared_ptr<BatchClient> m_batch_client;
        std::shared_ptr<BatchClient> m_batch_client_empty;
};

void BatchClientTest::SetUp()
{
    m_batch_status = std::make_shared<MockBatchStatus>();
    m_signal_shmem = std::make_shared<MockSharedMemory>(2 * sizeof(double));
    m_control_shmem = std::make_shared<MockSharedMemory>(sizeof(double));
    m_batch_client = std::make_shared<BatchClientImp>(2, 1,
                                                      m_batch_status,
                                                      m_signal_shmem,
                                                      m_control_shmem);
    m_batch_client_empty = std::make_shared<BatchClientImp>(0, 0,
                                                            m_batch_status,
                                                            nullptr,
                                                            nullptr);
}

TEST_F(BatchClientTest, read_batch)
{
    InSequence sequence;
    std::vector<double> result_expect = {12.34, 56.78};
    double *shmem_buffer =(double *)m_signal_shmem->pointer();
    shmem_buffer[0] = result_expect[0];
    shmem_buffer[1] = result_expect[1];
    EXPECT_CALL(*m_batch_status, send_message(BatchStatus::M_MESSAGE_READ))
        .Times(1);
    EXPECT_CALL(*m_batch_status, receive_message(BatchStatus::M_MESSAGE_CONTINUE))
        .Times(1);
    std::vector<double> result_actual = m_batch_client->read_batch();
    EXPECT_EQ(result_expect, result_actual);
}

TEST_F(BatchClientTest, write_batch)
{
    InSequence sequence;
    std::vector<double> settings_expect = {56.78};
    EXPECT_CALL(*m_batch_status, send_message(BatchStatus::M_MESSAGE_WRITE))
        .Times(1);
    EXPECT_CALL(*m_batch_status, receive_message(BatchStatus::M_MESSAGE_CONTINUE))
        .Times(1);
    m_batch_client->write_batch(settings_expect);
    double *shmem_buffer =(double *)m_control_shmem->pointer();
    EXPECT_EQ(settings_expect[0], shmem_buffer[0]);
}

TEST_F(BatchClientTest, read_batch_empty)
{
    std::vector<double> result_expect;
    EXPECT_CALL(*m_batch_status, send_message(_))
        .Times(0);
    EXPECT_CALL(*m_batch_status, receive_message(_))
        .Times(0);
    EXPECT_CALL(*m_batch_status, receive_message())
        .Times(0);
    std::vector<double> result_actual = m_batch_client_empty->read_batch();
    EXPECT_EQ(result_expect, result_actual);
}

TEST_F(BatchClientTest, write_batch_empty)
{
    std::vector<double> settings;
    EXPECT_CALL(*m_batch_status, send_message(_))
        .Times(0);
    EXPECT_CALL(*m_batch_status, receive_message(_))
        .Times(0);
    EXPECT_CALL(*m_batch_status, receive_message())
        .Times(0);
    m_batch_client_empty->write_batch(settings);
}

TEST_F(BatchClientTest, create_but_timeout)
{
    GEOPM_EXPECT_THROW_MESSAGE(
        BatchClient::make_unique("test-key", 1e-6, 1, 0),
        ENOENT, "Could not open shared memory with key");
}
