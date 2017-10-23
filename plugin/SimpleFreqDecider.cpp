/*
 * Copyright (c) 2015, 2016, 2017, Intel Corporation
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
#include <algorithm>

#include "geopm.h"
#include "geopm_message.h"
#include "geopm_plugin.h"
#include "geopm_hash.h"
#include "geopm_sched.h"

#include "SimpleFreqDecider.hpp"
#include "GoverningDecider.hpp"
#include "Exception.hpp"
#include <fstream>
#include <string>
#include <cmath>

#include "Region.hpp"

#include <iostream>
#include <stdlib.h>

int geopm_plugin_register(int plugin_type, struct geopm_factory_c *factory, void *dl_ptr)
{
    int err = 0;

    try {
        if (plugin_type == GEOPM_PLUGIN_TYPE_DECIDER) {
            geopm::IDecider *decider = new geopm::SimpleFreqDecider;
            geopm_factory_register(factory, decider, dl_ptr);
        }
    }
    catch (...) {
        err = geopm::exception_handler(std::current_exception());
    }

    return err;
}

namespace geopm
{
    void ctl_cpu_freq(std::vector<double> freq);

    SimpleFreqDecider::SimpleFreqDecider()
        : GoverningDecider()
        , m_cpu_info_path("/proc/cpuinfo")
        , m_cpu_freq_min_path("/sys/devices/system/cpu/cpu0/cpufreq/cpuinfo_min_freq")
        , m_cpu_freq_max_path("/sys/devices/system/cpu/cpu0/cpufreq/cpuinfo_max_freq")
        , m_freq_min(cpu_freq_min())
        , m_freq_max(cpu_freq_max())
        , m_freq_step(100e6)
        , m_num_cores(geopm_sched_num_cpu())
        , m_last_freq(NAN)
    {
        m_name = "simple_freq";
        parse_env_map();
    }

    SimpleFreqDecider::SimpleFreqDecider(const SimpleFreqDecider &other)
        : GoverningDecider(other)
        , m_cpu_info_path(other.m_cpu_info_path)
        , m_cpu_freq_min_path(other.m_cpu_freq_min_path)
        , m_cpu_freq_max_path(other.m_cpu_freq_max_path)
        , m_freq_min(other.m_freq_min)
        , m_freq_max(other.m_freq_max)
        , m_freq_step(other.m_freq_step)
        , m_num_cores(other.m_num_cores)
        , m_last_freq(other.m_last_freq)
        , m_rid_freq_map(other.m_rid_freq_map)
    {

    }

    SimpleFreqDecider::~SimpleFreqDecider()
    {

    }

    IDecider *SimpleFreqDecider::clone(void) const
    {
        return (IDecider*)(new SimpleFreqDecider(*this));
    }

    void SimpleFreqDecider::parse_env_map(void)
    {
        const char* env_freq_rid_map_str = getenv("GEOPM_SIMPLE_FREQ_RID_MAP");
        if (env_freq_rid_map_str) {
            std::string full_str(env_freq_rid_map_str);
            size_t begin_pos = 0;
            size_t colon_pos = full_str.find(':');
            while (colon_pos != std::string::npos) {
                size_t comma_pos = full_str.find(',', colon_pos);
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
                begin_pos = comma_pos + 1;
                colon_pos = full_str.find(':', begin_pos);
            }
        }
    }

    bool SimpleFreqDecider::update_policy(IRegion &curr_region, IPolicy &curr_policy)
    {
        // Never receiving a new policy power budget via geopm_policy_message_s, since we set according to frequencies, not policy.
        bool is_updated = false;
        is_updated = GoverningDecider::update_policy(curr_region, curr_policy);

        uint64_t rid = curr_region.identifier() & 0x00000000FFFFFFFF;
        double freq = m_last_freq;
        auto it = m_rid_freq_map.find(rid);
        if (it != m_rid_freq_map.end()) {
            freq = it->second;
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
            std::vector<double> freq_vec(m_num_cores, freq);

            curr_policy.ctl_cpu_freq(freq_vec);
            m_last_freq = freq;
        }

        // Don't do anything since we never get a new policy.
        return is_updated;
    }

    double SimpleFreqDecider::cpu_freq_sticker(void)
    {
        double result = NAN;
        const std::string key = "model name\t:";
        std::ifstream cpuinfo_file(m_cpu_info_path);
        if (!cpuinfo_file.is_open()) {
            throw Exception("SimpleFreqDecider::cpu_freq_sticker(): unable to open " + m_cpu_info_path,
                            errno ? errno : GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
        }
        while (isnan(result) && cpuinfo_file.good()) {
            std::string line;
            getline(cpuinfo_file, line);
            if (line.find(key) != std::string::npos) {
                size_t at_pos = line.find("@");
                size_t ghz_pos = line.find("GHz");
                if (at_pos != std::string::npos &&
                        ghz_pos != std::string::npos) {
                    try {
                        result = 1e9 * std::stod(line.substr(at_pos + 1, ghz_pos - at_pos));
                    }
                    catch (std::invalid_argument) {

                    }
                }
            }
        }
        cpuinfo_file.close();
        if (isnan(result)) {
            throw Exception("SimpleFreqDecider::cpu_freq_sticker(): unable to parse sticker frequency from " + m_cpu_info_path,
                            errno ? errno : GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
        }
        return result;
    }

    double SimpleFreqDecider::cpu_freq_min(void)
    {
        double result = NAN;
        const char* env_simple_freq_min = getenv("GEOPM_SIMPLE_FREQ_MIN");
        if (env_simple_freq_min) {
            try {
                result = std::stod(env_simple_freq_min);
            }
            catch (std::invalid_argument) {

            }
        }
        if (isnan(result)) {
            std::ifstream freq_file(m_cpu_freq_min_path);
            if (freq_file.is_open()) {
                std::string line;
                getline(freq_file, line);
                try {
                    result = 1e4 * std::stod(line);
                }
                catch (std::invalid_argument) {

                }
            }
        }
        if (isnan(result)) {
            try {
                result = cpu_freq_sticker() - 6 * m_freq_step;
            }
            catch (Exception) {

            }
        }
        if (isnan(result)) {
            throw Exception("SimpleFreqDecider::cpu_freq_min(): unable to parse minimum frequency",
                    errno ? errno : GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
        }
        return result;
    }

    double SimpleFreqDecider::cpu_freq_max(void)
    {
        double result = NAN;
        const char* env_simple_freq_max = getenv("GEOPM_SIMPLE_FREQ_MAX");
        if (env_simple_freq_max) {
            try {
                result = std::stod(env_simple_freq_max);
            }
            catch (std::invalid_argument) {

            }
        }
        if (isnan(result)) {
            std::ifstream freq_file(m_cpu_freq_max_path);
            if (freq_file.is_open()) {
                std::string line;
                getline(freq_file, line);
                try {
                    result = 1e4 * std::stod(line);
                }
                catch (std::invalid_argument) {

                }
            }
        }
        if (isnan(result)) {
            try {
                result = cpu_freq_sticker() + m_freq_step;
            }
            catch (Exception) {

            }
        }
        if (isnan(result)) {
            throw Exception("SimpleFreqDecider::cpu_freq_max(): unable to parse maximum frequency",
                            errno ? errno : GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
        }
        return result;
    }
}
