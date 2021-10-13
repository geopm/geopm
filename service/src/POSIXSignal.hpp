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

#ifndef POSIXSIGNAL_HPP_INCLUDE
#define POSIXSIGNAL_HPP_INCLUDE

#include <signal.h>
#include <memory>
#include <set>

namespace geopm
{
    class POSIXSignal
    {
        public:
            POSIXSignal() = default;
            virtual ~POSIXSignal() = default;
            /// @brief Factory method for POSIXSignal interface
            ///
            /// @return Unique pointer to an implementation of the
            ///         POSIXSignal interface.
            static std::unique_ptr<POSIXSignal> make_unique(void);
            virtual sigset_t make_sigset(const std::set<int> &signal_set) const = 0;
            /// @brief Wrapper around sigwait(2)
            /// @param set [in]
            /// @return Signal that was sent
            virtual int signal_wait(const sigset_t &sigset) const = 0;
    };

    class POSIXSignalImp : public POSIXSignal
    {
        public:
            POSIXSignalImp() = default;
            virtual ~POSIXSignalImp() = default;
            sigset_t make_sigset(const std::set<int> &signal_set) const override;
            int signal_wait(const sigset_t &signal_set) const override;
        private:
            void check_return(int err, const std::string &func_name) const;
    };
}

#endif
