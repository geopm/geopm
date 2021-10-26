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
#include "BatchClient.hpp"

#include <signal.h>
#include <unistd.h>

#include "geopm/Helper.hpp"
#include "geopm/SharedMemory.hpp"
#include "geopm/Exception.hpp"
#include "geopm/PlatformIO.hpp"
#include "BatchServer.hpp"
#include "POSIXSignal.hpp"


volatile static sig_atomic_t g_message_ready_count = 0;

static void action_sigcont(int signo, siginfo_t *siginfo, void *context)
{
    if (siginfo->si_value.sival_int == geopm::BatchServer::M_MESSAGE_READY) {
        ++g_message_ready_count;
    }
}


namespace geopm
{
    std::unique_ptr<BatchClient> BatchClient::make_unique(int server_pid,
                                                          const std::string &server_key,
                                                          int num_signal,
                                                          int num_control)
    {
        return geopm::make_unique<BatchClientImp>(server_pid, server_key,
                                                  num_signal, num_control);
    }

    BatchClientImp::BatchClientImp(int server_pid, const std::string &server_key,
                                   int num_signal, int num_control)
        : BatchClientImp(server_pid, num_signal, num_control,
                        POSIXSignal::make_unique(),
                        num_signal == 0 ? nullptr :
                            SharedMemory::make_unique_user("/geopm-service-" + server_key + "-signals", 1.0),
                        num_control == 0 ? nullptr :
                            SharedMemory::make_unique_user("/geopm-service-" + server_key + "-controls", 1.0))
    {

    }


    BatchClientImp::BatchClientImp(int server_pid, int num_signal, int num_control,
                                 std::shared_ptr<POSIXSignal> posix_signal,
                                 std::shared_ptr<SharedMemory> signal_shmem,
                                 std::shared_ptr<SharedMemory> control_shmem)
        : m_server_pid(server_pid)
        , m_num_signal(num_signal)
        , m_num_control(num_control)
        , m_posix_signal(posix_signal)
        , m_signal_shmem(signal_shmem)
        , m_control_shmem(control_shmem)
        , m_block_mask(m_posix_signal->make_sigset({SIGCONT}))
        , m_action_sigcont({})
        , m_is_blocked(false)
    {
        m_action_sigcont.sa_mask = m_block_mask;
        m_action_sigcont.sa_flags = SA_SIGINFO;
        m_action_sigcont.sa_sigaction = &action_sigcont;
        // Make sure the process mask is not blocking our signals
        // initially
        m_posix_signal->sig_proc_mask(SIG_UNBLOCK, &m_block_mask, nullptr);
    }

    std::vector<double> BatchClientImp::read_batch(void)
    {
        if (m_num_signal == 0) {
            return {};
        }
        critical_section_enter();
        m_posix_signal->sig_queue(m_server_pid, SIGCONT, BatchServer::M_MESSAGE_READ);
        while (g_message_ready_count == 0) {
            m_posix_signal->sig_suspend(&m_orig_mask);
        }
        --g_message_ready_count;
        critical_section_exit();
        auto lock = m_signal_shmem->get_scoped_lock();
        double *buffer = (double *)m_signal_shmem->pointer();
        std::vector<double> result(buffer, buffer + m_num_signal);
        return result;
    }

    void BatchClientImp::write_batch(std::vector<double> settings)
    {
        if (m_num_control == 0) {
            return;
        }
        if (settings.size() != (size_t)m_num_control) {
            throw Exception("BatchClientImp::write_batch(): settings vector length does not match the number of configured controls",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        auto lock = m_signal_shmem->get_scoped_lock();
        double *buffer = (double *)m_signal_shmem->pointer();
        std::copy(settings.begin(), settings.end(), buffer);
        critical_section_enter();
        m_posix_signal->sig_queue(m_server_pid, SIGCONT, BatchServer::M_MESSAGE_WRITE);
        while (g_message_ready_count == 0) {
            m_posix_signal->sig_suspend(&m_orig_mask);
        }
        --g_message_ready_count;
        critical_section_exit();
    }

    void BatchClientImp::critical_section_enter(void)
    {
        if (m_is_blocked) {
            return;
        }
        // Block signals for SIGCONT so we may use sigsuspend to wait
        m_posix_signal->sig_proc_mask(SIG_BLOCK, &m_block_mask, &m_orig_mask);
        g_message_ready_count = 0;
        m_posix_signal->sig_action(SIGCONT, &m_action_sigcont, &m_orig_action_sigcont);
        m_is_blocked = true;
    }

    void BatchClientImp::critical_section_exit(void)
    {
        if (!m_is_blocked) {
            return;
        }
        m_posix_signal->sig_action(SIGCONT, &m_orig_action_sigcont, nullptr);
        // Reset blocked signals
        m_posix_signal->sig_proc_mask(SIG_SETMASK, &m_orig_mask, nullptr);
        m_is_blocked = false;
    }
}
