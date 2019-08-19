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

#include "Endpoint.hpp"

#include <cmath>
#include <string.h>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <string>

#include "contrib/json11/json11.hpp"

#include "Environment.hpp"
#include "PlatformTopo.hpp"
#include "SharedMemory.hpp"
#include "SharedMemoryUser.hpp"
#include "Exception.hpp"
#include "Helper.hpp"
#include "Agent.hpp"
#include "config.h"

using json11::Json;

namespace geopm
{
    const std::string SHM_POLICY_POSTFIX = "-policy";
    const std::string SHM_SAMPLE_POSTFIX = "-sample";

    ShmemEndpoint::ShmemEndpoint(const std::string &data_path)
        : ShmemEndpoint(data_path, nullptr, nullptr, 0, 0)
    {

    }

    ShmemEndpoint::ShmemEndpoint(const std::string &path,
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

    ShmemEndpoint::~ShmemEndpoint()
    {

    }

    void ShmemEndpoint::open(void)
    {
        if (m_policy_shmem == nullptr) {
            size_t shmem_size = sizeof(struct geopm_endpoint_policy_shmem_s);
            m_policy_shmem = SharedMemory::make_unique(m_path + SHM_POLICY_POSTFIX, shmem_size);
        }
        if (m_sample_shmem == nullptr) {
            size_t shmem_size = sizeof(struct geopm_endpoint_sample_shmem_s);
            m_sample_shmem = SharedMemory::make_unique(m_path + SHM_SAMPLE_POSTFIX, shmem_size);
        }
        {
            auto lock = m_policy_shmem->get_scoped_lock();
            struct geopm_endpoint_policy_shmem_s *data = (struct geopm_endpoint_policy_shmem_s*)m_policy_shmem->pointer();
            *data = {};
        }
        {
            auto lock = m_sample_shmem->get_scoped_lock();
            struct geopm_endpoint_sample_shmem_s *data = (struct geopm_endpoint_sample_shmem_s*)m_sample_shmem->pointer();
            *data = {};
        }
        m_is_open = true;
    }

    void ShmemEndpoint::close(void)
    {
        m_sample_shmem->unlink();
        m_policy_shmem->unlink();
        m_sample_shmem.reset();
        m_policy_shmem.reset();
        m_is_open = false;
    }

    ShmemEndpoint::~ShmemEndpoint()
    {
        if (m_shmem) {
            m_shmem->unlink();
        }
    }

    FileEndpoint::FileEndpoint(const std::string &data_path, const std::string &agent_name)
        : FileEndpoint(data_path,
                       Agent::policy_names(agent_factory().dictionary(agent_name)))
    {

    }

    FileEndpoint::FileEndpoint(const std::string &path,
                               const std::vector<std::string> &policy_names)
        : m_path(path)
        , m_policy_names(policy_names)
    {

    }

    void FileEndpoint::open(void)
    {

    }

    void FileEndpoint::close(void)
    {

    }

    void ShmemEndpoint::write_policy(const std::vector<double> &policy)
    {
        if (policy.size() != m_num_policy) {
            throw Exception("ShmemEndpoint::" + std::string(__func__) + "(): size of policy does not match expected.",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        auto lock = m_policy_shmem->get_scoped_lock();
        auto data = (struct geopm_endpoint_policy_shmem_s *)m_policy_shmem->pointer();
        data->count = policy.size();
        std::copy(policy.begin(), policy.end(), data->values);
    }

    void FileEndpoint::write_policy(const std::vector<double> &policy)
    {
        if (policy.size() != m_policy_names.size()) {
            throw Exception("FileEndpoint::" + std::string(__func__) + "(): size of policy does not match policy names.",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        std::map<std::string, double> policy_value_map;
        for(size_t i = 0; i < m_policy_names.size(); ++i) {
            policy_value_map[m_policy_names[i]] = policy[i];
        }
        Json root (policy_value_map);
        geopm::write_file(m_path, root.dump());
    }

    geopm_time_s ShmemEndpoint::read_sample(std::vector<double> &sample)
    {
        if (sample.size() != m_num_sample) {
            throw Exception("ShmemEndpoint::" + std::string(__func__) + "(): output sample vector is incorrect size.",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        auto lock = m_sample_shmem->get_scoped_lock();
        struct geopm_endpoint_sample_shmem_s *data = (struct geopm_endpoint_sample_shmem_s *) m_sample_shmem->pointer(); // Managed by shmem subsystem.

        // Fill in missing sample values with NAN (default)
        int num_sample = data->count;
        sample = std::vector<double>(num_sample, NAN);
        std::copy(data->values, data->values + data->count, sample.begin());
        geopm_time_s result = data->timestamp;

        if (sample.size() != (size_t)num_sample) {
            throw Exception("ShmemEndpointUser::" + std::string(__func__) + "(): Data read from shmem does not match number of samples.",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        return result;
    }

    std::string ShmemEndpoint::get_agent(void)
    {
        auto lock = m_sample_shmem->get_scoped_lock();
        struct geopm_endpoint_sample_shmem_s *data = (struct geopm_endpoint_sample_shmem_s *) m_sample_shmem->pointer(); // Managed by shmem subsystem.

        char agent_name[NAME_MAX];
        std::copy(data->agent, data->agent + NAME_MAX, agent_name);
        std::string agent {agent_name};
        if (agent != "") {
            m_num_policy = Agent::num_policy(agent_factory().dictionary(agent_name));
            m_num_sample = Agent::num_sample(agent_factory().dictionary(agent_name));
        }
        return agent;
    }

    geopm_time_s FileEndpoint::read_sample(std::vector<double> &sample)
    {
        throw Exception("FileEndpoint::" + std::string(__func__) + "(): sending samples via file not yet supported",
                        GEOPM_ERROR_NOT_IMPLEMENTED, __FILE__, __LINE__);
        return {};
    }

    std::string FileEndpoint::get_agent(void)
    {
        throw Exception("FileEndpoint::" + std::string(__func__) + "(): get_agent via file not yet supported",
                        GEOPM_ERROR_NOT_IMPLEMENTED, __FILE__, __LINE__);
        return "";
    }

    /*********************************************************************************************************/

    ShmemEndpointUser::ShmemEndpointUser(const std::string &data_path, const std::string &agent_name)
        : ShmemEndpointUser(data_path, nullptr, nullptr, agent_name,
                            Agent::num_sample(agent_factory().dictionary(agent_name)))
    {

    }

    ShmemEndpointUser::ShmemEndpointUser(const std::string &path,
                                         std::unique_ptr<SharedMemoryUser> policy_shmem,
                                         std::unique_ptr<SharedMemoryUser> sample_shmem,
                                         const std::string &agent,
                                         size_t num_sample)
        : m_path(path)
        , m_policy_shmem(std::move(policy_shmem))
        , m_sample_shmem(std::move(sample_shmem))
        , m_num_sample(num_sample)

    {
        // attach to shared memory here and write agent
        /// @todo: use cases for separate attach method()?
        // Attach to policy shmem first; no agent will be written.  Once user attaches
        // to sample shmem, RM knows it has attached to both.
        if (m_policy_shmem == nullptr) {
            m_policy_shmem = SharedMemoryUser::make_unique(m_path + SHM_POLICY_POSTFIX,
                                                           environment().timeout());
        }
        if (m_sample_shmem == nullptr) {
            m_sample_shmem = SharedMemoryUser::make_unique(m_path + SHM_SAMPLE_POSTFIX,
                                                           environment().timeout());
        }
        auto lock = m_sample_shmem->get_scoped_lock();
        auto data = (struct geopm_endpoint_sample_shmem_s *)m_sample_shmem->pointer();
        strncpy(data->agent, agent.c_str(), NAME_MAX);

        // todo moveme
        m_num_policy = Agent::num_policy(agent_factory().dictionary(agent));
    }

    ShmemEndpointUser::~ShmemEndpointUser()
    {
        // detach from shared memory
        /// @todo: need for explict detach()?
        auto lock = m_sample_shmem->get_scoped_lock();
        auto data = (struct geopm_endpoint_sample_shmem_s *)m_sample_shmem->pointer();
        strncpy(data->agent, "", NAME_MAX);
    }

    FileEndpointUser::FileEndpointUser(const std::string &data_path, const std::string &agent_name)
        : FileEndpointUser(data_path,
                           Agent::policy_names(agent_factory().dictionary(agent_name)))
    {

    }

    FileEndpointUser::FileEndpointUser(const std::string &path,
                                       const std::vector<std::string> &policy_names)
        : m_path(path)
        , m_policy_names(policy_names)
    {

    }

    std::map<std::string, double> FileEndpointUser::parse_json(void)
    {
        std::map<std::string, double> policy_value_map;
        std::string json_str;

        json_str = read_file(m_path);

        // Begin JSON parse
        std::string err;
        Json root = Json::parse(json_str, err);
        if (!err.empty() || !root.is_object()) {
            throw Exception("FileEndpointUser::" + std::string(__func__) + "(): detected a malformed json config file: " + err,
                            GEOPM_ERROR_FILE_PARSE, __FILE__, __LINE__);
        }

        for (const auto &obj : root.object_items()) {
            if (obj.second.type() == Json::NUMBER) {
                policy_value_map.emplace(obj.first, obj.second.number_value());
            }
            else if (obj.second.type() == Json::STRING) {
                std::string tmp_val = obj.second.string_value();
                if (tmp_val.compare("NAN") == 0 || tmp_val.compare("NaN") == 0 || tmp_val.compare("nan") == 0) {
                    policy_value_map.emplace(obj.first, NAN);
                }
                else {
                    throw Exception("FileEndpointUser::" + std::string(__func__)  + ": unsupported type or malformed json config file",
                                    GEOPM_ERROR_FILE_PARSE, __FILE__, __LINE__);
                }
            }
            else {
                throw Exception("FileEndpointUser::" + std::string(__func__)  + ": unsupported type or malformed json config file",
                                GEOPM_ERROR_FILE_PARSE, __FILE__, __LINE__);
            }
        }

        return policy_value_map;
    }

    void ShmemEndpointUser::read_policy(std::vector<double> &policy)
    {
        if (policy.size() != m_num_policy) {
            throw Exception("ShmemEndpointUser::" + std::string(__func__) + "(): output policy vector is incorrect size.",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        auto lock = m_policy_shmem->get_scoped_lock();
        auto data = (struct geopm_endpoint_policy_shmem_s *) m_policy_shmem->pointer(); // Managed by shmem subsystem.

        if (data->count == policy.size()) {
            // Fill in missing policy values with NAN (default)
            // TODO: this doesn't make sense
            std::fill(policy.begin(), policy.end(), NAN);
            std::copy(data->values, data->values + data->count, policy.begin());
        }
        else if (data->count != 0) {
            // wrong size policy was written
            throw Exception("ShmemEndpointUser::" + std::string(__func__) + "(): Data read from shmem does not match number of policies.",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        // else empty policy indicates no policy written yet
    }

    void FileEndpointUser::read_policy(std::vector<double> &policy)
    {
        if (policy.size() != m_policy_names.size()) {
            throw Exception("FileEndpointUser::" + std::string(__func__) + "(): output policy vector is incorrect size.",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        if (m_policy_names.size() > 0) {
            std::map<std::string, double> policy_value_map = parse_json();
            policy.clear();
            for (auto name : m_policy_names) {
                auto it = policy_value_map.find(name);
                if (it != policy_value_map.end()) {
                    policy.emplace_back(policy_value_map.at(name));
                }
                else {
                    // Fill in missing policy values with NAN (default)
                    policy.emplace_back(NAN);
                }
            }
        }
    }

    void ShmemEndpointUser::write_sample(const std::vector<double> &sample)
    {
        // @todo: timeout is not an error; just throw out the sample
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

    void FileEndpointUser::write_sample(const std::vector<double> &sample)
    {
        // Does nothing, but should not throw when Controller is using a
        // FileEndpointUser to get the policy
    }

    geopm_time_s ShmemEndpointUser::read_sample(std::vector<double> &sample)
    {
        if (sample.size() != m_num_sample) {
            throw Exception("ShmemEndpointUser::" + std::string(__func__) + "(): output sample vector is incorrect size.",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        auto lock = m_sample_shmem->get_scoped_lock();
        auto data = (struct geopm_endpoint_sample_shmem_s *) m_sample_shmem->pointer(); // Managed by shmem subsystem.

        if (data->count == sample.size()) {
            std::copy(data->values, data->values + data->count, sample.begin());
        }
        else if (data->count != 0) {
            // wrong size sample was written
            throw Exception("ShmemEndpointUser::" + std::string(__func__) + "(): Data read from shmem does not match number of samples.",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        return data->timestamp;
    }

    std::string ShmemEndpointUser::get_agent(void)
    {
        return "";
    }
}

#ifdef __cplusplus
extern "C" {
#endif


typedef geopm::Endpoint geopm_endpoint_c;

int geopm_endpoint_create(const char *endpoint_name,
                          geopm_endpoint_c **endpoint)
{
    // todo: need to support files?
    *endpoint = (geopm_endpoint_c*)(new geopm::ShmemEndpoint(endpoint_name));
    return 0;
}

int geopm_endpoint_destroy(geopm_endpoint_c *endpoint)
{
    delete endpoint;
    return 0;
}

int geopm_endpoint_shmem_create(geopm_endpoint_c *endpoint)
{
    endpoint->open();
    return 0;
}

int geopm_endpoint_shmem_destroy(geopm_endpoint_c *endpoint)
{
    endpoint->close();
    return 0;
}

int geopm_endpoint_agent(geopm_endpoint_c *endpoint,
                         size_t agent_name_max,
                         char *agent_name)
{
    //TODO: null check, catch exceptions
    std::string agent = endpoint->get_agent();
    strncpy(agent_name, agent.c_str(), agent_name_max);
    return 0;
}

int geopm_endpoint_num_node(geopm_endpoint_c *endpoint,
                            int *num_node)
{
    return -1;
}

int geopm_endpoint_node_name(geopm_endpoint_c *endpoint,
                             int node_idx,
                             size_t node_name_max,
                             char *node_name)
{
    return -1;
}

int geopm_endpoint_write_policy(geopm_endpoint_c *endpoint,
                                size_t agent_num_policy,
                                const double *policy_array)
{
    std::vector<double> policy {policy_array, policy_array + agent_num_policy};
    endpoint->write_policy(policy);
    return 0;
}

int geopm_endpoint_read_sample(geopm_endpoint_c *endpoint,
                               size_t agent_num_sample,
                               double *sample_array,
                               double *sample_age_sec)
{
    std::vector<double> sample(agent_num_sample);
    endpoint->read_sample(sample);
    return 0;
}

#ifdef __cplusplus
}
#endif
