/*
 * Copyright (c) 2015 - 2023, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "EndpointUser.hpp"

#include <unistd.h>

#include <fstream>

#include "EndpointImp.hpp"  // for shmem region structs and constants
#include "geopm/Helper.hpp"
#include "Agent.hpp"
#include "Environment.hpp"
#include "geopm/SharedMemory.hpp"

#include "config.h"

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
                          Agent::num_sample(environment().agent()),
                          environment().profile(), "", hosts)
    {

    }

    EndpointUserImp::EndpointUserImp(const std::string &data_path,
                                     std::unique_ptr<SharedMemory> policy_shmem,
                                     std::unique_ptr<SharedMemory> sample_shmem,
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
            m_policy_shmem = SharedMemory::make_unique_user(m_path + EndpointImp::shm_policy_postfix(),
                                                            environment().timeout());
        }
        if (m_sample_shmem == nullptr) {
            m_sample_shmem = SharedMemory::make_unique_user(m_path + EndpointImp::shm_sample_postfix(),
                                                            environment().timeout());
        }
        auto lock = m_sample_shmem->get_scoped_lock();
        auto data = (struct geopm_endpoint_sample_shmem_s *)m_sample_shmem->pointer();
        if (agent_name.size() >= GEOPM_ENDPOINT_AGENT_NAME_MAX) {
            throw Exception("EndpointImp(): Agent name is too long for endpoint storage: " + agent_name,
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        if (profile_name.size() >= GEOPM_ENDPOINT_PROFILE_NAME_MAX) {
            throw Exception("EndpointImp(): Profile name is too long for endpoint storage: " + profile_name,
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        data->agent[GEOPM_ENDPOINT_AGENT_NAME_MAX - 1] = '\0';
        data->profile_name[GEOPM_ENDPOINT_PROFILE_NAME_MAX - 1] = '\0';
        strncpy(data->agent, agent_name.c_str(), GEOPM_ENDPOINT_AGENT_NAME_MAX - 1);
        strncpy(data->profile_name, profile_name.c_str(), GEOPM_ENDPOINT_PROFILE_NAME_MAX - 1);
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
        data->hostlist_path[GEOPM_ENDPOINT_HOSTLIST_PATH_MAX -1] = '\0';
        strncpy(data->hostlist_path, m_hostlist_path.c_str(), GEOPM_ENDPOINT_HOSTLIST_PATH_MAX - 1);
    }

    EndpointUserImp::~EndpointUserImp()
    {
        // detach from shared memory
        auto lock = m_sample_shmem->get_scoped_lock();
        auto data = (struct geopm_endpoint_sample_shmem_s *)m_sample_shmem->pointer();
        data->agent[0] = '\0';
        data->profile_name[0] = '\0';
        data->hostlist_path[0] = '\0';
        unlink(m_hostlist_path.c_str());
    }

    double EndpointUserImp::read_policy(std::vector<double> &policy)
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
        geopm_time_s ts = data->timestamp;
        return geopm_time_since(&ts);
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
