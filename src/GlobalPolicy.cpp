/*
 * Copyright (c) 2015, 2016, Intel Corporation
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
#include <fstream>
#include <streambuf>
#include <stdexcept>
#include <string>
#include <json-c/json.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <system_error>
#include <unistd.h>

#include "geopm_policy.h"
#include "Exception.hpp"
#include "GlobalPolicy.hpp"
#include "Platform.hpp"
#include "PlatformFactory.hpp"
#include "config.h"

extern "C"
{
    int geopm_policy_create(const char *in_config,
                            const char *out_config,
                            struct geopm_policy_c **policy)
    {
        int err = 0;
        *policy = NULL;

        try {
            *policy = (struct geopm_policy_c *)
                      (new geopm::GlobalPolicy(std::string(in_config ? in_config : ""),
                                               std::string(out_config ? out_config : "")));
        }
        catch (...) {
            err = geopm::exception_handler(std::current_exception());
        }

        return err;
    }

    int geopm_policy_destroy(struct geopm_policy_c *policy)
    {
        int err = 0;

        try {
            geopm::GlobalPolicy *policy_obj = (geopm::GlobalPolicy *)policy;
            if (policy_obj == NULL) {
                throw geopm::Exception(GEOPM_ERROR_POLICY_NULL, __FILE__, __LINE__);
            }
            delete policy_obj;
        }
        catch (...) {
            err = geopm::exception_handler(std::current_exception());
        }

        return err;
    }

    int geopm_policy_power(struct geopm_policy_c *policy, int power_budget)
    {
        int err = 0;

        try {
            geopm::GlobalPolicy *policy_obj = (geopm::GlobalPolicy *)policy;
            if (policy_obj == NULL) {
                throw geopm::Exception(GEOPM_ERROR_POLICY_NULL, __FILE__, __LINE__);
            }
            policy_obj->budget_watts(power_budget);
        }
        catch (...) {
            err = geopm::exception_handler(std::current_exception());
        }

        return err;
    }

    int geopm_policy_mode(struct geopm_policy_c *policy, int mode)
    {
        int err = 0;

        try {
            geopm::GlobalPolicy *policy_obj = (geopm::GlobalPolicy *)policy;
            if (policy_obj == NULL) {
                throw geopm::Exception(GEOPM_ERROR_POLICY_NULL, __FILE__, __LINE__);
            }
            policy_obj->mode(mode);
        }
        catch (...) {
            err = geopm::exception_handler(std::current_exception());
        }

        return err;
    }

    int geopm_policy_cpu_freq(struct geopm_policy_c *policy, int cpu_mhz)
    {
        int err = 0;

        try {
            geopm::GlobalPolicy *policy_obj = (geopm::GlobalPolicy *)policy;
            if (policy_obj == NULL) {
                throw geopm::Exception(GEOPM_ERROR_POLICY_NULL, __FILE__, __LINE__);
            }
            policy_obj->frequency_mhz(cpu_mhz);
        }
        catch (...) {
            err = geopm::exception_handler(std::current_exception());
        }

        return err;
    }
    int geopm_policy_full_perf(struct geopm_policy_c *policy, int num_cpu_full_perf)
    {
        int err = 0;

        try {
            geopm::GlobalPolicy *policy_obj = (geopm::GlobalPolicy *)policy;
            if (policy_obj == NULL) {
                throw geopm::Exception(GEOPM_ERROR_POLICY_NULL, __FILE__, __LINE__);
            }
            policy_obj->num_max_perf(num_cpu_full_perf);
        }
        catch (...) {
            err = geopm::exception_handler(std::current_exception());
        }

        return err;
    }

    int geopm_policy_tdp_percent(struct geopm_policy_c *policy, int percent)
    {
        int err = 0;

        try {
            geopm::GlobalPolicy *policy_obj = (geopm::GlobalPolicy *)policy;
            if (policy_obj == NULL) {
                throw geopm::Exception(GEOPM_ERROR_POLICY_NULL, __FILE__, __LINE__);
            }
            policy_obj->tdp_percent(percent);
        }
        catch (...) {
            err = geopm::exception_handler(std::current_exception());
        }

        return err;
    }

    int geopm_policy_affinity(struct geopm_policy_c *policy, int cpu_affinity)
    {
        int err = 0;

        try {
            geopm::GlobalPolicy *policy_obj = (geopm::GlobalPolicy *)policy;
            if (policy_obj == NULL) {
                throw geopm::Exception(GEOPM_ERROR_POLICY_NULL, __FILE__, __LINE__);
            }
            policy_obj->affinity(cpu_affinity);
        }
        catch (...) {
            err = geopm::exception_handler(std::current_exception());
        }

        return err;
    }

    int geopm_policy_goal(struct geopm_policy_c *policy, int geo_goal)
    {
        int err = 0;

        try {
            geopm::GlobalPolicy *policy_obj = (geopm::GlobalPolicy *)policy;
            if (policy_obj == NULL) {
                throw geopm::Exception(GEOPM_ERROR_POLICY_NULL, __FILE__, __LINE__);
            }
            policy_obj->goal(geo_goal);
        }
        catch (...) {
            err = geopm::exception_handler(std::current_exception());
        }

        return err;
    }

    int geopm_policy_tree_decider(struct geopm_policy_c *policy, const char *description)
    {
        int err = 0;

        try {
            geopm::GlobalPolicy *policy_obj = (geopm::GlobalPolicy *)policy;
            if (policy_obj == NULL) {
                throw geopm::Exception(GEOPM_ERROR_POLICY_NULL, __FILE__, __LINE__);
            }
            policy_obj->tree_decider(std::string(description));
        }
        catch (...) {
            err = geopm::exception_handler(std::current_exception());
        }

        return err;
    }

    int geopm_policy_leaf_decider(struct geopm_policy_c *policy, const char *description)
    {
        int err = 0;

        try {
            geopm::GlobalPolicy *policy_obj = (geopm::GlobalPolicy *)policy;
            if (policy_obj == NULL) {
                throw geopm::Exception(GEOPM_ERROR_POLICY_NULL, __FILE__, __LINE__);
            }
            policy_obj->leaf_decider(std::string(description));
        }
        catch (...) {
            err = geopm::exception_handler(std::current_exception());
        }

        return err;
    }

    int geopm_policy_platform(struct geopm_policy_c *policy, const char *description)
    {
        int err = 0;

        try {
            geopm::GlobalPolicy *policy_obj = (geopm::GlobalPolicy *)policy;
            if (policy_obj == NULL) {
                throw geopm::Exception(GEOPM_ERROR_POLICY_NULL, __FILE__, __LINE__);
            }
            policy_obj->platform(std::string(description));
        }
        catch (...) {
            err = geopm::exception_handler(std::current_exception());
        }

        return err;
    }

    int geopm_policy_write(const struct geopm_policy_c *policy)
    {
        int err = 0;

        try {
            geopm::GlobalPolicy *policy_obj = (geopm::GlobalPolicy *)policy;
            if (policy_obj == NULL) {
                throw geopm::Exception(GEOPM_ERROR_POLICY_NULL, __FILE__, __LINE__);
            }
            policy_obj->write();
        }
        catch (...) {
            err = geopm::exception_handler(std::current_exception());
        }

        return err;
    }

    int geopm_policy_enforce_static(const struct geopm_policy_c *policy)
    {
        int err = 0;

        try {
            geopm::GlobalPolicy *policy_obj = (geopm::GlobalPolicy *)policy;

            if (policy_obj == NULL) {
                throw geopm::Exception(GEOPM_ERROR_POLICY_NULL, __FILE__, __LINE__);
            }
            policy_obj->enforce_static_mode();
        }
        catch (...) {
            err = geopm::exception_handler(std::current_exception());
        }
        return err;
    }
}


namespace geopm
{
    GlobalPolicy::GlobalPolicy(std::string in_config, std::string out_config)
        :m_in_config(in_config)
        ,m_out_config(out_config)
        ,m_mode(-1)
        ,m_power_budget_watts(-1)
        ,m_flags(0)
        ,m_tree_decider("power_balancing")
        ,m_leaf_decider("power_governing")
        ,m_platform("rapl")
        ,m_is_shm_in(false)
        ,m_is_shm_out(false)
        ,m_do_read(false)
        ,m_do_write(false)
    {
        int shm_id;
        int err = 0;

        if (m_in_config.empty() && m_out_config.empty()) {
            throw Exception("GlobalPolicy: at least one of in_config or out_config must be set", GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }

        if (!m_out_config.empty()) {
            m_do_write = true;
            if (m_out_config[0] == '/' && m_out_config.find_last_of('/') == 0) {
                m_is_shm_out = true;
                mode_t old_mask = umask(0);
                shm_id = shm_open(m_out_config.c_str(), O_RDWR | O_CREAT | O_EXCL, S_IRUSR | S_IWUSR | S_IRGRP| S_IWGRP | S_IROTH| S_IWOTH);
                if (shm_id < 0) {
                    throw Exception("GlobalPolicy: Could not open shared memory region for root policy", errno, __FILE__, __LINE__);
                }

                err = ftruncate(shm_id, sizeof(struct m_policy_shmem_s));
                if (err) {
                    (void) shm_unlink(m_out_config.c_str());
                    (void) close(shm_id);
                    (void) umask(old_mask);
                    throw Exception("GlobalPolicy: Could not extend shared memory region with ftruncate for policy control", errno, __FILE__, __LINE__);
                }

                m_policy_shmem_out = (struct m_policy_shmem_s *) mmap(NULL, sizeof(struct m_policy_shmem_s),
                                     PROT_READ | PROT_WRITE, MAP_SHARED, shm_id, 0);
                if (m_policy_shmem_out == MAP_FAILED) {
                    (void) close(shm_id);
                    (void) umask(old_mask);
                    throw Exception("GlobalPolicy: Could not map shared memory region for root policy", errno, __FILE__, __LINE__);
                }
                err = close(shm_id);
                if (err) {
                    munmap(m_policy_shmem_out, sizeof(struct m_policy_shmem_s));
                    (void) umask(old_mask);
                    throw Exception("GlobalPolicy: Could not close file descriptor for root policy shared memory region", errno, __FILE__, __LINE__);
                }
                if (pthread_mutex_init(&(m_policy_shmem_out->lock), NULL) != 0) {
                    munmap(m_policy_shmem_out, sizeof(struct m_policy_shmem_s));
                    (void) umask(old_mask);
                    throw Exception("GlobalPolicy: Could not initialize pthread mutex for shared memory region", errno, __FILE__, __LINE__);
                }
                umask(old_mask);
            }
            else if (m_in_config == m_out_config) {
                throw Exception("GlobalPolicy::GlobalPolicy(): input config file and output config file cannot be the same", GEOPM_ERROR_INVALID, __FILE__, __LINE__);
            }
        }

        if (!m_in_config.empty()) {
            m_do_read = true;
            if (m_in_config[0] == '/' && m_in_config.find_last_of('/') == 0) {
                m_is_shm_in = true;
                shm_id = shm_open(m_in_config.c_str(), O_RDWR, 0);

                if (shm_id < 0) {
                    throw Exception("GlobalPolicy: Could not open shared memory region for root policy", errno, __FILE__, __LINE__);
                }
                m_policy_shmem_in = (struct m_policy_shmem_s *) mmap(NULL, sizeof(struct m_policy_shmem_s),
                                    PROT_READ | PROT_WRITE, MAP_SHARED, shm_id, 0);
                if (m_policy_shmem_in == MAP_FAILED) {
                    (void) close(shm_id);
                    throw Exception("GlobalPolicy: Could not map shared memory region for root policy", errno, __FILE__, __LINE__);
                }
                err = close(shm_id);
                if (err) {
                    munmap(m_policy_shmem_in, sizeof(struct m_policy_shmem_s));
                    throw Exception("GlobalPolicy: Could not close file descriptor for root policy shared memory region", errno, __FILE__, __LINE__);
                }
            }
            read();
        }
    }

    GlobalPolicy::~GlobalPolicy()
    {
        if (m_do_read) {
            if(m_is_shm_in) {
                if (munmap(m_policy_shmem_in, sizeof(struct geopm_policy_message_s))) {
                    throw Exception("GlobalPolicy: Could not unmap root policy shared memory region", errno, __FILE__, __LINE__);
                }
            }
        }
        if (m_do_write) {
            if(m_is_shm_out) {
                if (munmap(m_policy_shmem_out, sizeof(struct geopm_policy_message_s))) {
                    throw Exception("GlobalPolicy: Could not unmap root policy shared memory region", errno, __FILE__, __LINE__);
                }
                if (shm_unlink(m_out_config.c_str())) {
                    throw Exception("GlobalPolicy: Could not unlink shared memory region on GlobalPolicy destruction", errno, __FILE__, __LINE__);
                }
            }
        }
    }

    int GlobalPolicy::mode(void) const
    {
        return m_mode;
    }

    int GlobalPolicy::frequency_mhz(void) const
    {
        return m_flags.frequency_mhz();;
    }

    int GlobalPolicy::tdp_percent(void) const
    {
        return m_flags.tdp_percent();
    }

    int GlobalPolicy::budget_watts(void) const
    {
        return m_power_budget_watts;
    }

    int GlobalPolicy::affinity(void) const
    {
        return m_flags.affinity();;
    }

    int GlobalPolicy::goal(void) const
    {
        return m_flags.goal();
    }

    int GlobalPolicy::num_max_perf(void) const
    {
        return m_flags.num_max_perf();
    }

    const std::string &GlobalPolicy::tree_decider() const
    {
        return m_tree_decider;
    }

    const std::string &GlobalPolicy::leaf_decider() const
    {
        return m_leaf_decider;
    }

    const std::string &GlobalPolicy::platform() const
    {
        return m_platform;
    }

    void GlobalPolicy::policy_message(struct geopm_policy_message_s &policy_message)
    {
        if (m_is_shm_in) {
            read();
        }
        policy_message.mode = m_mode;
        policy_message.power_budget = m_power_budget_watts;
        policy_message.flags = m_flags.flags();
    }

    void GlobalPolicy::mode(int mode)
    {
        m_mode = mode;
    }

    void GlobalPolicy::frequency_mhz(int frequency)
    {
        m_flags.frequency_mhz(frequency);
    }

    void GlobalPolicy::tdp_percent(int percentage)
    {
        m_flags.tdp_percent(percentage);
    }

    void GlobalPolicy::budget_watts(int budget)
    {
        m_power_budget_watts = budget;
    }

    void GlobalPolicy::affinity(int affinity)
    {
        m_flags.affinity(affinity);;
    }

    void GlobalPolicy::goal(int geo_goal)
    {
        m_flags.goal(geo_goal);
    }

    void GlobalPolicy::num_max_perf(int num_big_cores)
    {
        m_flags.num_max_perf(num_big_cores);
    }

    void GlobalPolicy::tree_decider(const std::string &description)
    {
        m_tree_decider = description;
    }

    void GlobalPolicy::leaf_decider(const std::string &description)
    {
        m_leaf_decider = description;
    }

    void GlobalPolicy::platform(const std::string &description)
    {
        m_platform = description;
    }

    void GlobalPolicy::read(void)
    {
        if (m_is_shm_in) {
            int err;
            err = pthread_mutex_lock(&(m_policy_shmem_in->lock));
            if (err) {
                throw Exception("GlobalPolicy: Could not lock shared memory region for root of tree", err, __FILE__, __LINE__);
            }
            m_mode = m_policy_shmem_in->policy.mode;
            m_power_budget_watts = m_policy_shmem_in->policy.power_budget;
            m_flags.flags(m_policy_shmem_in->policy.flags);
            m_tree_decider.assign(m_policy_shmem_in->plugin.tree_decider);
            m_leaf_decider.assign(m_policy_shmem_in->plugin.leaf_decider);
            m_platform.assign(m_policy_shmem_in->plugin.platform);
            err = pthread_mutex_unlock(&(m_policy_shmem_in->lock));
            if (err) {
                throw Exception("GlobalPolicy: Could not unlock shared memory region for root of tree", err, __FILE__, __LINE__);
            }
        }
        else if (m_do_read) {
            std::string policy_string;
            std::string value_string;
            std::string key_string;
            std::string err_string;
            json_object *object;
            json_object *options_obj = NULL;
            json_object *mode_obj = NULL;
            enum json_type type;
            std::ifstream config_file_in;

            config_file_in.open(m_in_config.c_str(), std::ifstream::in);
            if (!config_file_in.is_open()) {
                throw Exception("GlobalPolicy: input configuration file \"" + m_in_config + "\" could not be opened", GEOPM_ERROR_INVALID, __FILE__, __LINE__);
            }
            config_file_in.seekg(0, std::ios::end);
            size_t file_size = config_file_in.tellg();
            if (file_size <= 0) {
                throw Exception("GlobalPolicy: input configuration file invalid", GEOPM_ERROR_INVALID, __FILE__, __LINE__);
            }
            policy_string.reserve(file_size);
            config_file_in.seekg(0, std::ios::beg);

            policy_string.assign((std::istreambuf_iterator<char>(config_file_in)),
                                 std::istreambuf_iterator<char>());

            object = json_tokener_parse(policy_string.c_str());

            type = json_object_get_type(object);

            if (type != json_type_object ) {
                throw Exception("GlobalPolicy: detected a malformed json config file", GEOPM_ERROR_FILE_PARSE, __FILE__, __LINE__);
            }

            json_object_object_foreach(object, key, val) {
                if (!strncmp(key, "mode", strlen("mode") + 1)) {
                    mode_obj = val;
                }
                else if (!strncmp(key, "options", strlen("options") + 1)) {
                    options_obj = val;
                }
                else {
                    throw Exception("GlobalPolicy: unsupported key or malformed json config file", GEOPM_ERROR_FILE_PARSE, __FILE__, __LINE__);
                }
            }

            if (mode_obj == NULL || options_obj == NULL) {
                throw Exception("GlobalPolicy: config file does not contain a mode or options", GEOPM_ERROR_FILE_PARSE, __FILE__, __LINE__);
            }
            if (json_object_get_type(mode_obj) != json_type_string) {
                throw Exception("GlobalPolicy: mode expected to be a string type", GEOPM_ERROR_FILE_PARSE, __FILE__, __LINE__);
            }
            if (json_object_get_type(options_obj) != json_type_object) {
                throw Exception("GlobalPolicy: mode expected to be an object type", GEOPM_ERROR_FILE_PARSE, __FILE__, __LINE__);
            }

            value_string.assign(json_object_get_string(mode_obj));
            if (!value_string.compare("tdp_balance_static")) {
                m_mode = GEOPM_POLICY_MODE_TDP_BALANCE_STATIC;
            }
            else if (!value_string.compare("freq_uniform_static")) {
                m_mode = GEOPM_POLICY_MODE_FREQ_UNIFORM_STATIC;
            }
            else if (!value_string.compare("freq_hybrid_static")) {
                m_mode = GEOPM_POLICY_MODE_FREQ_HYBRID_STATIC;
            }
            else if (!value_string.compare("perf_balance_dynamic")) {
                m_mode = GEOPM_POLICY_MODE_PERF_BALANCE_DYNAMIC;
            }
            else if (!value_string.compare("freq_uniform_dynamic")) {
                m_mode = GEOPM_POLICY_MODE_FREQ_UNIFORM_DYNAMIC;
            }
            else if (!value_string.compare("freq_hybrid_dynamic")) {
                m_mode = GEOPM_POLICY_MODE_FREQ_HYBRID_DYNAMIC;
            }

            json_object_object_foreach(options_obj, subkey, subval) {
                key_string.assign(subkey);
                if (!key_string.compare("tdp_percent")) {
                    if (json_object_get_type(subval) != json_type_int) {
                        throw Exception("GlobalPolicy: tdp_percent expected to be an integer type", GEOPM_ERROR_FILE_PARSE, __FILE__, __LINE__);
                    }
                    tdp_percent(json_object_get_int(subval));
                }
                else if (!key_string.compare("cpu_mhz")) {
                    if (json_object_get_type(subval) != json_type_int) {
                        throw Exception("GlobalPolicy: cpu_mhz expected to be an integer type", GEOPM_ERROR_FILE_PARSE, __FILE__, __LINE__);
                    }
                    frequency_mhz(json_object_get_int(subval));
                }
                else if (!key_string.compare("num_cpu_max_perf")) {
                    if (json_object_get_type(subval) != json_type_int) {
                        throw Exception("GlobalPolicy: num_cpu_max_perf expected to be an integer type", GEOPM_ERROR_FILE_PARSE, __FILE__, __LINE__);
                    }
                    num_max_perf(json_object_get_int(subval));
                }
                else if (!key_string.compare("affinity")) {
                    if (json_object_get_type(subval) != json_type_string) {
                        throw Exception("GlobalPolicy: affinity expected to be a string type", GEOPM_ERROR_FILE_PARSE, __FILE__, __LINE__);
                    }
                    value_string.assign(json_object_get_string(subval));
                    if (!value_string.compare("compact")) {
                        affinity(GEOPM_POLICY_AFFINITY_COMPACT);
                    }
                    else if (!value_string.compare("scatter")) {
                        affinity(GEOPM_POLICY_AFFINITY_SCATTER);
                    }
                    else {
                        err_string.assign("unsupported affinity type : ");
                        err_string.append(value_string);
                        throw Exception(("GlobalPolicy: " + err_string).c_str(), GEOPM_ERROR_FILE_PARSE, __FILE__, __LINE__);
                    }
                }
                else if (!key_string.compare("power_budget")) {
                    if (json_object_get_type(subval) != json_type_int) {
                        throw Exception("GlobalPolicy: power_budget expected to be an integer type", GEOPM_ERROR_FILE_PARSE, __FILE__, __LINE__);
                    }
                    budget_watts(json_object_get_int(subval));
                }
                else if (!key_string.compare("tree_decider")) {
                    if (json_object_get_type(subval) != json_type_string) {
                        throw Exception("GlobalPolicy: tree_decider expected to be a string type", GEOPM_ERROR_FILE_PARSE, __FILE__, __LINE__);
                    }
                    value_string.assign(json_object_get_string(subval));
                    tree_decider(value_string);
                }
                else if (!key_string.compare("leaf_decider")) {
                    if (json_object_get_type(subval) != json_type_string) {
                        throw Exception("GlobalPolicy: leaf_decider expected to be a string type", GEOPM_ERROR_FILE_PARSE, __FILE__, __LINE__);
                    }
                    value_string.assign(json_object_get_string(subval));
                    leaf_decider(value_string);
                }
                else if (!key_string.compare("platform")) {
                    if (json_object_get_type(subval) != json_type_string) {
                        throw Exception("GlobalPolicy: platform expected to be a string type", GEOPM_ERROR_FILE_PARSE, __FILE__, __LINE__);
                    }
                    value_string.assign(json_object_get_string(subval));
                    platform(value_string);
                }
                else {
                    err_string.assign("unknown option : ");
                    err_string.append(key_string);
                    throw Exception(("GlobalPolicy: " + err_string).c_str(), GEOPM_ERROR_FILE_PARSE, __FILE__, __LINE__);
                }
            }

            if (m_mode == GEOPM_POLICY_MODE_TDP_BALANCE_STATIC) {
                if (tdp_percent() < 0 || tdp_percent() > 100) {
                    throw Exception("GlobalPolicy: percent tdp must be between 0 and 100", GEOPM_ERROR_FILE_PARSE, __FILE__, __LINE__);
                }
            }
            if (m_mode == GEOPM_POLICY_MODE_FREQ_UNIFORM_STATIC) {
                if (frequency_mhz() < 0) {
                    throw Exception("GlobalPolicy: frequency is out of bounds", GEOPM_ERROR_FILE_PARSE, __FILE__, __LINE__);
                }
            }
            if (m_mode == GEOPM_POLICY_MODE_FREQ_HYBRID_STATIC) {
                if (frequency_mhz() < 0) {
                    throw Exception("GlobalPolicy: frequency is out of bounds", GEOPM_ERROR_FILE_PARSE, __FILE__, __LINE__);
                }
                if (num_max_perf() < 0) {
                    throw Exception("GlobalPolicy: number of max perf cpus is out of bounds", GEOPM_ERROR_FILE_PARSE, __FILE__, __LINE__);
                }
                if (affinity() != GEOPM_POLICY_AFFINITY_COMPACT &&
                    affinity() != GEOPM_POLICY_AFFINITY_SCATTER) {
                    throw Exception("GlobalPolicy: affinity must be set to 'scatter' or 'compact'", GEOPM_ERROR_FILE_PARSE, __FILE__, __LINE__);
                }
            }
            if (m_mode == GEOPM_POLICY_MODE_PERF_BALANCE_DYNAMIC) {
                if (budget_watts() < 0) {
                    throw Exception("GlobalPolicy: power budget is out of bounds", GEOPM_ERROR_FILE_PARSE, __FILE__, __LINE__);
                }
            }
            if (m_mode == GEOPM_POLICY_MODE_FREQ_UNIFORM_DYNAMIC) {
                if (budget_watts() < 0) {
                    throw Exception("GlobalPolicy: power budget is out of bounds", GEOPM_ERROR_FILE_PARSE, __FILE__, __LINE__);
                }
            }
            if (m_mode == GEOPM_POLICY_MODE_FREQ_HYBRID_DYNAMIC) {
                if (budget_watts() < 0) {
                    throw Exception("GlobalPolicy: power budget is out of bounds", GEOPM_ERROR_FILE_PARSE, __FILE__, __LINE__);
                }
                if (num_max_perf() < 0) {
                    throw Exception("GlobalPolicy: number of max perf cpus is out of bounds", GEOPM_ERROR_FILE_PARSE, __FILE__, __LINE__);
                }
                if (affinity() != GEOPM_POLICY_AFFINITY_COMPACT &&
                    affinity() != GEOPM_POLICY_AFFINITY_SCATTER) {
                    throw Exception("GlobalPolicy: affiniy must be set to 'scatter' or 'compact'", GEOPM_ERROR_FILE_PARSE, __FILE__, __LINE__);
                }
            }
            config_file_in.close();
        }
    }

    void GlobalPolicy::write()
    {
        if (!m_do_write) {
            throw Exception("GlobalPolicy: invalid operation, out_config not specified", GEOPM_ERROR_FILE_PARSE, __FILE__, __LINE__);
        }
        if (m_is_shm_out) {
            int err;

            err = pthread_mutex_lock(&(m_policy_shmem_out->lock));
            if (err) {
                throw Exception("GlobalPolicy: Could not lock shared memory region for resource manager", errno, __FILE__, __LINE__);
            }
            m_policy_shmem_out->policy.mode = m_mode;
            m_policy_shmem_out->policy.power_budget = m_power_budget_watts;
            m_policy_shmem_out->policy.flags = m_flags.flags();
            m_policy_shmem_out->plugin.tree_decider[NAME_MAX - 1] = '\0';
            strncpy(m_policy_shmem_out->plugin.tree_decider, m_tree_decider.c_str(), NAME_MAX - 1);
            m_policy_shmem_out->plugin.leaf_decider[NAME_MAX - 1] = '\0';
            strncpy(m_policy_shmem_out->plugin.leaf_decider, m_leaf_decider.c_str(), NAME_MAX - 1);
            m_policy_shmem_out->plugin.platform[NAME_MAX - 1] = '\0';
            strncpy(m_policy_shmem_out->plugin.platform, m_platform.c_str(), NAME_MAX - 1);
            err = pthread_mutex_unlock(&(m_policy_shmem_in->lock));
            if (err) {
                throw Exception("GlobalPolicy: Could not unlock shared memory region for resource manager", errno, __FILE__, __LINE__);
            }
        }
        else {
            json_object *policy, *options;
            std::string affinity_name;

            policy = json_object_new_object();
            options = json_object_new_object();

            switch (m_mode) {
                case GEOPM_POLICY_MODE_SHUTDOWN:
                    break;
                case GEOPM_POLICY_MODE_TDP_BALANCE_STATIC:
                    json_object_object_add(policy,"mode",json_object_new_string("tdp_balance_static"));
                    json_object_object_add(options,"tdp_percent",json_object_new_int(tdp_percent()));
                    json_object_object_add(policy,"options",options);
                    break;
                case GEOPM_POLICY_MODE_FREQ_UNIFORM_STATIC:
                    json_object_object_add(policy,"mode",json_object_new_string("freq_uniform_static"));
                    json_object_object_add(options,"cpu_mhz",json_object_new_int(frequency_mhz()));
                    json_object_object_add(policy,"options",options);
                    break;
                case GEOPM_POLICY_MODE_FREQ_HYBRID_STATIC:
                    json_object_object_add(policy,"mode",json_object_new_string("freq_hybrid_static"));
                    json_object_object_add(options,"cpu_mhz",json_object_new_int(frequency_mhz()));
                    json_object_object_add(options,"num_cpu_max_perf",json_object_new_int(num_max_perf()));
                    affinity_string(affinity(), affinity_name);
                    json_object_object_add(options,"affinity",json_object_new_string(affinity_name.c_str()));
                    json_object_object_add(policy,"options",options);
                    break;
                case GEOPM_POLICY_MODE_PERF_BALANCE_DYNAMIC:
                    json_object_object_add(policy,"mode",
                                           json_object_new_string("perf_balance_dynamic"));
                    json_object_object_add(options,"tree_decider",
                                           json_object_new_string(m_tree_decider.c_str()));
                    json_object_object_add(options,"leaf_decider",
                                           json_object_new_string(m_leaf_decider.c_str()));
                    json_object_object_add(options,"platform",
                                           json_object_new_string(m_platform.c_str()));
                    json_object_object_add(options,"power_budget",json_object_new_int(budget_watts()));
                    json_object_object_add(policy,"options",options);
                    break;
                case GEOPM_POLICY_MODE_FREQ_UNIFORM_DYNAMIC:
                    json_object_object_add(policy,"mode",
                                           json_object_new_string("freq_uniform_dynamic"));
                    json_object_object_add(options,"tree_decider",
                                           json_object_new_string(m_tree_decider.c_str()));
                    json_object_object_add(options,"leaf_decider",
                                           json_object_new_string(m_leaf_decider.c_str()));
                    json_object_object_add(options,"platform",
                                           json_object_new_string(m_platform.c_str()));
                    json_object_object_add(options,"power_budget",json_object_new_int(budget_watts()));
                    json_object_object_add(policy,"options",options);
                    break;
                case GEOPM_POLICY_MODE_FREQ_HYBRID_DYNAMIC:
                    json_object_object_add(policy,"mode",
                                           json_object_new_string("freq_hybrid_dynamic"));
                    json_object_object_add(options,"tree_decider",
                                           json_object_new_string(m_tree_decider.c_str()));
                    json_object_object_add(options,"leaf_decider",
                                           json_object_new_string(m_leaf_decider.c_str()));
                    json_object_object_add(options,"platform",
                                           json_object_new_string(m_platform.c_str()));
                    json_object_object_add(options,"power_budget",json_object_new_int(budget_watts()));
                    json_object_object_add(options,"num_cpu_max_perf",json_object_new_int(num_max_perf()));
                    affinity_string(affinity(), affinity_name);
                    json_object_object_add(options,"affinity",json_object_new_string(affinity_name.c_str()));
                    json_object_object_add(policy,"options",options);
                    break;
                default:
                    throw Exception("GlobalPolicy: invalid mode specified", GEOPM_ERROR_FILE_PARSE, __FILE__, __LINE__);
            }
            std::ofstream config_file_out;
            config_file_out.open(m_out_config.c_str(), std::ifstream::out);
            config_file_out << json_object_to_json_string(policy);
            config_file_out.close();
        }
    }

    void GlobalPolicy::affinity_string(int value, std::string &name)
    {
        switch (value) {
            case GEOPM_POLICY_AFFINITY_COMPACT:
                name.assign("compact");
                break;
            case GEOPM_POLICY_AFFINITY_SCATTER:
                name.assign("scatter");
                break;
            default:
                throw Exception("GlobalPolicy: invalid affinity specified", GEOPM_ERROR_FILE_PARSE, __FILE__, __LINE__);
                break;
        }
    }

    void GlobalPolicy::enforce_static_mode()
    {
        PlatformFactory platform_factory;
        Platform *platform = platform_factory.platform(std::string("rapl"));

        switch (m_mode) {
            case GEOPM_POLICY_MODE_TDP_BALANCE_STATIC:
                platform->tdp_limit(tdp_percent());
                break;
            case GEOPM_POLICY_MODE_FREQ_UNIFORM_STATIC:
                platform->manual_frequency(frequency_mhz(), 0, GEOPM_POLICY_AFFINITY_SCATTER);
                break;
            case GEOPM_POLICY_MODE_FREQ_HYBRID_STATIC:
                platform->manual_frequency(frequency_mhz(), num_max_perf(), affinity());
                break;
            default:
                throw Exception("GlobalPolicy: invalid mode specified", GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        };
    }
}
