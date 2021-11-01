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

#include "BatchServer.hpp"
#include "BatchStatus.hpp"
#include "MockBatchStatus.hpp"
#include "MockPlatformIO.hpp"
#include "MockPOSIXSignal.hpp"
#include "MockSharedMemory.hpp"
#include "geopm/Helper.hpp"

using testing::InSequence;
using testing::Return;
using testing::_;
using geopm::BatchServer;
using geopm::BatchServerImp;
using geopm::BatchStatus;
using geopm::POSIXSignal;


class BatchServerTest : public ::testing::Test
{
    protected:
        void SetUp();
        void TearDown();
        std::shared_ptr<MockPlatformIO> m_pio_ptr;
        std::shared_ptr<MockBatchStatus> m_batch_status;
        std::shared_ptr<MockPOSIXSignal> m_posix_signal;
        std::shared_ptr<MockSharedMemory> m_signal_shmem;
        std::shared_ptr<MockSharedMemory> m_control_shmem;
        int m_server_pid;
        int m_client_pid;
        std::vector<geopm_request_s> m_signal_config;
        std::vector<geopm_request_s> m_control_config;
        std::shared_ptr<BatchServerImp> m_batch_server;

};

void BatchServerTest::SetUp()
{
    m_pio_ptr = std::make_shared<MockPlatformIO>();
    m_batch_status = std::make_shared<MockBatchStatus>();
    m_posix_signal = std::make_shared<MockPOSIXSignal>();
    m_signal_shmem = std::make_shared<MockSharedMemory>(2 * sizeof(double));
    m_control_shmem = std::make_shared<MockSharedMemory>(sizeof(double));
    m_server_pid = 1234;
    m_client_pid = 4321;
    m_signal_config = {geopm_request_s {1, 0, "CPU_FREQUENCY"},
                       geopm_request_s {2, 1, "TEMPERATURE"}};
    m_control_config = {geopm_request_s {1, 0, "MAX_CPU_FREQUENCY"}};
    m_batch_server = std::make_shared<BatchServerImp>(m_client_pid,
                                                      m_signal_config,
                                                      m_control_config,
                                                      *m_pio_ptr,
                                                      m_batch_status,
                                                      m_posix_signal,
                                                      m_signal_shmem,
                                                      m_control_shmem,
                                                      m_server_pid);
}

void BatchServerTest::TearDown()
{
    if (m_batch_server->is_active()) {
        EXPECT_CALL(*m_posix_signal,
                    sig_queue(m_server_pid,
                              SIGTERM,
                              BatchStatus::M_MESSAGE_TERMINATE));
        m_batch_server.reset();
    }
}

TEST_F(BatchServerTest, get_server_pid)
{
    EXPECT_EQ(m_server_pid, m_batch_server->server_pid());
}

TEST_F(BatchServerTest, get_server_key)
{
    std::string key = m_batch_server->server_key();
    EXPECT_EQ(std::to_string(m_client_pid), key);
}

TEST_F(BatchServerTest, stop_batch)
{
    EXPECT_TRUE(m_batch_server->is_active());
    EXPECT_CALL(*m_posix_signal,
                sig_queue(m_server_pid,
                          SIGTERM,
                          BatchStatus::M_MESSAGE_TERMINATE));
    m_batch_server->stop_batch();
    EXPECT_FALSE(m_batch_server->is_active());
}

TEST_F(BatchServerTest, run_batch_read)
{
    InSequence sequence;

    int idx = 0;
    std::vector<double> result = {240.042, 250.052};

    for (const auto &request : m_signal_config) {
        EXPECT_CALL(*m_pio_ptr, push_signal(request.name, request.domain,
                                            request.domain_idx))
            .WillOnce(Return(idx))
            .RetiresOnSaturation();
        ++idx;
    }

    for (const auto &request : m_control_config) {
        EXPECT_CALL(*m_pio_ptr, push_control(request.name, request.domain,
                                             request.domain_idx))
            .WillOnce(Return(idx))
            .RetiresOnSaturation();
        ++idx;
    }

    EXPECT_CALL(*m_batch_status, receive_message())
        .WillOnce(Return(BatchStatus::M_MESSAGE_READ))
        .RetiresOnSaturation();

    EXPECT_CALL(*m_pio_ptr, read_batch())
        .Times(1);

    for (idx = 0; idx != 2; ++idx) {
        EXPECT_CALL(*m_pio_ptr, sample(idx))
            .WillOnce(Return(result[idx]))
            .RetiresOnSaturation();
    }

    EXPECT_CALL(*m_batch_status,
                send_message(BatchStatus::M_MESSAGE_CONTINUE))
        .Times(1)
        .RetiresOnSaturation();

    EXPECT_CALL(*m_batch_status, receive_message())
        .WillOnce(Return(BatchStatus::M_MESSAGE_QUIT))
        .RetiresOnSaturation();

    EXPECT_CALL(*m_batch_status,
                send_message(BatchStatus::M_MESSAGE_QUIT))
        .Times(1)
        .RetiresOnSaturation();

    m_batch_server->run_batch();

    double *data_ptr = (double *)(m_signal_shmem->pointer());
    EXPECT_EQ(result[0], data_ptr[0]);
    EXPECT_EQ(result[1], data_ptr[1]);
}

TEST_F(BatchServerTest, run_batch_write)
{
    InSequence sequence;

    int idx = 0;

    for (const auto &request : m_signal_config) {
        EXPECT_CALL(*m_pio_ptr, push_signal(request.name, request.domain,
                                            request.domain_idx))
            .WillOnce(Return(idx))
            .RetiresOnSaturation();
        ++idx;
    }

    for (const auto &request : m_control_config) {
        EXPECT_CALL(*m_pio_ptr, push_control(request.name, request.domain,
                                             request.domain_idx))
            .WillOnce(Return(idx))
            .RetiresOnSaturation();
        ++idx;
    }

    double control_expect = 260.062;
    double *data_ptr = (double *)(m_control_shmem->pointer());
    data_ptr[0] = control_expect;

    EXPECT_CALL(*m_batch_status, receive_message())
        .WillOnce(Return(BatchStatus::M_MESSAGE_WRITE))
        .RetiresOnSaturation();

    EXPECT_CALL(*m_pio_ptr, adjust(2, control_expect))
        .RetiresOnSaturation();

    EXPECT_CALL(*m_pio_ptr, write_batch())
        .Times(1);

    EXPECT_CALL(*m_batch_status,
                send_message(BatchStatus::M_MESSAGE_CONTINUE))
        .Times(1)
        .RetiresOnSaturation();

    EXPECT_CALL(*m_batch_status, receive_message())
        .WillOnce(Return(BatchStatus::M_MESSAGE_QUIT))
        .RetiresOnSaturation();

    EXPECT_CALL(*m_batch_status,
                send_message(BatchStatus::M_MESSAGE_QUIT))
        .Times(1)
        .RetiresOnSaturation();

    m_batch_server->run_batch();
}

TEST_F(BatchServerTest, create_shmem)
{

}
