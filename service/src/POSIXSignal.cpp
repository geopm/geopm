/*
 * Copyright (c) 2015 - 2022, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
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
