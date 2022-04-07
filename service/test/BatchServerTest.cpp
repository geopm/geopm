/*
 * Copyright (c) 2015 - 2022, Intel Corporation
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
#include "geopm/Exception.hpp"
#include "geopm/Helper.hpp"
#include "geopm_test.hpp"

#include "gtest/gtest.h"
#include "gmock/gmock.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <unistd.h>

#include <cerrno>
#include <string>
#include <sstream>
#include <iostream>

using testing::InSequence;
using testing::Return;
using testing::Throw;
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
        int fork_other(std::function<void(int, int)> child_process_func,
                       std::function<void(int, int)> parent_process_func,
                       bool child_is_server);
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
        std::shared_ptr<BatchServerImp> m_batch_server_child_process;

        static const std::string M_SHMEM_PREFIX;
        std::string m_shmem_prefix_signal;
        std::string m_shmem_prefix_control;
};

class BatchServerNameTest : public BatchServer, public ::testing::Test
{
    public:
        MOCK_METHOD(int, server_pid, (), (const, override));
        MOCK_METHOD(std::string, server_key, (), (const, override));
        MOCK_METHOD(void, stop_batch, (), (override));
        MOCK_METHOD(bool, is_active, (), (override));
};

const std::string BatchServerTest::M_SHMEM_PREFIX =
    "/dev/shm/geopm-service-batch-buffer-";

void BatchServerTest::SetUp()
{
    int this_pid = getpid();
    m_shmem_prefix_signal = M_SHMEM_PREFIX + std::to_string(this_pid) +
        "-signal";
    m_shmem_prefix_control = M_SHMEM_PREFIX + std::to_string(this_pid) +
        "-control";

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
                                                      "",
                                                      "",
                                                      *m_pio_ptr,
                                                      m_batch_status,
                                                      m_posix_signal,
                                                      m_signal_shmem,
                                                      m_control_shmem,
                                                      m_server_pid);

    m_batch_server_signals = std::make_shared<BatchServerImp>(
        m_client_pid,
        m_signal_config,
        std::vector<geopm_request_s>{},
        "",
        "",
        *m_pio_ptr,
        m_batch_status,
        m_posix_signal,
        m_signal_shmem,
        nullptr,
        m_server_pid);

    m_batch_server_controls = std::make_shared<BatchServerImp>(
        m_client_pid,
        std::vector<geopm_request_s>{},
        m_control_config,
        "",
        "",
        *m_pio_ptr,
        m_batch_status,
        m_posix_signal,
        nullptr,
        m_control_shmem,
        m_server_pid);

    m_batch_server_pid = std::make_shared<BatchServerImp>(
        this_pid,
        m_signal_config,
        m_control_config,
        m_shmem_prefix_signal,
        m_shmem_prefix_control,
        *m_pio_ptr,
        m_batch_status,
        m_posix_signal,
        m_signal_shmem,
        m_control_shmem,
        m_server_pid);

    m_batch_server_child_process = std::make_shared<BatchServerImp>(
        this_pid,
        m_signal_config,
        m_control_config,
        "",
        "",
        *m_pio_ptr,
        m_batch_status,
        m_posix_signal,
        m_signal_shmem,
        m_control_shmem,
        0);
}


void BatchServerTest::TearDown()
{
    if (m_batch_server->is_active()) {
        EXPECT_CALL(*m_posix_signal,
                    sig_queue(m_server_pid, SIGTERM, BatchStatus::M_MESSAGE_TERMINATE));
        m_batch_server.reset();
    }
    EXPECT_CALL(*m_posix_signal,
                sig_queue(m_server_pid, SIGTERM, BatchStatus::M_MESSAGE_TERMINATE))
        .Times(3);
    m_batch_server_signals.reset();
    m_batch_server_controls.reset();
    m_batch_server_pid.reset();
    m_batch_server_child_process.reset();
}

/**
 * @test Check BatchServerImp::server_pid() in isolation
 */
TEST_F(BatchServerTest, get_server_pid)
{
    EXPECT_EQ(m_server_pid, m_batch_server->server_pid());

    int server_pid = m_batch_server->server_pid();
    EXPECT_EQ(m_server_pid, server_pid);
}

/**
 * @test Check BatchServerImp::get_server_key() in isolation
 */
TEST_F(BatchServerTest, get_server_key)
{
    std::string key = m_batch_server->server_key();
    EXPECT_EQ(std::to_string(m_client_pid), key);
}

/**
 * @test Check that BatchServerImp::stop_batch() sends a SIGTERM signal to the server.
 */
TEST_F(BatchServerTest, stop_batch)
{
    EXPECT_TRUE(m_batch_server->is_active());
    EXPECT_CALL(*m_posix_signal,
                sig_queue(m_server_pid, SIGTERM, BatchStatus::M_MESSAGE_TERMINATE));
    m_batch_server->stop_batch();
    EXPECT_FALSE(m_batch_server->is_active());
}

/**
 * @test Check that BatchServerImp::stop_batch() throws an exception if it is called from the child process.
 *       Then check the catch block in BatchServerImp::stop_batch() by forcing a failure of POSIXSignalImp::sig_queue().
 */
TEST_F(BatchServerTest, stop_batch_exception)
{
    GEOPM_EXPECT_THROW_MESSAGE(m_batch_server_child_process->stop_batch(),
                               GEOPM_ERROR_RUNTIME,
                               "BatchServerImp::stop_batch(): must be called from parent process, not child process");

    EXPECT_CALL(*m_posix_signal,
                sig_queue(m_server_pid, SIGTERM, BatchStatus::M_MESSAGE_TERMINATE))
        .Times(1)
        /// @throws geopm::Exception to simulate a failure of the sigqueue(2) system call in POSIXSignalImp::sig_queue()
        .WillRepeatedly(Throw(geopm::Exception("POSIXSignal(): POSIX signal function call sigqueue() returned an error",
                              EINVAL, __FILE__, __LINE__)));

    GEOPM_EXPECT_THROW_MESSAGE(m_batch_server->stop_batch(),
                               EINVAL,
                               "POSIXSignal(): POSIX signal function call sigqueue() returned an error");
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

    std::vector<double> result = {240.042, 250.052};

    int idx = 0;
    for (const auto &request : m_signal_config) {
        EXPECT_CALL(*m_pio_ptr, push_signal(request.name, request.domain_type,
                                            request.domain_idx))
            .WillOnce(Return(idx))
            .RetiresOnSaturation();
        ++idx;
    }

    idx = 0;
    for (const auto &request : m_control_config) {
        EXPECT_CALL(*m_pio_ptr, push_control(request.name, request.domain_type,
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
 *       First the control requests are populated.
 *       Second the server recieves a message to read the batch.
 *       However there are no signals, so it does nothing.
 *       The server sends a message for the client to continue.
 *       The server recieved a message from the client to quit.
 */
TEST_F(BatchServerTest, run_batch_read_empty)
{
    InSequence sequence;

    int idx = 0;
    for (const auto &request : m_control_config) {
        EXPECT_CALL(*m_pio_ptr, push_control(request.name, request.domain_type,
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
        EXPECT_CALL(*m_pio_ptr, push_signal(request.name, request.domain_type,
                                            request.domain_idx))
            .WillOnce(Return(idx))
            .RetiresOnSaturation();
        ++idx;
    }

    // idx should be the same when we use write_batch()
    for (const auto &request : m_control_config) {
        EXPECT_CALL(*m_pio_ptr, push_control(request.name, request.domain_type,
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
        EXPECT_CALL(*m_pio_ptr, push_signal(request.name, request.domain_type,
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
 * @test Provides coverage for the case when BatchStatus::M_MESSAGE_TERMINATE is read message
 *       by the server in BatchServerImp::event_loop()
 */
TEST_F(BatchServerTest, receive_message_terminate)
{
    InSequence sequence;

    int idx = 0;
    for (const auto &request : m_signal_config) {
        EXPECT_CALL(*m_pio_ptr, push_signal(request.name, request.domain_type,
                                            request.domain_idx))
            .WillOnce(Return(idx))
            .RetiresOnSaturation();
        ++idx;
    }

    idx = 0;
    for (const auto &request : m_control_config) {
        EXPECT_CALL(*m_pio_ptr, push_control(request.name, request.domain_type,
                                             request.domain_idx))
            .WillOnce(Return(idx))
            .RetiresOnSaturation();
        ++idx;
    }

    EXPECT_CALL(*m_batch_status, receive_message())
        .WillOnce(Return(BatchStatus::M_MESSAGE_TERMINATE))
        .RetiresOnSaturation();

    m_batch_server->run_batch();
}

/**
 * @test Provides coverage for the case when an invalid message is read
 *       by the server in the BatchServerImp::event_loop().
 */
TEST_F(BatchServerTest, receive_message_default)
{
    InSequence sequence;

    int idx = 0;
    for (const auto &request : m_signal_config) {
        EXPECT_CALL(*m_pio_ptr, push_signal(request.name, request.domain_type,
                                            request.domain_idx))
            .WillOnce(Return(idx))
            .RetiresOnSaturation();
        ++idx;
    }

    idx = 0;
    for (const auto &request : m_control_config) {
        EXPECT_CALL(*m_pio_ptr, push_control(request.name, request.domain_type,
                                             request.domain_idx))
            .WillOnce(Return(idx))
            .RetiresOnSaturation();
        ++idx;
    }

    int ret_val = 127;  // a "random" number
    EXPECT_CALL(*m_batch_status, receive_message())
        .WillOnce(Return(ret_val))
        .RetiresOnSaturation();

    GEOPM_EXPECT_THROW_MESSAGE(m_batch_server->run_batch(),
                               GEOPM_ERROR_RUNTIME,
                               (std::string("BatchServerImp::run_batch(): Received unknown response from client: ") + std::to_string(ret_val)));
}

/**
 * @test Testing BatchStatus::receive_message() to throw an exception inside of BatchServerImp::read_message()
 *       This is needed to activate the catch blocks in BatchServerImp::read_message() and BatchServerImp::run_batch()
 */
TEST_F(BatchServerTest, receive_message_exception)
{
    InSequence sequence;

    int idx = 0;
    for (const auto &request : m_signal_config) {
        EXPECT_CALL(*m_pio_ptr, push_signal(request.name, request.domain_type,
                                            request.domain_idx))
            .WillOnce(Return(idx))
            .RetiresOnSaturation();
        ++idx;
    }

    idx = 0;
    for (const auto &request : m_control_config) {
        EXPECT_CALL(*m_pio_ptr, push_control(request.name, request.domain_type,
                                             request.domain_idx))
            .WillOnce(Return(idx))
            .RetiresOnSaturation();
        ++idx;
    }

    // This is where it throws the message.
    EXPECT_CALL(*m_batch_status, receive_message())
        .Times(1)
        /// @throws geopm::Exception() to simulate a failure of the read(2) system call in BatchStatusImp::receive_message().
        .WillRepeatedly([](){
            throw geopm::Exception("BatchStatusImp: System call failed: read(2)",
                                   EINTR, __FILE__, __LINE__);
            return '\0';
        });

    GEOPM_EXPECT_THROW_MESSAGE(m_batch_server->run_batch(),
                               EINTR,
                               "BatchStatusImp: System call failed: read(2)");
}

/**
 * @test Testing BatchStatusImp::send_message() to throw an exception inside of BatchServerImp::write_message()
 *       This is needed to activate the catch blocks in BatchServerImp::write_message() and BatchServerImp::run_batch()
 */
TEST_F(BatchServerTest, write_message_exception)
{
    InSequence sequence;

    int idx = 0;
    for (const auto &request : m_signal_config) {
        EXPECT_CALL(*m_pio_ptr, push_signal(request.name, request.domain_type,
                                            request.domain_idx))
            .WillOnce(Return(idx))
            .RetiresOnSaturation();
        ++idx;
    }

    idx = 0;
    for (const auto &request : m_control_config) {
        EXPECT_CALL(*m_pio_ptr, push_control(request.name, request.domain_type,
                                             request.domain_idx))
            .WillOnce(Return(idx))
            .RetiresOnSaturation();
        ++idx;
    }

    EXPECT_CALL(*m_batch_status, receive_message())
        .WillOnce(Return(BatchStatus::M_MESSAGE_QUIT))
        .RetiresOnSaturation();

    // This is where it throws the message.
    EXPECT_CALL(*m_batch_status,
                send_message(BatchStatus::M_MESSAGE_QUIT))
        .Times(1)
        /// @throws geopm::Exception to simulate failure of write(2) system call in BatchStatusImp::send_message()
        .WillRepeatedly(Throw(geopm::Exception("BatchStatusImp: System call failed: write(2)",
                              EINTR, __FILE__, __LINE__)));

    EXPECT_CALL(*m_batch_status,
                send_message(BatchStatus::M_MESSAGE_QUIT))
        .Times(1)
        .RetiresOnSaturation();

    GEOPM_EXPECT_THROW_MESSAGE(m_batch_server->run_batch(),
                               EINTR,
                               "BatchStatusImp: System call failed: write(2)");
}

/**
 * @test Testing SSTIOImp::read_batch() to throw an exception inside of BatchServerImp::read_and_update()
 *       This is needed to activate if (m_is_client_waiting) branch inside the catch block in BatchServerImp::run_batch()
 */
TEST_F(BatchServerTest, read_batch_exception)
{
    InSequence sequence;

    int idx = 0;
    for (const auto &request : m_signal_config) {
        EXPECT_CALL(*m_pio_ptr, push_signal(request.name, request.domain_type,
                                            request.domain_idx))
            .WillOnce(Return(idx))
            .RetiresOnSaturation();
        ++idx;
    }

    idx = 0;
    for (const auto &request : m_control_config) {
        EXPECT_CALL(*m_pio_ptr, push_control(request.name, request.domain_type,
                                             request.domain_idx))
            .WillOnce(Return(idx))
            .RetiresOnSaturation();
        ++idx;
    }

    // m_is_client_waiting = true;
    EXPECT_CALL(*m_batch_status, receive_message())
        .WillOnce(Return(BatchStatus::M_MESSAGE_READ))
        .RetiresOnSaturation();

    // This is where it throws the message.
    EXPECT_CALL(*m_pio_ptr, read_batch())
        .Times(1)
        /// @throws geopm::Exception to simulate failure of ioctl(2) system call in SSTIoctlImp::mbox() in SSTIOImp::read_batch()
        .WillRepeatedly(Throw(geopm::Exception("SSTIOImp::read_batch() mbox read failed",
                              EINVAL, __FILE__, __LINE__)));

    EXPECT_CALL(*m_batch_status,
                send_message(BatchStatus::M_MESSAGE_QUIT))
        .Times(1)
        .RetiresOnSaturation();

    GEOPM_EXPECT_THROW_MESSAGE(m_batch_server->run_batch(),
                               EINVAL,
                               "SSTIOImp::read_batch() mbox read failed");
}

/**
 * @test Check if we can create the shared memory.
 *       Check if the shared memory files for both the signals and controls were created in the /dev/shm/.
 *       Check the sizes of the files match the number of signals and controls.
 *       Check the uid and gid permissions of the files.
 */
TEST_F(BatchServerTest, create_shmem)
{
    int this_pid = getpid();
    uid_t uid = pid_to_uid(this_pid);
    gid_t gid = pid_to_gid(this_pid);

    const size_t signal_shmem_size  =
        (m_signal_config.size()  * sizeof(double)) +
        geopm::hardware_destructive_interference_size;
    const size_t control_shmem_size =
        (m_control_config.size() * sizeof(double)) +
        geopm::hardware_destructive_interference_size;

    m_batch_server_pid->create_shmem();

    struct stat data;
    int result = stat((m_shmem_prefix_signal).c_str(), &data);
    ASSERT_EQ(0, result); // check that the file exists
    // check the size of the file
    EXPECT_EQ(signal_shmem_size, (size_t)data.st_size);
    EXPECT_EQ(uid, data.st_uid); // check the chown succeeded in uid
    EXPECT_EQ(gid, data.st_gid); // check the chown succeeded in gid

    result = stat(m_shmem_prefix_control.c_str(), &data);
    ASSERT_EQ(0, result); // check that the file exists
    // check the size of the file
    EXPECT_EQ(control_shmem_size, (size_t)data.st_size);
    EXPECT_EQ(uid, data.st_uid); // check the chown succeeded in uid
    EXPECT_EQ(gid, data.st_gid); // check the chown succeeded in gid

    EXPECT_EQ(0, unlink(m_shmem_prefix_signal.c_str()));
    EXPECT_EQ(0, unlink(m_shmem_prefix_control.c_str()));
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

    std::function<char(void)> setup = [&counter](void)
    {
        if (counter == 0u) {
            ++counter;
        }
        return BatchStatus::M_MESSAGE_CONTINUE;
    };

    std::function<void(void)> run = [&counter](void)
    {
        if (counter == 1u) {
            ++counter;
        }
    };

    int forked_pid = m_batch_server_pid->fork_with_setup(setup, run);
    waitpid(forked_pid, NULL, 0);

    EXPECT_EQ(2u, counter);

    munmap(counter_mem, sizeof(size_t));
    counter_mem = nullptr;
}

/**
 * @test This is similar to BatchServerTest.fork_with_setup, except that it checks an error case.
 *       In the BatchServerImp::fork_with_setup() if the parent process received something other than
 *       BatchStatus::M_MESSAGE_CONTINUE over the pipe, it would cause an Exception to be thrown.
 *       This test guarantees coverage of the error case.
 */
TEST_F(BatchServerTest, fork_with_setup_exception)
{
    std::function<char(void)> setup = [](void)
    {
        return BatchStatus::M_MESSAGE_QUIT;
    };

    std::function<void(void)> run = [](void)
    {
    };

    // wait for any child process unless it gets overwritten with the actual PID of the child process.
    int forked_pid = 0;
    GEOPM_EXPECT_THROW_MESSAGE((forked_pid = m_batch_server_pid->fork_with_setup(setup, run)),
                               GEOPM_ERROR_RUNTIME,
                               "BatchServerImp: Receivied unexpected message from batch server at startup: \"113\"");
    waitpid(forked_pid, NULL, 0);
}

/**
 * @class CerrRedirect
 *
 * @details This class facilitates redirect the stderr to an internal buffer for recording purposes.
 */
class CerrRedirect {
    public:
        /**
         * @brief The redirect is NOT active when the object is created.
         */
        CerrRedirect()
        {
            m_cerr_backup = nullptr;
            m_is_active = false;
        }

        /**
         * @brief Restore the backup streambuf if it was not restored already.
         */
        ~CerrRedirect()
        {
            if (m_cerr_backup) {
                std::cerr.rdbuf(m_cerr_backup);
            }
            m_is_active = false;
        }

        /**
         * @brief Call this function first. Activates the redirect.
         *
         * @details After calling this function, all std::cerr will be redirected to the internal stringstream.
         *          This function saves the backup streambuf of std::cerr, and resets it to the internal stringstream buffer.
         */
        void open_redirect(void)
        {
            if (!m_is_active) {
                // Flush the remaining stderr into the terminal.
                std::cerr << std::flush;
                // Clear the stringstream
                m_buffer.clear();
                m_buffer.str(std::string());
                // Set the streambuf of the std::cerr equivalent to the streambuf of the stringstream.
                // Return the former streambuf of the std::cerr prior to the change.
                m_cerr_backup = std::cerr.rdbuf(m_buffer.rdbuf());
                m_is_active = true;
            }
        }

        /**
         * @brief This function deactivates the redirect. It should be called after open_redirect().
         *
         * @details This function restores the backup streambuf associated with the stderr, and closes the stringstream redirect.
         *
         * @return The internal string of the stringstream, which recorded the redirected output.
         */
        std::string close_redirect(void)
        {
            if (m_is_active) {
                // Flush the remaining stderr into the streambuf.
                std::cerr << std::flush;
                // Restore the streambuf of the std::cerr to it's original state.
                std::cerr.rdbuf(m_cerr_backup);
                m_is_active = false;

                return m_buffer.str();
            }
            else {
                return std::string();
            }
        }
    private:
        // records the redirected stderr into it's own streambuf
        std::stringstream m_buffer;
        // the backup streambuf associated with the stderr
        std::streambuf* m_cerr_backup;
        // is true if the redirect is active
        bool m_is_active;
};

/**
 * @test Check coverage of BatchServerImp::~BatchServerImp()
 *       Create new BatchServer objects, and have BatchServerImp::stop_batch() throw exceptions in the destructor.
 */
TEST_F(BatchServerTest, destructor_exceptions)
{
    ///
    /// Testing catch (const Exception &ex) {}
    ///

    // Create new BatchServer object
    std::shared_ptr<BatchServerImp> batch_server_exception =
        std::make_shared<BatchServerImp>(m_client_pid,
                                         m_signal_config,
                                         m_control_config,
                                         "",
                                         "",
                                         *m_pio_ptr,
                                         m_batch_status,
                                         m_posix_signal,
                                         m_signal_shmem,
                                         m_control_shmem,
                                         m_server_pid);

    EXPECT_CALL(*m_posix_signal,
                sig_queue(m_server_pid, SIGTERM,
                BatchStatus::M_MESSAGE_TERMINATE))
        .Times(1)
        /// @throws geopm::Exception to simulate a failure of the sigqueue(2) system call in POSIXSignalImp::sig_queue()
        .WillRepeatedly(Throw(
            geopm::Exception("POSIXSignal(): POSIX signal function call "
                             "sigqueue() returned an error", EINVAL, __FILE__,
                             __LINE__)));

    std::string expected_stderr_1 = "Warning: <geopm> "
                                    "BatchServerImp::~BatchServerImp(): "
                                    "Exception thrown in destructor: ";

    CerrRedirect c_redirect;
    c_redirect.open_redirect();
    batch_server_exception.reset();  // calling BatchServerImp::~BatchServerImp()
    std::string actual_stderr_1 = c_redirect.close_redirect();
    // Compare if the recorded stderr matches the expected stderr.
    EXPECT_TRUE(geopm::string_begins_with(actual_stderr_1, expected_stderr_1));

    ///
    /// Testing catch (...) {}
    ///

    // Create new BatchServer object
    batch_server_exception.reset(new BatchServerImp(m_client_pid,
                                                    m_signal_config,
                                                    m_control_config,
                                                    "",
                                                    "",
                                                    *m_pio_ptr,
                                                    m_batch_status,
                                                    m_posix_signal,
                                                    m_signal_shmem,
                                                    m_control_shmem,
                                                    m_server_pid));

    EXPECT_CALL(*m_posix_signal,
                sig_queue(m_server_pid, SIGTERM,
                BatchStatus::M_MESSAGE_TERMINATE))
        .Times(1)
        /// @throws (int)2 to simulate failure of sigqueue(2) system call in POSIXSignalImp::sig_queue()
        .WillRepeatedly(Throw(2));

    expected_stderr_1 = "Warning: <geopm> BatchServerImp::~BatchServerImp(): "
                        "Non-GEOPM exception thrown in destructor\n";

    c_redirect.open_redirect();
    batch_server_exception.reset();  // calling BatchServerImp::~BatchServerImp()
    actual_stderr_1 = c_redirect.close_redirect();
    // Compare if the recorded stderr matches the expected stderr.
    EXPECT_EQ(expected_stderr_1, actual_stderr_1);
}

/**
 * @brief Similar to the function in BatchStatusTest.cpp it forks a child process and sets up the
 *        synchronization mechanism via pipes to ensure that the process with the server configures itself
 *        first before the process with the client sends it a signal.
 *
 * @param child_process_func  Run this function after forking the child process and setting up the IPC.
 *                            This function holds the body of the child process, what it should do.
 *                            Takes the end of pipe and PID of parent process as parameters.
 *                            It is the write end of pipe if it's a server, and read end of pipe if it's a client.
 *
 * @param parent_process_func Similarly, this function holds the body of the parent process, what it should do after the fork.
 *                            Takes the end of pipe and PID of child process as parameters.
 *
 * @param child_is_server true if the child process is a server, false if the parent process is a server.
 *
 * @return int  The PID of the forked child process.
 */
int BatchServerTest::fork_other(std::function<void(int, int)> child_process_func,
                                std::function<void(int, int)> parent_process_func,
                                bool child_is_server)
{
    int main_pid = getpid();
    int pipe_fd[2];
    pipe(pipe_fd);
    int &write_pipe_fd = pipe_fd[1];
    int &read_pipe_fd  = pipe_fd[0];
    int result = fork();
    // child process //
    if (result == 0) {
        if (child_is_server) {
            close(read_pipe_fd);  // close read end of pipe
            child_process_func(write_pipe_fd, main_pid);  // pass write end of pipe
            close(write_pipe_fd);  // close write end of pipe
        }
        else {  // parent is server
            close(write_pipe_fd);  // close write end of pipe
            child_process_func(read_pipe_fd, main_pid);  // pass read end of pipe
            close(read_pipe_fd);  // close read end of pipe
        }
        // Use _Exit() instead of exit() to prevent Google Test from complaining about
        // leaked mock objects.
        _Exit(EXIT_SUCCESS);
    // parent process //
    }
    else {
        if (child_is_server) {
            close(write_pipe_fd);  // close write end of pipe
            parent_process_func(read_pipe_fd, result);  // pass read end of pipe
            close(read_pipe_fd);  // close read end of pipe
        }
        else {  // parent is server
            close(read_pipe_fd);  // close read end of pipe
            parent_process_func(write_pipe_fd, result);  // pass write end of pipe
            close(write_pipe_fd);  // close write end of pipe
        }
    }
    return result;
}

/**
 * @test Simulates the usual way that the BatchServer gets a terminate message from the user.
 *       This is via sending the SIGTERM signal which activates the action_sigterm()
 *       So this test represents a more accurate use case than BatchServerTest.receive_message_terminate
 *       This test forks a new child process, and creates a new BatchServer object in that child process,
 *       and then the parent process sends a terminate message to the BatchServer.
 */
TEST_F(BatchServerTest, fork_and_terminate_child)
{
    /// This function contains the child process, which is the server.
    /// It takes the write_pipe_fd and the PID of the parent process, which is the client.
    std::function<void(int, int)> child_process_func =
        [this](int write_pipe_fd, int client_pid)
    {
        // Create new BatchServer object
        std::shared_ptr<BatchServerImp> batch_server_test =
            std::make_shared<BatchServerImp>(
                client_pid,
                m_signal_config,
                m_control_config,
                "",
                "",
                *m_pio_ptr,
                m_batch_status,
                /* Create a real POSIXSignal because we want to use
                sig_action() */
                nullptr,
                m_signal_shmem,
                m_control_shmem,
                m_server_pid);

        // There is no EXPECT_CALL for m_posix_signal->make_sigset() and m_posix_signal->sig_action()
        // because we are using the real POSIXSignal instead of the mock object.

        // register the action_sigterm()
        // This uses the real POSIXSignal instead of the mock object.
        batch_server_test->child_register_handler();

        int idx = 0;
        for (const auto &request : m_signal_config) {
            EXPECT_CALL(*m_pio_ptr, push_signal(request.name,
                                                request.domain_type,
                                                request.domain_idx))
                .WillOnce(Return(idx))
                .RetiresOnSaturation();
            ++idx;
        }

        idx = 0;
        for (const auto &request : m_control_config) {
            EXPECT_CALL(*m_pio_ptr, push_control(request.name,
                                                 request.domain_type,
                                                 request.domain_idx))
                .WillOnce(Return(idx))
                .RetiresOnSaturation();
            ++idx;
        }

        // m_batch_status->receive_message() is called in BatchServerImp::read_message()
        // Internally it is blocking on the read() call, waiting on a message from the client.
        // Then the client sends it a SIGTERM, which activates action_sigterm()
        // and receive_message() throws an exception in the check_return().
        // This exception is caught in the BatchServerImp::read_message()
        // and char in_message = BatchStatus::M_MESSAGE_TERMINATE is returned to BatchServerImp::event_loop()
        EXPECT_CALL(*m_batch_status, receive_message())
            .WillOnce([&write_pipe_fd](){
                /* Extra code for synchronizing the server. */
                char unique_char = '!';
                write(write_pipe_fd, &unique_char, sizeof(unique_char));
                sleep(1024);
                throw geopm::Exception("BatchStatusImp: System call failed: "
                                       "read(2)", EINTR, __FILE__, __LINE__);
                // This is irrelevant, it doesn't get returned because of the throw statement.
                // This is just to match the return type of BatchStatusImp::receive_message()
                return '$';
            }).RetiresOnSaturation();

        // The server process is stopped at BatchStatus::receive_message() inside BatchServerImp::read_message()
        batch_server_test->run_batch();
    };

    /// This function contains the parent process, which is the client.
    /// It takes the read_pipe_fd and the PID of the child process, which is the server.
    std::function<void(int, int)> parent_process_func =
        [](int read_pipe_fd, int server_pid)
    {
        /* Extra code for synchronizing the client. */
        char unique_char;
        int ret = read(read_pipe_fd, &unique_char, sizeof(unique_char));
        if (ret == -1) {
            ret = errno;
        }

        /* Terminate the server process */
        sigval value;
        value.sival_int = BatchStatus::M_MESSAGE_TERMINATE;
        sigqueue(server_pid, SIGTERM, value);
    };

    int forked_pid = fork_other(child_process_func, parent_process_func, true);
    waitpid(forked_pid, NULL, 0);
}

/**
 * @test Simulates the usual way that the BatchServer gets a terminate message from the user.
 *       This is via sending the SIGTERM signal which activates the action_sigterm()
 *       So this test represents a more accurate use case than BatchServerTest.receive_message_terminate
 *       Unlike BatchServerTest.fork_and_terminate_child, this test the server is in the parent process,
 *       which enables LCOV to accurately mark the coverage lines.
 */
TEST_F(BatchServerTest, fork_and_terminate_parent)
{
    /// This function contains the child process, which is the client.
    /// It takes the read_pipe_fd and the PID of the parent process, which is the server.
    std::function<void(int, int)> child_process_func =
        [](int read_pipe_fd, int server_pid)
    {
        /* Extra code for synchronizing the client. */
        char unique_char = 'a';
        int ret = read(read_pipe_fd, &unique_char, sizeof(unique_char));
        if (ret == -1) {
            ret = errno;
        }

        /* Terminate the server */
        sigval value;
        value.sival_int = BatchStatus::M_MESSAGE_TERMINATE;
        sigqueue(server_pid, SIGTERM, value);
    };

    /// This function contains the parent process, which is the server.
    /// It takes the write_pipe_fd and the PID of the child process, which is the client.
    std::function<void(int, int)> parent_process_func =
        [this](int write_pipe_fd, int client_pid)
    {
        // Create new BatchServer object
        std::shared_ptr<BatchServerImp> batch_server_test =
            std::make_shared<BatchServerImp>(
                client_pid,
                m_signal_config,
                m_control_config,
                "",
                "",
                *m_pio_ptr,
                m_batch_status,
                /* Create a real POSIXSignal because we want to use
                sig_action() */
                nullptr,
                m_signal_shmem,
                m_control_shmem,
                m_server_pid);

        // There is no EXPECT_CALL for m_posix_signal->make_sigset() and m_posix_signal->sig_action()
        // because we are using the real POSIXSignal instead of the mock object.

        // register the action_sigterm()
        // This uses the real POSIXSignal instead of the mock object.
        batch_server_test->child_register_handler();

        int idx = 0;
        for (const auto &request : m_signal_config) {
            EXPECT_CALL(*m_pio_ptr, push_signal(request.name,
                                                request.domain_type,
                                                request.domain_idx))
                .WillOnce(Return(idx))
                .RetiresOnSaturation();
            ++idx;
        }

        idx = 0;
        for (const auto &request : m_control_config) {
            EXPECT_CALL(*m_pio_ptr, push_control(request.name,
                                                 request.domain_type,
                                                 request.domain_idx))
                .WillOnce(Return(idx))
                .RetiresOnSaturation();
            ++idx;
        }

        // m_batch_status->receive_message() is called in BatchServerImp::read_message()
        // Internally it is blocking on the read() call, waiting on a message from the client.
        // Then the client sends it a SIGTERM, which activates action_sigterm()
        // and receive_message() throws an exception in the check_return().
        // This exception is caught in the BatchServerImp::read_message()
        // and char in_message = BatchStatus::M_MESSAGE_TERMINATE is returned to BatchServerImp::event_loop()
        EXPECT_CALL(*m_batch_status, receive_message())
            .WillOnce([&write_pipe_fd](){
                /* Extra code for synchronizing the server. */
                char unique_char = '!';
                write(write_pipe_fd, &unique_char, sizeof(unique_char));
                sleep(1024);
                throw geopm::Exception("BatchStatusImp: System call failed: "
                                       "read(2)", EINTR, __FILE__, __LINE__);
                // This is irrelevant, it doesn't get returned because of the throw statement.
                // This is just to match the return type of BatchStatusImp::receive_message()
                return '$';
            }).RetiresOnSaturation();

        // The server process is stopped at BatchStatus::receive_message() inside BatchServerImp::read_message()
        batch_server_test->run_batch();
    };

    int forked_pid = fork_other(child_process_func, parent_process_func,
                                false);
    waitpid(forked_pid, NULL, 0);
}

/**
 * @test Enables coverage for the path of the SIGCHLD handler upon normal operation,
 *       when a forked child process terminates, and sends a SIGCHLD to the server.
 */
TEST_F(BatchServerTest, action_sigchld)
{
    /// This function contains the child process, which is the client.
    /// It takes the read_pipe_fd and the PID of the parent process, which is the server.
    std::function<void(int, int)> child_process_func =
        [](int read_pipe_fd, int server_pid)
    {
        /* Extra code for synchronizing the client. */
        char unique_char = 'a';
        int ret = read(read_pipe_fd, &unique_char, sizeof(unique_char));
        if (ret == -1) {
            ret = errno;
        }

        // When the child process exits, it sends a SIGCHLD to the parent process.
    };

    /// This function contains the parent process, which is the server.
    /// It takes the write_pipe_fd and the PID of the child process, which is the client.
    std::function<void(int, int)> parent_process_func =
        [this](int write_pipe_fd, int client_pid)
    {
        // Create new BatchServer object
        std::shared_ptr<BatchServerImp> batch_server_test =
            std::make_shared<BatchServerImp>(
                client_pid,
                m_signal_config,
                m_control_config,
                "",
                "",
                *m_pio_ptr,
                m_batch_status,
                /* Create a real POSIXSignal because we want to use
                sig_action() */
                nullptr,
                m_signal_shmem,
                m_control_shmem,
                m_server_pid);

        // There is no EXPECT_CALL for m_posix_signal->make_sigset() and m_posix_signal->sig_action()
        // because we are using the real POSIXSignal instead of the mock object.

        // register the action_sigchld()
        // This uses the real POSIXSignal instead of the mock object.
        batch_server_test->parent_register_handler();

        EXPECT_TRUE(batch_server_test->is_active());

        /* Extra code for synchronizing the server. */
        char unique_char = '!';
        write(write_pipe_fd, &unique_char, sizeof(unique_char));

        errno = 0;
        sleep(UINT_MAX);  // exits the sleep when it is interrupted by a signal

        EXPECT_FALSE(batch_server_test->is_active());
    };

    int forked_pid = fork_other(child_process_func, parent_process_func,
                                false);
    waitpid(forked_pid, NULL, 0);
}

/**
 * @test Enables coverage for the path of the SIGCHLD handler upon abnormal operation,
 *       when the server gets a SIGCHLD signal from a process which is not a child.
 *       This test sets g_sigchld_status to GEOPM_ERROR_RUNTIME.
 */
TEST_F(BatchServerTest, action_sigchld_error)
{
    /// This function contains the child process, which is the server.
    /// It takes the write_pipe_fd and the PID of the parent process, which is the client.
    std::function<void(int, int)> child_process_func =
        [this](int write_pipe_fd, int client_pid)
    {
        // Create new BatchServer object
        std::shared_ptr<BatchServerImp> batch_server_test =
            std::make_shared<BatchServerImp>(
                client_pid,
                m_signal_config,
                m_control_config,
                "",
                "",
                *m_pio_ptr,
                m_batch_status,
                /* Create a real POSIXSignal because we want to use
                sig_action() */
                nullptr,
                m_signal_shmem,
                m_control_shmem,
                m_server_pid);

        // There is no EXPECT_CALL for m_posix_signal->make_sigset() and m_posix_signal->sig_action()
        // because we are using the real POSIXSignal instead of the mock object.

        // register the action_sigchld()
        // This uses the real POSIXSignal instead of the mock object.
        batch_server_test->parent_register_handler();

        EXPECT_TRUE(batch_server_test->is_active());

        /* Extra code for synchronizing the server. */
        char unique_char = '!';
        write(write_pipe_fd, &unique_char, sizeof(unique_char));

        errno = 0;
        sleep(UINT_MAX);  // exits the sleep when it is interrupted by a signal

        CerrRedirect c_redirect;
        c_redirect.open_redirect();
        EXPECT_FALSE(batch_server_test->is_active());
        std::string actual_stderr = c_redirect.close_redirect();

        int error_value = ECHILD; // <geopm> No child processes
        std::string expected_stderr = " :  The batch server child process "
                                      "ended with non-zero status: ";
        expected_stderr += std::to_string(error_value) + " : \"" +
                           geopm::error_message(error_value) + "\"\n";

        // Compare if the recorded stderr matches the expected stderr.
        EXPECT_TRUE(actual_stderr.find(expected_stderr) != std::string::npos);
    };

    /// This function contains the parent process, which is the client.
    /// It takes the read_pipe_fd and the PID of the child process, which is the server.
    std::function<void(int, int)> parent_process_func =
        [](int read_pipe_fd, int server_pid)
    {
        /* Extra code for synchronizing the client. */
        char unique_char;
        int ret = read(read_pipe_fd, &unique_char, sizeof(unique_char));
        if (ret == -1) {
            ret = errno;
        }

        // Send a SIGCHLD to the server process, from the parent process, not from a child process,
        // triggering an error condition.
        sigval value;
        sigqueue(server_pid, SIGCHLD, value);
    };

    int forked_pid = fork_other(child_process_func, parent_process_func, true);
    waitpid(forked_pid, NULL, 0);
}

TEST_F(BatchServerNameTest, signal_shmem_key)
{
    const std::string server_key = "test";
    const std::string expected_shmem_key = M_SHMEM_PREFIX + server_key +
        "-signal";
    std::string signal_shmem_key = get_signal_shmem_key(server_key);
    EXPECT_EQ(expected_shmem_key, signal_shmem_key);
}

TEST_F(BatchServerNameTest, control_shmem_key)
{
    const std::string server_key = "test";
    const std::string expected_shmem_key = M_SHMEM_PREFIX + server_key +
        "-control";
    std::string control_shmem_key = get_control_shmem_key(server_key);
    EXPECT_EQ(expected_shmem_key, control_shmem_key);
}
