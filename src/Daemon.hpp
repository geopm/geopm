/*
 * Copyright (c) 2015 - 2023, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
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
