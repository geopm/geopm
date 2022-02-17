/*
 * Copyright (c) 2015 - 2022, Intel Corporation
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

#ifndef DAEMON_HPP_INCLUDE
#define DAEMON_HPP_INCLUDE

#include <memory>

namespace geopm
{
    /// Methods using the Endpoint interface in combination with other
    /// utilities to perform some system admin functions.
    class Daemon
    {
        public:
            virtual ~Daemon() = default;

            /// @brief Looks up a policy in the Daemon's PolicyStore given the attached
            ///        Controller's agent and profile name, and writes it back into the
            ///        policy side of the Daemon's Endpoint.  If no policy is found, an
            ///        error is returned.  If the Controller fails to attach within the
            ///        timeout , or detaches while this function is running, no policy
            ///        is written.
            ///
            /// @param timeout Range of time within which the Controller must attach.
            virtual void update_endpoint_from_policystore(double timeout) = 0;
            /// @brief Exits early from any ongoing wait loops in the Daemon, for example
            ///        in a call to update_endpoint_from_policystore().
            virtual void stop_wait_loop(void) = 0;
            /// @brief Resets the Daemon's endpoint to prepare for a future wait loop.
            virtual void reset_wait_loop(void) = 0;

            /// @param endpoint_name The shared memory prefix for the Endpoint
            ///
            /// @param db_path The path to the PolicyStore
            ///
            /// @return unique_ptr<Daemon> to a concrete DaemonImp object.
            static std::unique_ptr<Daemon> make_unique(const std::string &endpoint_name,
                                                       const std::string &db_path);

    };
}

#endif
