/*
 * Copyright (c) 2015 - 2022, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
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

TEST_F(BatchClientTest, write_batch_wrong_size)
{
    std::vector<double> wrong_size_vector = {12.58, 29.85, 93.21, 11.12};

    GEOPM_EXPECT_THROW_MESSAGE(
        m_batch_client->write_batch(wrong_size_vector),
        GEOPM_ERROR_INVALID,
        "BatchClientImp::write_batch(): settings vector length does not match the number of configured controls"
    );
}

TEST_F(BatchClientTest, write_batch_wrong_size_empty)
{
    std::vector<double> wrong_size_vector = {12.58, 29.85, 93.21, 11.12};

    GEOPM_EXPECT_THROW_MESSAGE(
        m_batch_client_empty->write_batch(wrong_size_vector),
        GEOPM_ERROR_INVALID,
        "BatchClientImp::write_batch(): settings vector length does not match the number of configured controls"
    );
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

TEST_F(BatchClientTest, stop_batch)
{
    InSequence sequence;

    EXPECT_CALL(*m_batch_status,
                send_message(BatchStatus::M_MESSAGE_QUIT))
    .Times(1)
    .RetiresOnSaturation();

    EXPECT_CALL(*m_batch_status,
                receive_message(BatchStatus::M_MESSAGE_QUIT))
    .Times(1)
    .RetiresOnSaturation();

    m_batch_client->stop_batch();
}
