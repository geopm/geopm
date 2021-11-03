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

#include <cerrno>

#include "POSIXSignal.hpp"
#include "geopm/Exception.hpp"
#include "geopm/Helper.hpp"

namespace geopm
{

    std::unique_ptr<POSIXSignal> POSIXSignal::make_unique(void)
    {
        return geopm::make_unique<POSIXSignalImp>();
    }

    sigset_t POSIXSignalImp::make_sigset(const std::set<int> &signal_set) const
    {
        sigset_t result;
        int err = sigemptyset(&result);
        check_return(err, "sigemptyset()");
        for (const auto &it : signal_set) {
            int err = sigaddset(&result, it);
            check_return(err, "sigaddset()");
        }
        return result;
    }

    POSIXSignal::m_info_s POSIXSignalImp::reduce_info(const siginfo_t &info) const
    {
        POSIXSignal::m_info_s custom_signal_info = {0};
        custom_signal_info.signo = info.si_signo;
        custom_signal_info.value = info.si_value.sival_int;
        custom_signal_info.pid   = info.si_pid;
        return custom_signal_info;
    }

    int POSIXSignalImp::sig_wait_info(const sigset_t *sigset, siginfo_t *info) const
    {
        int result = sigwaitinfo(sigset, info);
        check_return(result, "sigwaitinfo()");
        return result;
    }

    int POSIXSignalImp::sig_timed_wait(const sigset_t *sigset, siginfo_t *info,
                                       const timespec *timeout) const
    {
        int result = sigtimedwait(sigset, info, timeout);
        check_return(result, "sigtimedwait()");
        return result;
    }

    void POSIXSignalImp::sig_queue(pid_t pid, int sig, int value) const
    {
        union sigval signal_value = {0};
        signal_value.sival_int = value;
        check_return(sigqueue(pid, sig, signal_value), "sigqueue()");
    }

    void POSIXSignalImp::sig_action(int signum, const struct sigaction *act,
                                    struct sigaction *oldact) const
    {
        check_return(sigaction(signum, act, oldact), "sigaction()");
    }

    void POSIXSignalImp::sig_proc_mask(int how, const sigset_t *sigset, sigset_t *oldset) const
    {
        check_return(sigprocmask(how, sigset, oldset), "sigprocmask()");
    }

    void POSIXSignalImp::sig_suspend(const sigset_t *mask) const
    {
        sigsuspend(mask);
        if (errno != EINTR) {
            check_return(-1, "sigsuspend()");
        }
        errno = 0;
    }

    void POSIXSignalImp::check_return(int err, const std::string &func_name) const
    {
        if (err == -1) {
            throw Exception("POSIXSignal(): POSIX signal function call " + func_name +
                            " returned an error", errno, __FILE__, __LINE__);
        }
    }
}
