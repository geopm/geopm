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
        return geopm::make_unique<BatchStatusImp>(client_pid, server_key, true);
    }

    std::unique_ptr<BatchStatus>
    BatchStatus::make_unique_client(const std::string &server_key)
    {
        return geopm::make_unique<BatchStatusImp>(0, server_key, false);
    }

    BatchStatusImp::BatchStatusImp(int other_pid, const std::string &server_key,
                                   bool is_server)
        : BatchStatusImp(other_pid, server_key, is_server,
                         "/tmp/geopm-service-status")
    {

    }

    BatchStatusImp::BatchStatusImp(int other_pid, const std::string &server_key,
                                   bool is_server, const std::string &fifo_prefix)
        : m_fifo_prefix(fifo_prefix)
    {
        std::string read_fifo_path;
        std::string write_fifo_path;
        if (is_server) {
            read_fifo_path = m_fifo_prefix + "-in-" + server_key;
            write_fifo_path = m_fifo_prefix + "-out-" + server_key;
            check_return(mkfifo(read_fifo_path.c_str(), S_IRUSR | S_IWUSR), "mkfifo(3)");
            check_return(mkfifo(write_fifo_path.c_str(), S_IRUSR | S_IWUSR), "mkfifo(3)");
            int uid = pid_to_uid(other_pid);
            int gid = pid_to_gid(other_pid);
            check_return(chown(read_fifo_path.c_str(), uid, gid), "chown(2)");
            check_return(chown(write_fifo_path.c_str(), uid, gid), "chown(2)");
            m_write_fd = open(write_fifo_path.c_str(), O_WRONLY);
            check_return(m_write_fd, "open(2)");
            m_read_fd = open(read_fifo_path.c_str(), O_RDONLY);
            check_return(m_read_fd, "open(2)");
        }
        else {
            read_fifo_path = m_fifo_prefix + "-out-" + server_key;
            write_fifo_path = m_fifo_prefix + "-in-" + server_key;
            m_read_fd = open(read_fifo_path.c_str(), O_RDONLY);
            check_return(m_read_fd, "open(2)");
            m_write_fd = open(write_fifo_path.c_str(), O_WRONLY);
            check_return(m_write_fd, "open(2)");
        }
    }

    BatchStatusImp::~BatchStatusImp()
    {
        check_return(close(m_write_fd), "close(2)");
        check_return(close(m_read_fd), "close(2)");
    }

    void BatchStatusImp::send_message(char msg)
    {
        check_return(write(m_write_fd, &msg, 1), "write(2)");
    }

    char BatchStatusImp::receive_message(void)
    {
        char result = '\0';
        check_return(read(m_read_fd, &result, 1), "read(2)");
        return result;
    }

    void BatchStatusImp::receive_message(char expect)
    {
        char actual = receive_message();
        if (actual != expect) {
            std::ostringstream error_message;
            error_message << "BatchStatusImp::receive_message(): "
                          << "Expected message: \"" << expect
                          << "\" but received \"" <<   actual << "\"";
            throw Exception(error_message.str(),
                            GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
        }
    }

    void BatchStatusImp::check_return(int ret, const std::string &func_name)
    {
        if (ret == -1) {
            throw Exception("BatchStatusImp: System call failed: " + func_name,
                            errno ? errno : GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
        }
    }
}
