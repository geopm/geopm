/*
 * Copyright (c) 2015, Intel Corporation
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

#include "geopm_policy_message.h"
#include "GlobalPolicy.hpp"

namespace geopm
{
    GlobalPolicy::GlobalPolicy(std::string config)
        :m_mode(-1)
        ,m_cpu_freq_mhz(-1)
        ,m_num_max_perf(-1)
        ,m_percent_tdp(-1)
        ,m_power_budget_watts(-1)
    {
        parse(config);
    }

    GlobalPolicy::~GlobalPolicy() {}

    int GlobalPolicy::mode(void) const
    {
        return m_mode;
    }

    int GlobalPolicy::frequency_mhz(void) const
    {
        return m_cpu_freq_mhz;
    }

    int GlobalPolicy::percent_tdp(void) const
    {
        return m_cpu_freq_mhz;
    }

    int GlobalPolicy::budget_watts(void) const
    {
        return m_power_budget_watts;
    }

    int GlobalPolicy::affinity(void) const
    {
        return m_affinity;
    }

    void GlobalPolicy::mode(int mode)
    {
        m_mode = mode;
    }

    void GlobalPolicy::frequency_mhz(int frequency)
    {
        m_cpu_freq_mhz = frequency;
    }

    void GlobalPolicy::percent_tdp(int percentage)
    {
        m_cpu_freq_mhz = percentage;
    }

    void GlobalPolicy::budget_watts(int budget)
    {
        m_power_budget_watts = budget;
    }

    void GlobalPolicy::affinity(int affinity)
    {
        m_affinity = affinity;
    }

    void GlobalPolicy::parse(std::string path)
    {
        std::ifstream control_file("policy.conf");
        std::string policy_string;
        std::string value_string;
        std::string key_string;
        std::string err_string;
        json_object *object;
        json_object *options_obj = NULL;
        json_object *mode_obj = NULL;
        enum json_type type;

        control_file.seekg(0, std::ios::end);
        policy_string.reserve(control_file.tellg());
        control_file.seekg(0, std::ios::beg);

        policy_string.assign((std::istreambuf_iterator<char>(control_file)),
                             std::istreambuf_iterator<char>());

        control_file.close();

        object = json_tokener_parse(policy_string.c_str());

        type = json_object_get_type(object);

        if (type != json_type_object ) {
            throw std::runtime_error("detected a malformed json config file");;
        }

        json_object_object_foreach(object, key, val) {
            std::cout << "key: " << key << "\n";
            if (!strcmp(key, "mode")) {
                mode_obj = val;
            }
            else if (!strcmp(key, "options")) {
                options_obj = val;
            }
            else {
                throw std::runtime_error("unsupported key or malformed json config file");
            }
        }

        if (mode_obj == NULL || options_obj == NULL) {
            throw std::runtime_error("config file must contain a mode and options");
        }
        if (json_object_get_type(mode_obj) != json_type_string) {
            throw std::runtime_error("mode expected to be a string type");
        }
        if (json_object_get_type(options_obj) != json_type_object) {
            throw std::runtime_error("mode expected to be an object type");
        }

        value_string.assign(json_object_get_string(mode_obj));
        if (!value_string.compare("tdp_balance_static")) {
            m_mode = GEOPM_MODE_TDP_BALANCE_STATIC;
        }
        else if (!value_string.compare("freq_uniform_static")) {
            m_mode = GEOPM_MODE_FREQ_UNIFORM_STATIC;
        }
        else if (!value_string.compare("freq_hybrid_static")) {
            m_mode = GEOPM_MODE_FREQ_HYBRID_STATIC;
        }
        else if (!value_string.compare("perf_balance_dynamic")) {
            m_mode = GEOPM_MODE_PERF_BALANCE_DYNAMIC;
        }
        else if (!value_string.compare("freq_uniform_dynamic")) {
            m_mode = GEOPM_MODE_FREQ_UNIFORM_DYNAMIC;
        }
        else if (!value_string.compare("freq_hybrid_dynamic")) {
            m_mode = GEOPM_MODE_FREQ_HYBRID_DYNAMIC;
        }

        json_object_object_foreach(options_obj, subkey, subval) {
            key_string.assign(subkey);
            if (!key_string.compare("percent_tdp")) {
                if (json_object_get_type(subval) != json_type_int) {
                    throw std::runtime_error("percent_tdp expected to be an integer type");
                }
                m_percent_tdp = json_object_get_int(subval);
            }
            else if (!key_string.compare("cpu_mhz")) {
                if (json_object_get_type(subval) != json_type_int) {
                    throw std::runtime_error("cpu_mhz expected to be an integer type");
                }
                m_cpu_freq_mhz = json_object_get_int(subval);
            }
            else if (!key_string.compare("num_cpu_max_perf")) {
                if (json_object_get_type(subval) != json_type_int) {
                    throw std::runtime_error("num_cpu_max_perf expected to be an integer type");
                }
                m_num_max_perf = json_object_get_int(subval);
            }
            else if (!key_string.compare("affinity")) {
                if (json_object_get_type(subval) != json_type_string) {
                    throw std::runtime_error("affinity expected to be a string type");
                }
                value_string.assign(json_object_get_string(subval));
                if (!value_string.compare("compact")) {
                    m_affinity = 0;
                }
                else if (!value_string.compare("scatter")) {
                    m_affinity = 1;
                }
                else {
                    err_string.assign("unsupported affinity type : ");
                    err_string.append(value_string);
                    throw std::runtime_error(err_string.c_str());
                }
            }
            else if (!key_string.compare("power_budget")) {
                if (json_object_get_type(subval) != json_type_int) {
                    throw std::runtime_error("power_budget expected to be an integer type");
                }
                m_power_budget_watts = json_object_get_int(subval);
            }
            else {
                err_string.assign("unknown option : ");
                err_string.append(key_string);
                throw std::runtime_error(err_string.c_str());
            }
        }

        if (m_mode == GEOPM_MODE_TDP_BALANCE_STATIC) {
            if (m_percent_tdp < 0 || m_percent_tdp > 100) {
                throw std::runtime_error("percent tdp must be between 0 and 100");
            }
            std::cout << "mode=tdp_balance_static,percent_tdp=" << m_percent_tdp << "\n";
        }
        if (m_mode == GEOPM_MODE_FREQ_UNIFORM_STATIC) {
            if (m_cpu_freq_mhz < 0) {
                throw std::runtime_error("frequency is out of bounds");
            }
            std::cout << "mode=freq_uniform_static,cpu_mhz=" << m_cpu_freq_mhz << "\n";
        }
        if (m_mode == GEOPM_MODE_FREQ_HYBRID_STATIC) {
            if (m_cpu_freq_mhz < 0) {
                throw std::runtime_error("frequency is out of bounds");
            }
            if (m_num_max_perf < 0) {
                throw std::runtime_error("number of max perf cpus is out of bounds");
            }
            if (m_affinity < 0) {
                throw std::runtime_error("affiniy must be set to 'scatter' or 'compact'");
            }
            std::cout << "mode=freq_hybrid_static,cpu_mhz=" << m_cpu_freq_mhz
                      << ",num_cpu_max_perf=" << m_num_max_perf;
            if (m_affinity == GEOPM_FLAGS_BIG_CPU_TOPOLOGY_COMPACT) {
                std::cout << ",affinity=compact" << "\n";
            }
            if (m_affinity == GEOPM_FLAGS_BIG_CPU_TOPOLOGY_SCATTER) {
                std::cout << ",affinity=scatter" << "\n";
            }
        }
        if (m_mode == GEOPM_MODE_PERF_BALANCE_DYNAMIC) {
            if (m_power_budget_watts < 0) {
                throw std::runtime_error("power budget is out of bounds");
            }
            std::cout << "mode=perf_balance_dynamic,power_budget=" << m_power_budget_watts << "\n";
        }
        if (m_mode == GEOPM_MODE_FREQ_UNIFORM_DYNAMIC) {
            if (m_power_budget_watts < 0) {
                throw std::runtime_error("power budget is out of bounds");
            }
            std::cout << "mode=freq_uniform_dynamic,power_budget=" << m_power_budget_watts << "\n";
        }
        if (m_mode == GEOPM_MODE_FREQ_HYBRID_DYNAMIC) {
            if (m_power_budget_watts < 0) {
                throw std::runtime_error("power budget is out of bounds");
            }
            if (m_num_max_perf < 0) {
                throw std::runtime_error("number of max perf cpus is out of bounds");
            }
            if (m_affinity < 0) {
                throw std::runtime_error("affiniy must be set to 'scatter' or 'compact'");
            }
            std::cout << "mode=freq_hybrid_dynamic,power_budget=" << m_power_budget_watts
                      << ",num_cpu_max_perf=" << m_num_max_perf;
            if (m_affinity == GEOPM_FLAGS_BIG_CPU_TOPOLOGY_COMPACT) {
                std::cout << ",affinity=compact" << "\n";
            }
            if (m_affinity == GEOPM_FLAGS_BIG_CPU_TOPOLOGY_SCATTER) {
                std::cout << ",affinity=scatter" << "\n";
            }
        }
    }

    void GlobalPolicy::write(std::string config)
    {
        json_object *policy, *options;
        std::ofstream config_file;
        std::string affinity_name;

        if (config.empty()) {
            throw std::runtime_error("invalid path specified");
        }

        config_file.open(config.c_str(), std::ifstream::out);

        policy = json_object_new_object();
        options = json_object_new_object();

        switch (m_mode) {
            case GEOPM_MODE_SHUTDOWN:
                break;
            case GEOPM_MODE_TDP_BALANCE_STATIC:
                json_object_object_add(policy,"mode",json_object_new_string("tdp_balanced_static"));
                json_object_object_add(options,"percent_tdp",json_object_new_int(m_percent_tdp));
                json_object_object_add(policy,"options",options);
                break;
            case GEOPM_MODE_FREQ_UNIFORM_STATIC:
                json_object_object_add(policy,"mode",json_object_new_string("freq_uniform_static"));
                json_object_object_add(options,"cpu_mhz",json_object_new_int(m_cpu_freq_mhz));
                json_object_object_add(policy,"options",options);
                break;
            case GEOPM_MODE_FREQ_HYBRID_STATIC:
                json_object_object_add(policy,"mode",json_object_new_string("freq_hybrid_static"));
                json_object_object_add(options,"cpu_mhz",json_object_new_int(m_cpu_freq_mhz));
                json_object_object_add(options,"num_cpu_max_perf",json_object_new_int(m_num_max_perf));
                affinity_string(m_affinity, affinity_name);
                json_object_object_add(options,"affinity",json_object_new_string(affinity_name.c_str()));
                json_object_object_add(policy,"options",options);
                break;
            case GEOPM_MODE_PERF_BALANCE_DYNAMIC:
                json_object_object_add(policy,"mode",json_object_new_string("perf_balanced_dynamic"));
                json_object_object_add(options,"power_budget",json_object_new_int(m_power_budget_watts));
                json_object_object_add(policy,"options",options);
                break;
            case GEOPM_MODE_FREQ_UNIFORM_DYNAMIC:
                json_object_object_add(policy,"mode",json_object_new_string("freq_uniform_dynamic"));
                json_object_object_add(options,"power_budget",json_object_new_int(m_power_budget_watts));
                json_object_object_add(policy,"options",options);
                break;
            case GEOPM_MODE_FREQ_HYBRID_DYNAMIC:
                json_object_object_add(policy,"mode",json_object_new_string("freq_hybrid_dynamic"));
                json_object_object_add(options,"power_budget",json_object_new_int(m_power_budget_watts));
                json_object_object_add(options,"num_cpu_max_perf",json_object_new_int(m_num_max_perf));
                affinity_string(m_affinity, affinity_name);
                json_object_object_add(options,"affinity",json_object_new_string(affinity_name.c_str()));
                json_object_object_add(policy,"options",options);
                break;
            default:
                throw std::runtime_error("invalid mode specified");
        }
        config_file << json_object_to_json_string(policy);
        config_file.close();
    }

    void GlobalPolicy::affinity_string(int value, std::string &name)
    {
        switch (value) {
            case GEOPM_FLAGS_BIG_CPU_TOPOLOGY_COMPACT:
                name.assign("compact");
                break;
            case GEOPM_FLAGS_BIG_CPU_TOPOLOGY_SCATTER:
                name.assign("scatter");
                break;
            default:
                throw std::runtime_error("invalid affinity specified");
                break;
        }
    }
}
