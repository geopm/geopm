/*
 * Copyright (c) 2015 - 2022, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef ENDPOINT_HPP_INCLUDE
#define ENDPOINT_HPP_INCLUDE

#include <cstddef>

#include <memory>
#include <string>
#include <vector>
#include <set>

namespace geopm
{
    class Endpoint
    {
        public:
            virtual ~Endpoint() = default;
            /// @brief Create the shared memory regions belonging to
            ///        the Endpoint.
            virtual void open(void) = 0;
            /// @brief Unlink the shared memory regions belonging to
            ///        the Endpoint.
            virtual void close(void) = 0;
            /// @brief Write a set of policy values for the Agent.
            /// @param [in] policy The policy values.  The order is
            ///        specified by the Agent.
            virtual void write_policy(const std::vector<double> &policy) = 0;
            /// @brief Read a set of samples from the Agent.
            /// @param [out] sample The sample values.  The order is
            ///        specified by the Agent.
            /// @return The age of the sample in seconds.
            virtual double read_sample(std::vector<double> &sample) = 0;
            /// @brief Returns the Agent name, or empty string if no
            ///        Agent is attached.
            virtual std::string get_agent(void) = 0;
            /// @brief Blocks until an agent attaches to the endpoint,
            ///        a timeout is reached, or the operation is
            ///        canceled with stop_wait_loop().  Throws an
            ///        exception if the given timeout is reached
            ///        before an agent attaches.  The name of the
            ///        attached agent can be read with get_agent().
            virtual void wait_for_agent_attach(double timeout) = 0;
            /// @brief Blocks as long as the same agent is still
            ///        attached to the endpoint, a timeout is reached,
            ///        or the operation is canceled with
            ///        stop_wait_loop().  The name of the attached
            ///        agent can be read with get_agent().
            virtual void wait_for_agent_detach(double timeout) = 0;
            /// @brief Cancels any current wait loops in this
            ///        Endpoint.
            virtual void stop_wait_loop(void) = 0;
            /// @brief Re-enables wait loops occurring after this call.
            virtual void reset_wait_loop(void) = 0;
            /// @brief Returns the profile name associated with the
            ///        attached application, or empty if no controller
            ///        is attached.
            virtual std::string get_profile_name(void) = 0;
            /// @brief Returns the list of hostnames used by the
            ///        attached application, or empty if no controller
            ///        is attached.
            virtual std::set<std::string> get_hostnames(void) = 0;
            /// @brief Factory method for the Endpoint used to set the policy.
            static std::unique_ptr<Endpoint> make_unique(const std::string &data_path);
    };
}

#endif
