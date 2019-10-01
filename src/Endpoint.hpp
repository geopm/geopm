/*
 * Copyright (c) 2015, 2016, 2017, 2018, 2019, Intel Corporation
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
