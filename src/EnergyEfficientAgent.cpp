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

#include <sstream>

#include "geopm.h"
#include "geopm_hash.h"
#include "geopm_message.h"

#include "EnergyEfficientAgent.hpp"
#include "PlatformIO.hpp"
#include "PlatformTopo.hpp"
#include "Helper.hpp"
#include "Exception.hpp"
#include "config.h"

namespace geopm
{
    EnergyEfficientAgent::EnergyEfficientAgent()
        : EnergyEfficientAgent(platform_io(), platform_topo())
    {

    }

    EnergyEfficientAgent::EnergyEfficientAgent(IPlatformIO &plat_io, IPlatformTopo &topo)
        : m_platform_io(plat_io)
        , m_platform_topo(topo)
        , M_FREQ_MIN(cpu_freq_min())
        , M_FREQ_MAX(cpu_freq_max())
        , M_FREQ_STEP(get_limit("CPUINFO::FREQ_STEP"))
        , M_SEND_PERIOD(10)
        , m_last_freq(NAN)
        , m_curr_adapt_freq(NAN)
        , m_last_wait{{0, 0}}
        , m_runtime_idx(-1)
        , m_pkg_energy_idx(-1)
        , m_dram_energy_idx(-1)
    {
        parse_env_map();
        const char* env_freq_adapt_str = getenv("GEOPM_EFFICIENT_FREQ_ONLINE");
        if (env_freq_adapt_str) {
            m_is_adaptive = true;
        }
        init_platform_io();
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
    }

    bool EnergyEfficientAgent::descend(const std::vector<double> &in_policy,
                                       std::vector<std::vector<double> >&out_policy)
    {
        return true;
    }

    bool EnergyEfficientAgent::ascend(const std::vector<std::vector<double> > &in_sample,
                                      std::vector<double> &out_sample)
    {
#ifdef GEOPM_DEBUG
        if (out_sample.size() != m_num_sample) {
            throw Exception("EnergyEfficientAgent::ascend(): out_sample vector not correctly sized.",
                            GEOPM_ERROR_LOGIC, __FILE__, __LINE__);
        }
#endif
        bool result = false;
        if (m_num_ascend == 0) {
            std::vector<double> child_sample(in_sample.size());
            for (size_t sig_idx = 0; sig_idx < m_num_sample; ++sig_idx) {
                for (size_t child_idx = 0; child_idx < in_sample.size(); ++child_idx) {
                    child_sample[child_idx] = in_sample[child_idx][sig_idx];
                }
                out_sample[sig_idx] = m_agg_func[sig_idx](child_sample);
            }
            result = true;
        }
        ++m_num_ascend;
        if (m_num_ascend == M_SEND_PERIOD) {
           m_num_ascend = 0;
        }
        return result;
    }

    bool EnergyEfficientAgent::adjust_platform(const std::vector<double> &in_policy)
    {
        bool result = false;
        double freq = m_last_freq;
        auto it = m_rid_freq_map.find(geopm_region_id_hash(m_last_region_id));
        if (it != m_rid_freq_map.end()) {
            freq = it->second;
        }
        else if (m_is_adaptive) {
            if (!isnan(m_curr_adapt_freq)) {
                freq = m_curr_adapt_freq;
            }
            else {
                freq = M_FREQ_MAX - M_FREQ_STEP;
            }
        }
        else {
            switch(geopm_region_id_hint(m_last_region_id)) {
                // Hints for low CPU frequency
                case GEOPM_REGION_HINT_MEMORY:
                case GEOPM_REGION_HINT_NETWORK:
                case GEOPM_REGION_HINT_IO:
                    freq = M_FREQ_MIN;
                    break;

                // Hints for maximum CPU frequency
                case GEOPM_REGION_HINT_COMPUTE:
                case GEOPM_REGION_HINT_SERIAL:
                case GEOPM_REGION_HINT_PARALLEL:
                    freq = M_FREQ_MAX;
                    break;
                // Hint Inconclusive
                //case GEOPM_REGION_HINT_UNKNOWN:
                //case GEOPM_REGION_HINT_IGNORE:
                default:
                    freq = M_FREQ_MIN;
                    break;
            }
        }

        if (freq != m_last_freq) {
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
#ifdef GEOPM_DEBUG
        if (out_sample.size() != m_num_sample) {
            throw Exception("EnergyEfficientAgent::sample_platform(): out_sample vector not correctly sized.",
                            GEOPM_ERROR_LOGIC, __FILE__, __LINE__);
        }
#endif
        for (size_t sample_idx = 0; sample_idx < m_num_sample; ++sample_idx) {
            out_sample[sample_idx] = m_platform_io.sample(m_sample_idx[sample_idx]);
        }
        int num_domain = m_platform_topo.num_domain(PlatformTopo::M_DOMAIN_CPU);
        uint64_t current_region_id = geopm_signal_to_field(m_platform_io.sample(m_region_id_idx));
        if (m_is_adaptive) {
            if (current_region_id != GEOPM_REGION_ID_UNMARKED &&
                current_region_id != GEOPM_REGION_ID_UNDEFINED) {
                bool is_region_boundary = m_last_region_id != current_region_id;
                if (is_region_boundary) {
                    // set the freq for the current region (entry)
                    auto region_it = m_region_map.find(current_region_id);
                    if (region_it == m_region_map.end()) {
                        auto tmp = m_region_map.emplace(
                            current_region_id,
                            std::unique_ptr<EnergyEfficientRegion>(
                                new EnergyEfficientRegion(m_platform_io, M_FREQ_MIN,
                                    M_FREQ_MAX, M_FREQ_STEP,
                                    num_domain,
                                    m_runtime_idx,
                                    m_pkg_energy_idx,
                                    m_dram_energy_idx)));
                        region_it = tmp.first;
                    }
                    region_it->second->update_entry();

                    m_curr_adapt_freq = region_it->second->freq();
                }
                if (m_last_region_id != 0 && is_region_boundary) {
                    // update previous region (exit)
                    auto region_it = m_region_map.find(m_last_region_id);
                    if (region_it == m_region_map.end()) {
                        auto tmp = m_region_map.emplace(
                            m_last_region_id,
                            std::unique_ptr<EnergyEfficientRegion>(
                                new EnergyEfficientRegion(m_platform_io, M_FREQ_MIN,
                                    M_FREQ_MAX, M_FREQ_STEP,
                                    num_domain,
                                    m_runtime_idx,
                                    m_pkg_energy_idx,
                                    m_dram_energy_idx)));
                        region_it = tmp.first;
                    }
                    region_it->second->update_exit();
                }
            }
        }
        m_last_region_id = current_region_id;
        return true;
    }

    void EnergyEfficientAgent::wait(void)
    {
        static double M_WAIT_SEC = 0.005;
        geopm_time_s current_time;
        geopm_time(&current_time);
        while(geopm_time_diff(&m_last_wait, &current_time) < M_WAIT_SEC) {
            geopm_time(&current_time);
        }
        geopm_time(&m_last_wait);
    }

    std::vector<std::string> EnergyEfficientAgent::policy_names(void)
    {
        return {};
    }

    std::vector<std::string> EnergyEfficientAgent::sample_names(void)
    {
        return {"ENERGY_PACKAGE", "ENERGY_DRAM"};
    }

    std::vector<std::pair<std::string, std::string> > EnergyEfficientAgent::report_header(void)
    {
        return {};
    }

    std::vector<std::pair<std::string, std::string> > EnergyEfficientAgent::report_node(void)
    {
        std::vector<std::pair<std::string, std::string> > result;
        std::ostringstream oss;
        if (m_region_map.size() > 0) {
            for (const auto &region : m_region_map) {
                oss << region.first << ":" << region.second->freq() << " ";
            }
        }
        result.push_back({"Final freq map", oss.str()});
        return result;
    }

    std::map<uint64_t, std::vector<std::pair<std::string, std::string> > > EnergyEfficientAgent::report_region(void)
    {
        return {};
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
            if (domain_type == IPlatformTopo::M_DOMAIN_INVALID) {
                if (m_platform_io.signal_domain_type("CPUINFO::FREQ_STICKER") == IPlatformTopo::M_DOMAIN_INVALID) {
                    throw Exception("EnergyEfficientAgent: unable to parse min and sticker frequencies.",
                                    GEOPM_ERROR_DECIDER_UNSUPPORTED, __FILE__, __LINE__);
                }
            }
            else {
                result = m_platform_io.read_signal(sig_name, domain_type, 0);
            }
        }
        else if (sig_name == "CPUINFO::FREQ_MAX") {
            if (domain_type == IPlatformTopo::M_DOMAIN_INVALID) {
                if (m_platform_io.signal_domain_type("CPUINFO::FREQ_STICKER") == IPlatformTopo::M_DOMAIN_INVALID) {
                    throw Exception("EnergyEfficientAgent: unable to parse max and sticker frequencies.",
                                    GEOPM_ERROR_DECIDER_UNSUPPORTED, __FILE__, __LINE__);
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
            throw Exception("EnergyEfficientAgent: requested invalid signal name.",
                            GEOPM_ERROR_LOGIC, __FILE__, __LINE__);
        }
#endif
        return result;
    }

    void EnergyEfficientAgent::init_platform_io(void)
    {
        // All columns sampled will be in the trace
        for (auto sample : sample_names()) {
            m_sample_idx.push_back(m_platform_io.push_signal(sample,
                                                             IPlatformTopo::M_DOMAIN_BOARD,
                                                             0));
            m_agg_func.push_back(m_platform_io.agg_function(sample));
        }
        m_num_sample = m_sample_idx.size();

        uint64_t freq_domain_type = m_platform_io.control_domain_type("FREQUENCY");
        if (freq_domain_type == IPlatformTopo::M_DOMAIN_INVALID) {
            throw Exception("EnergyEfficientAgent: Platform does not support frequency control",
                            GEOPM_ERROR_DECIDER_UNSUPPORTED, __FILE__, __LINE__);
        }
        uint32_t num_freq_domain = m_platform_topo.num_domain(freq_domain_type);
        if (!num_freq_domain) {
            throw Exception("EnergyEfficientAgent: Platform does not support frequency control",
                            GEOPM_ERROR_DECIDER_UNSUPPORTED, __FILE__, __LINE__);
        }
        for (uint32_t dom_idx = 0; dom_idx != num_freq_domain; ++dom_idx) {
            int control_idx = m_platform_io.push_control("FREQUENCY", freq_domain_type, dom_idx);
            if (control_idx < 0) {
                throw Exception("EnergyEfficientAgent: Failed to enable frequency control in the platform.",
                                GEOPM_ERROR_DECIDER_UNSUPPORTED, __FILE__, __LINE__);
            }
            m_control_idx.push_back(control_idx);
        }
        m_region_id_idx = m_platform_io.push_signal("REGION_ID#", IPlatformTopo::M_DOMAIN_BOARD, 0);

        if (m_is_adaptive) {
            m_runtime_idx = m_platform_io.push_signal("REGION_RUNTIME", IPlatformTopo::M_DOMAIN_BOARD, 0);
            m_pkg_energy_idx = m_platform_io.push_signal("ENERGY_PACKAGE", IPlatformTopo::M_DOMAIN_BOARD, 0);
            m_dram_energy_idx = m_platform_io.push_signal("ENERGY_DRAM", IPlatformTopo::M_DOMAIN_BOARD, 0);
        }
    }

    void EnergyEfficientAgent::parse_env_map(void)
    {
        const char* env_freq_rid_map_str = getenv("GEOPM_EFFICIENT_FREQ_RID_MAP");
        if (env_freq_rid_map_str) {
            std::string full_str(env_freq_rid_map_str);
            size_t begin_pos = 0;
            size_t colon_pos = full_str.find(':');
            while (colon_pos != std::string::npos) {
                size_t comma_pos = full_str.find(',', colon_pos);
                if (comma_pos == std::string::npos) {
                    comma_pos = full_str.size();
                }
                std::string rid_str = full_str.substr(begin_pos, colon_pos - begin_pos);
                std::string freq_str = full_str.substr(colon_pos + 1, comma_pos - colon_pos - 1);
                if (!rid_str.empty() && !freq_str.empty()) {
                    try {
                        double freq = std::stod(freq_str);
                        uint64_t rid = geopm_crc32_str(0, rid_str.c_str());
                        m_rid_freq_map[rid] = freq;
                    }
                    catch (std::invalid_argument) {

                    }
                }
                if (comma_pos < full_str.size()) {
                    begin_pos = comma_pos + 1;
                    colon_pos = full_str.find(':', begin_pos);
                }
                else {
                    colon_pos = std::string::npos;
                }
            }
        }
    }

    double EnergyEfficientAgent::cpu_freq_min(void) const
    {
        double result = NAN;
        const char* env_efficient_freq_min = getenv("GEOPM_EFFICIENT_FREQ_MIN");
        if (env_efficient_freq_min) {
            try {
                result = std::stod(env_efficient_freq_min);
            }
            catch (std::invalid_argument) {

            }
        }
        if (isnan(result)) {
            result = get_limit("CPUINFO::FREQ_MIN");
        }

        return result;
    }

    double EnergyEfficientAgent::cpu_freq_max(void) const
    {
        double result = NAN;
        const char* env_efficient_freq_max = getenv("GEOPM_EFFICIENT_FREQ_MAX");
        if (env_efficient_freq_max) {
            try {
                result = std::stod(env_efficient_freq_max);
            }
            catch (std::invalid_argument) {

            }
        }
        if (isnan(result)) {
            result = get_limit("CPUINFO::FREQ_MAX");
        }

        return result;
    }
}
