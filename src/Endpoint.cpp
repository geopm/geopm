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
#include <iostream>
#include <string>
#include <fstream>

#include "contrib/json11/json11.hpp"

#include "Agent.hpp"
#include "Endpoint.hpp"
#include "SharedMemory.hpp"
#include "geopm_endpoint.h"
#include "Exception.hpp"
#include "Helper.hpp"
#include "geopm_env.h"
#include "config.h"

using json11::Json;

namespace geopm
{

    Endpoint::Endpoint()
        : m_policy_data(nullptr)
        , m_sample_data(nullptr)
        , m_is_using_shmem(false)
        , m_agent_name(geopm_env_agent())
        , m_policy(Agent::num_policy(agent_factory().dictionary(m_agent_name)))
        , m_sample(Agent::num_sample(agent_factory().dictionary(m_agent_name)))
        , m_policy_names(Agent::policy_names(agent_factory().dictionary(m_agent_name)))
        , m_sample_names(Agent::sample_names(agent_factory().dictionary(m_agent_name)))
    {
        std::cout << "In Endpoint::Endpoint() for Kontroller..." << std::endl;

        if (std::string(geopm_env_policy()).size() == 0) {
            throw Exception("Endpoint::" + std::string(__func__) + "(): No policy specified.",
                            GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
        }
        // If GEOPM_ENDPOINT is defined, it's required that the shmem region was already created.
        // If it isn't, the policy will be read from a JSON file.
        else if (std::string(geopm_env_endpoint()).size() > 0) {
            m_is_using_shmem = true;
            attach_shmem();
        }
        update(); // Updates internal state from either the shmem region or JSON
    }

    Endpoint::Endpoint(std::string endpoint)
        : m_policy_data(nullptr)
        , m_sample_data(nullptr)
    {
        std::cout << "In Endpoint::Endpoint() for commandline..." << std::endl;
        setenv("GEOPM_ENDPOINT", endpoint.c_str(), 1);
        geopm_env_load(); // This has to be here since the env was already parsed once when geopmpolicy_load was called.
    }

    Endpoint::~Endpoint()
    {
        std::cout << "In Endpoint::~Endpoint()..." << std::endl;
    }

    void Endpoint::update_sample(const std::vector<double> &values)
    {
        if (values.size() != m_sample_names.size()) {
            throw Exception("Endpoint::" + std::string(__func__) + "(): size of values does not match sample names.",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        m_sample = values;
    }

    void Endpoint::update(void)
    {
        if (m_is_using_shmem == true) {
            m_policy = read_policy_shmem();
            // write_sample_shmem(m_sample); /// @todo IMPLEMENTME
        }
        else {
            std::map<std::string, double> name_value_map = parse_json();
            if (name_value_map.size() != m_policy.size()) {
                throw Exception("Endpoint::" + std::string(__func__) + "(): JSON policy size does not match Agent's size.",
                                GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
            }
            m_policy.clear();
            for (size_t i = 0; i < m_policy_names.size(); ++i) {
                try {
                    m_policy.at(i) = name_value_map.at(m_policy_names.at(i));
                }
                catch (const std::out_of_range&) {
                    throw Exception("Endpoint::" + std::string(__func__) + "(): Signal \"" + m_policy_names.at(i) + "\" not found.",
                                    GEOPM_ERROR_INVALID, __FILE__, __LINE__);
                }
            }
        }
    }

    std::vector<double> Endpoint::read_policy_shmem(void)
    {
        std::vector<double> policy;

        if (m_policy_shmem_user == nullptr) {
            throw Exception("Endpoint::" + std::string(__func__) + "(): m_shmem is null.",
                            GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
        }

        int err = pthread_mutex_lock(&m_policy_data->lock); // Default mutex will block until this completes.
        if (err) {
            throw Exception("Endpoint::pthread_mutex_lock()", err, __FILE__, __LINE__);
        }

        if (m_policy_data->is_updated == 0) {
            (void) pthread_mutex_unlock(&m_policy_data->lock);
            throw Exception("Endpoint::" + std::string(__func__) + "(): reread of shm region requested before update.",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }

        policy = std::vector<double>(m_policy_data->values, m_policy_data->values + m_policy_data->count);
        m_policy_data->is_updated = 0;
        (void) pthread_mutex_unlock(&m_policy_data->lock);

        /// @todo This needs to go somewhere.  m_policy_names has to be populated though.
        // if (policy.size() != m_policy_names.size()) {
        //     throw Exception("Endpoint::" + std::string(__func__) + "(): Data read from shmem does not match size of policy names.",
        //                     GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        // }

        return policy;
    }

    std::map<std::string, double> Endpoint::parse_json(void)
    {
        std::map<std::string, double> signal_value_map;
        std::string json_str;

        json_str = read_file();

        // Begin JSON parse
        std::string err;
        Json root = Json::parse(json_str, err);
        if (!err.empty() || !root.is_object()) {
            throw Exception("Endpoint::" + std::string(__func__) + "(): detected a malformed json config file: " + err,
                            GEOPM_ERROR_FILE_PARSE, __FILE__, __LINE__);
        }

        for (const auto &obj : root.object_items()) {
            if (obj.second.type() == Json::NUMBER) {
                signal_value_map.emplace(obj.first, obj.second.number_value());
            }
            else {
                throw Exception("Json::" + std::string(__func__)  + ": unsupported type or malformed json config file",
                                GEOPM_ERROR_FILE_PARSE, __FILE__, __LINE__);
            }
        }

        return signal_value_map;
    }

    const std::string Endpoint::read_file(void)
    {
        std::string json_str;
        std::ifstream json_file_in(geopm_env_policy(), std::ifstream::in);

        if (!json_file_in.is_open()) {
            throw Exception("Endpoint::" + std::string(__func__) + "(): input configuration file \"" + geopm_env_policy() +
                            "\" could not be opened", GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }

        json_file_in.seekg(0, std::ios::end);
        size_t file_size = json_file_in.tellg();
        if (file_size <= 0) {
            throw Exception("Endpoint::" + std::string(__func__) + "(): input configuration file invalid",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        json_str.resize(file_size); // DO NOT modify json_str beyond this point.
        json_file_in.seekg(0, std::ios::beg);
        json_file_in.read(&json_str[0], file_size);
        json_file_in.close();

        return json_str;
    }

    void Endpoint::setup_mutex(pthread_mutex_t &lock)
    {
        pthread_mutexattr_t lock_attr;
        int err = pthread_mutexattr_init(&lock_attr);
        if (err) {
            throw Exception("Endpoint::" + std::string(__func__) + "()", GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
        }
        err = pthread_mutexattr_settype(&lock_attr, PTHREAD_MUTEX_ERRORCHECK);
        if (err) {
            throw Exception("Endpoint::" + std::string(__func__) + "()", GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
        }
        err = pthread_mutexattr_setpshared(&lock_attr, PTHREAD_PROCESS_SHARED);
        if (err) {
            throw Exception("Endpoint::" + std::string(__func__) + "()", GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
        }
        err = pthread_mutex_init(&lock, &lock_attr);
        if (err) {
            throw Exception("Endpoint::" + std::string(__func__) + "()", GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
        }
    }

    void Endpoint::shmem_create(void)
    {
        std::cout << "In Endpoint::shmem_create() " << std::endl;

        std::cout << "DEBUG - Policy shmem region = " << geopm_env_policy() << std::endl;
        std::cout << "DEBUG - Sample shmem region = " << geopm_env_sample() << std::endl;
        std::string policy_path (geopm_env_policy());

        if (policy_path[0] != '/' || policy_path.find_last_of('/') != 0) {
            throw Exception("Endpoint::" + std::string(__func__) + "(): Invalid policy path.",
                             GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }

        size_t shmem_size = sizeof(struct geopm_endpoint_shmem_s);
        SharedMemory policy_shmem (geopm_env_policy(), shmem_size, true);
        struct geopm_endpoint_shmem_s * policy_data = (struct geopm_endpoint_shmem_s *) policy_shmem.pointer();
        *policy_data = {};
        setup_mutex(policy_data->lock);

        SharedMemory sample_shmem (geopm_env_sample(), shmem_size, true);
        struct geopm_endpoint_shmem_s * sample_data = (struct geopm_endpoint_shmem_s *) sample_shmem.pointer();
        *sample_data = {};
        setup_mutex(sample_data->lock);
    }

    void Endpoint::shmem_destroy(void)
    {
        std::cout << "In Endpoint::shmem_destroy() " << std::endl;
        std::cout << "Policy shmem region = " << geopm_env_policy() << std::endl;
        std::cout << "Sample shmem region = " << geopm_env_sample() << std::endl;

        m_policy_shmem_user = geopm::make_unique<SharedMemoryUser>(geopm_env_policy(), 5);
        m_policy_shmem_user->unlink();
        m_sample_shmem_user = geopm::make_unique<SharedMemoryUser>(geopm_env_sample(), 5);
        m_sample_shmem_user->unlink();
    }

    void Endpoint::attach_shmem(void)
    {
        std::cout << "Endpoint::" << std::string(__func__) << "(): About to attach to " << geopm_env_policy() << std::endl;

        std::string policy_path (geopm_env_policy());

        if (policy_path[0] != '/' || policy_path.find_last_of('/') != 0) {
            throw Exception("Endpoint::" + std::string(__func__) + "(): Invalid policy path.",
                             GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }

        m_policy_shmem_user = geopm::make_unique<SharedMemoryUser>(geopm_env_policy(), 5);
        m_policy_data = (struct geopm_endpoint_shmem_s *) m_policy_shmem_user->pointer();
        m_sample_shmem_user = geopm::make_unique<SharedMemoryUser>(geopm_env_sample(), 5);
        m_sample_data = (struct geopm_endpoint_shmem_s *) m_sample_shmem_user->pointer();
    }

    std::string Endpoint::agent_name(void)
    {
        if (m_policy_data == nullptr) {
            throw Exception("Endpoint::" + std::string(__func__) + "(): No shmem region attached.",
                            GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
        }

        int err = pthread_mutex_lock(&m_policy_data->lock); // Default mutex will block until this completes.
        if (err) {
            throw Exception("Endpoint::" + std::string(__func__) + "(): ", err, __FILE__, __LINE__);
        }
        std::string name = m_policy_data->agent_name;
        pthread_mutex_unlock(&m_policy_data->lock);
        return name;
    }

    int Endpoint::num_node(void)
    {
        throw Exception("Endpoint::" + std::string(__func__) + "(): ", GEOPM_ERROR_NOT_IMPLEMENTED, __FILE__, __LINE__);
    }

    std::string Endpoint::node_name(int node_idx)
    {
        throw Exception("Endpoint::" + std::string(__func__) + "(): ", GEOPM_ERROR_NOT_IMPLEMENTED, __FILE__, __LINE__);
    }

    int Endpoint::num_policy(void) {
        std::string name = agent_name();
        return Agent::num_policy(agent_factory().dictionary(name));
    }

    void Endpoint::write_policy_shmem(std::vector<double> policy_array) {
        int err = pthread_mutex_lock(&m_policy_data->lock); // Default mutex will block until this completes.
        if (err) {
            throw Exception("Endpoint::" + std::string(__func__) + "(): ", err, __FILE__, __LINE__);
        }

        m_policy_data->is_updated = true;
        m_policy_data->count = policy_array.size();
        std::copy(policy_array.begin(), policy_array.end(), m_policy_data->values);

        pthread_mutex_unlock(&m_policy_data->lock);
    }

    std::vector<double> Endpoint::read_sample_shmem(void) {
        std::vector<double> samples;

        int err = pthread_mutex_lock(&m_sample_data->lock); // Default mutex will block until this completes.
        if (err) {
            throw Exception("Endpoint::" + std::string(__func__) + "(): ", err, __FILE__, __LINE__);
        }

        // if (m_sample_data->is_updated == 0) {
        //     (void) pthread_mutex_unlock(&m_sample_data->lock);
        //     throw Exception("Endpoint::" + std::string(__func__) + "(): reread of shm region requested before update.",
        //                     GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        // }

        samples = std::vector<double>(m_sample_data->values, m_sample_data->values + m_sample_data->count);
        // m_sample_data->is_updated = 0;
        (void) pthread_mutex_unlock(&m_sample_data->lock);

        /// @todo Check count matches expected.  Other error checks?

        return samples;
    }
}


int geopm_endpoint_create(const char *endpoint_name,
                          struct geopm_endpoint_c **endpoint)
{
    printf("Endpoint geopm_endpoint_create().\n");
    int err = 0;
    try {
        *endpoint = (struct geopm_endpoint_c *)(new geopm::Endpoint(endpoint_name));
    }
    catch (...) {
        err = geopm::exception_handler(std::current_exception(), true);
    }
    return err;
}

int geopm_endpoint_destroy(struct geopm_endpoint_c *endpoint)
{
    printf("Endpoint geopm_endpoint_delete().\n");
    int err = 0;
    geopm::Endpoint *endpoint_obj = (geopm::Endpoint *)endpoint;
    try {
        delete endpoint_obj;
    }
    catch (...) {
        err = geopm::exception_handler(std::current_exception(), true);
    }
    return err;
}

int geopm_endpoint_shmem_create(struct geopm_endpoint_c *endpoint)
{
    printf("Endpoint geopm_endpoint_shmem_create().\n");
    int err = 0;
    geopm::Endpoint *endpoint_obj = (geopm::Endpoint *)endpoint;
    try {
        endpoint_obj->shmem_create();
    }
    catch (...) {
        err = geopm::exception_handler(std::current_exception(), true);
    }
    return err;
}

int geopm_endpoint_shmem_destroy(struct geopm_endpoint_c *endpoint)
{
    printf("Endpoint geopm_endpoint_shmem_destroy().\n");
    int err = 0;
    geopm::Endpoint *endpoint_obj = (geopm::Endpoint *)endpoint;
    try {
        endpoint_obj->shmem_destroy();
    }
    catch (...) {
        err = geopm::exception_handler(std::current_exception(), true);
    }
    return err;
}

int geopm_endpoint_shmem_attach(struct geopm_endpoint_c *endpoint)
{
    printf("Endpoint geopm_endpoint_attach_shmem().\n");
    int err = 0;
    geopm::Endpoint *endpoint_obj = (geopm::Endpoint *)endpoint;
    try {
        endpoint_obj->attach_shmem();
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
    printf("Endpoint geopm_endpoint_agent().\n");
    int err = 0;
    geopm::Endpoint *endpoint_obj = (geopm::Endpoint *)endpoint;
    try {
        std::string name = endpoint_obj->agent_name();
        name.copy(agent_name, agent_name_max);
        // strncpy(agent_name, endpoint_obj->agent_name(), agent_name_max);
        if (name == "") {
            printf("BRGBRGBRG No Agent name found!!!!!!!!!!!!!! Agent not connected.\n");
        }
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
    geopm::Endpoint *endpoint_obj = (geopm::Endpoint *)endpoint;
    try {
        *num_node = endpoint_obj->num_node();
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
    geopm::Endpoint *endpoint_obj = (geopm::Endpoint *)endpoint;
    try {
        std::string name = endpoint_obj->node_name(node_idx);
        name.copy(node_name, node_name_max);
    }
    catch (...) {
        err = geopm::exception_handler(std::current_exception(), true);
    }
    return err;
}

int geopm_endpoint_agent_policy(struct geopm_endpoint_c *endpoint,
                                const double *policy_array)
{
    int err = 0;
    geopm::Endpoint *endpoint_obj = (geopm::Endpoint *)endpoint;
    try {
        int num_policy = endpoint_obj->num_policy();
        endpoint_obj->write_policy_shmem(std::vector<double>(policy_array, policy_array + num_policy));
    }
    catch (...) {
        err = geopm::exception_handler(std::current_exception(), true);
    }
    return err;
}

int geopm_endpoint_agent_sample(struct geopm_endpoint_c *endpoint,
                                double *sample_array,
                                double *sample_age_sec)
{
    int err = 0;
    geopm::Endpoint *endpoint_obj = (geopm::Endpoint *)endpoint;
    try {
        std::vector<double> samples = endpoint_obj->read_sample_shmem();
        *sample_age_sec = -1.0; /// @todo Implement me.
        std::copy(samples.begin(), samples.end(), sample_array);
    }
    catch (...) {
        err = geopm::exception_handler(std::current_exception(), true);
    }
    return err;
}

