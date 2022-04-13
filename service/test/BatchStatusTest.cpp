/*
 * Copyright (c) 2015 - 2022, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "config.h"

#include "BatchStatus.hpp"

#include "geopm/Helper.hpp"

#include "gtest/gtest.h"
#include "geopm_test.hpp"

#include <functional>
#include <unistd.h>
#include <cerrno>
#include <sys/wait.h>

using geopm::BatchStatus;
using geopm::BatchStatusImp;

class BatchStatusTest : public ::testing::Test
{
    protected:
        void SetUp(void);
        void TearDown(void);
        int fork_other(std::function<void(int)> child_process_func);
        std::unique_ptr<BatchStatus> make_test_server(int client_pid);
        std::unique_ptr<BatchStatus> make_test_client();
        std::unique_ptr<BatchStatus> make_test_client(
            const std::string &server_key);
        std::string m_server_prefix;
        std::string m_server_key;
        std::string m_status_path_in;
        std::string m_status_path_out;
};

void BatchStatusTest::SetUp(void)
{
    m_server_prefix = "/tmp/geopm-test-service-batch-status-";
    m_server_key = "test-key";

    // Explicitly force the fifo to be removed if it is already existing.
    m_status_path_in = m_server_prefix + m_server_key + "-in" ;
    m_status_path_out = m_server_prefix + m_server_key + "-out";
    (void)unlink(m_status_path_in.c_str());
    (void)unlink(m_status_path_out.c_str());
}

void BatchStatusTest::TearDown(void)
{
    (void)unlink(m_status_path_in.c_str());
    (void)unlink(m_status_path_out.c_str());
}

int BatchStatusTest::fork_other(std::function<void(int)> child_process_func)
{
    int pipe_fd[2];
    pipe(pipe_fd);
    int &write_pipe_fd = pipe_fd[1];
    int &read_pipe_fd  = pipe_fd[0];
    int result = fork();
    // child process //
    if (result == 0) {
        close(read_pipe_fd);  // close read end of pipe
        child_process_func(write_pipe_fd);  // pass write end of pipe
        close(write_pipe_fd);  // close write end of pipe
        exit(EXIT_SUCCESS);
    // parent process //
    } else {
        close(write_pipe_fd);  // close write end of pipe
        char unique_char;
        int ret = read(read_pipe_fd, &unique_char, sizeof(unique_char));
        if (ret == -1) {
            ret = errno;
        }
        close(read_pipe_fd);  // close read end of pipe
    }
    return result;
}

std::unique_ptr<BatchStatus> BatchStatusTest::make_test_server(int client_pid)
{
    return geopm::make_unique<geopm::BatchStatusServer>(client_pid,
                                                        m_server_key,
                                                        m_server_prefix);
}

std::unique_ptr<BatchStatus> BatchStatusTest::make_test_client()
{
    return make_test_client(m_server_key);
}

std::unique_ptr<BatchStatus> BatchStatusTest::make_test_client(
    const std::string &server_key)
{
    return geopm::make_unique<geopm::BatchStatusClient>(server_key,
                                                        m_server_prefix);
}


/************************************
 * Test Fixtures of BatchStatusTest *
 ************************************/

TEST_F(BatchStatusTest, client_send_to_server_fifo_expect)
{
    int client_pid = getpid();
    std::function<void(int)> child_process_func = [this, client_pid](int write_pipe_fd)
    {
        auto server_status = this->make_test_server(client_pid);
        /* Extra code for synchronizing the server process. */
        char unique_char = '!';
        write(write_pipe_fd, &unique_char, sizeof(unique_char));

        server_status->receive_message(BatchStatus::M_MESSAGE_READ);
    };
    int server_pid = fork_other(child_process_func);

    auto client_status = make_test_client();
    client_status->send_message(BatchStatus::M_MESSAGE_READ);
    waitpid(server_pid, nullptr, 0);  // reap child process
}

TEST_F(BatchStatusTest, server_send_to_client_fifo_expect)
{
    int client_pid = getpid();
    std::function<void(int)> child_process_func = [this, client_pid](int write_pipe_fd)
    {
        auto server_status = this->make_test_server(client_pid);
        /* Extra code for synchronizing the server process. */
        char unique_char = '!';
        write(write_pipe_fd, &unique_char, sizeof(unique_char));

        server_status->send_message(BatchStatus::M_MESSAGE_READ);
    };
    int server_pid = fork_other(child_process_func);

    auto client_status = make_test_client();
    client_status->receive_message(BatchStatus::M_MESSAGE_READ);
    waitpid(server_pid, nullptr, 0);  // reap child process
}

TEST_F(BatchStatusTest, server_send_to_client_fifo)
{
    int client_pid = getpid();
    std::function<void(int)> child_process_func = [this, client_pid](int write_pipe_fd)
    {
        auto server_status = this->make_test_server(client_pid);
        /* Extra code for synchronizing the server process. */
        char unique_char = '!';
        write(write_pipe_fd, &unique_char, sizeof(unique_char));

        server_status->send_message(BatchStatus::M_MESSAGE_READ);
    };
    int server_pid = fork_other(child_process_func);

    auto client_status = make_test_client();
    char result = client_status->receive_message();
    char expected = BatchStatus::M_MESSAGE_READ;
    EXPECT_EQ(result, expected);
    waitpid(server_pid, nullptr, 0);  // reap child process
}

TEST_F(BatchStatusTest, both_send_at_once_fifo_expect)
{
    int client_pid = getpid();
    std::function<void(int)> child_process_func = [this, client_pid](int write_pipe_fd)
    {
        auto server_status = this->make_test_server(client_pid);
        /* Extra code for synchronizing the server process. */
        char unique_char = '!';
        write(write_pipe_fd, &unique_char, sizeof(unique_char));

        server_status->send_message(BatchStatus::M_MESSAGE_WRITE);
        server_status->receive_message(BatchStatus::M_MESSAGE_READ);
    };
    int server_pid = fork_other(child_process_func);

    auto client_status = make_test_client();
    client_status->receive_message(BatchStatus::M_MESSAGE_WRITE);
    client_status->send_message(BatchStatus::M_MESSAGE_READ);
    waitpid(server_pid, nullptr, 0);  // reap child process
}

TEST_F(BatchStatusTest, server_and_client_do_nothing)
{
    int client_pid = getpid();
    std::function<void(int)> child_process_func = [this, client_pid](int write_pipe_fd)
    {
        auto server_status = this->make_test_server(client_pid);
        /* Extra code for synchronizing the server process. */
        char unique_char = '!';
        write(write_pipe_fd, &unique_char, sizeof(unique_char));
    };
    int server_pid = fork_other(child_process_func);

    auto client_status = make_test_client();
    waitpid(server_pid, nullptr, 0);  // reap child process
}

TEST_F(BatchStatusTest, client_send_to_server_fifo_incorrect_expect)
{
    int client_pid = getpid();
    std::function<void(int)> child_process_func = [this, client_pid](int write_pipe_fd)
    {
        auto server_status = this->make_test_server(client_pid);
        /* Extra code for synchronizing the server process. */
        char unique_char = '!';
        write(write_pipe_fd, &unique_char, sizeof(unique_char));

        GEOPM_EXPECT_THROW_MESSAGE(
            server_status->receive_message(BatchStatus::M_MESSAGE_CONTINUE),
            GEOPM_ERROR_RUNTIME,
            "BatchStatusImp::receive_message(): Expected message:"
        );
    };
    int server_pid = fork_other(child_process_func);

    auto client_status = make_test_client();
    client_status->send_message(BatchStatus::M_MESSAGE_READ);
    waitpid(server_pid, nullptr, 0);  // reap child process
}

TEST_F(BatchStatusTest, bad_client_key)
{
    int client_pid = getpid();
    std::function<void(int)> child_process_func = [this, client_pid](int write_pipe_fd)
    {
        auto server_status = this->make_test_server(client_pid);
        /* Extra code for synchronizing the server process. */
        char unique_char = '!';
        write(write_pipe_fd, &unique_char, sizeof(unique_char));
    };
    int server_pid = fork_other(child_process_func);

    auto client_status = make_test_client("bad_key");
    GEOPM_EXPECT_THROW_MESSAGE(
        client_status->send_message(BatchStatus::M_MESSAGE_READ),
        ENOENT,
        "BatchStatusImp: System call failed: open(2)"
    );
    waitpid(server_pid, nullptr, 0);  // reap child process
}
