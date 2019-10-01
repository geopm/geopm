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

#include "geopm_endpoint.h"
#include "EndpointImp.hpp"

#include <cmath>
#include <cstring>
#include <unistd.h>

#include <algorithm>
#include <string>
#include <fstream>

#include "Environment.hpp"
#include "SharedMemory.hpp"
#include "Exception.hpp"
#include "Helper.hpp"
#include "Agent.hpp"
#include "config.h"

namespace geopm
{
    std::unique_ptr<Endpoint> Endpoint::make_unique(const std::string &data_path)
    {
        return geopm::make_unique<EndpointImp>(data_path);
    }

    std::string EndpointImp::shm_policy_postfix(void)
    {
        return "-policy";
    }

    std::string EndpointImp::shm_sample_postfix(void)
    {
        return "-sample";
    }

    EndpointImp::EndpointImp(const std::string &data_path)
        : EndpointImp(data_path, nullptr, nullptr, 0, 0)
    {

    }

    EndpointImp::EndpointImp(const std::string &path,
                             std::unique_ptr<SharedMemory> policy_shmem,
                             std::unique_ptr<SharedMemory> sample_shmem,
                             size_t num_policy,
                             size_t num_sample)
        : m_path(path)
        , m_policy_shmem(std::move(policy_shmem))
        , m_sample_shmem(std::move(sample_shmem))
        , m_num_policy(num_policy)
        , m_num_sample(num_sample)
        , m_is_open(false)
    {

    }

    EndpointImp::~EndpointImp()
    {

    }

    void EndpointImp::open(void)
    {
        if (m_policy_shmem == nullptr) {
            size_t shmem_size = sizeof(struct geopm_endpoint_policy_shmem_s);
            m_policy_shmem = SharedMemory::make_unique(m_path + shm_policy_postfix(), shmem_size);
        }
        if (m_sample_shmem == nullptr) {
            size_t shmem_size = sizeof(struct geopm_endpoint_sample_shmem_s);
            m_sample_shmem = SharedMemory::make_unique(m_path + shm_sample_postfix(), shmem_size);
        }
        auto lock_p = m_policy_shmem->get_scoped_lock();
        struct geopm_endpoint_policy_shmem_s *data_p = (struct geopm_endpoint_policy_shmem_s*)m_policy_shmem->pointer();
        *data_p = {};

        auto lock_s = m_sample_shmem->get_scoped_lock();
        struct geopm_endpoint_sample_shmem_s *data_s = (struct geopm_endpoint_sample_shmem_s*)m_sample_shmem->pointer();
        *data_s = {};
        m_is_open = true;
    }

    void EndpointImp::close(void)
    {
        if (m_policy_shmem) {
            m_policy_shmem->unlink();
        }
        if (m_sample_shmem) {
            m_sample_shmem->unlink();
        }
        m_policy_shmem.reset();
        m_sample_shmem.reset();
        m_is_open = false;
    }

    void EndpointImp::write_policy(const std::vector<double> &policy)
    {
        if (!m_is_open) {
            throw Exception("EndpointImp::" + std::string(__func__) + "(): cannot use shmem before calling open()",
                            GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
        }
        if (policy.size() != m_num_policy) {
            throw Exception("EndpointImp::" + std::string(__func__) + "(): size of policy does not match expected.",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        auto policy_lock = m_policy_shmem->get_scoped_lock();
        auto data = (struct geopm_endpoint_policy_shmem_s *)m_policy_shmem->pointer();
        data->count = policy.size();
        std::copy(policy.begin(), policy.end(), data->values);
        geopm_time(&data->timestamp);
    }

    double EndpointImp::read_sample(std::vector<double> &sample)
    {
        if (!m_is_open) {
            throw Exception("EndpointImp::" + std::string(__func__) + "(): cannot use shmem before calling open()",
                            GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
        }
        if (sample.size() != m_num_sample) {
            throw Exception("EndpointImp::" + std::string(__func__) + "(): output sample vector is incorrect size.",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        auto lock = m_sample_shmem->get_scoped_lock();
        struct geopm_endpoint_sample_shmem_s *data = (struct geopm_endpoint_sample_shmem_s *) m_sample_shmem->pointer(); // Managed by shmem subsystem.

        int num_sample = data->count;
        std::copy(data->values, data->values + data->count, sample.begin());
        geopm_time_s ts = data->timestamp;
        if (sample.size() != (size_t)num_sample) {
            throw Exception("EndpointImpUser::" + std::string(__func__) + "(): Data read from shmem does not match number of samples.",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        return geopm_time_since(&ts);
    }

    std::string EndpointImp::get_agent(void)
    {
        if (!m_is_open) {
            throw Exception("EndpointImp::" + std::string(__func__) + "(): cannot use shmem before calling open()",
                            GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
        }
        auto lock = m_sample_shmem->get_scoped_lock();
        struct geopm_endpoint_sample_shmem_s *data = (struct geopm_endpoint_sample_shmem_s *) m_sample_shmem->pointer(); // Managed by shmem subsystem.

        char agent_name[GEOPM_ENDPOINT_AGENT_NAME_MAX];
        std::copy(data->agent, data->agent + GEOPM_ENDPOINT_AGENT_NAME_MAX, agent_name);
        std::string agent {agent_name};
        if (agent != "") {
            m_num_policy = Agent::num_policy(agent_factory().dictionary(agent_name));
            m_num_sample = Agent::num_sample(agent_factory().dictionary(agent_name));
        }
        return agent;
    }

    std::string EndpointImp::get_profile_name(void)
    {
        if (!m_is_open) {
            throw Exception("EndpointImp::" + std::string(__func__) + "(): cannot use shmem before calling open()",
                            GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
        }
        auto lock = m_sample_shmem->get_scoped_lock();
        struct geopm_endpoint_sample_shmem_s *data = (struct geopm_endpoint_sample_shmem_s *) m_sample_shmem->pointer(); // Managed by shmem subsystem.

        char profile_name[GEOPM_ENDPOINT_PROFILE_NAME_MAX];
        std::copy(data->profile_name, data->profile_name + GEOPM_ENDPOINT_PROFILE_NAME_MAX, profile_name);
        std::string profile {profile_name};
        return profile;
    }

    std::set<std::string> EndpointImp::get_hostnames(void)
    {
        if (!m_is_open) {
            throw Exception("EndpointImp::" + std::string(__func__) + "(): cannot use shmem before calling open()",
                            GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
        }
        auto lock = m_sample_shmem->get_scoped_lock();
        struct geopm_endpoint_sample_shmem_s *data = (struct geopm_endpoint_sample_shmem_s *) m_sample_shmem->pointer(); // Managed by shmem subsystem.

        // check for agent
        char agent_name[GEOPM_ENDPOINT_AGENT_NAME_MAX];
        std::copy(data->agent, data->agent + GEOPM_ENDPOINT_AGENT_NAME_MAX, agent_name);
        std::string agent {agent_name};
        std::set<std::string> result;
        if (agent != "") {
            char hostlist_path[GEOPM_ENDPOINT_HOSTLIST_PATH_MAX];
            std::copy(data->hostlist_path, data->hostlist_path + GEOPM_ENDPOINT_HOSTLIST_PATH_MAX, hostlist_path);
            std::string hostlist = read_file(hostlist_path);
            auto temp = string_split(hostlist, "\n");
            result.insert(temp.begin(), temp.end());
            // remove any blank lines
            result.erase(result.find(""));
        }
        return result;
    }
}

int geopm_endpoint_create(const char *endpoint_name,
                          geopm_endpoint_c **endpoint)
{
    int err = 0;
    try {
        *endpoint = (struct geopm_endpoint_c*)(new geopm::EndpointImp(endpoint_name));
    }
    catch (...) {
        err = geopm::exception_handler(std::current_exception(), true);
    }
    return err;
}

int geopm_endpoint_destroy(struct geopm_endpoint_c *endpoint)
{
    int err = 0;
    try {
        delete (geopm::EndpointImp*)endpoint;
    }
    catch (...) {
        err = geopm::exception_handler(std::current_exception(), true);
    }
    return err;
}

int geopm_endpoint_open(struct geopm_endpoint_c *endpoint)
{
    int err = 0;
    geopm::EndpointImp *end = (geopm::EndpointImp*)endpoint;
    try {
        end->open();
    }
    catch (...) {
        err = geopm::exception_handler(std::current_exception(), true);
    }
    return err;
}

int geopm_endpoint_close(struct geopm_endpoint_c *endpoint)
{
    int err = 0;
    geopm::EndpointImp *end = (geopm::EndpointImp*)endpoint;
    try {
        end->close();
    }
    catch (...) {
        err = geopm::exception_handler(std::current_exception(), true);
    }
    return err;
}

int geopm_endpoint_agent(struct geopm_endpoint_c *endpoint,
                         size_t agent_name_max,
                         char *agent_name)
{
    int err = 0;
    geopm::EndpointImp *end = (geopm::EndpointImp*)endpoint;
    try {
        std::string agent = end->get_agent();
        strncpy(agent_name, agent.c_str(), agent_name_max);
    }
    catch (...) {
        err = geopm::exception_handler(std::current_exception(), true);
    }
    return err;
}

int geopm_endpoint_profile_name(struct geopm_endpoint_c *endpoint,
                                size_t profile_name_max,
                                char *profile_name)
{
    int err = 0;
    geopm::EndpointImp *end = (geopm::EndpointImp*)endpoint;
    try {
        std::string profile = end->get_profile_name();
        strncpy(profile_name, profile.c_str(), profile_name_max);
    }
    catch (...) {
        err = geopm::exception_handler(std::current_exception(), true);
    }
    return err;
}

int geopm_endpoint_num_node(struct geopm_endpoint_c *endpoint,
                            int *num_node)
{
    int err = 0;
    geopm::EndpointImp *end = (geopm::EndpointImp*)endpoint;
    try {
        std::set<std::string> hostlist = end->get_hostnames();
        *num_node = hostlist.size();
    }
    catch (...) {
        err = geopm::exception_handler(std::current_exception(), true);
    }
    return err;
}

int geopm_endpoint_node_name(struct geopm_endpoint_c *endpoint,
                             int node_idx,
                             size_t node_name_max,
                             char *node_name)
{
    int err = 0;
    geopm::EndpointImp *end = (geopm::EndpointImp*)endpoint;
    try {
        std::set<std::string> temp = end->get_hostnames();
        std::vector<std::string> hostlist{temp.begin(), temp.end()};
        if (node_idx >= 0 && (size_t)node_idx < hostlist.size()) {
            strncpy(node_name, hostlist[node_idx].c_str(), node_name_max);
        }
        else {
            err = GEOPM_ERROR_INVALID;
        }
    }
    catch (...) {
        err = geopm::exception_handler(std::current_exception(), true);
    }
    return err;
}

int geopm_endpoint_write_policy(struct geopm_endpoint_c *endpoint,
                                size_t agent_num_policy,
                                const double *policy_array)
{
    int err = 0;
    geopm::EndpointImp *end = (geopm::EndpointImp*)endpoint;
    try {
        std::vector<double> policy(policy_array, policy_array + agent_num_policy);
        end->write_policy(policy);
    }
    catch (...) {
        err = geopm::exception_handler(std::current_exception(), true);
    }
    return err;
}

int geopm_endpoint_read_sample(struct geopm_endpoint_c *endpoint,
                               size_t agent_num_sample,
                               double *sample_array,
                               double *sample_age_sec)
{
    int err = 0;
    geopm::EndpointImp *end = (geopm::EndpointImp*)endpoint;
    try {
        std::vector<double> sample(agent_num_sample);
        *sample_age_sec = end->read_sample(sample);
    }
    catch (...) {
        err = geopm::exception_handler(std::current_exception(), true);
    }
    return err;
}
