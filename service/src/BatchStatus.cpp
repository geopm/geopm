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
#include "POSIXSignal.hpp"

#include "geopm/Exception.hpp"
#include "geopm/Helper.hpp"

#include <cerrno>
#include <sstream>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

namespace geopm
{
    std::unique_ptr<BatchStatus>
    BatchStatus::make_unique_server(int client_pid,
                                    const std::string &server_key)
    {
        // calling the server constructor
        return geopm::make_unique<BatchStatusFIFO>(client_pid, server_key);
    }

    std::unique_ptr<BatchStatus>
    BatchStatus::make_unique_client(const std::string &server_key)
    {
        // calling the client constructor
        return geopm::make_unique<BatchStatusFIFO>(server_key);
    }

    std::unique_ptr<BatchStatus>
    BatchStatus::make_unique_signal(int other_pid)
    {
        // calling the signal constructor
        return geopm::make_unique<BatchStatusSignal>(other_pid);
    }

    // The constructor which is called by both the client and the server for signals.
    BatchStatusSignal::BatchStatusSignal(int other_pid)
    : other_pid(other_pid),
      m_signal(new POSIXSignalImp)
    {
        //
    }

    BatchStatusSignal::~BatchStatusSignal()
    {
        delete m_signal; m_signal = nullptr;
    }

    void BatchStatusSignal::send_message(char msg)
    {
        m_signal->sig_queue(other_pid, SIGCONT, msg);
    }

    char BatchStatusSignal::receive_message(void)
    {
        sigset_t signal_set = m_signal->make_sigset({SIGCONT});
        siginfo_t signal_info = {0};
        m_signal->sig_wait_info(&signal_set, &signal_info);
        POSIXSignal::m_info_s info = m_signal->reduce_info(signal_info);
        char actual = info.value;
        return actual;
    }

    void BatchStatusSignal::receive_message(char expect)
    {
        sigset_t signal_set = m_signal->make_sigset({SIGCONT});
        siginfo_t signal_info = {0};
        m_signal->sig_wait_info(&signal_set, &signal_info);
        POSIXSignal::m_info_s info = m_signal->reduce_info(signal_info);
        char actual = info.value;
        if (actual != expect) {
            std::ostringstream error_message;
            error_message << "BatchStatusSignal::receive_message(): "
                          << "Expected message: \"" << expect
                          << "\" but received \"" <<   actual << "\"";
            throw geopm::Exception(
                error_message.str(),
                GEOPM_ERROR_RUNTIME,
                __FILE__,
                __LINE__
            );
        }
    }

    // The constructor which is called by the client.
    BatchStatusFIFO::BatchStatusFIFO(const std::string &server_key,
                                     const std::string &fifo_prefix = "/tmp/geopm-service-status")
    : m_fifo_prefix(fifo_prefix),
      m_read_fifo_path(m_fifo_prefix + "-out-" + server_key),
      m_write_fifo_path(m_fifo_prefix + "-in-" + server_key),
      m_read_fd(-1),
      m_write_fd(-1),
      open_function(client_open),
      close_function(nullptr),
    {
        // Assume that the server itself will make the fifo.
    }

    // The constructor which is called by the server.
    BatchStatusFIFO::BatchStatusFIFO(int client_pid, const std::string &server_key,
                                     const std::string &fifo_prefix = "/tmp/geopm-service-status")
    : m_fifo_prefix(fifo_prefix),
      m_read_fifo_path(m_fifo_prefix + "-in-" + server_key),
      m_write_fifo_path(m_fifo_prefix + "-out-" + server_key),
      m_read_fd(-1),
      m_write_fd(-1),
      open_function(server_open),
      close_function(close)
    {
        // The server first creates the fifo in the file system.
        check_return(
            mkfifo(m_read_fifo_path.c_str(), S_IRUSR | S_IWUSR),
            "mkfifo(3)"
        );
        check_return(
            mkfifo(m_write_fifo_path.c_str(), S_IRUSR | S_IWUSR),
            "mkfifo(3)"
        );

        // Then the server grants the client ownership of the fifo.
        int uid = pid_to_uid(client_pid);
        int gid = pid_to_gid(client_pid);
        check_return(
            chown(m_read_fifo_path.c_str(), uid, gid),
            "chown(2)"
        );
        check_return(
            chown(m_write_fifo_path.c_str(), uid, gid),
            "chown(2)"
        );
    }

    BatchStatusFIFO::~BatchStatusFIFO()
    {
        check_return(close_file(m_write_fd), "close(2)");
        check_return(close_file(m_read_fd), "close(2)");
    }

    void BatchStatusFIFO::server_open(void)
    {
        m_write_fd = open(m_write_fifo_path.c_str(), O_WRONLY);
        check_return(m_write_fd, "open(2)");
        m_read_fd = open(m_read_fifo_path.c_str(), O_RDONLY);
        check_return(m_read_fd, "open(2)");
    }

    void BatchStatusFIFO::client_open(void)
    {
        m_read_fd = open(m_read_fifo_path.c_str(), O_RDONLY);
        check_return(m_read_fd, "open(2)");
        m_write_fd = open(m_write_fifo_path.c_str(), O_WRONLY);
        check_return(m_write_fd, "open(2)");
    }

    void BatchStatusFIFO::send_message(char msg)
    {
        if (!is_open()) open_fifo();
        check_return(write(m_write_fd, &msg, sizeof(char)), "write(2)");
    }

    char BatchStatusFIFO::receive_message(void)
    {
        if (!is_open()) open_fifo();
        char result = '\0';
        check_return(read(m_read_fd, &result, sizeof(char)), "read(2)");
        return result;
    }

    void BatchStatusFIFO::receive_message(char expect)
    {
        if (!is_open()) open_fifo();
        char actual = receive_message();
        if (actual != expect) {
            std::ostringstream error_message;
            error_message << "BatchStatusFIFO::receive_message(): "
                          << "Expected message: \"" << expect
                          << "\" but received \"" <<   actual << "\"";
            throw geopm::Exception(
                error_message.str(),
                GEOPM_ERROR_RUNTIME,
                __FILE__,
                __LINE__
            );
        }
    }

    void BatchStatusFIFO::check_return(int ret, const std::string &func_name)
    {
        if (ret == -1) {
            throw geopm::Exception(
                "BatchStatusFIFO: System call failed: " + func_name,
                errno ? errno : GEOPM_ERROR_RUNTIME,
                __FILE__,
                __LINE__
            );
        }
    }
}
