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

#include <hwloc.h>
#include <stdlib.h>
#include <cmath>
#include <cctype>
#include <algorithm>
#include <fstream>
#include <string>

#include "geopm.h"
#include "geopm_message.h"
#include "geopm_hash.h"
#include "geopm_sched.h"

#include "PlatformIO.hpp"
#include "PlatformTopo.hpp"
#include "Policy.hpp"
#include "GoverningDecider.hpp"
#include "Exception.hpp"
#include "Region.hpp"
#include "EfficientFreqDecider.hpp"
#include "EfficientFreqRegion.hpp"

#define GEOPM_EFFICIENT_FREQ_DECIDER_PLUGIN_NAME "efficient_freq"

namespace geopm
{
    EfficientFreqDecider::EfficientFreqDecider()
        : EfficientFreqDecider(platform_io(),
                               platform_topo())
    {

    }

    EfficientFreqDecider::EfficientFreqDecider(IPlatformIO &pio,
                                               IPlatformTopo &ptopo)
        : GoverningDecider()
        , m_platform_io(pio)
        , m_platform_topo(ptopo)
        , m_freq_min(cpu_freq_min())
        , m_freq_max(cpu_freq_max())
        , m_freq_step(get_limit("CPUINFO::FREQ_STEP"))
        , m_num_cpu(m_platform_topo.num_domain(IPlatformTopo::M_DOMAIN_CPU))
        , m_last_freq(NAN)
    {
        m_name = plugin_name();
        parse_env_map();
        const char* env_freq_adapt_str = getenv("GEOPM_EFFICIENT_FREQ_ONLINE");
        if (env_freq_adapt_str) {
            m_is_adaptive = true;
        }
        init_platform_io();
    }

    EfficientFreqDecider::~EfficientFreqDecider()
    {

    }

    double EfficientFreqDecider::get_limit(const std::string &sig_name)
    {
        const int domain_type = m_platform_io.signal_domain_type(sig_name);
        double result = NAN;
        const double sticker_freq = m_platform_io.read_signal("CPUINFO::FREQ_STICKER", domain_type, 0);
        if (sig_name == "CPUINFO::FREQ_MIN") {
            if (domain_type == IPlatformTopo::M_DOMAIN_INVALID) {
                if (m_platform_io.signal_domain_type("CPUINFO::FREQ_STICKER") == IPlatformTopo::M_DOMAIN_INVALID) {
                    throw Exception("EfficientFreqDecider: unable to parse min and sticker frequencies.",
                                    GEOPM_ERROR_DECIDER_UNSUPPORTED, __FILE__, __LINE__);
                }
                result = sticker_freq - 6 * m_freq_step;
            }
            else {
                result = m_platform_io.read_signal(sig_name, domain_type, 0);
            }
        }
        else if (sig_name == "CPUINFO::FREQ_MAX") {
            if (domain_type == IPlatformTopo::M_DOMAIN_INVALID) {
                if (m_platform_io.signal_domain_type("CPUINFO::FREQ_STICKER") == IPlatformTopo::M_DOMAIN_INVALID) {
                    throw Exception("EfficientFreqDecider: unable to parse max and sticker frequencies.",
                                    GEOPM_ERROR_DECIDER_UNSUPPORTED, __FILE__, __LINE__);
                }
                result = sticker_freq + m_freq_step;
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
            throw Exception("EfficientFreqDecider: requested invalid signal name.",
                            GEOPM_ERROR_LOGIC, __FILE__, __LINE__);
        }
#endif
        return result;
    }

    void EfficientFreqDecider::init_platform_io(void)
    {
        uint64_t freq_domain_type = m_platform_io.control_domain_type("FREQUENCY");
        if (freq_domain_type == IPlatformTopo::M_DOMAIN_INVALID) {
            throw Exception("EfficientFreqDecider: Platform does not support frequency control",
                            GEOPM_ERROR_DECIDER_UNSUPPORTED, __FILE__, __LINE__);
        }
        uint32_t num_freq_domain = m_platform_topo.num_domain(freq_domain_type);
        if (!num_freq_domain) {
            throw Exception("EfficientFreqDecider: Platform does not support frequency control",
                            GEOPM_ERROR_DECIDER_UNSUPPORTED, __FILE__, __LINE__);
        }
        for (uint32_t dom_idx = 0; dom_idx != num_freq_domain; ++dom_idx) {
            int control_idx = m_platform_io.push_control("FREQUENCY", freq_domain_type, dom_idx);
            if (control_idx < 0) {
                throw Exception("EfficientFreqDecider: Failed to enable frequency control in the platform.",
                                GEOPM_ERROR_DECIDER_UNSUPPORTED, __FILE__, __LINE__);
            }
            m_control_idx.push_back(control_idx);
        }
    }

    std::string EfficientFreqDecider::plugin_name(void)
    {
        return GEOPM_EFFICIENT_FREQ_DECIDER_PLUGIN_NAME;
    }

    std::unique_ptr<IDecider> EfficientFreqDecider::make_plugin(void)
    {
        return std::unique_ptr<IDecider>(new EfficientFreqDecider);
    }

    void EfficientFreqDecider::parse_env_map(void)
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

    bool EfficientFreqDecider::update_policy(IRegion &curr_region, IPolicy &curr_policy)
    {
        bool is_updated = false;
        is_updated = GoverningDecider::update_policy(curr_region, curr_policy);
        int num_domain = curr_policy.num_domain();
        auto curr_region_id = curr_region.identifier();
        uint64_t rid = curr_region_id & 0x00000000FFFFFFFF;
        double freq = m_last_freq;
        auto it = m_rid_freq_map.find(rid);
        if (it != m_rid_freq_map.end()) {
            freq = it->second;
        }
        else if (m_is_adaptive) {
            bool is_region_boundary = m_region_last != nullptr &&
                m_region_last->identifier() != curr_region_id;
            if (m_region_last == nullptr || is_region_boundary) {
                // set the freq for the current region (entry)
                auto region_it = m_region_map.find(curr_region_id);
                if (region_it == m_region_map.end()) {
                    auto tmp = m_region_map.emplace(
                        curr_region_id,
                        std::unique_ptr<EfficientFreqRegion>(
                            new EfficientFreqRegion(&curr_region, m_freq_min,
                                                    m_freq_max, m_freq_step,
                                                    num_domain)));
                    region_it = tmp.first;
                }
                region_it->second->update_entry();

                freq = region_it->second->freq(); // will get set below
            }
            if (is_region_boundary) {
                // update previous region (exit)
                auto last_region_id = m_region_last->identifier();
                auto region_it = m_region_map.find(last_region_id);
                if (region_it == m_region_map.end()) {
                    auto tmp = m_region_map.emplace(
                        last_region_id,
                        std::unique_ptr<EfficientFreqRegion>(
                            new EfficientFreqRegion(m_region_last, m_freq_min,
                                                    m_freq_max, m_freq_step,
                                                    num_domain)));
                    region_it = tmp.first;
                }
                region_it->second->update_exit();
            }
            m_region_last = &curr_region;
        }
        else {
            switch(curr_region.hint()) {

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
                    freq = m_freq_min;
                    break;
            }
        }

        if (freq != m_last_freq) {
            for (auto ctl_idx : m_control_idx) {
                m_platform_io.adjust(ctl_idx, freq);
            }
            m_last_freq = freq;
            is_updated = true;
        }

        return is_updated;
    }

    double EfficientFreqDecider::cpu_freq_min(void)
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

    double EfficientFreqDecider::cpu_freq_max(void)
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
