/*
 * Copyright (c) 2015 - 2024, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef MOCKSPOSIXSIGNAL_HPP_INCLUDE
#define MOCKSPOSIXSIGNAL_HPP_INCLUDE

#include "gmock/gmock.h"

#include "POSIXSignal.hpp"
#include "signal.h"


class MockPOSIXSignal : public geopm::POSIXSignal {
    public:
        MOCK_METHOD(sigset_t, make_sigset, (const std::set<int> &signal_set), (const, override));
        MOCK_METHOD(geopm::POSIXSignal::m_info_s, reduce_info, (const siginfo_t &info), (const, override));
        MOCK_METHOD(int, sig_wait_info, (const sigset_t *sigset, siginfo_t *info), (const, override));
        MOCK_METHOD(int, sig_timed_wait, (const sigset_t *sigset, siginfo_t *info, const struct timespec *timeout), (const, override));
        MOCK_METHOD(void, sig_queue, (pid_t pid, int sig, int value), (const, override));
        MOCK_METHOD(void, sig_action, (int signum, const struct sigaction *act, struct sigaction *oldact), (const, override));
        MOCK_METHOD(void, sig_proc_mask, (int how, const sigset_t *sigset, sigset_t *oldset), (const, override));
        MOCK_METHOD(void, sig_suspend, (const sigset_t *mask), (const, override));
};

#endif
