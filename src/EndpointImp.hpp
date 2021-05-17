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

#ifndef ENDPOINTIMP_HPP_INCLUDE
#define ENDPOINTIMP_HPP_INCLUDE

#include <pthread.h>
#include <limits.h>

#include "geopm_endpoint.h"
#include "geopm_time.h"
#include "Endpoint.hpp"

namespace geopm
{
    struct geopm_endpoint_policy_shmem_header {
        geopm_time_s timestamp;   // 16 bytes
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
        /// @brief Time that the memory was last updated.
        geopm_time_s timestamp;
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

    class EndpointImp : public Endpoint
    {
        public:
            EndpointImp() = delete;
            EndpointImp(const EndpointImp &other) = delete;

            EndpointImp(const std::string &data_path);
            EndpointImp(const std::string &data_path,
                        std::shared_ptr<SharedMemory> policy_shmem,
                        std::shared_ptr<SharedMemory> sample_shmem,
                        size_t num_policy,
                        size_t num_sample);
            virtual ~EndpointImp();

            void open(void) override;
            void close(void) override;
            void write_policy(const std::vector<double> &policy) override;
            double read_sample(std::vector<double> &sample) override;
            std::string get_agent(void) override;
            void wait_for_agent_attach(double timeout) override;
            void wait_for_agent_detach(double timeout) override;
            void stop_wait_loop(void) override;
            void reset_wait_loop(void) override;
            std::string get_profile_name(void) override;
            std::set<std::string> get_hostnames(void) override;
            static std::string shm_policy_postfix(void);
            static std::string shm_sample_postfix(void);
        private:
            std::string m_path;
            std::shared_ptr<SharedMemory> m_policy_shmem;
            std::shared_ptr<SharedMemory> m_sample_shmem;
            size_t m_num_policy;
            size_t m_num_sample;
            bool m_is_open;
            volatile bool m_continue_loop;
    };
}

#endif
