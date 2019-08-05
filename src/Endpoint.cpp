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
#include "SharedMemoryImp.hpp"
#include "Exception.hpp"
#include "Helper.hpp"
#include "Agent.hpp"
#include "config.h"

using json11::Json;

namespace geopm
{
    ShmemEndpoint::ShmemEndpoint(const std::string &data_path, bool is_policy)
        : ShmemEndpoint(data_path, is_policy, environment().agent())
    {
    }

    ShmemEndpoint::ShmemEndpoint(const std::string &data_path, bool is_policy, const std::string &agent_name)
        : ShmemEndpoint(data_path,
                       nullptr,
                       is_policy ? Agent::policy_names(agent_factory().dictionary(agent_name)) :
                                   Agent::sample_names(agent_factory().dictionary(agent_name)))
    {
    }

    ShmemEndpoint::ShmemEndpoint(const std::string &path, std::unique_ptr<SharedMemory> shmem,
                                 const std::vector<std::string> &signal_names)
        : m_path(path)
        , m_signal_names(signal_names)
        , m_shmem(std::move(shmem))
        , m_data(nullptr)
        , m_samples_up(signal_names.size())
    {
        if (m_shmem == nullptr) {
            size_t shmem_size = sizeof(struct geopm_endpoint_shmem_s);
            m_shmem = geopm::make_unique<SharedMemoryImp>(m_path, shmem_size);
        }

        auto lock = m_shmem->get_scoped_lock();
        m_data = (struct geopm_endpoint_shmem_s *) m_shmem->pointer();
        *m_data = {};
    }

    FileEndpoint::FileEndpoint(const std::string &data_path, bool is_policy)
        : FileEndpoint(data_path, is_policy, environment().agent())
    {
    }

    FileEndpoint::FileEndpoint(const std::string &data_path, bool is_policy, const std::string &agent_name)
        : FileEndpoint(data_path,
                       is_policy ? Agent::policy_names(agent_factory().dictionary(agent_name)) :
                                   Agent::sample_names(agent_factory().dictionary(agent_name)))
    {

    }

    FileEndpoint::FileEndpoint(const std::string &path,
                               const std::vector<std::string> &signal_names)
        : m_path(path)
        , m_signal_names(signal_names)
        , m_samples_up(signal_names.size())
    {

    }

    ShmemEndpoint::~ShmemEndpoint()
    {
        if (m_shmem) {
            m_shmem->unlink();
        }
    }

    void ShmemEndpoint::adjust(const std::vector<double> &settings)
    {
        if (settings.size() != m_signal_names.size()) {
            throw Exception("ShmemEndpoint::" + std::string(__func__) + "(): size of settings does not match signal names.",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        m_samples_up = settings;
    }

    void ShmemEndpoint::write_batch(void)
    {
        write_shmem();
    }

    void FileEndpoint::adjust(const std::vector<double> &settings)
    {
        if (settings.size() != m_signal_names.size()) {
            throw Exception("FileEndpoint::" + std::string(__func__) + "(): size of settings does not match signal names.",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        m_samples_up = settings;
    }

    void FileEndpoint::write_batch(void)
    {
        write_file();
    }

    std::vector<std::string> ShmemEndpoint::signal_names(void) const
    {
        return m_signal_names;
    }

    std::vector<std::string> FileEndpoint::signal_names(void) const
    {
        return m_signal_names;
    }

    void FileEndpoint::write_file(void)
    {
        std::map<std::string, double> signal_value_map;
        for(size_t i = 0; i < m_signal_names.size(); ++i) {
            signal_value_map[m_signal_names[i]] = m_samples_up[i];
        }

        Json root (signal_value_map);
        geopm::write_file(m_path, root.dump());
    }

    void ShmemEndpoint::write_shmem(void)
    {
        auto lock = m_shmem->get_scoped_lock();
        m_data->is_updated = true;
        m_data->count = m_samples_up.size();
        std::copy(m_samples_up.begin(), m_samples_up.end(), m_data->values);
    }

    /*********************************************************************************************************/

    ShmemEndpointUser::ShmemEndpointUser(const std::string &data_path, bool is_policy)
        : ShmemEndpointUser(data_path, is_policy, environment().agent())
    {
    }

    ShmemEndpointUser::ShmemEndpointUser(const std::string &data_path, bool is_policy, const std::string &agent_name)
        : ShmemEndpointUser(data_path,
                            nullptr,
                            is_policy ? Agent::policy_names(agent_factory().dictionary(agent_name)) :
                                        Agent::sample_names(agent_factory().dictionary(agent_name)))
    {
    }

    ShmemEndpointUser::ShmemEndpointUser(const std::string &path, std::unique_ptr<SharedMemoryUser> shmem, const std::vector<std::string> &signal_names)
        : m_path(path)
        , m_signal_names(signal_names)
        , m_shmem(std::move(shmem))
        , m_data(nullptr)
    {
        read_batch();
    }

    FileEndpointUser::FileEndpointUser(const std::string &data_path, bool is_policy)
        : FileEndpointUser(data_path, is_policy, environment().agent())
    {
    }

    FileEndpointUser::FileEndpointUser(const std::string &data_path, bool is_policy, const std::string &agent_name)
        : FileEndpointUser(data_path,
                           is_policy ? Agent::policy_names(agent_factory().dictionary(agent_name)) :
                                       Agent::sample_names(agent_factory().dictionary(agent_name)))
    {
    }

    FileEndpointUser::FileEndpointUser(const std::string &path, const std::vector<std::string> &signal_names)
        : m_path(path)
        , m_signal_names(signal_names)
    {
        read_batch();
    }

    std::map<std::string, double> FileEndpointUser::parse_json(void)
    {
        std::map<std::string, double> signal_value_map;
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
                signal_value_map.emplace(obj.first, obj.second.number_value());
            }
            else if (obj.second.type() == Json::STRING) {
                std::string tmp_val = obj.second.string_value();
                if (tmp_val.compare("NAN") == 0 || tmp_val.compare("NaN") == 0 || tmp_val.compare("nan") == 0) {
                    signal_value_map.emplace(obj.first, NAN);
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

        return signal_value_map;
    }

    void ShmemEndpointUser::read_shmem(void)
    {
        if (m_shmem == nullptr) {
            m_shmem = geopm::make_unique<SharedMemoryUserImp>(m_path, environment().timeout());
        }

        auto lock = m_shmem->get_scoped_lock();
        m_data = (struct geopm_endpoint_shmem_s *) m_shmem->pointer(); // Managed by shmem subsystem.

        if (m_data->is_updated == 0) {
            throw Exception("ShmemEndpointUser::" + std::string(__func__) + "(): reread of shm region requested before update.",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }

        // Fill in missing policy values with NAN (default)
        m_signals_down = std::vector<double>(m_signal_names.size(), NAN);
        std::copy(m_data->values, m_data->values + m_data->count, m_signals_down.begin());

        m_data->is_updated = 0;

        if (m_signals_down.size() != m_signal_names.size()) {
            throw Exception("ShmemEndpointUser::" + std::string(__func__) + "(): Data read from shmem does not match size of signal names.",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
    }

    void ShmemEndpointUser::read_batch(void)
    {
        read_shmem();
    }

    std::vector<double> ShmemEndpointUser::sample(void) const
    {
        return m_signals_down;
    }

    void FileEndpointUser::read_batch(void)
    {
        if (m_signal_names.size() > 0) {
            std::map<std::string, double> signal_value_map = parse_json();
            m_signals_down.clear();
            for (auto signal : m_signal_names) {
                auto it = signal_value_map.find(signal);
                if (it != signal_value_map.end()) {
                    m_signals_down.emplace_back(signal_value_map.at(signal));
                }
                else {
                    // Fill in missing policy values with NAN (default)
                    m_signals_down.emplace_back(NAN);
                }
            }
        }
    }

    std::vector<double> FileEndpointUser::sample(void) const
    {
        return m_signals_down;
    }

    bool ShmemEndpointUser::is_update_available(void)
    {
        if(m_data == nullptr) {
            throw Exception("ShmemEndpointUser::" + std::string(__func__) + "(): m_data is null", GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        return m_data->is_updated != 0;
    }

    bool FileEndpointUser::is_update_available(void)
    {
        /// @todo: this method may be more useful as private method in shmem only
        return false;
    }

    std::vector<std::string> ShmemEndpointUser::signal_names(void) const
    {
        return m_signal_names;
    }

    std::vector<std::string> FileEndpointUser::signal_names(void) const
    {
        return m_signal_names;
    }

    std::unique_ptr<Endpoint> Endpoint::create_endpoint(const std::string &policy_path)
    {
        std::unique_ptr<Endpoint> endpoint;
        if (policy_path[0] == '/' && policy_path.find_last_of('/') == 0) {
            endpoint = geopm::make_unique<ShmemEndpoint>(policy_path, true);
        }
        else {
            endpoint = geopm::make_unique<FileEndpoint>(policy_path, true);
        }
        return endpoint;
    }

    std::unique_ptr<EndpointUser> EndpointUser::create_endpoint_user(const std::string &policy_path)
    {
        std::unique_ptr<EndpointUser> endpoint;
        if (policy_path[0] == '/' && policy_path.find_last_of('/') == 0) {
            endpoint = geopm::make_unique<ShmemEndpointUser>(policy_path, true);
        }
        else {
            endpoint = geopm::make_unique<FileEndpointUser>(policy_path, true);
        }
        return endpoint;
    }
}
