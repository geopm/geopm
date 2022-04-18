/*
 * Copyright (c) 2015 - 2022, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "FrequencyMapAgent.hpp"

#include <sstream>
#include <cmath>
#include <iomanip>
#include <iostream>
#include <utility>
#include <algorithm>

#include "geopm/json11.hpp"

#include "geopm_hash.h"

#include "Environment.hpp"
#include "PlatformIOProf.hpp"
#include "geopm/PlatformIO.hpp"
#include "geopm/PlatformTopo.hpp"
#include "FrequencyGovernor.hpp"
#include "geopm/Helper.hpp"
#include "geopm/Exception.hpp"
#include "geopm_debug.hpp"
#include "config.h"

using json11::Json;

namespace geopm
{
    FrequencyMapAgent::FrequencyMapAgent()
        : FrequencyMapAgent(PlatformIOProf::platform_io(), platform_topo())
    {

    }

    FrequencyMapAgent::FrequencyMapAgent(const std::map<uint64_t, double>& hash_freq_map,
                                         const std::set<uint64_t>& default_freq_hash,
                                         PlatformIO &plat_io, const PlatformTopo &topo)
        : FrequencyMapAgent(plat_io, topo)
    {
        m_hash_freq_map = hash_freq_map;
        m_default_freq_hash = default_freq_hash;
    }

    FrequencyMapAgent::FrequencyMapAgent(PlatformIO &plat_io, const PlatformTopo &topo)
        : M_WAIT_SEC(0.002)
        , m_platform_io(plat_io)
        , m_platform_topo(topo)
        , m_wait_time(time_zero())
        , m_uncore_min_ctl_idx(-1)
        , m_uncore_max_ctl_idx(-1)
        , m_last_uncore_freq(NAN)
        , m_num_children(0)
        , m_is_policy_updated(false)
        , m_do_write_batch(false)
        , m_is_adjust_initialized(false)
        , m_is_real_policy(false)
        , m_freq_ctl_domain_type(GEOPM_DOMAIN_INVALID)
        , m_num_freq_ctl_domain(0)
        , m_core_freq_min(NAN)
        , m_core_freq_max(NAN)
        , m_uncore_init_min(NAN)
        , m_uncore_init_max(NAN)
        , m_default_freq(NAN)
        , m_uncore_freq(NAN)
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
        if (level == 0) {
            m_num_children = 0;
            init_platform_io();
        }
        else {
            m_num_children = fan_in[level - 1];
        }
    }

    void FrequencyMapAgent::validate_policy(std::vector<double> &policy) const
    {
        GEOPM_DEBUG_ASSERT(policy.size() == M_NUM_POLICY,
                           "FrequencyMapAgent::" + std::string(__func__) +
                           "(): policy vector not correctly sized.");

        if (is_all_nan(policy)) {
            // All-NAN policy may be received before the first policy
            /// @todo: in the future, this should not be accepted by this agent.
            return;
        }

        if (std::isnan(policy[M_POLICY_FREQ_DEFAULT])) {
            throw Exception("FrequencyMapAgent::" + std::string(__func__) +
                            "(): default frequency must be provided in policy.",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        if (policy[M_POLICY_FREQ_DEFAULT] > m_core_freq_max ||
            policy[M_POLICY_FREQ_DEFAULT] < m_core_freq_min) {
            throw Exception("FrequencyMapAgent::" + std::string(__func__) +
                            "(): default frequency out of range: " +
                            std::to_string(policy[M_POLICY_FREQ_DEFAULT]) + ".",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }

        // Validate all (hash, frequency) pairs
        std::set<double> policy_regions;
        for (auto it = policy.begin() + M_POLICY_FIRST_HASH;
             it != policy.end() && std::next(it) != policy.end(); std::advance(it, 2)) {
            auto mapped_freq = *(it + 1);

            if (!std::isnan(*it)) {
                // We are using a static cast rather than reinterpreting the
                // memory so that regions can be input to this policy in the
                // same form they are output from a report.
                auto region = static_cast<uint64_t>(*it);
                if (std::isnan(mapped_freq)) {
                    throw Exception("FrequencyMapAgent::" + std::string(__func__) +
                                    "(): mapped region with no frequency assigned.",
                                    GEOPM_ERROR_INVALID, __FILE__, __LINE__);
                }
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

    bool FrequencyMapAgent::is_all_nan(const std::vector<double> &vec)
    {
        return std::all_of(vec.begin(), vec.end(),
                           [](double x) -> bool { return std::isnan(x); });
    }

    void FrequencyMapAgent::update_policy(const std::vector<double> &policy)
    {
        if (is_all_nan(policy) && !m_is_real_policy) {
            // All-NAN policy is ignored until first real policy is received
            m_is_policy_updated = false;
            return;
        }
        else if (is_all_nan(policy)) {
            throw Exception("FrequencyMapAgent::update_policy(): received invalid all-NAN policy.",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        m_is_real_policy = true;

        std::map<uint64_t, double> old_freq_map = m_hash_freq_map;
        m_hash_freq_map.clear();
        for (auto it = policy.begin() + M_POLICY_FIRST_HASH;
             it != policy.end() && std::next(it) != policy.end();
             std::advance(it, 2)) {
            if (!std::isnan(*it)) {
                auto hash = static_cast<uint64_t>(*it);
                auto freq = *(it + 1);

                // Not valid to have NAN freq for hash.
                // This is a logic error because it is checked by
                // validate policy, which the controller should
                // call before this function.
                GEOPM_DEBUG_ASSERT(!std::isnan(freq),
                                   "mapped region with no frequency assigned.");
                m_hash_freq_map[hash] = freq;
            }
        }
        m_is_policy_updated = false;
        // check if policy changed
        if (m_default_freq != policy[M_POLICY_FREQ_DEFAULT]) {
            m_is_policy_updated = true;
            m_default_freq = policy[M_POLICY_FREQ_DEFAULT];
        }
        if (m_hash_freq_map != old_freq_map) {
            m_is_policy_updated = true;
        }
        if (!std::isnan(policy[M_POLICY_FREQ_UNCORE]) &&
            m_uncore_freq != policy[M_POLICY_FREQ_UNCORE]) {
            m_is_policy_updated = true;
        }
        m_uncore_freq = policy[M_POLICY_FREQ_UNCORE];
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
            if (child_policy.size() != M_NUM_POLICY) {
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

        m_do_write_batch = false;

        if (!m_is_adjust_initialized) {
            // adjust all controls once in case not applied by policy
            for (size_t ctl_idx = 0; ctl_idx < (size_t) m_num_freq_ctl_domain; ++ctl_idx) {
                // @todo: this is bad; agent should be able to use aliases
                double val = m_platform_io.read_signal("MSR::PERF_CTL:FREQ", m_freq_ctl_domain_type, ctl_idx);
                m_platform_io.adjust(m_freq_control_idx[ctl_idx], val);
            }
            m_platform_io.adjust(m_uncore_min_ctl_idx, m_uncore_init_min);
            m_platform_io.adjust(m_uncore_max_ctl_idx, m_uncore_init_max);
            m_is_adjust_initialized = true;
        }

        if (is_all_nan(in_policy) && !m_is_real_policy) {
            // All-NAN policy may be received before the first policy
            return;
        }

        double freq = NAN;
        for (size_t ctl_idx = 0; ctl_idx < (size_t) m_num_freq_ctl_domain; ++ctl_idx) {
            const uint64_t curr_hash = m_last_hash[ctl_idx];
            auto it = m_hash_freq_map.find(curr_hash);
            if (it != m_hash_freq_map.end()) {
                freq = it->second;
            }
            else {
                m_default_freq_hash.insert(curr_hash);
                freq = m_default_freq;
            }
            if (m_last_freq[ctl_idx] != freq) {
                m_last_freq[ctl_idx] = freq;
                m_platform_io.adjust(m_freq_control_idx[ctl_idx], freq);
                m_do_write_batch = true;
            }
        }

        // adjust fixed uncore freq
        if (m_last_uncore_freq != m_uncore_freq) {
            if (!std::isnan(m_uncore_freq)) {
                m_platform_io.adjust(m_uncore_min_ctl_idx, m_uncore_freq);
                m_platform_io.adjust(m_uncore_max_ctl_idx, m_uncore_freq);
                m_do_write_batch = true;
            }
            else if (!std::isnan(m_last_uncore_freq)) {
                m_platform_io.adjust(m_uncore_min_ctl_idx, m_uncore_init_min);
                m_platform_io.adjust(m_uncore_max_ctl_idx, m_uncore_init_max);
                m_do_write_batch = true;
            }
            m_last_uncore_freq = m_uncore_freq;
        }
    }

    bool FrequencyMapAgent::do_write_batch(void) const
    {
        return m_do_write_batch;
    }

    void FrequencyMapAgent::sample_platform(std::vector<double> &out_sample)
    {
        for (size_t ctl_idx = 0; ctl_idx < (size_t) m_num_freq_ctl_domain; ++ctl_idx) {
            m_last_hash[ctl_idx] = m_platform_io.sample(m_hash_signal_idx[ctl_idx]);
        }
    }

    void FrequencyMapAgent::wait(void)
    {
        while(geopm_time_since(&m_wait_time) < M_WAIT_SEC) {

        }
        geopm_time(&m_wait_time);
    }

    std::vector<std::string> FrequencyMapAgent::policy_names(void)
    {

        std::vector<std::string> names{"FREQ_DEFAULT", "FREQ_UNCORE"};
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

        std::map<uint64_t, double> full_map(m_hash_freq_map);
        for (const auto &region : m_default_freq_hash) {
            full_map.insert({region, m_default_freq});
        }

        std::map<std::string, Json> temp_map;
        for (const auto &region : full_map)
        {
            std::ostringstream key_ss;
            key_ss << "0x" << std::hex << std::setfill('0') << std::setw(16) << std::fixed;
            key_ss << region.first;
            std::string key_string = key_ss.str();
            temp_map[key_string] = region.second;
        }
        Json my_json = Json(temp_map);

        std::string frequency_map_data = my_json.dump();
        frequency_map_data.erase(std::remove(frequency_map_data.begin(), frequency_map_data.end(), '"'), frequency_map_data.end());
        result.push_back(std::make_pair("Frequency map", frequency_map_data));

        return result;
    }

    std::map<uint64_t, std::vector<std::pair<std::string, std::string> > > FrequencyMapAgent::report_region(void) const
    {
        std::map<uint64_t, std::vector<std::pair<std::string, std::string> > > result;
        for (const auto &region : m_hash_freq_map) {
            result[region.first].push_back(std::make_pair("frequency-map", std::to_string(region.second)));
        }
        for (const auto region : m_default_freq_hash) {
            result[region].push_back(std::make_pair("frequency-map", std::to_string(m_default_freq)));
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
        if (policy.size() != M_NUM_POLICY) {
            throw Exception("FrequencyMapAgent::enforce_policy(): policy vector incorrectly sized.",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }

        if (is_all_nan(policy)) {
            // All-NAN policy is invalid
            throw Exception("FrequencyMapAgent::enforce_policy(): received invalid all-NAN policy.",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);

            return;
        }
        m_platform_io.write_control("CPU_FREQUENCY_CONTROL", GEOPM_DOMAIN_BOARD, 0,
                                    policy[M_POLICY_FREQ_DEFAULT]);
        if (!std::isnan(policy[M_POLICY_FREQ_UNCORE])) {
                m_platform_io.write_control("MSR::UNCORE_RATIO_LIMIT:MIN_RATIO", GEOPM_DOMAIN_BOARD, 0,
                                            policy[M_POLICY_FREQ_UNCORE]);
                m_platform_io.write_control("MSR::UNCORE_RATIO_LIMIT:MAX_RATIO", GEOPM_DOMAIN_BOARD, 0,
                                            policy[M_POLICY_FREQ_UNCORE]);
        }
    }

    void FrequencyMapAgent::init_platform_io(void)
    {
        m_freq_ctl_domain_type = m_platform_io.control_domain_type("CPU_FREQUENCY_CONTROL");
        m_num_freq_ctl_domain = m_platform_topo.num_domain(m_freq_ctl_domain_type);
        m_last_hash = std::vector<uint64_t>(m_num_freq_ctl_domain,
                                            GEOPM_REGION_HASH_UNMARKED);
        m_last_freq = std::vector<double>(m_num_freq_ctl_domain, NAN);
        for (size_t ctl_idx = 0; ctl_idx < (size_t) m_num_freq_ctl_domain; ++ctl_idx) {
            m_hash_signal_idx.push_back(m_platform_io.push_signal("REGION_HASH",
                                                                  m_freq_ctl_domain_type,
                                                                  ctl_idx));
            m_freq_control_idx.push_back(m_platform_io.push_control("CPU_FREQUENCY_CONTROL",
                                                                    m_freq_ctl_domain_type,
                                                                    ctl_idx));
        }
        m_uncore_min_ctl_idx = m_platform_io.push_control("MSR::UNCORE_RATIO_LIMIT:MIN_RATIO", GEOPM_DOMAIN_BOARD, 0);
        m_uncore_max_ctl_idx = m_platform_io.push_control("MSR::UNCORE_RATIO_LIMIT:MAX_RATIO", GEOPM_DOMAIN_BOARD, 0);

        m_core_freq_min = m_platform_io.read_signal("CPU_FREQUENCY_MIN", GEOPM_DOMAIN_BOARD, 0);
        m_core_freq_max = m_platform_io.read_signal("CPU_FREQUENCY_MAX", GEOPM_DOMAIN_BOARD, 0);
        m_uncore_init_min = m_platform_io.read_signal("MSR::UNCORE_RATIO_LIMIT:MIN_RATIO", GEOPM_DOMAIN_BOARD, 0);
        m_uncore_init_max = m_platform_io.read_signal("MSR::UNCORE_RATIO_LIMIT:MAX_RATIO", GEOPM_DOMAIN_BOARD, 0);
    }
}
