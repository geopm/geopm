/*
 * Copyright (c) 2015 - 2022, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
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

    BatchStatusImp::BatchStatusImp(int read_fd, int write_fd)
        : m_read_fd{read_fd}
        , m_write_fd{write_fd}
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
    BatchStatusServer::BatchStatusServer(int client_pid,
                                         const std::string &server_key)
        : BatchStatusServer(client_pid, server_key, M_DEFAULT_FIFO_PREFIX)
    {
    }

    BatchStatusServer::BatchStatusServer(int client_pid,
                                         const std::string &server_key,
                                         const std::string &fifo_prefix)
        : BatchStatusImp(-1, -1)
        , m_read_fifo_path(fifo_prefix + server_key + "-in")
        , m_write_fifo_path(fifo_prefix + server_key + "-out")
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

        (void)unlink(m_read_fifo_path.c_str());
        (void)unlink(m_write_fifo_path.c_str());
    }

    void BatchStatusServer::open_fifo(void)
    {
        if (m_read_fd == -1 || m_write_fd == -1) {
            m_write_fd = open(m_write_fifo_path.c_str(), O_WRONLY);
            check_return(m_write_fd, "open(2)");
            m_read_fd = open(m_read_fifo_path.c_str(), O_RDONLY);
            check_return(m_read_fd, "open(2)");

            check_return(unlink(m_read_fifo_path.c_str()),  "unlink(2)");
            check_return(unlink(m_write_fifo_path.c_str()), "unlink(2)");
        }
    }

    /**************************************
     * Members of class BatchStatusClient *
     **************************************/

    // The constructor which is called by the client.
    BatchStatusClient::BatchStatusClient(const std::string &server_key)
        : BatchStatusClient(server_key, M_DEFAULT_FIFO_PREFIX)
    {
    }

    BatchStatusClient::BatchStatusClient(const std::string &server_key,
                                         const std::string &fifo_prefix)
        : BatchStatusImp(-1, -1)
        , m_read_fifo_path(fifo_prefix +  server_key + "-out")
        , m_write_fifo_path(fifo_prefix + server_key + "-in" )
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
