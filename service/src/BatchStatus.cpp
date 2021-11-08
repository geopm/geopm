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
    /********************************
     * Members of class BatchStatus *
     ********************************/

    std::unique_ptr<BatchStatus>
    BatchStatus::make_unique_server(int client_pid,
                                    const std::string &server_key)
    {
        // calling the server constructor
        return geopm::make_unique<BatchStatusServer>(client_pid, server_key);
    }

    std::unique_ptr<BatchStatus>
    BatchStatus::make_unique_client(const std::string &server_key)
    {
        // calling the client constructor
        return geopm::make_unique<BatchStatusClient>(server_key);
    }

    /***********************************
     * Members of class BatchStatusImp *
     ***********************************/

    BatchStatusImp::BatchStatusImp(int m_read_fd, int m_write_fd)
        : BatchStatus{}
        , m_read_fd{m_read_fd}
        , m_write_fd{m_write_fd}
    {

    }

    void BatchStatusImp::send_message(char msg)
    {
        open_fifo();
        check_return(write(m_write_fd, &msg, sizeof(char)), "write(2)");
    }

    char BatchStatusImp::receive_message(void)
    {
        open_fifo();
        char result = '\0';
        check_return(read(m_read_fd, &result, sizeof(char)), "read(2)");
        return result;
    }

    void BatchStatusImp::receive_message(char expect)
    {
        open_fifo();
        char actual = receive_message();
        if (actual != expect) {
            std::ostringstream error_message;
            error_message << "BatchStatusImp::receive_message(): "
                          << "Expected message: \"" << expect
                          << "\" but received \"" <<   actual << "\"";
            throw Exception(error_message.str(), GEOPM_ERROR_RUNTIME,
                            __FILE__, __LINE__);
        }
    }

    void BatchStatusImp::check_return(int ret, const std::string &func_name)
    {
        if (ret == -1) {
            throw Exception("BatchStatusImp: System call failed: " + func_name,
                            errno ? errno : GEOPM_ERROR_RUNTIME,
                            __FILE__, __LINE__);
        }
    }

    /**************************************
     * Members of class BatchStatusServer *
     **************************************/

    // The constructor which is called by the server.
    BatchStatusServer::BatchStatusServer(int client_pid, const std::string &server_key)
        : BatchStatusImp(-1, -1)
        , m_read_fifo_path(M_FIFO_PREFIX + server_key + "-in")
        , m_write_fifo_path(M_FIFO_PREFIX + server_key + "-out")
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

    BatchStatusServer::~BatchStatusServer()
    {
        if (m_read_fd != -1) {
            check_return(close(m_read_fd), "close(2)");
        }
        if (m_write_fd != -1) {
            check_return(close(m_write_fd), "close(2)");
        }

        check_return(unlink(m_read_fifo_path.c_str()),  "unlink(2)");
        check_return(unlink(m_write_fifo_path.c_str()), "unlink(2)");
    }

    void BatchStatusServer::open_fifo(void)
    {
        if (m_read_fd == -1 || m_write_fd == -1) {
            m_write_fd = open(m_write_fifo_path.c_str(), O_WRONLY);
            check_return(m_write_fd, "open(2)");
            m_read_fd = open(m_read_fifo_path.c_str(), O_RDONLY);
            check_return(m_read_fd, "open(2)");
        }
    }

    /**************************************
     * Members of class BatchStatusClient *
     **************************************/

    // The constructor which is called by the client.
    BatchStatusClient::BatchStatusClient(const std::string &server_key)
    : BatchStatusImp(-1, -1)
    , m_read_fifo_path(std::string(M_FIFO_PREFIX) +  server_key + "-out")
    , m_write_fifo_path(std::string(M_FIFO_PREFIX) + server_key + "-in" )
    {
        // Assume that the server itself will make the fifo.
    }

    BatchStatusClient::~BatchStatusClient()
    {
        if (m_write_fd != -1) {
            check_return(close(m_write_fd), "close(2)");
        }
        if (m_read_fd != -1) {
            check_return(close(m_read_fd), "close(2)");
        }
    }

    void BatchStatusClient::open_fifo(void)
    {
        if (m_read_fd == -1 || m_write_fd == -1) {
            m_read_fd = open(m_read_fifo_path.c_str(), O_RDONLY);
            check_return(m_read_fd, "open(2)");
            m_write_fd = open(m_write_fifo_path.c_str(), O_WRONLY);
            check_return(m_write_fd, "open(2)");
        }
    }
}
