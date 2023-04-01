/*
 * Copyright (c) 2015 - 2023, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "config.h"

#include "EpochIOGroup.hpp"

#include <cmath>

#include "geopm/PlatformTopo.hpp"
#include "ApplicationSampler.hpp"
#include "geopm/Helper.hpp"
#include "geopm/Exception.hpp"
#include "geopm/Agg.hpp"
#include "geopm_debug.hpp"
#include "record.hpp"

namespace geopm
{
    const std::set<std::string> EpochIOGroup::m_valid_signal_name = {
        "EPOCH::EPOCH_COUNT",
        "EPOCH_COUNT",
    };

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
        , m_per_cpu_count(m_num_cpu, 0.0)
        , m_is_batch_read(false)
    {

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
        /// update_records() will get called by controller
        auto records = m_app.get_records();
        for (const auto &record : records) {
            if (record.event == EVENT_EPOCH_COUNT) {
                for (int cpu_idx : m_app.client_cpu_set(record.process)) {
                    m_per_cpu_count[cpu_idx] = (double)record.signal;
                }
            }
        }
        std::vector<bool> is_valid(m_num_cpu, false);
        for (int pid : m_app.client_pids()) {
            for (int cpu_idx : m_app.client_cpu_set(pid)) {
                is_valid[cpu_idx] = true;
            }
        }
        for (int cpu_idx = 0; cpu_idx != m_num_cpu; ++cpu_idx) {
            if (!is_valid[cpu_idx]) {
                m_per_cpu_count[cpu_idx] = NAN;
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
                        GEOPM_ERROR_NOT_IMPLEMENTED, __FILE__, __LINE__);
    }

    void EpochIOGroup::write_control(const std::string &control_name, int domain_type, int domain_idx, double setting)
    {
        throw Exception("EpochIOGroup::write_control(): there are no controls supported by the EpochIOGroup",
                        GEOPM_ERROR_NOT_IMPLEMENTED, __FILE__, __LINE__);
    }

    void EpochIOGroup::save_control(void)
    {

    }

    void EpochIOGroup::restore_control(void)
    {

    }

    std::string EpochIOGroup::name(void) const
    {
        return plugin_name();
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
        if (!is_valid_signal(signal_name)) {
            throw Exception("EpochIOGroup::signal_behavior(): " + signal_name +
                            " not valid for EpochIOGroup",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        return IOGroup::M_SIGNAL_BEHAVIOR_MONOTONE;
    }

    void EpochIOGroup::save_control(const std::string &save_path)
    {

    }

    void EpochIOGroup::restore_control(const std::string &save_path)
    {

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
