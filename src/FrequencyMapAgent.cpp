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

#include "FrequencyMapAgent.hpp"

#include <sstream>
#include <cmath>
#include <iomanip>
#include <iostream>
#include <utility>

#include "contrib/json11/json11.hpp"

#include "geopm.h"
#include "geopm_hash.h"

#include "Environment.hpp"
#include "PlatformIO.hpp"
#include "PlatformTopo.hpp"
#include "FrequencyGovernor.hpp"
#include "Helper.hpp"
#include "Exception.hpp"
#include "config.h"

using json11::Json;

namespace geopm
{
    static std::map<uint64_t, double> parse_env_map(void)
    {
        std::map<uint64_t, double> frequency_map;
        std::string env_map_str = environment().frequency_map();
        if (env_map_str.length()) {
            std::cerr << "Warning: <geopm> Use of the GEOPM_FREQUENCY_MAP "
                         "environment variable is deprecated. Frequency maps "
                         "will only be set via "
                      << FrequencyMapAgent::plugin_name()
                      << " agent policies in the future.\n";
            std::string err;
            Json root = Json::parse(env_map_str, err);
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
                frequency_map[hash] = obj.second.number_value();
            }
        }
        return frequency_map;
    }

    FrequencyMapAgent::FrequencyMapAgent()
        : FrequencyMapAgent(platform_io(), platform_topo(), FrequencyGovernor::make_shared(), parse_env_map())
    {

    }

    FrequencyMapAgent::FrequencyMapAgent(PlatformIO &plat_io, const PlatformTopo &topo,
                                         std::shared_ptr<FrequencyGovernor> gov, std::map<uint64_t, double> frequency_map)
        : M_PRECISION(16)
        , m_platform_io(plat_io)
        , m_platform_topo(topo)
        , m_freq_governor(gov)
        , m_hash_freq_map(frequency_map)
        , m_last_wait(GEOPM_TIME_REF)
        , m_level(-1)
        , m_num_children(0)
        , m_is_policy_updated(false)
        , m_is_initialized_with_map(!frequency_map.empty())
    {

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

    void FrequencyMapAgent::validate_policy(std::vector<double> &policy) const
    {
#ifdef GEOPM_DEBUG
        if (!is_valid_policy_size(policy)) {
            throw Exception("FrequencyMapAgent::" + std::string(__func__) + "(): policy vector not correctly sized.",
                            GEOPM_ERROR_LOGIC, __FILE__, __LINE__);
        }
#endif
        m_freq_governor->validate_policy(policy[M_POLICY_FREQ_MIN],
                                         policy[M_POLICY_FREQ_MAX]);

        // Validate all (hash, frequency) pairs
        std::set<double> policy_regions;
        for (auto it = policy.begin() + M_POLICY_FIRST_HASH;
             it < policy.end() && std::next(it) < policy.end(); std::advance(it, 2)) {
            auto mapped_freq = *(it + 1);

            if (!std::isnan(*it)) {
                // We are using a static cast rather than reinterpreting the
                // memory so that regions can be input to this policy in the
                // same form they are output from a report.
                auto region = static_cast<uint64_t>(*it);
                // A valid region will either set or clear its mapped frequency.
                // Just make sure it does not have multiple definitions.
                if (!policy_regions.insert(region).second) {
                    throw Exception("FrequencyMapAgent policy has multiple entries for region: " +
                                        std::to_string(region),
                                    GEOPM_ERROR_INVALID, __FILE__, __LINE__);
                }
            }
            else if (!std::isnan(mapped_freq)) {
                // An invalid region is only a problem if we are trying to map
                // a frequency to it. Otherwise (NaN, NaN) just ignore it.
                throw Exception("FrequencyMapAgent policy maps a NaN region with frequency: " +
                                    std::to_string(mapped_freq),
                                GEOPM_ERROR_INVALID, __FILE__, __LINE__);
            }
        }
    }

    void FrequencyMapAgent::update_policy(const std::vector<double> &policy)
    {
        m_is_policy_updated = m_freq_governor->set_frequency_bounds(
            policy[M_POLICY_FREQ_MIN], policy[M_POLICY_FREQ_MAX]);

        for (auto it = policy.begin() + M_POLICY_FIRST_HASH;
             it < policy.end() && std::next(it) < policy.end(); std::advance(it, 2)) {
            if (!std::isnan(*it)) {
                auto hash = static_cast<uint64_t>(*it);
                auto freq = *(it + 1);
                if (std::isnan(freq)) {
                    auto num_erased = m_hash_freq_map.erase(hash);
                    if (num_erased != 0) {
                        m_is_policy_updated = true;
                    }
                }
                else {
                    auto inserted = m_hash_freq_map.emplace(hash, freq);
                    if (inserted.second) {
                        m_is_policy_updated = true;
                    }
                    else {
                        if (inserted.first->second != freq) {
                            inserted.first->second = freq;
                            m_is_policy_updated = true;
                        }
                    }
                }
            }
        }
    }

    void FrequencyMapAgent::split_policy(const std::vector<double> &in_policy,
                                         std::vector<std::vector<double> > &out_policy)
    {
#ifdef GEOPM_DEBUG
        if (out_policy.size() != (size_t) m_num_children) {
            throw Exception("FrequencyMapAgent::" + std::string(__func__) + "(): out_policy vector not correctly sized.",
                            GEOPM_ERROR_LOGIC, __FILE__, __LINE__);
        }
        for (auto &child_policy : out_policy) {
            if (!is_valid_policy_size(child_policy)) {
                throw Exception("FrequencyMapAgent::" + std::string(__func__) + "(): child_policy vector not correctly sized.",
                                GEOPM_ERROR_LOGIC, __FILE__, __LINE__);
            }
        }
#endif
        update_policy(in_policy);

        if (m_is_policy_updated) {
            std::fill(out_policy.begin(), out_policy.end(), in_policy);
        }
    }

    bool FrequencyMapAgent::do_send_policy(void) const
    {
        return m_is_policy_updated;
    }

    void FrequencyMapAgent::aggregate_sample(const std::vector<std::vector<double> > &in_sample,
                                             std::vector<double> &out_sample)
    {

    }

    bool FrequencyMapAgent::do_send_sample(void) const
    {
        return false;
    }

    void FrequencyMapAgent::adjust_platform(const std::vector<double> &in_policy)
    {
        update_policy(in_policy);
        double freq = NAN;
        double freq_min = m_freq_governor->get_frequency_min();
        double freq_max = m_freq_governor->get_frequency_max();
        std::vector<double> target_freq;
        for (size_t ctl_idx = 0; ctl_idx < (size_t) m_num_freq_ctl_domain; ++ctl_idx) {
            const uint64_t curr_hash = m_last_region[ctl_idx].hash;
            const uint64_t curr_hint = m_last_region[ctl_idx].hint;
            auto it = m_hash_freq_map.find(curr_hash);
            if (it != m_hash_freq_map.end()) {
                freq = it->second;
            }
            else {
                switch(curr_hint) {
                    // Hints for low CPU frequency
                    case GEOPM_REGION_HINT_MEMORY:
                    case GEOPM_REGION_HINT_NETWORK:
                    case GEOPM_REGION_HINT_IO:
                        freq = freq_min;
                        break;

                    // Hints for maximum CPU frequency
                    case GEOPM_REGION_HINT_COMPUTE:
                    case GEOPM_REGION_HINT_SERIAL:
                    case GEOPM_REGION_HINT_PARALLEL:
                        freq = freq_max;
                        break;
                    // Hint Inconclusive
                    //case GEOPM_REGION_HINT_UNKNOWN:
                    //case GEOPM_REGION_HINT_IGNORE:
                    default:
                        freq = freq_max;
                        break;
                }
            }
            m_hash_freq_map[m_last_region[ctl_idx].hash] = freq;
            target_freq.push_back(freq);
        }

        m_freq_governor->adjust_platform(target_freq);
    }

    bool FrequencyMapAgent::do_write_batch(void) const
    {
        return m_freq_governor->do_write_batch();
    }

    void FrequencyMapAgent::sample_platform(std::vector<double> &out_sample)
    {
        for (size_t ctl_idx = 0; ctl_idx < (size_t) m_num_freq_ctl_domain; ++ctl_idx) {
            m_last_region[ctl_idx].hash = m_platform_io.sample(m_signal_idx[M_SIGNAL_REGION_HASH][ctl_idx]);
            m_last_region[ctl_idx].hint = m_platform_io.sample(m_signal_idx[M_SIGNAL_REGION_HINT][ctl_idx]);
        }
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
        std::vector<std::string> names{"FREQ_MIN", "FREQ_MAX"};
        names.reserve(M_NUM_POLICY);

        for (size_t i = 0; names.size() < M_NUM_POLICY; ++i) {
            names.emplace_back("HASH_" + std::to_string(i));
            names.emplace_back("FREQ_" + std::to_string(i));
        }

        return names;
    }

    std::vector<std::string> FrequencyMapAgent::sample_names(void)
    {
        return {};
    }

    std::vector<std::pair<std::string, std::string> > FrequencyMapAgent::report_header(void) const
    {
        return {};
    }

    std::vector<std::pair<std::string, std::string> > FrequencyMapAgent::report_host(void) const
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

    std::vector<std::function<std::string(double)> > FrequencyMapAgent::trace_formats(void) const
    {
        return {};
    }

    void FrequencyMapAgent::trace_values(std::vector<double> &values)
    {

    }

    void FrequencyMapAgent::enforce_policy(const std::vector<double> &policy) const
    {
        if (!is_valid_policy_size(policy)) {
            throw Exception("FrequencyMapAgent::enforce_policy(): policy vector incorrectly sized.",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        m_platform_io.write_control("FREQUENCY", GEOPM_DOMAIN_BOARD, 0, policy[M_POLICY_FREQ_MAX]);
    }

    void FrequencyMapAgent::init_platform_io(void)
    {
        const struct m_region_info_s DEFAULT_REGION { .hash = GEOPM_REGION_HASH_UNMARKED,
                                                      .hint = GEOPM_REGION_HINT_UNKNOWN};
        m_freq_governor->init_platform_io();
        const int freq_ctl_domain_type = m_freq_governor->frequency_domain_type();
        m_num_freq_ctl_domain = m_platform_topo.num_domain(freq_ctl_domain_type);
        m_last_region = std::vector<struct m_region_info_s>(m_num_freq_ctl_domain,
                                                            DEFAULT_REGION);
        std::vector<std::string> signal_names = {"REGION_HASH", "REGION_HINT"};
        for (size_t sig_idx = 0; sig_idx < signal_names.size(); ++sig_idx) {
            m_signal_idx.push_back(std::vector<int>());
            for (size_t ctl_idx = 0; ctl_idx < (size_t) m_num_freq_ctl_domain; ++ctl_idx) {
                m_signal_idx[sig_idx].push_back(m_platform_io.push_signal(signal_names[sig_idx],
                                                                          freq_ctl_domain_type,
                                                                          ctl_idx));
            }
        }
    }

    bool FrequencyMapAgent::is_valid_policy_size(const std::vector<double> &policy) const
    {
        bool ret = true;
        if (m_is_initialized_with_map) {
            if (policy.size() != M_POLICY_FIRST_HASH) {
                ret = false;
            }
        }
        else {
            if (policy.size() > M_NUM_POLICY ||
                policy.size() < M_POLICY_FIRST_HASH || policy.size() % 2) {
                ret = false;
            }
        }
        return ret;
    }
}
