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

#include "EpochIOGroup.hpp"

#include <cmath>

#include "PlatformTopo.hpp"
#include "ApplicationSampler.hpp"
#include "Helper.hpp"
#include "Exception.hpp"
#include "Agg.hpp"
#include "geopm_debug.hpp"
#include "record.hpp"

namespace geopm
{
    EpochIOGroup::EpochIOGroup()
        : EpochIOGroup(platform_topo(),
                       ApplicationSampler::application_sampler())
    {

    }

    EpochIOGroup::EpochIOGroup(const PlatformTopo &topo,
                               ApplicationSampler &app)
        : m_topo(topo)
        , m_app(app)
        , m_num_cpu(m_topo.num_domain(GEOPM_DOMAIN_CPU))
        , m_per_cpu_count(m_num_cpu, NAN)
        , m_is_batch_read(false)
        , m_is_initialized(false)
    {

    }

    const std::set<std::string> EpochIOGroup::m_valid_signal_name = {
        "EPOCH::EPOCH_COUNT",
        "EPOCH_COUNT",
    };

    void EpochIOGroup::init(void)
    {
        int cpu_idx = 0;
        for (const auto &proc : m_app.per_cpu_process()) {
            if (proc != -1) {
                m_process_cpu_map[proc].insert(cpu_idx);
                m_per_cpu_count[cpu_idx] = 0;
            }
            ++cpu_idx;
        }
        m_is_initialized = true;
    }

    std::set<std::string> EpochIOGroup::signal_names(void) const
    {
        return m_valid_signal_name;
    }

    std::set<std::string> EpochIOGroup::control_names(void) const
    {
        return {};
    }

    bool EpochIOGroup::is_valid_signal(const std::string &signal_name) const
    {
        return m_valid_signal_name.find(signal_name) != m_valid_signal_name.end();
    }

    bool EpochIOGroup::is_valid_control(const std::string &control_name) const
    {
        return false;
    }

    int EpochIOGroup::signal_domain_type(const std::string &signal_name) const
    {
        int result = GEOPM_DOMAIN_INVALID;
        if (is_valid_signal(signal_name)) {
            result = GEOPM_DOMAIN_CPU;
        }
        return result;
    }

    int EpochIOGroup::control_domain_type(const std::string &control_name) const
    {
        return GEOPM_DOMAIN_INVALID;
    }

    int EpochIOGroup::push_signal(const std::string &signal_name, int domain_type, int domain_idx)
    {
        if (!is_valid_signal(signal_name)) {
            throw Exception("EpochIOGroup::push_signal(): signal_name " + signal_name +
                            " not valid for EpochIOGroup",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        check_domain(domain_type, domain_idx);
        if (m_is_batch_read) {
            throw Exception("EpochIOGroup::push_signal(): cannot push signal after call to read_batch().",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }

        // check if already pushed
        int result = -1;
        auto it = m_cpu_signal_map.find(domain_idx);
        if (it != m_cpu_signal_map.end()) {
            result = it->second;
        }
        if (result == -1) {
            result = m_active_signal.size();
            m_active_signal.push_back(domain_idx);
            m_cpu_signal_map[domain_idx] = result;
        }
        return result;
    }

    int EpochIOGroup::push_control(const std::string &control_name, int domain_type, int domain_idx)
    {
        throw Exception("EpochIOGroup::push_control(): there are no controls supported by the EpochIOGroup",
                        GEOPM_ERROR_INVALID, __FILE__, __LINE__);
    }

    void EpochIOGroup::read_batch(void)
    {
        if (!m_is_initialized) {
            init();
        }
        /// update_records() will get called by controller
        auto records = m_app.get_records();
        for (const auto &record : records) {
            GEOPM_DEBUG_ASSERT(m_process_cpu_map.find(record.process) != m_process_cpu_map.end(),
                               "Process " + std::to_string(record.process) + " in record not found");
            if (record.event == EVENT_EPOCH_COUNT) {
                const auto &cpu_set = m_process_cpu_map.at(record.process);
                for (const auto &cpu_idx : cpu_set) {
                    m_per_cpu_count[cpu_idx] = (double)record.signal;
                }
            }
        }
        m_is_batch_read = true;
    }

    void EpochIOGroup::write_batch(void)
    {

    }

    double EpochIOGroup::sample(int batch_idx)
    {
        if (!m_is_batch_read) {
            throw Exception("EpochIOGroup::sample(): signal has not been read",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        if (batch_idx < 0 || (size_t)batch_idx >= m_active_signal.size()) {
            throw Exception("EpochIOGroup::sample(): batch_idx out of range",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        int cpu_idx = m_active_signal[batch_idx];
#ifdef GEOPM_DEBUG
        if (cpu_idx < 0 || cpu_idx >= m_num_cpu) {
            throw Exception("EpochIOGroup::sample(): invalid cpu_idx saved in map.",
                            GEOPM_ERROR_LOGIC, __FILE__, __LINE__);
        }
#endif
        return m_per_cpu_count[cpu_idx];
    }

    void EpochIOGroup::adjust(int batch_idx, double setting)
    {
        throw Exception("EpochIOGroup::adjust(): there are no controls supported by the EpochIOGroup",
                        GEOPM_ERROR_INVALID, __FILE__, __LINE__);
    }

    double EpochIOGroup::read_signal(const std::string &signal_name, int domain_type, int domain_idx)
    {
        throw Exception("EpochIOGroup: read_signal() is not supported for this IOGroup.",
                        GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
    }

    void EpochIOGroup::write_control(const std::string &control_name, int domain_type, int domain_idx, double setting)
    {
        throw Exception("EpochIOGroup::write_control(): there are no controls supported by the EpochIOGroup",
                        GEOPM_ERROR_INVALID, __FILE__, __LINE__);
    }

    void EpochIOGroup::save_control(void)
    {

    }

    void EpochIOGroup::restore_control(void)
    {

    }

    std::string EpochIOGroup::plugin_name(void)
    {
        return "EPOCH";
    }

    std::unique_ptr<IOGroup> EpochIOGroup::make_plugin(void)
    {
        return geopm::make_unique<EpochIOGroup>();
    }

    std::function<double(const std::vector<double> &)> EpochIOGroup::agg_function(const std::string &signal_name) const
    {
        if (!is_valid_signal(signal_name)) {
            throw Exception("EpochIOGroup::agg_function(): " + signal_name +
                            "not valid for EpochIOGroup",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        return Agg::min;
    }

    std::function<std::string(double)> EpochIOGroup::format_function(const std::string &signal_name) const
    {
        if (!is_valid_signal(signal_name)) {
            throw Exception("EpochIOGroup::format_function(): " + signal_name +
                            " not valid for EpochIOGroup",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        return string_format_integer;
    }

    std::string EpochIOGroup::signal_description(const std::string &signal_name) const
    {
        if (!is_valid_signal(signal_name)) {
            throw Exception("EpochIOGroup::signal_description(): " + signal_name +
                            " not valid for EpochIOGroup",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        return "Number of epoch events sampled from the process on the given CPU";
    }

    std::string EpochIOGroup::control_description(const std::string &control_name) const
    {
        throw Exception("EpochIOGroup::control_description(): there are no controls supported by the EpochIOGroup",
                        GEOPM_ERROR_INVALID, __FILE__, __LINE__);
    }

    int EpochIOGroup::signal_behavior(const std::string &signal_name) const
    {
        return -1;
    }

    void EpochIOGroup::check_domain(int domain_type, int domain_idx) const
    {
        if (domain_type != GEOPM_DOMAIN_CPU) {
            throw Exception("EpochIOGroup::check_domain(): signals not defined for domain " + std::to_string(domain_type),
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        if (domain_idx < 0 || domain_idx >= m_num_cpu) {
            throw Exception("EpochIOGroup::check_domain(): invalid domain index: "
                            + std::to_string(domain_idx),
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
    }
}
