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
#include "DBusServer.hpp"

#include <signal.h>
#include <unistd.h>

#include "geopm/Exception.hpp"
#include "geopm/PlatformIO.hpp"
#include "geopm/SharedMemory.hpp"
#include "POSIXSignal.hpp"
#include "geopm_internal.h"

namespace geopm
{
    DBusServerImp::DBusServerImp(int client_pid,
                                 const std::vector<geopm_request_s> &signal_config,
                                 const std::vector<geopm_request_s> &control_config)
        : DBusServerImp(client_pid, signal_config, control_config,
                        platform_io(), nullptr, nullptr, nullptr)
    {

    }


    DBusServerImp::DBusServerImp(int client_pid,
                                 const std::vector<geopm_request_s> &signal_config,
                                 const std::vector<geopm_request_s> &control_config,
                                 PlatformIO &pio,
                                 std::shared_ptr<POSIXSignal> posix_signal,
                                 std::shared_ptr<SharedMemory> signal_shmem,
                                 std::shared_ptr<SharedMemory> control_shmem)
        : m_client_pid(client_pid)
        , m_signal_config(signal_config)
        , m_control_config(control_config)
        , m_pio(pio)
        , m_signal_shmem(signal_shmem)
        , m_control_shmem(control_shmem)
        , m_posix_signal(posix_signal)
        , m_server_key(std::to_string(m_client_pid))
        , m_server_pid(0)
        , m_is_active(false)
    {
        if (m_posix_signal == nullptr) {
            // This is not a unit test, so actually do the fork()
            m_posix_signal = POSIXSignal::make_unique();
            int parent_pid = getpid();
            // TODO: Register a handler for SIGCHLD
            int forked_pid = fork();
            if (forked_pid == 0) {
                size_t signal_size = signal_config.size() * sizeof(double);
                size_t control_size = control_config.size() * sizeof(double);
                std::string shmem_prefix = "/geopm-service-" + m_server_key;
                // TODO: Manage ownership
                // int uid = m_posix_signal->pid_to_uid(client_pid);
                // int gid = m_posix_signal->pid_to_gid(client_pid);
                if (signal_size != 0) {
                    m_signal_shmem = SharedMemory::make_unique_owner(
                        shmem_prefix + "-signals", signal_size);
                    // Requires a chown if server is different user than client
                    // m_signal_shmem->chown(gid, uid);
                }
                if (control_size != 0) {
                    m_control_shmem = SharedMemory::make_unique_owner(
                        shmem_prefix + "-controls", control_size);
                    // Requires a chown if server is different user than client
                    // m_control_shmem->chown(gid, uid);
                }
                run_batch(parent_pid);
                exit(0);
            }
            sigset_t sigset = posix_signal->make_sigset({SIGCONT});
            siginfo_t siginfo {};
            timespec timeout = {1, 0};
            try {
                posix_signal->sig_timed_wait(&sigset, &siginfo, &timeout);
            }
            catch (const Exception &ex) {
                throw Exception("DBusServer: Timed out waiting for batch server to start",
                                GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
            }
            m_server_pid = forked_pid;
        }
        m_is_active = true;
    }

    DBusServerImp::~DBusServerImp()
    {
        stop_batch();
    }

    int DBusServerImp::server_pid(void) const
    {
        return m_server_pid;
    }

    std::string DBusServerImp::server_key(void) const
    {
        return m_server_key;
    }

    void DBusServerImp::stop_batch(void)
    {
        if (m_is_active) {
            m_posix_signal->sig_queue(m_server_pid, SIGTERM, 0);
            sigset_t sigset = m_posix_signal->make_sigset({SIGCHLD});
            siginfo_t siginfo {};
            timespec timeout = {1, 0};
            try {
                m_posix_signal->sig_timed_wait(&sigset, &siginfo, &timeout);
            }
            catch (const Exception &ex) {
                throw Exception("DBusServer: Timed out waiting for batch server to stop",
                                GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
            }
            m_is_active = false;
        }
    }

    void DBusServerImp::run_batch(int parent_pid)
    {
        // TODO: Register a handler for SIGTERM SIGIO
        push_requests();
        bool do_read = m_signal_config.size() != 0;
        bool do_write = m_control_config.size() != 0;
        if ((do_read && m_signal_config.size() * sizeof(double) > m_signal_shmem->size()) ||
            (do_write && m_control_config.size() * sizeof(double) > m_control_shmem->size())) {
            throw Exception("DBusServer::run_batch(): Input configuration are too large for shared memory provided",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        // Signal that the server is ready
        sigset_t sigset = m_posix_signal->make_sigset({SIGTERM, SIGIO});
        siginfo_t siginfo {};
        m_posix_signal->sig_queue(parent_pid, SIGCONT, 0);

        // Start event loop
        int signo = 0;
        while (signo != SIGTERM) {
            signo = m_posix_signal->sig_wait_info(&sigset, &siginfo);
            POSIXSignal::m_info_s info = m_posix_signal->reduce_info(siginfo);
            if (signo == SIGIO) {
                if (do_read && info.value == M_VALUE_READ) {
                    read_and_update();
                }
                else if (do_write && info.value == M_VALUE_WRITE) {
                    update_and_write();
                }
                m_posix_signal->sig_queue(m_client_pid, SIGCONT, 0);
            }
        }
    }

    bool DBusServerImp::is_active(void) const
    {
        return m_is_active;
    }

    void DBusServerImp::push_requests(void)
    {
        for (const auto &req : m_signal_config) {
            m_signal_idx.push_back(
                m_pio.push_signal(req.name, req.domain, req.domain_idx));
        }
        for (const auto &req : m_control_config) {
            m_control_idx.push_back(
                m_pio.push_control(req.name, req.domain, req.domain_idx));
        }
    }

    void DBusServerImp::read_and_update(void)
    {
        m_pio.read_batch();
        auto lock = m_signal_shmem->get_scoped_lock();
        double *buffer = (double *)m_signal_shmem->pointer();
        int buffer_idx = 0;
        for (const auto &idx : m_signal_idx) {
            buffer[buffer_idx] = m_pio.sample(idx);
            ++buffer_idx;
        }
    }

    void DBusServerImp::update_and_write(void)
    {
        auto lock = m_control_shmem->get_scoped_lock();
        double *buffer = (double *)m_control_shmem->pointer();
        int buffer_idx = 0;
        for (const auto &idx : m_control_idx) {
            m_pio.adjust(idx, buffer[buffer_idx]);
            ++buffer_idx;
        }
        m_pio.write_batch();
    }
}
