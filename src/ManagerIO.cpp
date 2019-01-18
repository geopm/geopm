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

#include <fstream>
#include <sstream>
#include <algorithm>
#include <string>
#include <cmath>
#include <string.h>

#include "contrib/json11/json11.hpp"

#include "ManagerIO.hpp"
#include "PlatformTopo.hpp"
#include "SharedMemory.hpp"
#include "Exception.hpp"
#include "Helper.hpp"
#include "Agent.hpp"
#include "geopm_env.h"
#include "config.h"

using json11::Json;

namespace geopm
{

    ManagerIO::ManagerIO(const std::string &data_path, bool is_policy)
        : ManagerIO(data_path, is_policy, geopm_env_agent())
    {
    }

    ManagerIO::ManagerIO(const std::string &data_path, bool is_policy, const std::string &agent_name)
        : ManagerIO(data_path,
                    nullptr,
                    is_policy ? Agent::policy_names(agent_factory().dictionary(agent_name)) :
                                Agent::sample_names(agent_factory().dictionary(agent_name)))
    {
    }

    ManagerIO::ManagerIO(const std::string &path, std::unique_ptr<ISharedMemory> shmem,
                         const std::vector<std::string> &signal_names)
        : m_path(path)
        , m_signal_names(signal_names)
        , m_shmem(std::move(shmem))
        , m_data(nullptr)
        , m_samples_up(signal_names.size())
        , m_is_shm_data((m_path[0] == '/' && m_path.find_last_of('/') == 0))
    {
        if (m_shmem == nullptr && m_is_shm_data) {
            size_t shmem_size = sizeof(struct geopm_manager_shmem_s);
            m_shmem = geopm::make_unique<SharedMemory>(m_path, shmem_size);
        }

        if (m_is_shm_data) {
            m_data = (struct geopm_manager_shmem_s *) m_shmem->pointer();
            *m_data = {};
            setup_mutex(m_data->lock);
        }
    }

    void ManagerIO::setup_mutex(pthread_mutex_t &lock)
    {
        pthread_mutexattr_t lock_attr;
        int err = pthread_mutexattr_init(&lock_attr);
        if (err) {
            throw Exception("ProfileTable: pthread mutex initialization", GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
        }
        err = pthread_mutexattr_settype(&lock_attr, PTHREAD_MUTEX_ERRORCHECK);
        if (err) {
            throw Exception("ProfileTable: pthread mutex initialization", GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
        }
        err = pthread_mutexattr_setpshared(&lock_attr, PTHREAD_PROCESS_SHARED);
        if (err) {
            throw Exception("ProfileTable: pthread mutex initialization", GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
        }
        err = pthread_mutex_init(&lock, &lock_attr);
        if (err) {
            throw Exception("ProfileTable: pthread mutex initialization", GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
        }
    }

    void ManagerIO::adjust(const std::vector<double> &settings)
    {
        if (settings.size() != m_signal_names.size()) {
            throw Exception("ManagerIO::" + std::string(__func__) + "(): size of settings does not match signal names.",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        m_samples_up = settings;
    }

    void ManagerIO::adjust(const std::string &signal_name, double setting)
    {
        // TODO : The next line does a linear search.  This may need optimized in the future.
        auto signal_it = std::find(m_signal_names.begin(), m_signal_names.end(), signal_name);

        if (signal_it != m_signal_names.end()) {
            size_t signal_idx = std::distance(m_signal_names.begin(), signal_it);
            m_samples_up[signal_idx] = setting;
        }
        else {
            throw Exception("ManagerIO::" + std::string(__func__)  + "(): requested signal \"" + signal_name
                            + "\" does not exist in m_signal_names.  Was it passed to the constructor?",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
    }

    void ManagerIO::write_batch(void)
    {
        if (m_is_shm_data) {
            write_shmem();
        }
        else {
            write_file();
        }
    }

    std::vector<std::string> ManagerIO::signal_names(void) const
    {
        return m_signal_names;
    }

    void ManagerIO::write_file(void)
    {
        std::ofstream json_file_out(m_path, std::ifstream::out);
        std::map<std::string, double> signal_value_map;

        if (!json_file_out.is_open()) {
            throw Exception("ManagerIOSampler::" + std::string(__func__) + "(): output file \"" + m_path +
                            "\" could not be opened", GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }

        for(size_t i = 0; i < m_signal_names.size(); ++i) {
            signal_value_map[m_signal_names[i]] = m_samples_up[i];
        }

        Json root (signal_value_map);
        json_file_out << root.dump();
        json_file_out.close();
    }

    void ManagerIO::write_shmem(void)
    {
        int err = pthread_mutex_lock(&m_data->lock); // Default mutex will block until this completes.
        if (err) {
            throw Exception("ManagerIOSampler::pthread_mutex_lock()", err, __FILE__, __LINE__);
        }

        m_data->is_updated = true;
        m_data->count = m_samples_up.size();
        std::copy(m_samples_up.begin(), m_samples_up.end(), m_data->values);

        pthread_mutex_unlock(&m_data->lock);
    }

    /*********************************************************************************************************/

    ManagerIOSampler::ManagerIOSampler(const std::string &data_path, bool is_policy)
        : ManagerIOSampler(data_path, is_policy, geopm_env_agent())
    {
    }

    ManagerIOSampler::ManagerIOSampler(const std::string &data_path, bool is_policy, const std::string &agent_name)
        : ManagerIOSampler(data_path,
                           nullptr,
                           is_policy ? Agent::policy_names(agent_factory().dictionary(agent_name)) :
                                       Agent::sample_names(agent_factory().dictionary(agent_name)))
    {
    }

    ManagerIOSampler::ManagerIOSampler(const std::string &path, std::unique_ptr<ISharedMemoryUser> shmem, const std::vector<std::string> &signal_names)
        : m_path(path)
        , m_signal_names(signal_names)
        , m_shmem(std::move(shmem))
        , m_data(nullptr)
        , m_is_shm_data(m_path[0] == '/' && m_path.find_last_of('/') == 0)
    {
        read_batch();
    }

    std::map<std::string, double> ManagerIOSampler::parse_json(void)
    {
        std::map<std::string, double> signal_value_map;
        std::string json_str;

        json_str = read_file(m_path);

        // Begin JSON parse
        std::string err;
        Json root = Json::parse(json_str, err);
        if (!err.empty() || !root.is_object()) {
            throw Exception("ManagerIOSampler::" + std::string(__func__) + "(): detected a malformed json config file: " + err,
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
                    throw Exception("Json::" + std::string(__func__)  + ": unsupported type or malformed json config file",
                                    GEOPM_ERROR_FILE_PARSE, __FILE__, __LINE__);
                }
            }
            else {
                throw Exception("Json::" + std::string(__func__)  + ": unsupported type or malformed json config file",
                                GEOPM_ERROR_FILE_PARSE, __FILE__, __LINE__);
            }
        }

        return signal_value_map;
    }

    void ManagerIOSampler::read_shmem(void)
    {
        if (m_shmem == nullptr) {
            m_shmem = geopm::make_unique<SharedMemoryUser>(m_path, geopm_env_profile_timeout());
        }

        m_data = (struct geopm_manager_shmem_s *) m_shmem->pointer(); // Managed by shmem subsystem.

        int err = pthread_mutex_lock(&m_data->lock); // Default mutex will block until this completes.
        if (err) {
            throw Exception("ManagerIOSampler::pthread_mutex_lock()", err, __FILE__, __LINE__);
        }

        if (m_data->is_updated == 0) {
            (void) pthread_mutex_unlock(&m_data->lock);
            throw Exception("ManagerIOSampler::" + std::string(__func__) + "(): reread of shm region requested before update.",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }

        m_signals_down = std::vector<double>(m_data->values, m_data->values + m_data->count);
        m_data->is_updated = 0;
        (void) pthread_mutex_unlock(&m_data->lock);

        if (m_signals_down.size() != m_signal_names.size()) {
            throw Exception("ManagerIOSampler::" + std::string(__func__) + "(): Data read from shmem does not match size of signal names.",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
    }

    bool ManagerIOSampler::is_valid_signal(const std::string &signal_name) const
    {
        return std::find(m_signal_names.begin(), m_signal_names.end(), signal_name) != m_signal_names.end();
    }

    void ManagerIOSampler::read_batch(void)
    {
        if (m_is_shm_data == true) {
            read_shmem();
        }
        else {
            if (m_signal_names.size() > 0) {
                std::map<std::string, double> signal_value_map = parse_json();
                m_signals_down.clear();
                for (auto signal : m_signal_names) {
                    try {
                        m_signals_down.emplace_back(signal_value_map.at(signal));
                    }
                    catch (const std::out_of_range&) {
                        throw Exception("ManagerIOSampler::" + std::string(__func__) + "(): Signal \"" + signal + "\" not found.",
                                        GEOPM_ERROR_INVALID, __FILE__, __LINE__);
                    }
                }
            }
        }
    }

    std::vector<double> ManagerIOSampler::sample(void) const
    {
        return m_signals_down;
    }

    double ManagerIOSampler::sample(const std::string &signal_name) const
    {
        if (!is_valid_signal(signal_name)) {
            throw Exception("ManagerIOGroup::" + std::string(__func__) + "(): " + signal_name + " not valid for ManagerIOGroup.",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }

        auto it = std::distance(m_signal_names.begin(), std::find(m_signal_names.begin(), m_signal_names.end(), signal_name));

        return m_signals_down.at(it);
    }

    bool ManagerIOSampler::is_update_available(void)
    {
        if(m_data == nullptr) {
            throw Exception("ManagerIOSampler::" + std::string(__func__) + "(): m_data is null", GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        return m_data->is_updated != 0;
    }

    std::vector<std::string> ManagerIOSampler::signal_names(void) const
    {
        return m_signal_names;
    }
}
