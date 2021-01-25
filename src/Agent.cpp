/*
 * Copyright (c) 2015, 2016, 2017, 2018, 2019, 2020, Intel Corporation
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

#include "config.h"

#include "Agent.hpp"

#include <cmath>
#include <cstring>

#include <sstream>
#include <iostream>
#include <mutex>

#include "geopm_agent.h"
#include "geopm_plugin.hpp"
#include "MonitorAgent.hpp"
#include "PowerBalancerAgent.hpp"
#include "PowerGovernorAgent.hpp"
#include "EnergyEfficientAgent.hpp"
#include "FrequencyMapAgent.hpp"
#include "Environment.hpp"
#include "Helper.hpp"

namespace geopm
{
    const std::string Agent::m_num_sample_string = "NUM_SAMPLE";
    const std::string Agent::m_num_policy_string = "NUM_POLICY";
    const std::string Agent::m_sample_prefix = "SAMPLE_";
    const std::string Agent::m_policy_prefix = "POLICY_";
    const std::string Agent::M_PLUGIN_PREFIX = "libgeopmagent_";

    AgentFactory::AgentFactory()
    {
        register_plugin(MonitorAgent::plugin_name(),
                        MonitorAgent::make_plugin,
                        Agent::make_dictionary(MonitorAgent::policy_names(),
                                               MonitorAgent::sample_names()));
        register_plugin(PowerBalancerAgent::plugin_name(),
                        PowerBalancerAgent::make_plugin,
                        Agent::make_dictionary(PowerBalancerAgent::policy_names(),
                                               PowerBalancerAgent::sample_names()));
        register_plugin(PowerGovernorAgent::plugin_name(),
                        PowerGovernorAgent::make_plugin,
                        Agent::make_dictionary(PowerGovernorAgent::policy_names(),
                                               PowerGovernorAgent::sample_names()));
        register_plugin(FrequencyMapAgent::plugin_name(),
                        FrequencyMapAgent::make_plugin,
                        Agent::make_dictionary(FrequencyMapAgent::policy_names(),
                                               FrequencyMapAgent::sample_names()));
    }


    AgentFactory &agent_factory(void)
    {
        static AgentFactory instance;
        static bool is_once = true;
        static std::once_flag flag;
        if (is_once) {
            is_once = false;
            std::call_once(flag, plugin_load, Agent::M_PLUGIN_PREFIX);
        }
        return instance;
    }

    std::vector<std::string> Agent::agent_names(void)
    {
        return agent_factory().plugin_names();
    }

    std::unique_ptr<Agent> Agent::make_unique(const std::string &agent_name)
    {
        return agent_factory().make_plugin(agent_name);
    }

    std::vector<std::function<std::string(double)> > Agent::trace_formats(void) const
    {
#ifdef GEOPM_DEBUG
        static bool is_once = true;
        if (is_once) {
            is_once = false;
            std::cerr << "Warning: <geopm> Use of geopm::Agent::trace_formats() is deprecated, each Agent will be required implement this method in the future.\n";
        }
#endif
        return {};
    }

    int Agent::num_sample(const std::map<std::string, std::string> &dictionary)
    {
        auto it = dictionary.find(m_num_sample_string);
        if (it == dictionary.end()) {
            throw Exception("Agent::num_sample(): "
                            "Agent was not registered with plugin factory with the correct dictionary.",
                            GEOPM_ERROR_LOGIC, __FILE__, __LINE__);
        }
        int num_sample = std::stoi(it->second);
        if (num_sample < 0) {
            throw Exception("Agent::num_sample(): "
                            "Agent was not registered with plugin factory with the correct dictionary.",
                            GEOPM_ERROR_LOGIC, __FILE__, __LINE__);
        }
        return num_sample;
    }

    int Agent::num_sample(const std::string &agent_name)
    {
        return Agent::num_sample(agent_factory().dictionary(agent_name));
    }

    int Agent::num_policy(const std::map<std::string, std::string> &dictionary)
    {
        auto it = dictionary.find(m_num_policy_string);
        if (it == dictionary.end()) {
            throw Exception("Agent::num_policy(): "
                            "Agent was not registered with plugin factory with the correct dictionary.",
                            GEOPM_ERROR_LOGIC, __FILE__, __LINE__);
        }
        int num_policy = std::stoi(it->second);
        if (num_policy < 0) {
            throw Exception("Agent::num_policy(): "
                            "Agent was not registered with plugin factory with the correct dictionary.",
                            GEOPM_ERROR_LOGIC, __FILE__, __LINE__);
        }
        return num_policy;
    }

    int Agent::num_policy(const std::string &agent_name)
    {
        return Agent::num_policy(agent_factory().dictionary(agent_name));
    }

    std::vector<std::string> Agent::sample_names(const std::map<std::string, std::string> &dictionary)
    {
        size_t num_names = num_sample(dictionary);
        std::vector<std::string> result(num_names);
        for (size_t name_idx = 0; name_idx != num_names; ++name_idx) {
            std::string key = m_sample_prefix + std::to_string(name_idx);
            auto it = dictionary.find(key);
            if (it == dictionary.end()) {
                throw Exception("Agent::send_up_names(): Poorly formatted dictionary, could not find key: " + key,
                                GEOPM_ERROR_INVALID, __FILE__, __LINE__);
            }
            result[name_idx] = it->second;
        }
        return result;
    }

    std::vector<std::string> Agent::sample_names(const std::string &agent_name)
    {
        return Agent::sample_names(agent_factory().dictionary(agent_name));
    }

    std::vector<std::string> Agent::policy_names(const std::map<std::string, std::string> &dictionary)
    {
        size_t num_names = num_policy(dictionary);
        std::vector<std::string> result(num_names);

        for (size_t name_idx = 0; name_idx != num_names; ++name_idx) {
            std::string key = m_policy_prefix + std::to_string(name_idx);
            auto it = dictionary.find(key);
            if (it == dictionary.end()) {
                throw Exception("Agent::send_down_names(): Poorly formatted dictionary, could not find key: " + key,
                                GEOPM_ERROR_INVALID, __FILE__, __LINE__);
            }
            result[name_idx] = it->second;
        }
        return result;
    }

    std::vector<std::string> Agent::policy_names(const std::string &agent_name)
    {
        return Agent::policy_names(agent_factory().dictionary(agent_name));
    }

    std::map<std::string, std::string> Agent::make_dictionary(const std::vector<std::string> &policy_names,
                                                              const std::vector<std::string> &sample_names)
    {
        std::map<std::string, std::string> result;
        for (size_t sample_idx = 0; sample_idx != sample_names.size(); ++sample_idx) {
            std::string key = m_sample_prefix + std::to_string(sample_idx);
            result[key] = sample_names[sample_idx];
        }
        result[m_num_sample_string] = std::to_string(sample_names.size());

        for (size_t policy_idx = 0; policy_idx != policy_names.size(); ++policy_idx) {
            std::string key = m_policy_prefix + std::to_string(policy_idx);
            result[key] = policy_names[policy_idx];
        }
        result[m_num_policy_string] = std::to_string(policy_names.size());
        return result;
    }

    void Agent::aggregate_sample(const std::vector<std::vector<double> > &in_sample,
                                 const std::vector<std::function<double(const std::vector<double>&)> > &agg_func,
                                 std::vector<double> &out_sample)
    {
        size_t num_children = in_sample.size();
        std::vector<double> child_sample(num_children);
        for (size_t sig_idx = 0; sig_idx < out_sample.size(); ++sig_idx) {
            for (size_t child_idx = 0; child_idx < num_children; ++child_idx) {
                child_sample[child_idx] = in_sample[child_idx][sig_idx];
            }
            out_sample[sig_idx] = agg_func[sig_idx](child_sample);
        }
    }
}

int geopm_agent_supported(const char *agent_name)
{
    int err = 0;
    try {
        geopm::Agent::num_policy(agent_name);
    }
    catch (const geopm::Exception &ex) {
        if (ex.err_value() == GEOPM_ERROR_INVALID) {
            err = GEOPM_ERROR_NO_AGENT;
        }
        else {
            err = ex.err_value();
        }
    }
    catch (...) {
        err = geopm::exception_handler(std::current_exception(), false);
    }
    return err;
}
int geopm_agent_num_policy(const char *agent_name,
                           int *num_policy)
{
    int err = 0;
    try {
        *num_policy = geopm::Agent::num_policy(agent_name);
    }
    catch (const geopm::Exception &ex) {
        if (ex.err_value() == GEOPM_ERROR_INVALID) {
            err = GEOPM_ERROR_NO_AGENT;
        }
        else {
            err = ex.err_value();
        }
    }
    catch (...) {
        err = geopm::exception_handler(std::current_exception(), false);
    }
    return err;
}

int geopm_agent_num_sample(const char *agent_name,
                           int *num_sample)
{
    int err = 0;
    try {
        *num_sample = geopm::Agent::num_sample(agent_name);
    }
    catch (const geopm::Exception &ex) {
        if (ex.err_value() == GEOPM_ERROR_INVALID) {
            err = GEOPM_ERROR_NO_AGENT;
        }
        else {
            err = ex.err_value();
        }
    }
    catch (...) {
        err = geopm::exception_handler(std::current_exception(), false);
    }
    return err;
}

int geopm_agent_policy_name(const char *agent_name,
                            int policy_idx,
                            size_t policy_name_max,
                            char *policy_name)
{
    int num_policy;
    int err = geopm_agent_num_policy(agent_name, &num_policy);
    if (!err && (policy_idx < 0 || policy_idx >= num_policy)) {
        err = GEOPM_ERROR_INVALID;
    }
    if (!err) {
        try {
            std::string policy_name_cxx = geopm::Agent::policy_names(agent_name).at(policy_idx);
            if (policy_name_cxx.size() >= policy_name_max) {
                err = E2BIG;
            }
            if (!err) {
                strncpy(policy_name, policy_name_cxx.c_str(), policy_name_max);
                policy_name[policy_name_max - 1] = '\0';
            }
        }
        catch (const geopm::Exception &ex) {
            if (ex.err_value() == GEOPM_ERROR_INVALID) {
                err = GEOPM_ERROR_NO_AGENT;
            }
            else {
                err = ex.err_value();
            }
        }
        catch (...) {
            err = geopm::exception_handler(std::current_exception(), false);
        }
    }
    return err;
}

int geopm_agent_sample_name(const char *agent_name,
                            int sample_idx,
                            size_t sample_name_max,
                            char *sample_name)
{
    int num_sample;
    int err = geopm_agent_num_sample(agent_name, &num_sample);
    if (!err && (sample_idx < 0 || sample_idx >= num_sample)) {
        err = GEOPM_ERROR_INVALID;
    }
    if (!err) {
        try {
            std::string sample_name_cxx = geopm::Agent::sample_names(agent_name).at(sample_idx);
            if (sample_name_cxx.size() >= sample_name_max) {
                err = E2BIG;
            }
            if (!err) {
                strncpy(sample_name, sample_name_cxx.c_str(), sample_name_max);
                sample_name[sample_name_max - 1] = '\0';
            }
        }
        catch (const geopm::Exception &ex) {
            if (ex.err_value() == GEOPM_ERROR_INVALID) {
                err = GEOPM_ERROR_NO_AGENT;
            }
            else {
                err = ex.err_value();
            }
        }
        catch (...) {
            err = geopm::exception_handler(std::current_exception(), false);
        }
    }
    return err;

}

int geopm_agent_policy_json(const char *agent_name,
                            const double *policy_array,
                            size_t json_string_max,
                            char *json_string)
{
    int num_policy = 0;
    int err = geopm_agent_num_policy(agent_name, &num_policy);
    if (!err) {
        err = geopm_agent_policy_json_partial(agent_name, num_policy, policy_array,
                                              json_string_max, json_string);
    }
    return err;
}

int geopm_agent_policy_json_partial(const char *agent_name,
                                    size_t policy_array_size,
                                    const double *policy_array,
                                    size_t json_string_max,
                                    char *json_string)
{
    std::stringstream output_str;
    char policy_name[json_string_max];
    std::string policy_value;
    int num_policy = 0;
    int err = geopm_agent_num_policy(agent_name, &num_policy);
    if (!err && (num_policy < 0 || policy_array_size > (size_t)num_policy)) {
        err = GEOPM_ERROR_INVALID;
    }
    try {
        if (!err) {
            output_str << "{";
            for (size_t i = 0; !err && i < policy_array_size; ++i) {
                if (i > 0) {
                    output_str << ", ";
                }
                err = geopm_agent_policy_name(agent_name, i, json_string_max, policy_name);
                if (std::isnan(policy_array[i])) {
                    policy_value = "\"NAN\"";
                }
                else {
                    policy_value = geopm::string_format_double(policy_array[i]);
                }
                output_str << "\"" << policy_name << "\": " << policy_value;
            }
            output_str << "}";
        }
    }
    catch (...) {
        err = geopm::exception_handler(std::current_exception(), false);
    }

    if (!err) {
        if (output_str.str().size() >= json_string_max) {
            err = GEOPM_ERROR_INVALID;
        }
        if (!err) {
            strncpy(json_string, output_str.str().c_str(), json_string_max);
            json_string[json_string_max - 1] = '\0';
        }
    }
    return err;
}

int geopm_agent_name(int agent_idx,
                     size_t agent_name_max,
                     char *agent_name)
{
    int err = 0;
    try {
        std::vector<std::string> agent_names = geopm::Agent::agent_names();
        if (agent_names.at(agent_idx).size() >= agent_name_max) {
            err = GEOPM_ERROR_INVALID;
        }
        if (!err) {
            strncpy(agent_name, agent_names.at(agent_idx).c_str(), agent_name_max);
            agent_name[agent_name_max - 1] = '\0';
        }
    }
    catch (...) {
        err = geopm::exception_handler(std::current_exception(), false);
    }
    return err;
}

int geopm_agent_num_avail(int* num_agent)
{
    int err = 0;
    try {
        std::vector<std::string> agent_names = geopm::Agent::agent_names();
        *num_agent = agent_names.size();
    }
    catch (...) {
        err = geopm::exception_handler(std::current_exception(), false);
    }
    return err;
}

// geopm_agent_enforce_policy is defined in Controller.cpp
