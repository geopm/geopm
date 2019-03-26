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

#include "EnergyEfficientRegion.hpp"
#include "EnergyEfficientAgent.hpp"
#include "PlatformIO.hpp"
#include "PlatformTopo.hpp"
#include "Helper.hpp"
#include "Exception.hpp"
#include "config.h"

using json11::Json;

namespace geopm
{
    EnergyEfficientAgent::EnergyEfficientAgent()
        : EnergyEfficientAgent(platform_io(), platform_topo())
    {

    }

    EnergyEfficientAgent::EnergyEfficientAgent(PlatformIO &plat_io, PlatformTopo &topo)
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

    }

    std::string EnergyEfficientAgent::plugin_name(void)
    {
        return "energy_efficient";
    }

    std::unique_ptr<Agent> EnergyEfficientAgent::make_plugin(void)
    {
        return geopm::make_unique<EnergyEfficientAgent>();
    }

    void EnergyEfficientAgent::init(int level, const std::vector<int> &fan_in, bool is_level_root)
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

    bool EnergyEfficientAgent::update_freq_range(const std::vector<double> &in_policy)
    {
#ifdef GEOPM_DEBUG
        if (in_policy.size() != M_NUM_POLICY) {
            throw Exception("EnergyEfficientAgent::" + std::string(__func__) + "(): in_policy vector not correctly sized.",
                            GEOPM_ERROR_LOGIC, __FILE__, __LINE__);
        }
#endif
        bool result = false;
        double target_freq_min = std::isnan(in_policy[M_POLICY_FREQ_MIN]) ? get_limit("CPUINFO::FREQ_MIN") : in_policy[M_POLICY_FREQ_MIN];
        double target_freq_max = std::isnan(in_policy[M_POLICY_FREQ_MAX]) ? get_limit("CPUINFO::FREQ_MAX") : in_policy[M_POLICY_FREQ_MAX];
        if (std::isnan(target_freq_min) || std::isnan(target_freq_max) ||
            target_freq_min > target_freq_max) {
            throw Exception("EnergyEfficientAgent::" + std::string(__func__) + "(): invalid frequency bounds.",
                            GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
        }
        if (m_freq_min != target_freq_min ||
            m_freq_max != target_freq_max) {
            m_freq_min = target_freq_min;
            m_freq_max = target_freq_max;
            for (auto &eer : m_region_map) {
                eer.second->update_freq_range(m_freq_min, m_freq_max, M_FREQ_STEP);
            }
            result = true;
        }
        return result;
    }

    void EnergyEfficientAgent::validate_policy(std::vector<double> &policy) const
    {

    }

    bool EnergyEfficientAgent::descend(const std::vector<double> &in_policy,
                                       std::vector<std::vector<double> >&out_policy)
    {
#ifdef GEOPM_DEBUG
        if (out_policy.size() != (size_t) m_num_children) {
            throw Exception("EnergyEfficientAgent::" + std::string(__func__) + "(): out_policy vector not correctly sized.",
                            GEOPM_ERROR_LOGIC, __FILE__, __LINE__);
        }
#endif
        bool result = update_freq_range(in_policy);

        if (result) {
            for (auto &child_policy : out_policy) {
#ifdef GEOPM_DEBUG
                if (child_policy.size() != M_NUM_POLICY) {
                    throw Exception("EnergyEfficientAgent::" + std::string(__func__) + "(): child_policy vector not correctly sized.",
                                    GEOPM_ERROR_LOGIC, __FILE__, __LINE__);
                }
#endif
                child_policy[M_POLICY_FREQ_MIN] = m_freq_min;
                child_policy[M_POLICY_FREQ_MAX] = m_freq_max;
            }
        }
        return result;
    }

    bool EnergyEfficientAgent::ascend(const std::vector<std::vector<double> > &in_sample,
                                      std::vector<double> &out_sample)
    {
        return false;
    }

    bool EnergyEfficientAgent::adjust_platform(const std::vector<double> &in_policy)
    {
        update_freq_range(in_policy);
        bool result = false;
        double freq = m_last_freq;
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
        m_adapt_freq_map[m_last_region.first] = freq;

        if (freq != m_last_freq) {
            /// freq initialized to m_last_freq but frequency bounds may have changed since
            /// the time of the caching.  Need to make sure when we're within our bounds
            if (m_freq_min > freq) {
                freq = m_freq_min;
            }
            else if (m_freq_max < freq) {
                freq = m_freq_max;
            }
            for (auto ctl_idx : m_control_idx) {
                m_platform_io.adjust(ctl_idx, freq);
            }
            m_last_freq = freq;
            result = true;
        }
        return result;
    }

    bool EnergyEfficientAgent::sample_platform(std::vector<double> &out_sample)
    {
        const uint64_t current_region_hash = m_platform_io.sample(m_signal_idx[M_SIGNAL_REGION_HASH]);
        const uint64_t current_region_hint = m_platform_io.sample(m_signal_idx[M_SIGNAL_REGION_HINT]);
        const bool is_region_boundary = m_last_region.first != current_region_hash;
        // update current region (entry)
        if (is_region_boundary) {
            const bool is_not_invalid = current_region_hash != GEOPM_REGION_HASH_INVALID;
            double target_freq = NAN;
            if (current_region_hint == GEOPM_REGION_HINT_COMPUTE) {
                /// Compute regions are assigned maximum frequency, no learning
                target_freq = m_freq_max;
            }
            else if (is_not_invalid) {
                // set the freq for the current region (entry)
                auto curr_region_it = m_region_map.find(current_region_hash);
                if (curr_region_it == m_region_map.end()) {
                    auto tmp = m_region_map.emplace(current_region_hash, std::unique_ptr<EnergyEfficientRegion>(
                                                    new EnergyEfficientRegion(m_platform_io,
                                                        m_freq_min, m_freq_max, M_FREQ_STEP,
                                                        m_signal_idx[M_SIGNAL_RUNTIME])));
                    curr_region_it = tmp.first;
                }
                target_freq = curr_region_it->second->freq();
            }
            else {
                throw Exception("EnergyEfficientAgent::" + std::string(__func__) + "(): unexpected (region_hash:hint)",
                                GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
            }
            m_adapt_freq_map[current_region_hash] = target_freq;
        }
        // update previous region (exit)
        if (is_region_boundary) {
            const uint64_t last_region_hash = m_last_region.first;
            const uint64_t last_region_hint = m_last_region.second;
            const bool is_not_invalid = last_region_hash != GEOPM_REGION_HASH_INVALID;
            if (is_not_invalid && last_region_hash != GEOPM_REGION_HASH_INVALID &&
                    last_region_hint != GEOPM_REGION_HINT_COMPUTE) {
                auto last_region_it = m_region_map.find(last_region_hash);
                if (last_region_it == m_region_map.end()) {
                    throw Exception("EnergyEfficientAgent::" + std::string(__func__) + "(): region exit before entry detected.",
                                    GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
                }
                last_region_it->second->update_exit();
            }
            m_last_region = std::make_pair(current_region_hash, current_region_hint);
        }
        return false;
    }

    void EnergyEfficientAgent::wait(void)
    {
        static double M_WAIT_SEC = 0.005;
        while(geopm_time_since(&m_last_wait) < M_WAIT_SEC) {

        }
        geopm_time(&m_last_wait);
    }

    std::vector<std::string> EnergyEfficientAgent::policy_names(void)
    {
        return {"FREQ_MIN", "FREQ_MAX"};
    }

    std::vector<std::string> EnergyEfficientAgent::sample_names(void)
    {
        return {};
    }

    std::vector<std::pair<std::string, std::string> > EnergyEfficientAgent::report_header(void) const
    {
        return {};
    }

    std::vector<std::pair<std::string, std::string> > EnergyEfficientAgent::report_node(void) const
    {
        std::vector<std::pair<std::string, std::string> > result;
        std::ostringstream oss;
        oss << std::setprecision(EnergyEfficientAgent::M_PRECISION) << std::scientific;
        for (const auto &region : m_region_map) {
            oss << "\n\t0x" << std::hex << std::setfill('0') << std::setw(16) << std::fixed;
            oss << region.first;
            oss << std::setfill('\0') << std::setw(0) << std::scientific;
            oss << ":" << region.second->freq();
        }
        oss << "\n";
        result.push_back({"Final online freq map", oss.str()});

        return result;
    }


    std::map<uint64_t, std::vector<std::pair<std::string, std::string> > > EnergyEfficientAgent::report_region(void) const
    {
        std::map<uint64_t, std::vector<std::pair<std::string, std::string> > > result;
        // If region is in this map, online learning was used to set frequency
        for (const auto &region : m_region_map) {
            /// @todo re-implement with m_region_map and m_hash_freq_map keys as pair (hash + hint)
            result[region.first].push_back(std::make_pair("requested-online-frequency", std::to_string(region.second->freq())));
        }
        return result;
    }

    std::vector<std::string> EnergyEfficientAgent::trace_names(void) const
    {
        return {};
    }

    void EnergyEfficientAgent::trace_values(std::vector<double> &values)
    {
    }

    double EnergyEfficientAgent::get_limit(const std::string &sig_name) const
    {
        const int domain_type = m_platform_io.signal_domain_type(sig_name);
        double result = NAN;
        const double sticker_freq = m_platform_io.read_signal("CPUINFO::FREQ_STICKER", domain_type, 0);
        if (sig_name == "CPUINFO::FREQ_MIN") {
            if (domain_type == GEOPM_DOMAIN_INVALID) {
                if (m_platform_io.signal_domain_type("CPUINFO::FREQ_STICKER") == GEOPM_DOMAIN_INVALID) {
                    throw Exception("EnergyEfficientAgent::" + std::string(__func__) + "(): unable to parse min and sticker frequencies.",
                                    GEOPM_ERROR_AGENT_UNSUPPORTED, __FILE__, __LINE__);
                }
            }
            else {
                result = m_platform_io.read_signal(sig_name, domain_type, 0);
            }
        }
        else if (sig_name == "CPUINFO::FREQ_MAX") {
            if (domain_type == GEOPM_DOMAIN_INVALID) {
                if (m_platform_io.signal_domain_type("CPUINFO::FREQ_STICKER") == GEOPM_DOMAIN_INVALID) {
                    throw Exception("EnergyEfficientAgent::" + std::string(__func__) + "(): unable to parse max and sticker frequencies.",
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
            throw Exception("EnergyEfficientAgent::" + std::string(__func__) + "(): requested invalid signal name.",
                            GEOPM_ERROR_LOGIC, __FILE__, __LINE__);
        }
#endif
        return result;
    }

    void EnergyEfficientAgent::init_platform_io(void)
    {
        const int freq_ctl_domain_type = GEOPM_DOMAIN_BOARD; //m_platform_io.control_domain_type("FREQUENCY");
        const int num_freq_ctl_domain = m_platform_topo.num_domain(freq_ctl_domain_type);
        for (int ctl_dom_idx = 0; ctl_dom_idx != num_freq_ctl_domain; ++ctl_dom_idx) {
            m_control_idx.push_back(m_platform_io.push_control("FREQUENCY",
                        freq_ctl_domain_type, ctl_dom_idx));
        }

        std::vector<std::string> signal_names = {"REGION_HASH", "REGION_HINT", "REGION_RUNTIME"};

        /// @todo handle per frequency control domain cardinality
        for (size_t sig_idx = 0; sig_idx < signal_names.size(); ++sig_idx) {
            m_signal_idx.push_back(m_platform_io.push_signal(signal_names[sig_idx],
                        freq_ctl_domain_type, sig_idx));
        }
    }
}
