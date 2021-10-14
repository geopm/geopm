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
#include "POSIXSignal.hpp"

#include <memory>

#include "gtest/gtest.h"
#include "geopm_test.hpp"

using geopm::POSIXSignal;
using geopm::POSIXSignalImp;

class POSIXSignalTest : public :: testing :: Test
{
    public:
        void SetUp(void);
    protected:
        std::shared_ptr<POSIXSignalImp> m_posix_sig;
};

void POSIXSignalTest::SetUp(void)
{
    m_posix_sig = std::make_shared<POSIXSignalImp>();
}

TEST_F(POSIXSignalTest, make_sigset)
{
    sigset_t sigset = m_posix_sig->make_sigset({SIGCONT, SIGSTOP});
    EXPECT_EQ(1, sigismember(&sigset, SIGCONT));
    EXPECT_EQ(1, sigismember(&sigset, SIGSTOP));
    EXPECT_EQ(0, sigismember(&sigset, SIGIO));
    EXPECT_EQ(0, sigismember(&sigset, SIGCHLD));
    GEOPM_EXPECT_THROW_MESSAGE(m_posix_sig->make_sigset({-1}),
                               EINVAL, "invalid signal number");
}

TEST_F(POSIXSignalTest, reduce_info)
{
    siginfo_t siginfo {};
    int expect_signal = SIGCHLD;
    int expect_pid = 1234;
    int expect_value = 4321;

    siginfo.si_signo = expect_signal;
    siginfo.si_pid = expect_pid;
    siginfo.si_value.sival_int = expect_value;

    POSIXSignal::m_info_s info = m_posix_sig->reduce_info(siginfo);

    EXPECT_EQ(expect_signal, info.signo);
    EXPECT_EQ(expect_pid, info.pid);
    EXPECT_EQ(expect_value, info.value);
}
