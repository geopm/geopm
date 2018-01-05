/*
 * Copyright (c) 2015, 2016, 2017, 2018, Intel Corporation
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

#include <stdlib.h>
#include <signal.h>
#include <vector>
#include <string.h>

#include "geopm_signal_handler.h"
#include "geopm_env.h"
#include "Exception.hpp"
#include "config.h"

static volatile sig_atomic_t g_signal_number = -1;
static struct sigaction g_signal_action;

extern "C"
{
    static void geopm_signal_handler(int signum)
    {
        g_signal_number = signum;
    }
}

namespace geopm
{

    class SignalHandler
    {
        public:
            SignalHandler();
            virtual ~SignalHandler();
            void init(void) const;
            void check(void) const;
            void revert(void) const;
        protected:
            /// @brief All signals that terminate the process.
            std::vector<int> m_signals;
            std::vector<struct sigaction> m_old_action;
    };

    SignalHandler::SignalHandler()
        : m_signals({SIGHUP, SIGINT, SIGQUIT, SIGILL,
                     SIGTRAP, SIGABRT, SIGFPE, SIGUSR1,
                     SIGSEGV, SIGUSR2, SIGPIPE, SIGALRM,
                     SIGTERM, SIGCONT, SIGTSTP,
                     SIGTTIN, SIGTTOU})
        , m_old_action(m_signals.size())
    {
        int err;
        g_signal_action.sa_handler = geopm_signal_handler;
        sigemptyset(&g_signal_action.sa_mask);
        g_signal_action.sa_flags = 0;
        auto act_it = m_old_action.begin();
        for (auto sig_it = m_signals.begin(); sig_it != m_signals.end(); ++sig_it, ++act_it) {
            err = sigaction(*sig_it, NULL, &(*act_it));
            if (err) {
                throw Exception("SignalHandler: Could not retrieve original handler", errno ? errno : GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
            }
        }
        init();
    }

    SignalHandler::~SignalHandler()
    {
        // As a singleton, the destructor never gets called.
        revert();
    }

    void SignalHandler::init(void) const
    {
        int err;
        if (g_signal_number == -1) {
            g_signal_number = 0;
            struct sigaction old_action;
            for (auto sig_it = m_signals.begin(); sig_it != m_signals.end(); ++sig_it) {
                err = sigaction(*sig_it, NULL, &old_action);
                if (err) {
                    throw Exception("SignalHandler: Could not retrieve original handler", errno ? errno : GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
                }
                if (old_action.sa_handler != SIG_IGN) {
                    err = sigaction(*sig_it, &g_signal_action, NULL);
                    if (err) {
                        throw Exception("SignalHandler: Could not replace original handler", errno ? errno : GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
                    }
                }
            }
        }
    }

    void SignalHandler::revert(void) const
    {
        int err;
        if (g_signal_number > 0) {
            g_signal_number = -1;
            auto act_it = m_old_action.begin();
            for (auto sig_it = m_signals.begin(); sig_it != m_signals.end(); ++sig_it, ++act_it) {
                err = sigaction(*sig_it, &(*act_it), NULL);
                if (err) {
                    throw Exception("SignalHandler: Could not restore original handler", errno ? errno : GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
                }
            }
        }
    }

    void SignalHandler::check(void) const
    {
        if (g_signal_number > 0) {
            int old_signal = g_signal_number;
            geopm_signal_handler_revert();
            throw SignalException(old_signal);
        }
    }

    static const SignalHandler &signal_handler(void)
    {
        static const SignalHandler instance;
        return instance;
    }

}

extern "C"
{
    void geopm_signal_handler_register(void)
    {
        geopm::signal_handler().init();
    }

    void geopm_signal_handler_check(void)
    {
        geopm::signal_handler().check();
    }

    void geopm_signal_handler_revert(void)
    {
        geopm::signal_handler().revert();
    }
}
