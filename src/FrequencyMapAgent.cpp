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

#include <sstream>
#include <cmath>
#include <iomanip>

#include "contrib/json11/json11.hpp"

#include "geopm.h"
#include "geopm_hash.h"

#include "FrequencyMapAgent.hpp"
#include "PlatformIO.hpp"
#include "PlatformTopo.hpp"
#include "Helper.hpp"
#include "Exception.hpp"
#include "config.h"

using json11::Json;

namespace geopm
{
    FrequencyMapAgent::FrequencyMapAgent()
        : FrequencyMapAgent(platform_io(), platform_topo())
    {

    }

    FrequencyMapAgent::FrequencyMapAgent(PlatformIO &plat_io, PlatformTopo &topo)
        : M_PRECISION(16)
        , m_platform_io(plat_io)
        , m_platform_topo(topo)
        , m_freq_min(NAN)
        , m_freq_max(NAN)
        , M_FREQ_STEP(get_limit("CPUINFO::FREQ_STEP"))
        , m_last_freq(NAN)
        , m_last_region(std::make_pair(GEOPM_REGION_HASH_INVALID, GEOPM_REGION_HINT_UNKNOWN))
        , m_last_wait(GEOPM_TIME_REF)
        , m_level(-1)
        , m_num_children(0)
    {
        parse_env_map();
    }

    std::string FrequencyMapAgent::plugin_name(void)
    {
        return "frequency_map";
    }

    std::unique_ptr<Agent> FrequencyMapAgent::make_plugin(void)
    {
        return geopm::make_unique<FrequencyMapAgent>();
    }

    void FrequencyMapAgent::init(int level, const std::vector<int> &fan_in, bool is_level_root)
    {
        m_level = level;
        if (m_level == 0) {
            m_num_children = 0;
            init_platform_io();
        }
        else {
            m_num_children = fan_in[level - 1];
        }
    }

    bool FrequencyMapAgent::policy(std::vector<double> &policy)
    {
        /// @todo more checks
#ifdef GEOPM_DEBUG
        if (policy.size() != M_NUM_POLICY) {
            throw Exception("FrequencyMapAgent::" + std::string(__func__) + "(): policy vector not correctly sized.",
                            GEOPM_ERROR_LOGIC, __FILE__, __LINE__);
        }
#endif
        bool result = false;
        double target_freq_min = std::isnan(policy[M_POLICY_FREQ_MIN]) ? get_limit("CPUINFO::FREQ_MIN") : policy[M_POLICY_FREQ_MIN];
        double target_freq_max = std::isnan(policy[M_POLICY_FREQ_MAX]) ? get_limit("CPUINFO::FREQ_MAX") : policy[M_POLICY_FREQ_MAX];
        if (std::isnan(target_freq_min) || std::isnan(target_freq_max) ||
            target_freq_min > target_freq_max) {
            throw Exception("FrequencyMapAgent::" + std::string(__func__) + "(): invalid frequency bounds.",
                            GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
        }
        policy[M_POLICY_FREQ_MIN] = target_freq_min;
        policy[M_POLICY_FREQ_MAX] = target_freq_max;
        /// @todo is there an m_policy member?  update here and return true
        return result;
    }

    bool FrequencyMapAgent::update_policy(const std::vector<double> &policy)
    {
        bool result = false;
        if (m_freq_min != policy[M_POLICY_FREQ_MIN] ||
            m_freq_max != policy[M_POLICY_FREQ_MAX]) {
            m_freq_min = policy[M_POLICY_FREQ_MIN];
            m_freq_max = policy[M_POLICY_FREQ_MAX];
            result = true;
        }
        return result;
    }

    void FrequencyMapAgent::descend(std::vector<std::vector<double> >&out_policy) const
    {
#ifdef GEOPM_DEBUG
        if (out_policy.size() != (size_t) m_num_children) {
            throw Exception("FrequencyMapAgent::" + std::string(__func__) + "(): out_policy vector not correctly sized.",
                            GEOPM_ERROR_LOGIC, __FILE__, __LINE__);
        }
#endif
        for (auto &child_policy : out_policy) {
#ifdef GEOPM_DEBUG
            if (child_policy.size() != M_NUM_POLICY) {
                throw Exception("FrequencyMapAgent::" + std::string(__func__) + "(): child_policy vector not correctly sized.",
                                GEOPM_ERROR_LOGIC, __FILE__, __LINE__);
            }
#endif
            child_policy[M_POLICY_FREQ_MIN] = m_freq_min;
            child_policy[M_POLICY_FREQ_MAX] = m_freq_max;
        }
    }

    bool FrequencyMapAgent::ascend(const std::vector<std::vector<double> > &in_sample,
                                   std::vector<double> &out_sample)
    {
        return false;
    }

    bool FrequencyMapAgent::adjust_platform()
    {
        double freq = NAN;
        auto it = m_hash_freq_map.find(m_last_region.first);
        if (it != m_hash_freq_map.end()) {
            freq = it->second;
        }
        else {
            switch(m_last_region.second) {
                // Hints for low CPU frequency
                case GEOPM_REGION_HINT_MEMORY:
                case GEOPM_REGION_HINT_NETWORK:
                case GEOPM_REGION_HINT_IO:
                    freq = m_freq_min;
                    break;

                // Hints for maximum CPU frequency
                case GEOPM_REGION_HINT_COMPUTE:
                case GEOPM_REGION_HINT_SERIAL:
                case GEOPM_REGION_HINT_PARALLEL:
                    freq = m_freq_max;
                    break;
                // Hint Inconclusive
                //case GEOPM_REGION_HINT_UNKNOWN:
                //case GEOPM_REGION_HINT_IGNORE:
                default:
                    freq = m_freq_max;
                    break;
            }
        }

        if (GEOPM_REGION_HASH_INVALID != m_last_region.first) {
            m_hash_freq_map[m_last_region.first] = freq;
        }

        bool result = false;
        if (freq != m_last_freq) {
            for (auto ctl_idx : m_control_idx) {
                m_platform_io.adjust(ctl_idx, freq);
            }
            m_last_freq = freq;
            result = true;
        }
        return result;
    }

    bool FrequencyMapAgent::sample_platform(std::vector<double> &out_sample)
    {
        const uint64_t current_region_hash = m_platform_io.sample(m_signal_idx[M_SIGNAL_REGION_HASH]);
        const uint64_t current_region_hint = m_platform_io.sample(m_signal_idx[M_SIGNAL_REGION_HINT]);
        if (current_region_hash != GEOPM_REGION_HASH_INVALID) {
            m_last_region = std::make_pair(current_region_hash, current_region_hint);
        }
        return false;
    }

    void FrequencyMapAgent::wait(void)
    {
        static double M_WAIT_SEC = 0.005;
        while(geopm_time_since(&m_last_wait) < M_WAIT_SEC) {

        }
        geopm_time(&m_last_wait);
    }

    std::vector<std::string> FrequencyMapAgent::policy_names(void)
    {
        return {"FREQ_MIN", "FREQ_MAX"};
    }

    std::vector<std::string> FrequencyMapAgent::sample_names(void)
    {
        return {};
    }

    std::vector<std::pair<std::string, std::string> > FrequencyMapAgent::report_header(void) const
    {
        return {};
    }

    std::vector<std::pair<std::string, std::string> > FrequencyMapAgent::report_node(void) const
    {
        std::vector<std::pair<std::string, std::string> > result;
        std::ostringstream oss;
        oss << std::setprecision(M_PRECISION) << std::scientific;
        for (const auto &region : m_hash_freq_map) {
            oss << "\n\t0x" << std::hex << std::setfill('0') << std::setw(16) << std::fixed;
            oss << region.first;
            oss << std::setfill('\0') << std::setw(0) << std::scientific;
            oss << ":" << region.second;
        }
        oss << "\n";
        result.push_back(std::make_pair("Frequency map", oss.str()));

        return result;
    }

    std::map<uint64_t, std::vector<std::pair<std::string, std::string> > > FrequencyMapAgent::report_region(void) const
    {
        std::map<uint64_t, std::vector<std::pair<std::string, std::string> > > result;
        // If region is in this map, offline static frequency or hint was used
        for (const auto &region : m_hash_freq_map) {
            /// @todo re-implement with m_region_map and m_hash_freq_map keys as pair (hash + hint)
            result[region.first].push_back(std::make_pair("frequency-map", std::to_string(region.second)));
        }
        return result;
    }

    std::vector<std::string> FrequencyMapAgent::trace_names(void) const
    {
        return {};
    }

    void FrequencyMapAgent::trace_values(std::vector<double> &values)
    {
    }

    double FrequencyMapAgent::get_limit(const std::string &sig_name) const
    {
        const int domain_type = m_platform_io.signal_domain_type(sig_name);
        double result = NAN;
        const double sticker_freq = m_platform_io.read_signal("CPUINFO::FREQ_STICKER", domain_type, 0);
        if (sig_name == "CPUINFO::FREQ_MIN") {
            if (domain_type == PlatformTopo::M_DOMAIN_INVALID) {
                if (m_platform_io.signal_domain_type("CPUINFO::FREQ_STICKER") == PlatformTopo::M_DOMAIN_INVALID) {
                    throw Exception("FrequencyMapAgent::" + std::string(__func__) + "(): unable to parse min and sticker frequencies.",
                                    GEOPM_ERROR_AGENT_UNSUPPORTED, __FILE__, __LINE__);
                }
            }
            else {
                result = m_platform_io.read_signal(sig_name, domain_type, 0);
            }
        }
        else if (sig_name == "CPUINFO::FREQ_MAX") {
            if (domain_type == PlatformTopo::M_DOMAIN_INVALID) {
                if (m_platform_io.signal_domain_type("CPUINFO::FREQ_STICKER") == PlatformTopo::M_DOMAIN_INVALID) {
                    throw Exception("FrequencyMapAgent::" + std::string(__func__) + "(): unable to parse max and sticker frequencies.",
                                    GEOPM_ERROR_AGENT_UNSUPPORTED, __FILE__, __LINE__);
                }
                result = sticker_freq + M_FREQ_STEP;
            }
            else {
                result = m_platform_io.read_signal(sig_name, domain_type, 0);
            }
        }
        else if (sig_name == "CPUINFO::FREQ_STEP") {
            result = m_platform_io.read_signal(sig_name, domain_type, 0);
        }
#ifdef GEOPM_DEBUG
        else {
            throw Exception("FrequencyMapAgent::" + std::string(__func__) + "(): requested invalid signal name.",
                            GEOPM_ERROR_LOGIC, __FILE__, __LINE__);
        }
#endif
        return result;
    }

    void FrequencyMapAgent::init_platform_io(void)
    {
        const int freq_ctl_domain_type = m_platform_io.control_domain_type("FREQUENCY");
        const int num_freq_ctl_domain = m_platform_topo.num_domain(freq_ctl_domain_type);
        for (int ctl_dom_idx = 0; ctl_dom_idx != num_freq_ctl_domain; ++ctl_dom_idx) {
            m_control_idx.push_back(m_platform_io.push_control("FREQUENCY",
                                                               freq_ctl_domain_type, ctl_dom_idx));
        }

        std::vector<std::string> signal_names = {"REGION_HASH", "REGION_HINT"};
        for (auto const&signal_name : signal_names) {
            m_signal_idx.push_back(m_platform_io.push_signal(signal_name,
                                   PlatformTopo::M_DOMAIN_BOARD,
                                   0));
        }
    }

    void FrequencyMapAgent::parse_env_map(void)
    {
        const char* env_map_str = getenv("GEOPM_FREQUENCY_MAP");
        if (env_map_str) {
            std::string full_str(env_map_str);
            std::string err;
            Json root = Json::parse(full_str, err);
            if (!err.empty() || !root.is_object()) {
                throw Exception("FrequencyMapAgent::" + std::string(__func__) + "(): detected a malformed json config file: " + err,
                                GEOPM_ERROR_FILE_PARSE, __FILE__, __LINE__);
            }
            for (const auto &obj : root.object_items()) {
                if (obj.second.type() != Json::NUMBER) {
                    throw Exception("FrequencyMapAgent::" + std::string(__func__) +
                                    ": Region best-fit frequency must be a number",
                                    GEOPM_ERROR_FILE_PARSE, __FILE__, __LINE__);
                }
                uint64_t hash = geopm_crc32_str(obj.first.c_str());
                m_hash_freq_map[hash] = obj.second.number_value();
            }
        }
    }
}
