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
#include <fstream>
#include <streambuf>
#include <stdexcept>
#include <string>
#include <sstream>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <system_error>
#include <unistd.h>

#include "contrib/json11/json11.hpp"

#include "geopm_policy.h"
#include "Exception.hpp"
#include "GlobalPolicy.hpp"
#include "Platform.hpp"
#include "PlatformFactory.hpp"
#include "geopm_version.h"
#include "geopm_env.h"
#include "config.h"

using json11::Json;

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

    int geopm_policy_tdp_percent(struct geopm_policy_c *policy, double percent)
    {
        int err = 0;

        try {
            geopm::GlobalPolicy *policy_obj = (geopm::GlobalPolicy *)policy;
            if (policy_obj == NULL) {
                throw geopm::Exception(GEOPM_ERROR_POLICY_NULL, __FILE__, __LINE__);
            }
            // round the value to the closest integer
            policy_obj->tdp_percent((int)(percent + 0.5));
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
        : m_in_config(in_config)
        , m_out_config(out_config)
        , m_mode(GEOPM_POLICY_MODE_STATIC)
        , m_power_budget_watts(-1)
        , m_flags(nullptr)
        , m_tree_decider("none")
        , m_leaf_decider("none")
        , m_platform("rapl")
        , m_is_shm_in(false)
        , m_is_shm_out(false)
        , m_do_read(false)
        , m_do_write(false)
    {
        int shm_id;
        int err = 0;

        m_flags = std::unique_ptr<IPolicyFlags>(new PolicyFlags(0));
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
        if (m_in_config.empty() && m_out_config.empty()) {
            m_tree_decider = "static_policy";
            m_leaf_decider = "static_policy";
        }
    }

    GlobalPolicy::~GlobalPolicy()
    {
        if (m_do_read && m_is_shm_in) {
            if (munmap(m_policy_shmem_in, sizeof(struct geopm_policy_message_s))) {
#ifdef GEOPM_DEBUG
                std::cerr << "Warning: " << Exception("GlobalPolicy: Could not unmap root policy shared memory region",
                                                      errno ? errno : GEOPM_ERROR_RUNTIME, __FILE__, __LINE__).what() << std::endl;
#endif
            }
        }
        if (m_do_write && m_is_shm_out) {
            if (munmap(m_policy_shmem_out, sizeof(struct geopm_policy_message_s))) {
#ifdef GEOPM_DEBUG
                std::cerr << "Warning: " << Exception("GlobalPolicy: Could not unmap root policy shared memory region",
                                                      errno ? errno : GEOPM_ERROR_RUNTIME, __FILE__, __LINE__).what() << std::endl;
#endif
            }
            if (shm_unlink(m_out_config.c_str())) {
#ifdef GEOPM_DEBUG
                std::cerr << "Warning: " << Exception("GlobalPolicy: Could not unlink shared memory region on GlobalPolicy destruction",
                                                      errno ? errno : GEOPM_ERROR_RUNTIME, __FILE__, __LINE__).what() << std::endl;
#endif
            }
        }
    }

    int GlobalPolicy::mode(void) const
    {
        return m_mode;
    }

    int GlobalPolicy::frequency_mhz(void) const
    {
        return m_flags->frequency_mhz();;
    }

    int GlobalPolicy::tdp_percent(void) const
    {
        return m_flags->tdp_percent();
    }

    int GlobalPolicy::budget_watts(void) const
    {
        return m_power_budget_watts;
    }

    int GlobalPolicy::affinity(void) const
    {
        return m_flags->affinity();;
    }

    int GlobalPolicy::goal(void) const
    {
        return m_flags->goal();
    }

    int GlobalPolicy::num_max_perf(void) const
    {
        return m_flags->num_max_perf();
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
        policy_message.flags = m_flags->flags();
    }

    void GlobalPolicy::mode(int mode)
    {
        m_mode = mode;
    }

    void GlobalPolicy::frequency_mhz(int frequency)
    {
        m_flags->frequency_mhz(frequency);
    }

    void GlobalPolicy::tdp_percent(int percentage)
    {
        m_flags->tdp_percent(percentage);
    }

    void GlobalPolicy::budget_watts(int budget)
    {
        m_power_budget_watts = budget;
    }

    void GlobalPolicy::affinity(int affinity)
    {
        m_flags->affinity(affinity);;
    }

    void GlobalPolicy::goal(int geo_goal)
    {
        m_flags->goal(geo_goal);
    }

    void GlobalPolicy::num_max_perf(int num_big_cores)
    {
        m_flags->num_max_perf(num_big_cores);
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
            read_shm();
        }
        else if (m_do_read) {
            read_json();
        }
        check_valid();
    }

    void GlobalPolicy::read_json(void)
    {
        std::string policy_string;
        std::string value_string;
        std::string key_string;
        std::string err_string;
        std::ifstream config_file_in;

        config_file_in.open(m_in_config, std::ifstream::in);
        if (!config_file_in.is_open()) {
            std::ostringstream ex_str;
            ex_str << "GlobalPolicy::read(): input configuration file \""
                   << m_in_config << "\" could not be opened";
            throw Exception(ex_str.str(), GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        config_file_in.seekg(0, std::ios::end);
        size_t file_size = config_file_in.tellg();
        if (file_size <= 0) {
            throw Exception("GlobalPolicy::read(): input configuration file invalid",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        policy_string.reserve(file_size);
        config_file_in.seekg(0, std::ios::beg);

        policy_string.assign((std::istreambuf_iterator<char>(config_file_in)),
                             std::istreambuf_iterator<char>());

        std::string err;
        Json root = Json::parse(policy_string, err);
        if (!err.empty() || !root.is_object()) {
            throw Exception("GlobalPolicy::read(): detected a malformed json config file: " + err,
                            GEOPM_ERROR_FILE_PARSE, __FILE__, __LINE__);
        }

        Json mode_obj;
        Json options_obj;
        for (const auto &obj : root.object_items()) {
            if (obj.first == "mode") {
                mode_obj = obj.second;
            }
            else if (obj.first == "options") {
                options_obj = obj.second;
            }
            else {
                throw Exception("GlobalPolicy::read(): unsupported key or malformed json config file",
                                GEOPM_ERROR_FILE_PARSE, __FILE__, __LINE__);
            }
        }
        if (!mode_obj.is_null()) {
            read_json_mode(&mode_obj);
        }
        if (!options_obj.is_null()) {
            read_json_options(&options_obj);
        }
        config_file_in.close();
    }

    void GlobalPolicy::read_json_options(Json *options_obj)
    {
        std::string value_string;

        if (!options_obj->is_object()) {
            throw Exception("GlobalPolicy::read(): options expected to be an object type",
                            GEOPM_ERROR_FILE_PARSE, __FILE__, __LINE__);
        }

        for (const auto &obj : options_obj->object_items()) {
            std::string key_string = obj.first;
            Json subval = obj.second;

            if (key_string == "tdp_percent") {
                if (subval.is_number() && floor(subval.number_value()) == subval.number_value()) {
                    tdp_percent(subval.number_value());
                }
                else {
                    throw Exception("GlobalPolicy::read(): tdp_percent expected to be a double type",
                                    GEOPM_ERROR_FILE_PARSE, __FILE__, __LINE__);
                }
            }
            else if (key_string == "cpu_mhz") {
                if (subval.is_number() && floor(subval.number_value()) == subval.number_value()) {
                    frequency_mhz(subval.number_value());
                } else {
                    throw Exception("GlobalPolicy::read(): cpu_mhz expected to be an integer type",
                                    GEOPM_ERROR_FILE_PARSE, __FILE__, __LINE__);
                }
            }
            else if (key_string == "num_cpu_max_perf") {
                if (subval.is_number() && floor(subval.number_value()) == subval.number_value()) {
                    num_max_perf(subval.number_value());
                }
                else {
                    throw Exception("GlobalPolicy::read(): num_cpu_max_perf expected to be an integer type",
                                    GEOPM_ERROR_FILE_PARSE, __FILE__, __LINE__);
                }
            }
            else if (key_string == "affinity") {
                if (!subval.is_string()) {
                    throw Exception("GlobalPolicy::read(): affinity expected to be a string type",
                                    GEOPM_ERROR_FILE_PARSE, __FILE__, __LINE__);
                }
                std::string value_string = subval.string_value();
                if (value_string == "compact") {
                    affinity(GEOPM_POLICY_AFFINITY_COMPACT);
                }
                else if (value_string == "scatter") {
                    affinity(GEOPM_POLICY_AFFINITY_SCATTER);
                }
                else {
                    std::ostringstream ex_str;
                    ex_str << "GlobalPolicy: unsupported affinity type: " << value_string;
                    throw Exception(ex_str.str(), GEOPM_ERROR_FILE_PARSE, __FILE__, __LINE__);
                }
            }
            else if (key_string == "power_budget") {
                if (subval.is_number() && floor(subval.number_value()) == subval.number_value()) {
                    budget_watts(subval.number_value());
                }
                else {
                    throw Exception("GlobalPolicy::read(): power_budget expected to be an integer type",
                                    GEOPM_ERROR_FILE_PARSE, __FILE__, __LINE__);
                }
            }
            else if (key_string == "tree_decider") {
                if (!subval.is_string()) {
                    throw Exception("GlobalPolicy::read(): tree_decider expected to be a string type",
                                    GEOPM_ERROR_FILE_PARSE, __FILE__, __LINE__);
                }
                std::string value_string = subval.string_value();
                tree_decider(value_string);
            }
            else if (key_string == "leaf_decider") {
                if (!subval.is_string()) {
                    throw Exception("GlobalPolicy::read(): leaf_decider expected to be a string type",
                                    GEOPM_ERROR_FILE_PARSE, __FILE__, __LINE__);
                }
                std::string value_string = subval.string_value();
                leaf_decider(value_string);
            }
            else if (key_string == "platform") {
                if (!subval.is_string()) {
                    throw Exception("GlobalPolicy::read(): platform expected to be a string type",
                                    GEOPM_ERROR_FILE_PARSE, __FILE__, __LINE__);
                }
                std::string value_string = subval.string_value();
                platform(value_string);
            }
            else {
                std::ostringstream ex_str;
                ex_str << "GlobalPolicy::read(): unknown option \"" << key_string << "\"";
                throw Exception(ex_str.str(), GEOPM_ERROR_FILE_PARSE, __FILE__, __LINE__);
            }
        }
    }

    void GlobalPolicy::check_valid(void)
    {
        if (m_mode == GEOPM_POLICY_MODE_TDP_BALANCE_STATIC) {
            if (tdp_percent() < 0 || tdp_percent() > 100) {
                throw Exception("GlobalPolicy::check_valid(): percent tdp must be between 0 and 100",
                                GEOPM_ERROR_FILE_PARSE, __FILE__, __LINE__);
            }
        }
        if (m_mode == GEOPM_POLICY_MODE_FREQ_UNIFORM_STATIC) {
            if (frequency_mhz() < 0) {
                throw Exception("GlobalPolicy::check_valid(): frequency is out of bounds",
                                GEOPM_ERROR_FILE_PARSE, __FILE__, __LINE__);
            }
        }
        if (m_mode == GEOPM_POLICY_MODE_FREQ_HYBRID_STATIC) {
            if (frequency_mhz() < 0) {
                throw Exception("GlobalPolicy::check_valid(): frequency is out of bounds",
                                GEOPM_ERROR_FILE_PARSE, __FILE__, __LINE__);
            }
            if (num_max_perf() < 0) {
                throw Exception("GlobalPolicy::check_valid(): number of max perf cpus is out of bounds",
                                GEOPM_ERROR_FILE_PARSE, __FILE__, __LINE__);
            }
            if (affinity() != GEOPM_POLICY_AFFINITY_COMPACT &&
                affinity() != GEOPM_POLICY_AFFINITY_SCATTER) {
                throw Exception("GlobalPolicy::check_valid(): affinity must be set to 'scatter' or 'compact'",
                                GEOPM_ERROR_FILE_PARSE, __FILE__, __LINE__);
            }
        }
        if (m_mode == GEOPM_POLICY_MODE_PERF_BALANCE_DYNAMIC) {
            if (budget_watts() < 0) {
                throw Exception("GlobalPolicy::check_valid(): power budget is out of bounds",
                                GEOPM_ERROR_FILE_PARSE, __FILE__, __LINE__);
            }
        }
        if (m_mode == GEOPM_POLICY_MODE_FREQ_UNIFORM_DYNAMIC) {
            if (budget_watts() < 0) {
                throw Exception("GlobalPolicy::check_valid(): power budget is out of bounds",
                                GEOPM_ERROR_FILE_PARSE, __FILE__, __LINE__);
            }
        }
        if (m_mode == GEOPM_POLICY_MODE_FREQ_HYBRID_DYNAMIC) {
            if (budget_watts() < 0) {
                throw Exception("GlobalPolicy::check_valid(): power budget is out of bounds",
                                GEOPM_ERROR_FILE_PARSE, __FILE__, __LINE__);
            }
            if (num_max_perf() < 0) {
                throw Exception("GlobalPolicy::check_valid(): number of max perf cpus is out of bounds",
                                GEOPM_ERROR_FILE_PARSE, __FILE__, __LINE__);
            }
            if (affinity() != GEOPM_POLICY_AFFINITY_COMPACT &&
                affinity() != GEOPM_POLICY_AFFINITY_SCATTER) {
                throw Exception("GlobalPolicy::check_valid(): affiniy must be set to 'scatter' or 'compact'",
                                GEOPM_ERROR_FILE_PARSE, __FILE__, __LINE__);
            }
        }
        if ((m_mode == GEOPM_POLICY_MODE_TDP_BALANCE_STATIC ||
             m_mode == GEOPM_POLICY_MODE_FREQ_UNIFORM_STATIC ||
             m_mode == GEOPM_POLICY_MODE_FREQ_HYBRID_STATIC) &&
            (m_tree_decider != "static_policy" ||
             m_leaf_decider != "static_policy")) {
            if (m_leaf_decider == "none" && m_tree_decider == "none") {
                m_leaf_decider = "static_policy";
                m_tree_decider = "static_policy";
            }
            else {
                throw Exception("GlobalPolicy::check_valid(): cannot set mode to static unless the deciders are static",
                                GEOPM_ERROR_INVALID, __FILE__, __LINE__);
            }
        }
        if ((m_mode == GEOPM_POLICY_MODE_PERF_BALANCE_DYNAMIC ||
             m_mode == GEOPM_POLICY_MODE_FREQ_UNIFORM_DYNAMIC ||
             m_mode == GEOPM_POLICY_MODE_FREQ_HYBRID_DYNAMIC) &&
            (m_tree_decider!= "power_balancing" ||
             m_leaf_decider != "power_governing")) {
            if (m_leaf_decider == "none" && m_tree_decider == "none") {
                m_leaf_decider = "power_governing";
                m_tree_decider = "power_balancing";
            }
            else {
                throw Exception("GlobalPolicy::check_valid(): dynamic mode does not match the required decider",
                                GEOPM_ERROR_INVALID, __FILE__, __LINE__);
            }
        }
        if (m_mode == GEOPM_POLICY_MODE_STATIC &&
            (m_tree_decider != "static_policy" ||
             m_leaf_decider != "static_policy")) {
            if (m_leaf_decider == "none" && m_tree_decider == "none") {
                m_leaf_decider = "static_policy";
                m_tree_decider = "static_policy";
            }
            else {
                throw Exception("GlobalPolicy::check_valid(): static mode cannnot set when either the tree or leaf decider are dynamic", GEOPM_ERROR_INVALID, __FILE__, __LINE__);
            }
        }
    }

    void GlobalPolicy::read_json_mode(Json *mode_obj)
    {
        if (!mode_obj->is_string()) {
            throw Exception("GlobalPolicy::read(): mode expected to be a string type",
                            GEOPM_ERROR_FILE_PARSE, __FILE__, __LINE__);
        }
        std::string value_string = mode_obj->string_value();
        if (value_string == "tdp_balance_static") {
            m_mode = GEOPM_POLICY_MODE_TDP_BALANCE_STATIC;
        }
        else if (value_string == "freq_uniform_static") {
            m_mode = GEOPM_POLICY_MODE_FREQ_UNIFORM_STATIC;
        }
        else if (value_string == "freq_hybrid_static") {
            m_mode = GEOPM_POLICY_MODE_FREQ_HYBRID_STATIC;
        }
        else if (value_string == "perf_balance_dynamic") {
            m_mode = GEOPM_POLICY_MODE_PERF_BALANCE_DYNAMIC;
        }
        else if (value_string == "freq_uniform_dynamic") {
            m_mode = GEOPM_POLICY_MODE_FREQ_UNIFORM_DYNAMIC;
        }
        else if (value_string == "freq_hybrid_dynamic") {
            m_mode = GEOPM_POLICY_MODE_FREQ_HYBRID_DYNAMIC;
        }
        else if (value_string == "static") {
            m_mode = GEOPM_POLICY_MODE_STATIC;
        }
        else if (value_string == "dynamic") {
            m_mode = GEOPM_POLICY_MODE_DYNAMIC;
        }
        else {
            throw Exception("GlobalPolicy: invalid mode specified", GEOPM_ERROR_FILE_PARSE, __FILE__, __LINE__);
        }
    }

    void GlobalPolicy::read_shm(void)
    {
        int err;
        err = pthread_mutex_lock(&(m_policy_shmem_in->lock));
        if (err) {
            throw Exception("GlobalPolicy::read_shm(): Could not lock shared memory region for root of tree",
                            err, __FILE__, __LINE__);
        }
        m_mode = m_policy_shmem_in->policy.mode;
        m_power_budget_watts = m_policy_shmem_in->policy.power_budget;
        m_flags->flags(m_policy_shmem_in->policy.flags);
        m_tree_decider = m_policy_shmem_in->plugin.tree_decider;
        m_leaf_decider = m_policy_shmem_in->plugin.leaf_decider;
        m_platform = m_policy_shmem_in->plugin.platform;
        err = pthread_mutex_unlock(&(m_policy_shmem_in->lock));
        if (err) {
            throw Exception("GlobalPolicy::read(): Could not unlock shared memory region for root of tree",
                            err, __FILE__, __LINE__);
        }
    }

    void GlobalPolicy::write()
    {
        if (!m_do_write) {
            throw Exception("GlobalPolicy: invalid operation, out_config not specified",
                            GEOPM_ERROR_FILE_PARSE, __FILE__, __LINE__);
        }
        check_valid();
        if (m_is_shm_out) {
            write_shm();
        }
        else {
            write_json();
        }
    }

    void GlobalPolicy::write_json(void)
    {
        Json policy;
        Json options;

        switch (m_mode) {
            case GEOPM_POLICY_MODE_SHUTDOWN:
                break;
            case GEOPM_POLICY_MODE_TDP_BALANCE_STATIC:
                policy = Json::object {
                    {"mode", "tdp_balance_static"},
                    {"options", Json::object {
                        {"tdp_percent", tdp_percent()}}}};
                break;
            case GEOPM_POLICY_MODE_FREQ_UNIFORM_STATIC:
                policy = Json::object {
                    {"mode", "freq_uniform_static"},
                    {"options", Json::object {
                        {"cpu_mhz", frequency_mhz()}}}};
                break;
            case GEOPM_POLICY_MODE_FREQ_HYBRID_STATIC:
                policy = Json::object {
                    {"mode", "freq_hybrid_static"},
                    {"options", Json::object {
                        {"cpu_mhz", frequency_mhz()},
                        {"num_cpu_max_perf", num_max_perf()},
                        {"affinity", affinity_string(affinity())}}}};
                break;
            case GEOPM_POLICY_MODE_PERF_BALANCE_DYNAMIC:
                policy = Json::object {
                    {"mode", "perf_balance_dynamic"},
                    {"options", Json::object {
                        {"tree_decider", m_tree_decider},
                        {"leaf_decider", m_leaf_decider},
                        {"platform", m_platform},
                        {"power_budget", budget_watts()}}}};
                break;
            case GEOPM_POLICY_MODE_FREQ_UNIFORM_DYNAMIC:
                policy = Json::object {
                    {"mode", "freq_uniform_dynamic"},
                    {"options", Json::object {
                        {"tree_decider", m_tree_decider},
                        {"leaf_decider", m_leaf_decider},
                        {"platform", m_platform},
                        {"power_budget", budget_watts()}}}};
                break;
            case GEOPM_POLICY_MODE_FREQ_HYBRID_DYNAMIC:
                policy = Json::object {
                    {"mode", "freq_hybrid_dynamic"},
                    {"options", Json::object {
                        {"tree_decider", m_tree_decider},
                        {"leaf_decider", m_leaf_decider},
                        {"platform", m_platform},
                        {"power_budget", budget_watts()},
                        {"num_cpu_max_perf", num_max_perf()},
                        {"affinity", affinity_string(affinity())}}}};
                break;
            case GEOPM_POLICY_MODE_STATIC:
                policy = Json::object {
                    {"mode", "static"},
                    {"options", Json::object {
                        {"platform", m_platform}}}};
                break;
            case GEOPM_POLICY_MODE_DYNAMIC:
                policy = Json::object {
                    {"mode", "dynamic"},
                    {"options", Json::object {
                        {"tdp_percent", tdp_percent()},
                        {"cpu_mhz", frequency_mhz()},
                        {"num_cpu_max_perf", num_max_perf()},
                        {"affinity", affinity_string(affinity())},
                        {"platform", m_platform},
                        {"power_budget", budget_watts()},
                        {"tree_decider", m_tree_decider},
                        {"leaf_decider", m_leaf_decider}}}};
                break;
            default:
                throw Exception("GlobalPolicy: invalid mode specified",
                                GEOPM_ERROR_FILE_PARSE, __FILE__, __LINE__);
        }

        std::ofstream config_file_out;
        config_file_out.open(m_out_config.c_str(), std::ifstream::out);
        config_file_out << policy.dump();
        config_file_out.close();
    }

    void GlobalPolicy::write_shm(void)
    {
        int err = pthread_mutex_lock(&(m_policy_shmem_out->lock));
        if (err) {
            throw Exception("GlobalPolicy: Could not lock shared memory region for resource manager",
                            errno, __FILE__, __LINE__);
        }
        m_policy_shmem_out->policy.mode = m_mode;
        m_policy_shmem_out->policy.power_budget = m_power_budget_watts;
        m_policy_shmem_out->policy.flags = m_flags->flags();
        m_policy_shmem_out->plugin.tree_decider[NAME_MAX - 1] = '\0';
        strncpy(m_policy_shmem_out->plugin.tree_decider, m_tree_decider.c_str(), NAME_MAX - 1);
        m_policy_shmem_out->plugin.leaf_decider[NAME_MAX - 1] = '\0';
        strncpy(m_policy_shmem_out->plugin.leaf_decider, m_leaf_decider.c_str(), NAME_MAX - 1);
        m_policy_shmem_out->plugin.platform[NAME_MAX - 1] = '\0';
        strncpy(m_policy_shmem_out->plugin.platform, m_platform.c_str(), NAME_MAX - 1);
        err = pthread_mutex_unlock(&(m_policy_shmem_in->lock));
        if (err) {
            throw Exception("GlobalPolicy: Could not unlock shared memory region for resource manager",
                            errno, __FILE__, __LINE__);
        }
    }

    std::string GlobalPolicy::affinity_string(int value)
    {
        std::string name;
        switch (value) {
            case GEOPM_POLICY_AFFINITY_COMPACT:
                name = "compact";
                break;
            case GEOPM_POLICY_AFFINITY_SCATTER:
                name = "scatter";
                break;
            default:
                throw Exception("GlobalPolicy: invalid affinity specified",
                                GEOPM_ERROR_FILE_PARSE, __FILE__, __LINE__);
                break;
        }
        return name;
    }

    void GlobalPolicy::enforce_static_mode()
    {
        PlatformFactory platform_factory;
        Platform *platform = platform_factory.platform(std::string("rapl"), true);

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
                throw Exception("GlobalPolicy: invalid mode specified",
                                GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        };
    }

    std::string GlobalPolicy::mode_string() const
    {
        switch (m_mode) {
            case GEOPM_POLICY_MODE_TDP_BALANCE_STATIC:
                return "TDP_BALANCE_STATIC";
            case GEOPM_POLICY_MODE_FREQ_UNIFORM_STATIC:
                return "FREQ_UNIFORM_STATIC";
            case GEOPM_POLICY_MODE_FREQ_HYBRID_STATIC:
                return "FREQ_HYBRID_STATIC";
            case GEOPM_POLICY_MODE_PERF_BALANCE_DYNAMIC:
                return "PERF_BALANCE_DYNAMIC";
            case GEOPM_POLICY_MODE_FREQ_UNIFORM_DYNAMIC:
                return "FREQ_UNIFORM_DYNAMIC";
            case GEOPM_POLICY_MODE_FREQ_HYBRID_DYNAMIC:
                return "FREQ_HYBRID_DYNAMIC";
            case GEOPM_POLICY_MODE_STATIC:
                return "STATIC";
            case GEOPM_POLICY_MODE_DYNAMIC:
                return "DYNAMIC";
            case GEOPM_POLICY_MODE_SHUTDOWN:
                return "SHUTDOWN";
            default:
                throw Exception("GlobalPolicy: Unable to convert invalid mode",
                                GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
    }

    std::string GlobalPolicy::header(void) const
    {
        std::ostringstream header_stream;
        header_stream << "# \"geopm_version\" : \"" << geopm_version() << "\"," << std::endl
                      << "# \"profile_name\" : \"" << geopm_env_profile() << "\"," << std::endl
                      << "# \"power_budget\" : " << budget_watts() << "," << std::endl
                      << "# \"tree_decider\" : \"" << tree_decider() << "\"," << std::endl
                      << "# \"leaf_decider\" : \"" << leaf_decider() << "\"," << std::endl;

        return header_stream.str();
    }

}

std::ostream& operator<<(std::ostream &os, const geopm::IGlobalPolicy *obj)
{
    os << "Policy Mode: " << obj->mode_string() << std::endl;
    os << "Tree Decider: " << obj->tree_decider() << std::endl;
    os << "Leaf Decider: " << obj->leaf_decider() << std::endl;
    os << "Power Budget: " << obj->budget_watts() << std::endl;
    return os;
}
