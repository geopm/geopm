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

#include "EndpointUser.hpp"

#include <unistd.h>

#include <fstream>

#include "EndpointImp.hpp"  // for shmem region structs and constants
#include "Helper.hpp"
#include "Agent.hpp"
#include "Environment.hpp"
#include "SharedMemoryUser.hpp"

namespace geopm
{
    std::unique_ptr<EndpointUser> EndpointUser::make_unique(const std::string &policy_path,
                                                            const std::set<std::string> &hosts)
    {
        return geopm::make_unique<EndpointUserImp>(policy_path, hosts);
    }

    EndpointUserImp::EndpointUserImp(const std::string &data_path,
                                     const std::set<std::string> &hosts)
        : EndpointUserImp(data_path, nullptr, nullptr, environment().agent(),
                          Agent::num_sample(agent_factory().dictionary(environment().agent())),
                          environment().profile(), "", hosts)
    {

    }

    EndpointUserImp::EndpointUserImp(const std::string &data_path,
                                     std::unique_ptr<SharedMemoryUser> policy_shmem,
                                     std::unique_ptr<SharedMemoryUser> sample_shmem,
                                     const std::string &agent_name,
                                     int num_sample,
                                     const std::string &profile_name,
                                     const std::string &hostlist_path,
                                     const std::set<std::string> &hosts)
        : m_path(data_path)
        , m_policy_shmem(std::move(policy_shmem))
        , m_sample_shmem(std::move(sample_shmem))
        , m_num_sample(num_sample)
    {
        // Attach to shared memory here and send across agent,
        // profile, hostname list.  Once user attaches to sample
        // shmem, RM knows it has attached to both policy and sample.
        if (m_policy_shmem == nullptr) {
            m_policy_shmem = SharedMemoryUser::make_unique(m_path + EndpointImp::shm_policy_postfix(),
                                                           environment().timeout());
        }
        if (m_sample_shmem == nullptr) {
            m_sample_shmem = SharedMemoryUser::make_unique(m_path + EndpointImp::shm_sample_postfix(),
                                                           environment().timeout());
        }
        auto lock = m_sample_shmem->get_scoped_lock();
        auto data = (struct geopm_endpoint_sample_shmem_s *)m_sample_shmem->pointer();
        strncpy(data->agent, agent_name.c_str(), GEOPM_ENDPOINT_AGENT_NAME_MAX);
        strncpy(data->profile_name, profile_name.c_str(), GEOPM_ENDPOINT_PROFILE_NAME_MAX);
        /// write hostnames to file
        m_hostlist_path = hostlist_path;
        if (m_hostlist_path == "") {
            char temp_path[NAME_MAX] = "/tmp/geopm_hostlist_XXXXXX";
            int hostlist_fd = mkstemp(temp_path);
            if (hostlist_fd == -1) {
                throw Exception("Failed to create temporary file for endpoint hostlist.",
                                GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
            }
            close(hostlist_fd);
            m_hostlist_path = std::string(temp_path);
        }
        std::ofstream outfile(m_hostlist_path);
        for (const auto &host : hosts) {
            outfile << host << "\n";
        }
        strncpy(data->hostlist_path, m_hostlist_path.c_str(), GEOPM_ENDPOINT_HOSTLIST_PATH_MAX);
    }

    EndpointUserImp::~EndpointUserImp()
    {
        // detach from shared memory
        auto lock = m_sample_shmem->get_scoped_lock();
        auto data = (struct geopm_endpoint_sample_shmem_s *)m_sample_shmem->pointer();
        strncpy(data->agent, "", GEOPM_ENDPOINT_AGENT_NAME_MAX);
        strncpy(data->profile_name, "", GEOPM_ENDPOINT_PROFILE_NAME_MAX);
        strncpy(data->hostlist_path, "", GEOPM_ENDPOINT_HOSTLIST_PATH_MAX);
        unlink(m_hostlist_path.c_str());
    }

    void EndpointUserImp::read_policy(std::vector<double> &policy)
    {
        auto lock = m_policy_shmem->get_scoped_lock();
        auto data = (struct geopm_endpoint_policy_shmem_s *) m_policy_shmem->pointer(); // Managed by shmem subsystem.

        int num_policy = data->count;
        if (policy.size() < (size_t)num_policy) {
            throw Exception("EndpointUserImp::" + std::string(__func__) + "(): Data read from shmem does not fit in policy vector.",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        // Fill in missing policy values with NAN (default)
        std::fill(policy.begin(), policy.end(), NAN);
        std::copy(data->values, data->values + data->count, policy.begin());
    }

    void EndpointUserImp::write_sample(const std::vector<double> &sample)
    {
        if (sample.size() != m_num_sample) {
            throw Exception("ShmemEndpoint::" + std::string(__func__) + "(): size of sample does not match expected.",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        auto lock = m_sample_shmem->get_scoped_lock();
        auto data = (struct geopm_endpoint_sample_shmem_s *)m_sample_shmem->pointer();
        data->count = sample.size();
        std::copy(sample.begin(), sample.end(), data->values);
        // also update timestamp
        geopm_time(&data->timestamp);
    }
}
