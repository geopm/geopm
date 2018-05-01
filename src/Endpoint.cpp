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

#include "Agent.hpp"
#include "Endpoint.hpp"
#include "SharedMemory.hpp"
#include "geopm_endpoint.h"
#include "Exception.hpp"
#include "Helper.hpp"
#include "geopm_env.h"
#include "config.h"

namespace geopm
{

    Endpoint::Endpoint(std::string endpoint)
        : m_data(nullptr)
    {
        std::cout << "In Endpoint::Endpoint()..." << std::endl;
        setenv("GEOPM_ENDPOINT", endpoint.c_str(), 1);
        geopm_env_load(); // This has to be here since the env was already parsed once when geopmpolicy_load was called.
    }

    Endpoint::~Endpoint()
    {
        std::cout << "In Endpoint::~Endpoint()..." << std::endl;
    }

    void Endpoint::setup_mutex(pthread_mutex_t &lock)
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

    void Endpoint::create_shmem(void)
    {
        std::cout << "In Endpoint::create_shmem() " << std::endl;

        std::cout << "DEBUG - Policy shmem region = " << geopm_env_policy() << std::endl;
        // std::cout << "DEBUG - Sample shmem region = " << geopm_env_sample() << std::endl;

        size_t shmem_size = sizeof(struct geopm_endpoint_shmem_s);
        m_shmem = geopm::make_unique<SharedMemory>(geopm_env_policy(), shmem_size, true);
        // m_shmem = geopm::make_unique<SharedMemory>(geopm_env_sample(), shmem_size, true);

        m_data = (struct geopm_endpoint_shmem_s *) m_shmem->pointer();
        *m_data = {};
        setup_mutex(m_data->lock);
    }

    void Endpoint::destroy_shmem(void)
    {
        std::cout << "In Endpoint::destroy_shmem() " << std::endl;
        std::cout << "Policy shmem region = " << geopm_env_policy() << std::endl;
        std::cout << "Sample shmem region = " << geopm_env_sample() << std::endl;

        m_shmem_user = geopm::make_unique<SharedMemoryUser>(geopm_env_policy(), 5);
        m_shmem_user->unlink();
    }

    void Endpoint::attach_shmem(void)
    {
        std::cout << "Endpoint::" << std::string(__func__) << "(): About to attach to " << geopm_env_policy() << std::endl;
        m_shmem_user = geopm::make_unique<SharedMemoryUser>(geopm_env_policy(), 5);
        m_data = (struct geopm_endpoint_shmem_s *) m_shmem_user->pointer();
    }

    std::string Endpoint::agent_name(void)
    {
        if (m_data == nullptr) {
            throw Exception("Endpoint::" + std::string(__func__) + "(): No shmem region attached.",
                            GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
        }

        int err = pthread_mutex_lock(&m_data->lock); // Default mutex will block until this completes.
        if (err) {
            throw Exception("Endpoint::" + std::string(__func__) + "(): ", err, __FILE__, __LINE__);
        }
        std::string name = m_data->agent_name;
        pthread_mutex_unlock(&m_data->lock);
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

    void Endpoint::policy(std::vector<double> policy_array) {
        int err = pthread_mutex_lock(&m_data->lock); // Default mutex will block until this completes.
        if (err) {
            throw Exception("Endpoint::" + std::string(__func__) + "(): ", err, __FILE__, __LINE__);
        }

        m_data->is_updated = true;
        m_data->count = policy_array.size();
        std::copy(policy_array.begin(), policy_array.end(), m_data->values);

        pthread_mutex_unlock(&m_data->lock);
    }

    std::vector<double> Endpoint::sample(void) {
        std::vector<double> samples;

        int err = pthread_mutex_lock(&m_data->lock); // Default mutex will block until this completes.
        if (err) {
            throw Exception("Endpoint::" + std::string(__func__) + "(): ", err, __FILE__, __LINE__);
        }

        // if (m_data->is_updated == 0) {
        //     (void) pthread_mutex_unlock(&m_data->lock);
        //     throw Exception("ManagerIOSampler::" + std::string(__func__) + "(): reread of shm region requested before update.",
        //                     GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        // }

        samples = std::vector<double>(m_data->values, m_data->values + m_data->count);
        // m_data->is_updated = 0;
        (void) pthread_mutex_unlock(&m_data->lock);

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

int geopm_endpoint_create_shmem(struct geopm_endpoint_c *endpoint)
{
    printf("Endpoint geopm_endpoint_create_shmem().\n");
    int err = 0;
    geopm::Endpoint *endpoint_obj = (geopm::Endpoint *)endpoint;
    try {
        endpoint_obj->create_shmem();
    }
    catch (...) {
        err = geopm::exception_handler(std::current_exception(), true);
    }
    return err;
}

int geopm_endpoint_destroy_shmem(struct geopm_endpoint_c *endpoint)
{
    printf("Endpoint geopm_endpoint_destroy_shmem().\n");
    int err = 0;
    geopm::Endpoint *endpoint_obj = (geopm::Endpoint *)endpoint;
    try {
        endpoint_obj->destroy_shmem();
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
        endpoint_obj->policy(std::vector<double>(policy_array, policy_array + num_policy));
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
        std::vector<double> samples = endpoint_obj->sample();
        *sample_age_sec = -1.0; /// @todo Implement me.
        std::copy(samples.begin(), samples.end(), sample_array);
    }
    catch (...) {
        err = geopm::exception_handler(std::current_exception(), true);
    }
    return err;
}

