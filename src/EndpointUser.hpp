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

#ifndef ENDPOINTUSER_HPP_INCLUDE
#define ENDPOINTUSER_HPP_INCLUDE

#include <cstddef>

#include <vector>
#include <memory>
#include <string>
#include <set>

namespace geopm
{
    class SharedMemoryUser;

    class EndpointUser
    {
        public:
            EndpointUser() = default;
            virtual ~EndpointUser() = default;
            /// @brief Read the latest policy values.  All NAN indicates
            ///        that a policy has not been written yet.
            /// @param [out] policy The policy values read. The order
            ///        is specified by the Agent.
            virtual void read_policy(std::vector<double> &policy) = 0;
            /// @brief Write sample values and update the sample age.
            /// @param [in] sample The values to write.  The order is
            ///        specified by the Agent.
            virtual void write_sample(const std::vector<double> &sample) = 0;
            /// @brief Factory method for the EndpointUser receiving
            ///        the policy.
            static std::unique_ptr<EndpointUser> make_unique(const std::string &policy_path,
                                                             const std::set<std::string> &hosts);
    };

    class ShmemEndpointUser : public EndpointUser
    {
        public:
            ShmemEndpointUser() = delete;
            ShmemEndpointUser(const ShmemEndpointUser &other) = delete;
            ShmemEndpointUser(const std::string &data_path,
                              const std::set<std::string> &hosts);
            ShmemEndpointUser(const std::string &data_path,
                              std::unique_ptr<SharedMemoryUser> policy_shmem,
                              std::unique_ptr<SharedMemoryUser> sample_shmem,
                              const std::string &agent_name,
                              int num_sample,
                              const std::string &profile_name,
                              const std::string &hostlist_path,
                              const std::set<std::string> &hosts);
            virtual ~ShmemEndpointUser();
            void read_policy(std::vector<double> &policy) override;
            void write_sample(const std::vector<double> &sample) override;
        private:
            std::string m_path;
            std::unique_ptr<SharedMemoryUser> m_policy_shmem;
            std::unique_ptr<SharedMemoryUser> m_sample_shmem;
            std::string m_hostlist_path;
            size_t m_num_sample;
    };
}

#endif
