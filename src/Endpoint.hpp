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
#include <pthread.h>
#include <limits.h>

#include <memory>
#include <string>
#include <vector>
#include <set>

#include "geopm_time.h"
#include "geopm_endpoint.h"

namespace geopm
{
    struct geopm_endpoint_policy_shmem_header {
        size_t count;         // 8 bytes
        double values;        // 8 bytes
    };

    struct geopm_endpoint_sample_shmem_header {
        geopm_time_s timestamp;   // 16 bytes
        char agent[GEOPM_ENDPOINT_AGENT_NAME_MAX]; // 256 bytes
        char profile_name[GEOPM_ENDPOINT_PROFILE_NAME_MAX];   // 256 bytes
        char hostlist_path[GEOPM_ENDPOINT_HOSTLIST_PATH_MAX];  // 512 bytes
        size_t count;             // 8 bytes
        double values;            // 8 bytes
    };

    struct geopm_endpoint_policy_shmem_s {
        /// @brief Specifies the size of the following array.
        size_t count;
        /// @brief Holds resource manager data.
        double values[(4096 - offsetof(struct geopm_endpoint_policy_shmem_header, values)) / sizeof(double)];
    };

    struct geopm_endpoint_sample_shmem_s {
        /// @brief Time that the memory was last updated.
        geopm_time_s timestamp;
        /// @brief Holds the name of the Agent attached, if any.
        char agent[GEOPM_ENDPOINT_AGENT_NAME_MAX];
        /// @brief Holds the profile name associated with the
        ///        attached job.
        char profile_name[GEOPM_ENDPOINT_PROFILE_NAME_MAX];
        /// @brief Path to a file containing the list of hostnames
        ///        in the attached job.
        char hostlist_path[GEOPM_ENDPOINT_HOSTLIST_PATH_MAX];
        /// @brief Specifies the size of the following array.
        size_t count;
        /// @brief Holds resource manager data.
        double values[(4096 - offsetof(struct geopm_endpoint_sample_shmem_header, values)) / sizeof(double)];
    };

    static_assert(sizeof(struct geopm_endpoint_policy_shmem_s) == 4096, "Alignment issue with geopm_endpoint_policy_shmem_s.");
    static_assert(sizeof(struct geopm_endpoint_sample_shmem_s) == 4096, "Alignment issue with geopm_endpoint_sample_shmem_s.");

    class SharedMemory;

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
            /// @return The sample timestamp.
            virtual geopm_time_s read_sample(std::vector<double> &sample) = 0;
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

    class ShmemEndpoint : public Endpoint
    {
        public:
            ShmemEndpoint() = delete;
            ShmemEndpoint(const ShmemEndpoint &other) = delete;

            ShmemEndpoint(const std::string &data_path);
            ShmemEndpoint(const std::string &data_path,
                          std::unique_ptr<SharedMemory> policy_shmem,
                          std::unique_ptr<SharedMemory> sample_shmem,
                          size_t num_policy,
                          size_t num_sample);
            virtual ~ShmemEndpoint();

            void open(void) override;
            void close(void) override;
            void write_policy(const std::vector<double> &policy) override;
            geopm_time_s read_sample(std::vector<double> &sample) override;
            std::string get_agent(void) override;
            std::string get_profile_name(void) override;
            std::set<std::string> get_hostnames(void) override;
        private:
            std::string m_path;
            std::unique_ptr<SharedMemory> m_policy_shmem;
            std::unique_ptr<SharedMemory> m_sample_shmem;
            size_t m_num_policy;
            size_t m_num_sample;
            bool m_is_open;
    };

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
