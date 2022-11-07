/*
 * Copyright (c) 2015 - 2022, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "config.h"
#include "BatchServer.hpp"

#include <cstdlib>
#include <cerrno>
#include <unistd.h>
#include <wait.h>
#include <iostream>

#include "geopm_error.h"
#include "geopm/Exception.hpp"
#include "geopm/PlatformIO.hpp"
#include "geopm/SharedMemory.hpp"
#include "geopm/Helper.hpp"
#include "geopm/PlatformIO.hpp"
#include "BatchStatus.hpp"
#include "POSIXSignal.hpp"
#include "geopm_debug.hpp"

volatile static sig_atomic_t g_sigterm_count = 0;
volatile static sig_atomic_t g_sigchld_count = 0;
volatile static sig_atomic_t g_sigchld_status = 0;
volatile static sig_atomic_t g_wait_status = 0;

static void action_sigterm(int signo, siginfo_t *siginfo, void *context)
{
    if (siginfo->si_value.sival_int == geopm::BatchStatus::M_MESSAGE_TERMINATE) {
        ++g_sigterm_count;
    }
}

static void action_sigchld(int signo, siginfo_t *siginfo, void *context)
{
    int child_status = 0;
    int child_pid = 0;
    child_pid = wait(&child_status);
    if (child_pid == -1) {
        g_wait_status = errno ? errno : GEOPM_ERROR_RUNTIME;
        g_sigchld_status = GEOPM_ERROR_RUNTIME;
    }
    else if (child_status != 0) {
        g_sigchld_status = child_status;
    }
    ++g_sigchld_count;
}

namespace geopm
{
    std::unique_ptr<BatchServer>
    BatchServer::make_unique(int client_pid,
                             const std::vector<geopm_request_s> &signal_config,
                             const std::vector<geopm_request_s> &control_config)
    {
        return geopm::make_unique<BatchServerImp>(client_pid, signal_config,
                                                  control_config);
    }

    std::string BatchServer::get_signal_shmem_key(
        const std::string &server_key)
    {
        return M_SHMEM_PREFIX + server_key + "-signal";
    }

    std::string BatchServer::get_control_shmem_key(
        const std::string &server_key)
    {
        return M_SHMEM_PREFIX + server_key + "-control";
    }

    BatchServerImp::BatchServerImp(
        int client_pid,
        const std::vector<geopm_request_s> &signal_config,
        const std::vector<geopm_request_s> &control_config)
        : BatchServerImp(client_pid, signal_config, control_config, "", "",
                         platform_io(), nullptr, nullptr, nullptr, nullptr, 0)
    {
        // Fork the server when calling real constructor.
        auto setup = [this]() {
            this->child_register_handler();
            this->create_shmem();
            return BatchStatus::M_MESSAGE_CONTINUE;
        };
        auto run = [this]() {
            this->run_batch();
        };
        parent_register_handler();
        m_server_pid = fork_with_setup(setup, run);
    }

    BatchServerImp::BatchServerImp(
        int client_pid,
        const std::vector<geopm_request_s> &signal_config,
        const std::vector<geopm_request_s> &control_config,
        const std::string &signal_shmem_key,
        const std::string &control_shmem_key,
        PlatformIO &pio,
        std::shared_ptr<BatchStatus> batch_status,
        std::shared_ptr<POSIXSignal> posix_signal,
        std::shared_ptr<SharedMemory> signal_shmem,
        std::shared_ptr<SharedMemory> control_shmem,
        int server_pid)
        : m_client_pid(client_pid)
        , m_server_key(std::to_string(m_client_pid))
        , m_signal_config(signal_config)
        , m_control_config(control_config)
        , m_signal_shmem_key(!signal_shmem_key.empty() ? signal_shmem_key :
                              BatchServer::get_signal_shmem_key(
                                  m_server_key))
        , m_control_shmem_key(!control_shmem_key.empty() ?
                               control_shmem_key :
                               BatchServer::get_control_shmem_key(
                                  m_server_key))
        , m_pio(pio)
        , m_signal_shmem(signal_shmem)
        , m_control_shmem(control_shmem)
        , m_batch_status(batch_status != nullptr ?
                         batch_status : BatchStatus::make_unique_server(m_client_pid,
                                                                        m_server_key))
        , m_posix_signal(posix_signal != nullptr ?
                         posix_signal : POSIXSignal::make_unique())
        , m_server_pid(server_pid)
        , m_is_active(true)
        , m_is_client_attached(false)
        , m_is_client_waiting(false)
    {

    }

    BatchServerImp::~BatchServerImp()
    {
        if (m_server_pid != 0 && m_is_active) {
            try {
                stop_batch();
            }
            catch (const Exception &ex) {
                std::cerr << "Warning: <geopm> BatchServerImp::~BatchServerImp(): Exception thrown in destructor: " <<  ex.what() << "\n";
            }
            catch (...) {
                std::cerr << "Warning: <geopm> BatchServerImp::~BatchServerImp(): Non-GEOPM exception thrown in destructor\n";
            }
        }
        if (m_signal_shmem != nullptr) {
            m_signal_shmem->unlink();
        }

        if (m_control_shmem != nullptr) {
            m_control_shmem->unlink();
        }
    }

    int BatchServerImp::server_pid(void) const
    {
        return m_server_pid;
    }

    std::string BatchServerImp::server_key(void) const
    {
        return m_server_key;
    }

    void BatchServerImp::stop_batch(void)
    {
        if (m_server_pid == 0) {
            throw Exception("BatchServerImp::stop_batch(): must be called from parent process, not child process",
                            GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
        }
        if (is_active()) {
            try {
                m_posix_signal->sig_queue(m_server_pid, SIGTERM, BatchStatus::M_MESSAGE_TERMINATE);
            }
            catch (const Exception &ex) {
                if (ex.err_value() != ESRCH) {
                    throw;
                }
            }
            m_is_active = false;
        }
    }

    char BatchServerImp::read_message(void)
    {
        char in_message = BatchStatus::M_MESSAGE_TERMINATE;
        try {
            in_message = m_batch_status->receive_message();
        }
        catch (const Exception &ex) {
            // If we were not interrupted by SIGTERM with correct errno value rethrow
            if (ex.err_value() != EINTR ||
                g_sigterm_count == 0) {
                throw Exception("BatchServer::" + std::string(__func__) + " The client is unresponsive",
                                GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
            }
        }
        if (!m_is_client_attached) {
            if (m_signal_shmem != nullptr) {
                m_signal_shmem->unlink();
            }

            if (m_control_shmem != nullptr) {
                m_control_shmem->unlink();
            }
            m_is_client_attached = true;
        }
        return in_message;
    }

    void BatchServerImp::write_message(char out_message)
    {
        try {
            m_batch_status->send_message(out_message);
            m_is_client_waiting = false;
        }
        catch (const Exception &ex) {
            // If we were not interrupted by SIGTERM with correct errno value rethrow
            if (ex.err_value() != EINTR ||
                g_sigterm_count == 0) {
                throw Exception("BatchServer::" + std::string(__func__) + " The client is unresponsive",
                                GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
            }
        }
    }

    void BatchServerImp::run_batch(void)
    {
        push_requests();
        try {
            event_loop();
        }
        catch (const Exception &ex) {
            if (m_is_client_waiting) {
                std::cerr << "Warning: <geopm>: " << __FILE__ << ":" << __LINE__
                          << " Batch server was terminated while client was waiting: sending client quit message\n";
                m_batch_status->send_message(BatchStatus::M_MESSAGE_QUIT);
                std::cerr << "Warning: <geopm>: " << __FILE__ << ":" << __LINE__
                          << " Batch server was terminated while client was waiting: client received quit message\n";
                m_is_client_waiting = false;
            }
            else if (std::string(ex.what()).find("Received unknown response from client: 0") != std::string::npos) {
                std::cerr << "Warning: <geopm>: " << __FILE__ << ":" << __LINE__
                          << " Batch client " << m_client_pid << " terminated while server " << getpid() << " was waiting\n";
            }
            throw;
        }
    }

    void BatchServerImp::event_loop(void)
    {
        // Start event loop
        char out_message = BatchStatus::M_MESSAGE_CONTINUE;
        while (out_message == BatchStatus::M_MESSAGE_CONTINUE &&
               g_sigterm_count == 0) {
            char in_message = read_message();
            switch (in_message) {
                case BatchStatus::M_MESSAGE_READ:
                    m_is_client_waiting = true;
                    read_and_update();
                    break;
                case BatchStatus::M_MESSAGE_WRITE:
                    m_is_client_waiting = true;
                    update_and_write();
                    break;
                case BatchStatus::M_MESSAGE_QUIT:
                    m_is_client_waiting = true;
                    out_message = BatchStatus::M_MESSAGE_QUIT;
                    break;
                case BatchStatus::M_MESSAGE_TERMINATE:
                    out_message = BatchStatus::M_MESSAGE_TERMINATE;
                    break;
                default:
                    throw Exception("BatchServerImp::run_batch(): Received unknown response from client: " +
                                    std::to_string(in_message), GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
                    break;
            }
            // If in_message came from client send response
            if (in_message != BatchStatus::M_MESSAGE_TERMINATE) {
                write_message(out_message);
            }
        }
    }

    bool BatchServerImp::is_active(void)
    {
        if (g_sigchld_count != 0) {
            m_is_active = false;
            --g_sigchld_count;
            if (g_wait_status != 0) {
                char err_msg[NAME_MAX];
                geopm_error_message(g_wait_status, err_msg, NAME_MAX);
                std::cerr << "Warning: <geopm> " << __FILE__ << ":" << __LINE__
                          << " :  Received SIGCHLD but wait() failed: "
                          << g_wait_status << " : \"" << err_msg << "\"\n";
                g_wait_status = 0;
                g_sigchld_status = 0;
            }
            else if (g_sigchld_status != 0) {
                std::cerr << "Warning: <geopm> " << __FILE__ << ":" << __LINE__
                          << " :  The batch server child process ended with non-zero status: "
                          << g_sigchld_status << "\n";
                g_sigchld_status = 0;
            }
        }
        return m_is_active;
    }

    void BatchServerImp::push_requests(void)
    {
        for (const auto &req : m_signal_config) {
            m_signal_handle.push_back(
                m_pio.push_signal(req.name, req.domain_type, req.domain_idx));
        }
        for (const auto &req : m_control_config) {
            m_control_handle.push_back(
                m_pio.push_control(req.name, req.domain_type, req.domain_idx));
        }
    }

    void BatchServerImp::read_and_update(void)
    {
        if (m_signal_config.size() == 0) {
            return;
        }

        m_pio.read_batch();
        double *shmem_buffer = (double *)m_signal_shmem->pointer();
        int buffer_idx = 0;
        for (const auto &handle : m_signal_handle) {
            shmem_buffer[buffer_idx] = m_pio.sample(handle);
            ++buffer_idx;
        }
    }

    void BatchServerImp::update_and_write(void)
    {
        if (m_control_config.size() == 0) {
            return;
        }

        double *shmem_buffer = (double *)m_control_shmem->pointer();
        int buffer_idx = 0;
        for (const auto &handle : m_control_handle) {
            m_pio.adjust(handle, shmem_buffer[buffer_idx]);
            ++buffer_idx;
        }
        m_pio.write_batch();
    }

    void BatchServerImp::create_shmem(void)
    {
        // Create shared memory regions
        size_t signal_size  = m_signal_config.size()  * sizeof(double);
        size_t control_size = m_control_config.size() * sizeof(double);
        int uid = pid_to_uid(m_client_pid);
        int gid = pid_to_gid(m_client_pid);
        if (signal_size != 0) {
            m_signal_shmem = SharedMemory::make_unique_owner_secure(
                m_signal_shmem_key, signal_size);
            // Requires a chown if server is different user than client
            m_signal_shmem->chown(uid, gid);
        }
        if (control_size != 0) {
            m_control_shmem = SharedMemory::make_unique_owner_secure(
                m_control_shmem_key, control_size);
            // Requires a chown if server is different user than client
            m_control_shmem->chown(uid, gid);
        }
    }

    void BatchServerImp::child_register_handler(void)
    {
        int signo = SIGTERM;
        g_sigterm_count = 0;
        struct sigaction action = {};
        action.sa_mask = m_posix_signal->make_sigset({signo});
        action.sa_flags = SA_SIGINFO; // Do not set SA_RESTART so read will fail
        action.sa_sigaction = &action_sigterm;
        m_posix_signal->sig_action(signo, &action, nullptr);
    }

    void BatchServerImp::parent_register_handler(void)
    {
        int signo = SIGCHLD;
        g_sigchld_count = 0;
        struct sigaction action = {};
        action.sa_mask = m_posix_signal->make_sigset({signo});
        action.sa_flags = SA_SIGINFO;
        action.sa_sigaction = &action_sigchld;
        m_posix_signal->sig_action(signo, &action, nullptr);
    }

    int BatchServerImp::fork_with_setup(std::function<char(void)> setup,
                                        std::function<void(void)> run)
    {
        int pipe_fd[2];
        check_return(pipe(pipe_fd), "pipe(2)");
        int forked_pid = fork();
        check_return(forked_pid, "fork(2)");
        if (forked_pid == 0) {
            try {
                check_return(close(pipe_fd[0]), "close(2)");
                char msg = setup();  // BatchStatus::M_MESSAGE_CONTINUE
                check_return(write(pipe_fd[1], &msg, 1), "write(2)");
                check_return(close(pipe_fd[1]), "close(2)");
                run();
                m_signal_shmem.reset();
                m_control_shmem.reset();
            }
            catch (const std::runtime_error &ex) {
                std::cerr << "Warning: <geopm>: " << __FILE__ << ":" << __LINE__
                          << " Batch server was terminated with exception: "
                          << ex.what() << "\n";
            }
            catch (...) {
                std::cerr << "Warning: <geopm>: " << __FILE__ << ":" << __LINE__
                          << " Batch server was terminated with unknown exception\n";
            }
            _Exit(0);
        }
        check_return(close(pipe_fd[1]), "close(2)");
        char msg = '\0';
        check_return(read(pipe_fd[0], &msg, 1), "read(2)");
        if (msg != BatchStatus::M_MESSAGE_CONTINUE) {
            throw Exception("BatchServerImp: Receivied unexpected message from batch server at startup: \"" +
                            std::to_string(msg) + "\"", GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
        }
        check_return(close(pipe_fd[0]), "close(2)");
        return forked_pid;
    }

    void BatchServerImp::check_return(int ret, const std::string &func_name) const
    {
        if (ret == -1) {
            throw Exception("BatchServerImp: System call failed: " + func_name,
                            errno ? errno : GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
        }
    }
}
