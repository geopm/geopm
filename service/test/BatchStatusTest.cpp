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

#include "BatchStatus.hpp"

#include "gtest/gtest.h"
#include <functional>
#include <unistd.h>
#include <sys/wait.h>

using geopm::BatchStatus;
using geopm::BatchStatusFIFO;

class BatchStatusTest : public ::testing::Test
{
    protected:
        void SetUp(void);
        void TearDown(void);
        int fork_other(std::function<void(void)> other_func);
        std::string m_server_prefix;
        std::string m_server_key;
};

int BatchStatusTest::fork_other(std::function<void(void)> other_func)
{
    int result = fork();
    if (result == 0) {
        other_func();
        exit(0);
    }
    return result;
}
void BatchStatusTest::SetUp(void)
{
    m_server_prefix = "batch-status";
    m_server_key = "test-key";
}

void BatchStatusTest::TearDown(void)
{
    std::string path = m_server_prefix + "-in-" + m_server_key;
    (void)unlink(path.c_str());
    path = m_server_prefix + "-out-" + m_server_key;
    (void)unlink(path.c_str());
}

TEST_F(BatchStatusTest, client_send_to_server_fifo)
{
    int client_pid = getpid();
    std::function<void(void)> test_func = [this, client_pid]()
    {
        auto server_status = BatchStatus::make_unique_server(
            client_pid, this->m_server_key
        );
        server_status->receive_message(BatchStatus::M_MESSAGE_READ);
    };
    int server_pid = fork_other(test_func);
    sleep(1);  // context switch
    auto client_status = BatchStatus::make_unique_client(m_server_key);
    client_status->send_message(BatchStatus::M_MESSAGE_READ);
    waitpid(server_pid, nullptr, 0);  // reap child process
}

TEST_F(BatchStatusTest, server_send_to_client_fifo)
{
    int client_pid = getpid();
    std::function<void(void)> test_func = [this, client_pid]()
    {
        auto server_status = BatchStatus::make_unique_server(
            client_pid, this->m_server_key
        );
        server_status->send_message(BatchStatus::M_MESSAGE_READ);
    };
    int server_pid = fork_other(test_func);
    sleep(1);  // context switch
    auto client_status = BatchStatus::make_unique_client(m_server_key);
    client_status->receive_message(BatchStatus::M_MESSAGE_READ);
    waitpid(server_pid, nullptr, 0);  // reap child process
}

TEST_F(BatchStatusTest, both_send_at_once_fifo)
{
    int client_pid = getpid();
    std::function<void(void)> test_func = [this, client_pid]()
    {
        auto server_status = BatchStatus::make_unique_server(
            client_pid, this->m_server_key
        );
        server_status->send_message(BatchStatus::M_MESSAGE_WRITE);
        server_status->receive_message(BatchStatus::M_MESSAGE_READ);
    };
    int server_pid = fork_other(test_func);
    sleep(1);  // context switch
    auto client_status = BatchStatus::make_unique_client(m_server_key);
    client_status->receive_message(BatchStatus::M_MESSAGE_WRITE);
    client_status->send_message(BatchStatus::M_MESSAGE_READ);
    waitpid(server_pid, nullptr, 0);  // reap child process
}

TEST_F(BatchStatusTest, client_send_to_server_signal)
{
    int client_pid = getpid();
    std::function<void(void)> test_func = [this, client_pid]()
    {
        auto server_status = BatchStatus::make_unique_signal(client_pid);
        server_status->receive_message(BatchStatus::M_MESSAGE_READ);
    };
    int server_pid = fork_other(test_func);
    sleep(1);  // context switch
    auto client_status = BatchStatus::make_unique_signal(server_pid);
    client_status->send_message(BatchStatus::M_MESSAGE_READ);
    waitpid(server_pid, nullptr, 0);  // reap child process
}

TEST_F(BatchStatusTest, server_send_to_client_signal)
{
    int client_pid = getpid();
    std::function<void(void)> test_func = [this, client_pid]()
    {
        auto server_status = BatchStatus::make_unique_signal(client_pid);
        server_status->send_message(BatchStatus::M_MESSAGE_READ);
    };
    int server_pid = fork_other(test_func);
    sleep(1);  // context switch
    auto client_status = BatchStatus::make_unique_signal(server_pid);
    client_status->receive_message(BatchStatus::M_MESSAGE_READ);
    waitpid(server_pid, nullptr, 0);  // reap child process
}

TEST_F(BatchStatusTest, both_send_at_once_signal)
{
    int client_pid = getpid();
    std::function<void(void)> test_func = [this, client_pid]()
    {
        auto server_status = BatchStatus::make_unique_signal(client_pid);
        server_status->send_message(BatchStatus::M_MESSAGE_WRITE);
        server_status->receive_message(BatchStatus::M_MESSAGE_READ);
    };
    int server_pid = fork_other(test_func);
    sleep(1);  // context switch
    auto client_status = BatchStatus::make_unique_signal(server_pid);
    client_status->receive_message(BatchStatus::M_MESSAGE_WRITE);
    client_status->send_message(BatchStatus::M_MESSAGE_READ);
    waitpid(server_pid, nullptr, 0);  // reap child process
}
