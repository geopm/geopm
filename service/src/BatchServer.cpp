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

#include <cstdlib>
#include <string>
#include <sstream>
#include <signal.h>
#include <unistd.h>

#include "geopm/Exception.hpp"
#include "geopm/PlatformIO.hpp"
#include "geopm/SharedMemory.hpp"
#include "geopm/Helper.hpp"
#include "geopm/PlatformIO.hpp"
#include "POSIXSignal.hpp"

volatile static sig_atomic_t g_message_read_count = 0;
volatile static sig_atomic_t g_message_write_count = 0;
volatile static sig_atomic_t g_message_ready_count = 0;
volatile static sig_atomic_t g_message_quit_count = 0;
volatile static sig_atomic_t g_message_child_count = 0;

static void action_sigcont(int signo, siginfo_t *siginfo, void *context)
{
    switch (siginfo->si_value.sival_int) {
        case geopm::BatchServer::M_MESSAGE_READ:
            ++g_message_read_count;
            break;
        case geopm::BatchServer::M_MESSAGE_WRITE:
            ++g_message_write_count;
            break;
        case geopm::BatchServer::M_MESSAGE_READY:
            ++g_message_ready_count;
            break;
        case geopm::BatchServer::M_MESSAGE_QUIT:
            ++g_message_quit_count;
            break;
        default:
            break;
    }
}

static void action_sigchld(int signo, siginfo_t *siginfo, void *context)
{
    ++g_message_child_count;
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
    BatchServerImp::BatchServerImp(int client_pid,
                                   const std::vector<geopm_request_s> &signal_config,
                                   const std::vector<geopm_request_s> &control_config)
        : BatchServerImp(client_pid, signal_config, control_config,
                        platform_io(), nullptr, nullptr, nullptr)
    {

    }

    BatchServerImp::BatchServerImp(int client_pid,
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
        , m_is_blocked(false)
    {
        bool is_test = (m_posix_signal != nullptr);
        if (!is_test) {
            m_posix_signal = POSIXSignal::make_unique();
        }
        m_block_mask = m_posix_signal->make_sigset({SIGCONT,
                                                    SIGCHLD});
        if (!is_test) {
            // This is not a unit test, so actually do the fork()

            // Make sure the process mask is not blocking our signals
            // initially
            m_posix_signal->sig_proc_mask(SIG_UNBLOCK, &m_block_mask, nullptr);
            critical_region_enter();
            int parent_pid = getpid();
            int forked_pid = fork();
            if (forked_pid == 0) {
                create_shmem();
                int client_uid = pid_to_uid(client_pid);
                int client_gid = pid_to_gid(client_pid);
                int err = setgid(client_gid);
                if (err == -1) {
                    throw Exception("BatchServerImp() Could not call setgid()",
                                    errno ? errno : GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
                }
                err = setuid(client_uid);
                if (err == -1) {
                    throw Exception("BatchServerImp() Could not call setuid()",
                                    errno ? errno : GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
                }
                m_posix_signal->sig_queue(parent_pid, SIGCONT, M_MESSAGE_READY);
                run_batch(parent_pid);
                critical_region_exit();
                exit(0);
            }
            while (g_message_ready_count == 0) {
                m_posix_signal->sig_suspend(&m_orig_mask);
            }
            --g_message_ready_count;
            critical_region_exit();
            m_server_pid = forked_pid;
        }
        m_is_active = true;
    }

    BatchServerImp::~BatchServerImp()
    {
        stop_batch();
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
        if (m_is_active) {
            critical_region_enter();
            m_posix_signal->sig_queue(m_server_pid, SIGCONT, M_MESSAGE_QUIT);
            while (g_message_child_count == 0) {
                m_posix_signal->sig_suspend(&m_orig_mask);
            }
            --g_message_child_count;
            critical_region_exit();
            m_is_active = false;
        }
    }

    void BatchServerImp::run_batch(int parent_pid)
    {
        push_requests();
        // Start event loop
        while (g_message_quit_count == 0) {
            m_posix_signal->sig_suspend(&m_orig_mask);
            int num_cont = 0;
            if (g_message_read_count != 0) {
                read_and_update();
                ++num_cont;
                --g_message_read_count;
            }
            if (g_message_write_count != 0) {
                update_and_write();
                ++num_cont;
                --g_message_write_count;
            }
            for (; num_cont != 0; --num_cont) {
                m_posix_signal->sig_queue(m_client_pid, SIGCONT, M_MESSAGE_READY);
            }
        }
    }

    bool BatchServerImp::is_active(void) const
    {
        return m_is_active;
    }

    void BatchServerImp::push_requests(void)
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

    void BatchServerImp::read_and_update(void)
    {
        if (m_signal_config.size() == 0) {
            return;
        }

        m_pio.read_batch();
        auto lock = m_signal_shmem->get_scoped_lock();
        double *buffer = (double *)m_signal_shmem->pointer();
        int buffer_idx = 0;
        for (const auto &idx : m_signal_idx) {
            buffer[buffer_idx] = m_pio.sample(idx);
            ++buffer_idx;
        }
    }

    void BatchServerImp::update_and_write(void)
    {
        if (m_control_config.size() == 0) {
            return;
        }

        auto lock = m_control_shmem->get_scoped_lock();
        double *buffer = (double *)m_control_shmem->pointer();
        int buffer_idx = 0;
        for (const auto &idx : m_control_idx) {
            m_pio.adjust(idx, buffer[buffer_idx]);
            ++buffer_idx;
        }
        m_pio.write_batch();
    }


    void BatchServerImp::create_shmem(void)
    {
        // Create shared memory regions
        size_t signal_size = m_signal_config.size() * sizeof(double);
        size_t control_size = m_control_config.size() * sizeof(double);
        std::string shmem_prefix = "/geopm-service-" + m_server_key;
        int uid = pid_to_uid(m_client_pid);
        int gid = pid_to_gid(m_client_pid);
        if (signal_size != 0) {
            m_signal_shmem = SharedMemory::make_unique_owner_secure(
                shmem_prefix + "-signals", signal_size);
            // Requires a chown if server is different user than client
            m_signal_shmem->chown(gid, uid);
        }
        if (control_size != 0) {
            m_control_shmem = SharedMemory::make_unique_owner_secure(
                shmem_prefix + "-controls", control_size);
            // Requires a chown if server is different user than client
            m_control_shmem->chown(gid, uid);
        }
    }

    void BatchServerImp::critical_region_enter(void)
    {
        if (m_is_blocked) {
            return;
        }
        m_posix_signal->sig_proc_mask(SIG_BLOCK, &m_block_mask, &m_orig_mask);
        g_message_read_count = 0;
        g_message_write_count = 0;
        g_message_ready_count = 0;
        g_message_quit_count = 0;
        g_message_child_count = 0;
        struct sigaction action = {};
        action.sa_mask = m_block_mask;
        action.sa_flags = SA_SIGINFO;
        action.sa_sigaction = &action_sigcont;
        m_posix_signal->sig_action(SIGCONT, &action, &m_orig_action_sigcont);
        action.sa_sigaction = &action_sigchld;
        m_posix_signal->sig_action(SIGCHLD, &action, &m_orig_action_sigchld);
        m_is_blocked = true;
    }

    void BatchServerImp::critical_region_exit(void)
    {
        if (!m_is_blocked) {
            return;
        }
        m_posix_signal->sig_action(SIGCONT, &m_orig_action_sigcont, nullptr);
        m_posix_signal->sig_action(SIGCHLD, &m_orig_action_sigchld, nullptr);
        m_posix_signal->sig_proc_mask(SIG_SETMASK, &m_orig_mask, nullptr);
        m_is_blocked = false;
    }
}
