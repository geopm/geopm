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
#include <map>

#include "geopm_time.h"

namespace geopm
{
    struct geopm_endpoint_policy_shmem_header {
        size_t count;         // 8 bytes
        double values;        // 8 bytes
    };

    struct geopm_endpoint_sample_shmem_header {
        geopm_time_s timestamp;   // 8 bytes
        char agent[256];          // 256 bytes
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
        char agent[256];
        /// @brief Specifies the size of the following array.
        size_t count;
        /// @brief Holds resource manager data.
        double values[(4096 - offsetof(struct geopm_endpoint_sample_shmem_header, values)) / sizeof(double)];
    };

    static_assert(sizeof(struct geopm_endpoint_policy_shmem_s) == 4096, "Alignment issue with geopm_endpoint_policy_shmem_s.");
    static_assert(sizeof(struct geopm_endpoint_sample_shmem_s) == 4096, "Alignment issue with geopm_endpoint_sample_shmem_s.");

    class Endpoint
    {
        public:
            Endpoint() = default;
            virtual ~Endpoint() = default;
            /// @brief Write a set of policy values for the Agent.
            /// @param [in] policy The policy values.  The order is
            ///        specified by the Agent.
            virtual void write_policy(const std::vector<double> &policy) = 0;
            /// @brief Read a set of samples from the Agent.
            /// @param [in] sample The sample values.  The order is
            ///        specified by the Agent.
            /// @return The sample timestamp.
            virtual geopm_time_s read_sample(std::vector<double> &sample) = 0;
            /// @brief Returns the Agent name, or empty string if no
            ///        Agent is attached.
            virtual std::string get_agent(void) = 0;
            /// @brief Factory method for the Endpoint writing the
            ///        policy.  If the policy path contains exactly
            ///        one / as the first character, a shared memory
            ///        endpoint will be used.  Otherwise, the policy
            ///        will be loaded from a file.
            /// @param [in] policy_path Location of the policy shmem or file.
            /// @param [in] agent_name Agent that the policy applies to.
            static std::unique_ptr<Endpoint> create_endpoint(const std::string &policy_path,
                                                             const std::string &agent_name);
    };

    class SharedMemory;

    class ShmemEndpoint : public Endpoint
    {
        public:
            ShmemEndpoint() = delete;
            ShmemEndpoint(const ShmemEndpoint &other) = delete;

            ShmemEndpoint(const std::string &data_path, const std::string &agent_name);
            ShmemEndpoint(const std::string &data_path,
                          std::unique_ptr<SharedMemory> policy_shmem,
                          std::unique_ptr<SharedMemory> sample_shmem,
                          size_t num_policy,
                          size_t num_sample);
            ~ShmemEndpoint();

            void write_policy(const std::vector<double> &policy) override;
            geopm_time_s read_sample(std::vector<double> &sample) override;
            std::string get_agent(void) override;
        private:
            std::string m_path;
            std::unique_ptr<SharedMemory> m_policy_shmem;
            std::unique_ptr<SharedMemory> m_sample_shmem;
            size_t m_num_policy;
            size_t m_num_sample;
    };

    class FileEndpoint : public Endpoint
    {
        public:
            FileEndpoint() = delete;
            FileEndpoint(const FileEndpoint &other) = delete;

            FileEndpoint(const std::string &data_path, const std::string &agent_name);
            FileEndpoint(const std::string &data_path,
                         const std::vector<std::string> &policy_names);

            ~FileEndpoint() = default;

            void write_policy(const std::vector<double> &policy) override;
            geopm_time_s read_sample(std::vector<double> &sample) override;
            std::string get_agent(void) override;
        private:
            void write_file(const std::vector<double> &values);

            std::string m_path;
            std::vector<std::string> m_policy_names;
    };

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
            ///        the policy.  If the policy path contains
            ///        exactly one / as the first character, a shared
            ///        memory endpoint will be used.  Otherwise, the
            ///        policy will be loaded from a file.
            static std::unique_ptr<EndpointUser> create_endpoint_user(const std::string &policy_path);
    };

    class SharedMemoryUser;

    class ShmemEndpointUser : public EndpointUser
    {
        public:
            ShmemEndpointUser() = delete;
            ShmemEndpointUser(const ShmemEndpointUser &other) = delete;
            ShmemEndpointUser(const std::string &data_path);
            ShmemEndpointUser(const std::string &data_path,
                              std::unique_ptr<SharedMemoryUser> policy_shmem,
                              std::unique_ptr<SharedMemoryUser> sample_shmem,
                              const std::string &agent_name);
            ~ShmemEndpointUser();

            void read_policy(std::vector<double> &policy) override;
            void write_sample(const std::vector<double> &sample) override;
        private:
            std::string m_path;
            std::unique_ptr<SharedMemoryUser> m_policy_shmem;
            std::unique_ptr<SharedMemoryUser> m_sample_shmem;
            size_t m_num_policy;
            size_t m_num_sample;
    };

    class FileEndpointUser : public EndpointUser
    {
        public:
            FileEndpointUser() = delete;
            FileEndpointUser(const FileEndpointUser &other) = delete;
            FileEndpointUser(const std::string &data_path);
            FileEndpointUser(const std::string &data_path,
                             const std::vector<std::string> &policy_names);
            ~FileEndpointUser() = default;
            void read_policy(std::vector<double> &policy) override;
            void write_sample(const std::vector<double> &sample) override;
        private:
            std::map<std::string, double> parse_json(void);

            std::string m_path;
            std::vector<std::string> m_policy_names;
    };
}

#endif
