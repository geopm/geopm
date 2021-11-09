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
