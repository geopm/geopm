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
#include "geopm_test.hpp"

#include "gtest/gtest.h"
#include "gmock/gmock.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <unistd.h>

#include <string>     // for std::string

using testing::InSequence;
using testing::Return;
using testing::_;
using geopm::BatchServer;
using geopm::BatchServerImp;
using geopm::BatchStatus;
using geopm::POSIXSignal;
using geopm::pid_to_uid;
using geopm::pid_to_gid;


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
        std::shared_ptr<BatchServerImp> m_batch_server_signals;
        std::shared_ptr<BatchServerImp> m_batch_server_controls;
        std::shared_ptr<BatchServerImp> m_batch_server_pid;

};

void BatchServerTest::SetUp()
{
    m_pio_ptr = std::make_shared<MockPlatformIO>();
    m_batch_status = std::make_shared<MockBatchStatus>();
    m_posix_signal = std::make_shared<MockPOSIXSignal>();
    m_server_pid = 1234;
    m_client_pid = 4321;
    m_signal_shmem = std::make_shared<MockSharedMemory>(2 * sizeof(double));
    m_control_shmem = std::make_shared<MockSharedMemory>(sizeof(double));
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

    m_batch_server_signals = std::make_shared<BatchServerImp>(m_client_pid,
                                                              m_signal_config,
                                                              std::vector<geopm_request_s>{},
                                                              *m_pio_ptr,
                                                              m_batch_status,
                                                              m_posix_signal,
                                                              m_signal_shmem,
                                                              nullptr,
                                                              m_server_pid);

    m_batch_server_controls = std::make_shared<BatchServerImp>(m_client_pid,
                                                               std::vector<geopm_request_s>{},
                                                               m_control_config,
                                                               *m_pio_ptr,
                                                               m_batch_status,
                                                               m_posix_signal,
                                                               nullptr,
                                                               m_control_shmem,
                                                               m_server_pid);

    m_batch_server_pid = std::make_shared<BatchServerImp>(getpid(),
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
    EXPECT_CALL(*m_posix_signal,
                sig_queue(m_server_pid,
                          SIGTERM,
                          BatchStatus::M_MESSAGE_TERMINATE))
        .Times(3);
    m_batch_server_signals.reset();
    m_batch_server_controls.reset();
    m_batch_server_pid.reset();
}

/**
 * @test BatchServerImp::server_pid() in isolation
 */
TEST_F(BatchServerTest, get_server_pid)
{
    EXPECT_EQ(m_server_pid, m_batch_server->server_pid());

    int server_pid = m_batch_server->server_pid();
    EXPECT_EQ(m_server_pid, server_pid);
}

/**
 * @test BatchServerImp::get_server_key() in isolation
 */
TEST_F(BatchServerTest, get_server_key)
{
    std::string key = m_batch_server->server_key();
    EXPECT_EQ(std::to_string(m_client_pid), key);
}

/**
 * @test Check BatchServerImp::stop_batch() to send SIGTERM signal to the server.
 */
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

/**
 * @test Check BatchServerImp::run_batch() under normal operation, given a sequence of read requests.
 *       First the requests signals and controls are filled up.
 *       Second the server recieves a message to read the batch.
 *       It then samples both signals, and writes the results into the shared memory buffer.
 *       The server sends a message for the client to continue.
 *       The server recieved a message from the client to quit.
 */
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

    for (idx = 0; idx < 2; ++idx) {
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

/**
 * @test Check BatchServerImp::run_batch() when there are no signals and you try to read.
 *       First the requests only controls are filled up.
 *       Second the server recieves a message to read the batch.
 *       However there are no signals, so it does nothing.
 *       The server sends a message for the client to continue.
 *       The server recieved a message from the client to quit.
 */
TEST_F(BatchServerTest, run_batch_read_empty)
{
    InSequence sequence;

    int idx = 0;
    std::vector<double> result = {};

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

    m_batch_server_controls->run_batch();
}

/**
 * @test Check BatchServerImp::run_batch() under normal operation, given a sequence of write requests.
 *       First the requests signals and controls are filled up.
 *       Second the server recieves a message to write the batch.
 *       It then reads the values from the shared memory and writes all of the pushed controls to the platform.
 *       The server sends a message for the client to continue.
 *       The server recieved a message from the client to quit.
 */
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

/**
 * @test Check BatchServerImp::run_batch() when there are no controls and you try to write.
 *       First the requests only signals are filled up.
 *       Second the server recieves a message to write the batch.
 *       However there are no controls, so it does nothing.
 *       The server sends a message for the client to continue.
 *       The server recieved a message from the client to quit.
 */
TEST_F(BatchServerTest, run_batch_write_empty)
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

    EXPECT_CALL(*m_batch_status, receive_message())
        .WillOnce(Return(BatchStatus::M_MESSAGE_WRITE))
        .RetiresOnSaturation();

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

    m_batch_server_signals->run_batch();
}

/**
 * @test Check if we can create the shared memory.
 *       Check if the shared memory files for both the signals and controls were created in the /dev/shm/
 *       Check the sizes of the files match the number of signals and controls.
 *       Check the uid and gid permissions of the files.
 */
TEST_F(BatchServerTest, create_shmem)
{
    int this_pid = getpid();
    uid_t uid = pid_to_uid(this_pid);
    gid_t gid = pid_to_gid(this_pid);

    const size_t signal_shmem_size  = (m_signal_config.size()  * sizeof(double)) + geopm::hardware_destructive_interference_size;
    const size_t control_shmem_size = (m_control_config.size() * sizeof(double)) + geopm::hardware_destructive_interference_size;

    const std::string shmem_dir = "/dev/shm/";
    const std::string shmem_prefix_signal  = shmem_dir + std::string(BatchServer::M_SHMEM_PREFIX) + std::to_string(this_pid) + "-signal";
    const std::string shmem_prefix_control = shmem_dir + std::string(BatchServer::M_SHMEM_PREFIX) + std::to_string(this_pid) + "-control";

    m_batch_server_pid->create_shmem();

    struct stat data;
    int result = stat((shmem_prefix_signal).c_str(), &data);
    ASSERT_EQ(0, result);                                // check that the file exists
    EXPECT_EQ(signal_shmem_size, (size_t)data.st_size);  // check the size of the file
    EXPECT_EQ(uid, data.st_uid);                         // check the chown succeeded in uid
    EXPECT_EQ(gid, data.st_gid);                         // check the chown succeeded in gid

    result = stat(shmem_prefix_control.c_str(), &data);
    ASSERT_EQ(0, result);                                 // check that the file exists
    EXPECT_EQ(control_shmem_size, (size_t)data.st_size);  // check the size of the file
    EXPECT_EQ(uid, data.st_uid);                          // check the chown succeeded in uid
    EXPECT_EQ(gid, data.st_gid);                          // check the chown succeeded in gid

    EXPECT_EQ(0, unlink(shmem_prefix_signal.c_str()));
    EXPECT_EQ(0, unlink(shmem_prefix_control.c_str()));
}

/**
 * @test Check forking the batch server process.
 *       Check that the setup() function is called prior to the run() function.
 *       Check that the message came over the pipe used for synchronization.
 */
TEST_F(BatchServerTest, fork_with_setup)
{
    size_t *counter_mem = (size_t*) mmap(
        NULL,                        // void *addr
        sizeof(size_t),              // size_t length
        PROT_READ  | PROT_WRITE,     // int prot
        MAP_SHARED | MAP_ANONYMOUS,  // int flags
        -1,                          // int fd
        0);                          // off_t offset
    size_t &counter = *counter_mem;
    counter = 0;

    std::function<void(void)> setup = [&counter](void)
    {
        if (counter == 0u) {
            ++counter;
        }
    };

    std::function<void(void)> run = [&counter, this](void)
    {
        if (counter == 1u) {
            ++counter;
        }
        this->TearDown();
    };

    int forked_pid = m_batch_server_pid->fork_with_setup(setup, run);
    waitpid(forked_pid, NULL, 0);

    EXPECT_EQ(2u, counter);

    munmap(counter_mem, sizeof(size_t));
    counter_mem = nullptr;
}
