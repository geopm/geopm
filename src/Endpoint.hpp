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

#ifndef ENDPOINT_HPP_INCLUDE
#define ENDPOINT_HPP_INCLUDE

#include <string>
#include <memory>
#include <cstddef>
#include <pthread.h>

namespace geopm
{
    class ISharedMemory;
    class ISharedMemoryUser;

    struct geopm_endpoint_shmem_header {
        pthread_mutex_t lock; // 40 bytes
        uint8_t is_updated;   // 1 byte + 7 bytes of padding
        char agent_name[256]; // 64 chars * 4 bytes per char
        size_t count;         // 8 bytes
        double values;        // 8 bytes
    };

    struct geopm_endpoint_shmem_s {
        /// @brief Lock to ensure r/w consistency between GEOPM and the resource endpoint.
        pthread_mutex_t lock;
        /// @brief Enables notification of updates to GEOPM.
        uint8_t is_updated;
        /// @brief Specified the name of the agent that is connected.
        char agent_name[256];
        /// @brief Specifies the size of the following array.
        size_t count;
        /// @brief Holds resource endpoint data.
        double values[(4096 - offsetof(struct geopm_endpoint_shmem_header, values)) / sizeof(double)];
    };

    class IEndpoint
    {
        public:
            IEndpoint() = default;
            virtual ~IEndpoint() = default;
    };

    class Endpoint : public IEndpoint
    {
        public:
            Endpoint(std::string endpoint);
            Endpoint(const Endpoint &other) = delete;
            ~Endpoint();
            void create_shmem(void);
            void destroy_shmem(void);
            void attach_shmem(void);
            static void setup_mutex(pthread_mutex_t &lock);
            std::string agent_name(void);
            int num_node(void);
            std::string node_name(int node_idx);
            int num_policy(void);
            void policy(std::vector<double> policy_array);
            std::vector<double> sample(void);

        private:
            std::unique_ptr<ISharedMemory> m_shmem;
            std::unique_ptr<ISharedMemoryUser> m_shmem_user;
            struct geopm_endpoint_shmem_s *m_data;
    };

}

#endif
