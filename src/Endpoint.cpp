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

    ShmemEndpoint::ShmemEndpoint(const std::string &data_path, const std::string &agent_name)
        : ShmemEndpoint(data_path,
                        nullptr,
                        Agent::num_policy(agent_factory().dictionary(agent_name)))
    {

    }

    ShmemEndpoint::ShmemEndpoint(const std::string &path,
                                 std::unique_ptr<SharedMemory> policy_shmem,
                                 size_t num_policy)
        : m_path(path)
        , m_policy_shmem(std::move(policy_shmem))
        , m_num_policy(num_policy)
    {
        if (m_policy_shmem == nullptr) {
            size_t shmem_size = sizeof(struct geopm_endpoint_policy_shmem_s);
            m_policy_shmem = SharedMemory::make_unique(m_path + SHM_POLICY_POSTFIX, shmem_size);
        }

        auto lock = m_policy_shmem->get_scoped_lock();
        auto data = (struct geopm_endpoint_policy_shmem_s *) m_policy_shmem->pointer();
        *data = {};
    }

    ShmemEndpoint::~ShmemEndpoint()
    {
        if (m_policy_shmem) {
            m_policy_shmem->unlink();
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

    /*********************************************************************************************************/

    ShmemEndpointUser::ShmemEndpointUser(const std::string &data_path)
        : ShmemEndpointUser(data_path,
                            nullptr)
    {

    }

    ShmemEndpointUser::ShmemEndpointUser(const std::string &path,
                                         std::unique_ptr<SharedMemoryUser> policy_shmem)
        : m_path(path)
        , m_policy_shmem(std::move(policy_shmem))
    {
        // attach to shared memory here and write agent
        /// @todo: use cases for separate attach method()?
        // Attach to policy shmem first; no agent will be written.  Once user attaches
        // to sample shmem, RM knows it has attached to both.
        if (m_policy_shmem == nullptr) {
            m_policy_shmem = SharedMemoryUser::make_unique(m_path + SHM_POLICY_POSTFIX,
                                                           environment().timeout());
        }
    }

    ShmemEndpointUser::~ShmemEndpointUser()
    {

    }

    FileEndpointUser::FileEndpointUser(const std::string &data_path)
        : FileEndpointUser(data_path,
                           Agent::policy_names(agent_factory().dictionary(environment().agent())))
    {

    }

    FileEndpointUser::FileEndpointUser(const std::string &path, const std::vector<std::string> &policy_names)
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
        auto lock = m_policy_shmem->get_scoped_lock();
        auto data = (struct geopm_endpoint_policy_shmem_s *) m_policy_shmem->pointer(); // Managed by shmem subsystem.

        int num_policy = data->count;
        if (policy.size() < (size_t)num_policy) {
            throw Exception("ShmemEndpointUser::" + std::string(__func__) + "(): Data read from shmem does not fit in policy vector.",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        // Fill in missing policy values with NAN (default)
        std::fill(policy.begin(), policy.end(), NAN);
        std::copy(data->values, data->values + data->count, policy.begin());
    }

    void FileEndpointUser::read_policy(std::vector<double> &policy)
    {
        // @todo: don't resize the policy vector
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

    std::unique_ptr<EndpointUser> EndpointUser::create_endpoint_user(const std::string &policy_path)
    {
        std::unique_ptr<EndpointUser> endpoint;
        if (policy_path[0] == '/' && policy_path.find_last_of('/') == 0) {
            endpoint = geopm::make_unique<ShmemEndpointUser>(policy_path);
        }
        else {
            endpoint = geopm::make_unique<FileEndpointUser>(policy_path);
        }
        return endpoint;
    }
}
