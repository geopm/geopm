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

#include <cpuid.h>
#include <iomanip>
#include <cmath>
#include <algorithm>
#include <numeric>
#include <iostream>

#include "geopm_sched.h"
#include "geopm_hash.h"
#include "geopm.h"
#include "PlatformIO.hpp"
#include "PlatformIOInternal.hpp"
#include "PlatformTopo.hpp"
#include "MSRIOGroup.hpp"
#include "TimeIOGroup.hpp"
#include "CombinedSignal.hpp"
#include "Exception.hpp"
#include "Helper.hpp"
#include "Agg.hpp"

#include "config.h"

namespace geopm
{
    IPlatformIO &platform_io(void)
    {
        static PlatformIO instance;
        return instance;
    }

    PlatformIO::PlatformIO()
        : PlatformIO({}, platform_topo())
    {

    }

    PlatformIO::PlatformIO(std::list<std::shared_ptr<IOGroup> > iogroup_list,
                           IPlatformTopo &topo)
        : m_is_active(false)
        , m_platform_topo(topo)
        , m_iogroup_list(iogroup_list)
        , m_do_restore(false)
    {
        if (m_iogroup_list.size() == 0) {
            for (const auto &it : iogroup_factory().plugin_names()) {
                try {
                    register_iogroup(iogroup_factory().make_plugin(it));
                }
                catch (const geopm::Exception &ex) {
#ifdef GEOPM_DEBUG
                    std::cerr << "<geopm> Warning: failed to load " << it << " IOGroup.  "
                              << "GEOPM may not work properly unless an alternate "
                              << "IOGroup plugin is loaded to provide signals/controls "
                              << "required by the Controller and Agent."
                              << std::endl;
                    std::cerr << "The error was: " << ex.what() << std::endl;
#endif
                }
            }
        }
    }

    void PlatformIO::register_iogroup(std::shared_ptr<IOGroup> iogroup)
    {
        if (m_do_restore) {
            throw Exception("PlatformIO::register_iogroup(): "
                            "IOGroup cannot be registered after a call to save_control()",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        m_iogroup_list.push_back(iogroup);
    }

    std::shared_ptr<IOGroup> PlatformIO::find_signal_iogroup(const std::string &signal_name) const
    {

        std::shared_ptr<IOGroup> result = nullptr;
        bool is_found = false;
        for (auto it = m_iogroup_list.rbegin();
             !is_found && it != m_iogroup_list.rend();
             ++it) {
            if ((*it)->is_valid_signal(signal_name)) {
                result = *it;
                is_found = true;
            }
        }
        return result;
    }

    std::shared_ptr<IOGroup> PlatformIO::find_control_iogroup(const std::string &control_name) const
    {
        std::shared_ptr<IOGroup> result = nullptr;
        bool is_found = false;
        for (auto it = m_iogroup_list.rbegin();
             !is_found && it != m_iogroup_list.rend();
             ++it) {
            if ((*it)->is_valid_control(control_name)) {
                result = *it;
                is_found = true;
            }
        }
        return result;
    }

    std::set<std::string> PlatformIO::signal_names(void) const
    {
        /// @todo better handling for signals provided by PlatformIO
        /// These depend on ENERGY signals and should not be available
        /// if ENERGY_PACKAGE and ENERGY_DRAM are not available.
        std::set<std::string> result {"POWER_PACKAGE", "POWER_DRAM"};
        for (const auto &io_group : m_iogroup_list) {
            auto names = io_group->signal_names();
            result.insert(names.begin(), names.end());
        }
        return result;
    }

    std::set<std::string> PlatformIO::control_names(void) const
    {
        std::set<std::string> result;
        for (const auto &io_group : m_iogroup_list) {
            auto names = io_group->control_names();
            result.insert(names.begin(), names.end());
        }
        return result;
    }

    int PlatformIO::signal_domain_type(const std::string &signal_name) const
    {
        int result = PlatformTopo::M_DOMAIN_INVALID;
        bool is_found = false;
        auto iogroup = find_signal_iogroup(signal_name);
        if (iogroup != nullptr) {
            is_found = true;
            result = iogroup->signal_domain_type(signal_name);
        }
        /// @todo better handling for signals provided by PlatformIO
        if (!is_found) {
            if (signal_name == "POWER_PACKAGE") {
                result = signal_domain_type("ENERGY_PACKAGE");
                is_found = true;
            }
            if (signal_name == "POWER_DRAM") {
                result = signal_domain_type("ENERGY_DRAM");
                is_found = true;
            }
            if (signal_name == "TEMPERATURE_CORE") {
                result = signal_domain_type("TEMPERATURE_CORE_UNDER");
                is_found = true;
            }
            if (signal_name == "TEMPERATURE_PACKAGE") {
                result = signal_domain_type("TEMPERATURE_PKG_UNDER");
                is_found = true;
            }
        }
        if (!is_found) {
            throw Exception("PlatformIO::signal_domain_type(): signal name \"" +
                            signal_name + "\" not found",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        return result;
    }

    int PlatformIO::control_domain_type(const std::string &control_name) const
    {
        int result = PlatformTopo::M_DOMAIN_INVALID;
        auto iogroup = find_control_iogroup(control_name);
        if (iogroup != nullptr) {
            result = iogroup->control_domain_type(control_name);
        }
        else {
            throw Exception("PlatformIO::control_domain_type(): control name \"" +
                            control_name + "\" not found",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        return result;
    }

    int PlatformIO::push_signal(const std::string &signal_name,
                                int domain_type,
                                int domain_idx)
    {
        if (m_is_active) {
            throw Exception("PlatformIO::push_signal(): pushing signals after read_batch() or adjust().",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        int result = -1;
        auto sig_tup = std::make_tuple(signal_name, domain_type, domain_idx);
        auto sig_tup_it = m_existing_signal.find(sig_tup);
        if (sig_tup_it != m_existing_signal.end()) {
            result = sig_tup_it->second;
        }
        if (result == -1) {
            auto iogroup = find_signal_iogroup(signal_name);
            if (iogroup != nullptr) {
                if (domain_type == iogroup->signal_domain_type(signal_name)) {
                    int group_signal_idx = iogroup->push_signal(signal_name, domain_type, domain_idx);
                    result = m_active_signal.size();
                    m_existing_signal[sig_tup] = result;
                    m_active_signal.emplace_back(iogroup, group_signal_idx);
                }
                else {
                    result = push_signal_convert_domain(signal_name, domain_type, domain_idx);
                    m_existing_signal[sig_tup] = result;
                }
            }
        }
        if (result == -1 && signal_name.find("POWER") != std::string::npos) {
            result = push_signal_power(signal_name, domain_type, domain_idx);
            m_existing_signal[sig_tup] = result;
        }
        if (result == -1 && signal_name.find("TEMPERATURE") != std::string::npos) {
            result = push_signal_temperature(signal_name, domain_type, domain_idx);
            m_existing_signal[sig_tup] = result;
        }
        if (result == -1) {
            throw Exception("PlatformIO::push_signal(): no support for signal name \"" +
                            signal_name + "\" and domain type \"" +
                            std::to_string(domain_type) + "\"",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        return result;
    }

    int PlatformIO::push_signal_power(const std::string &signal_name,
                                      int domain_type,
                                      int domain_idx)
    {
        int result = -1;
        if (signal_name == "POWER_PACKAGE" || signal_name == "POWER_DRAM") {
            int energy_idx = -1;
            if (signal_name == "POWER_PACKAGE") {
                energy_idx = push_signal("ENERGY_PACKAGE", domain_type, domain_idx);
            }
            else if (signal_name == "POWER_DRAM") {
                energy_idx = push_signal("ENERGY_DRAM", domain_type, domain_idx);
            }

            int time_idx = push_signal("TIME", PlatformTopo::M_DOMAIN_BOARD, 0);
            result = m_active_signal.size();

            register_combined_signal(result,
                                     {time_idx, energy_idx},
                                     std::unique_ptr<CombinedSignal>(new DerivativeCombinedSignal));

            m_active_signal.emplace_back(nullptr, result);
        }
        return result;
    }

    int PlatformIO::push_signal_temperature(const std::string &signal_name,
                                            int domain_type,
                                            int domain_idx)
    {
        int result = -1;
        if (signal_name == "TEMPERATURE_CORE" || signal_name == "TEMPERATURE_PACKAGE") {
            int max_idx = push_signal("TEMPERATURE_MAX", domain_type, domain_idx);
            int under_idx = -1;
            if (signal_name == "TEMPERATURE_CORE") {
                under_idx = push_signal("TEMPERATURE_CORE_UNDER", domain_type, domain_idx);
            }
            else if (signal_name =="TEMPERATURE_PACKAGE") {
                under_idx = push_signal("TEMPERATURE_PKG_UNDER", domain_type, domain_idx);
            }
            result = m_active_signal.size();
            register_combined_signal(result,
                                     {max_idx, under_idx},
                                     std::unique_ptr<CombinedSignal>(
                                         new CombinedSignal(
                                             [] (const std::vector<double> &val) -> double {
#ifdef GEOPM_DEBUG
                                                 if (val.size() != 2) {
                                                     throw Exception("PlatformIO::push_signal_temperature(): expected two values to subtract for temperature.",
                                                                     GEOPM_ERROR_LOGIC, __FILE__, __LINE__);
                                                 }
#endif
                                                 return val[0] - val[1];
                                             } )));

            m_active_signal.emplace_back(nullptr, result);
        }
        return result;
    }

    int PlatformIO::push_signal_convert_domain(const std::string &signal_name,
                                               int domain_type,
                                               int domain_idx)
    {
        int result = -1;
        int base_domain_type = signal_domain_type(signal_name);
        if (m_platform_topo.is_domain_within(base_domain_type, domain_type)) {
            std::set<int> cpus = m_platform_topo.domain_cpus(domain_type, domain_idx);
            std::set<int> base_domain_idx;
            for (auto it : cpus) {
                base_domain_idx.insert(m_platform_topo.domain_idx(base_domain_type, it));
            }
            std::vector<int> signal_idx;
            for (auto it : base_domain_idx) {
                signal_idx.push_back(push_signal(signal_name, base_domain_type, it));
            }
            result = push_combined_signal(signal_name, domain_type, domain_idx, signal_idx);
        }
        return result;
    }

    int PlatformIO::push_combined_signal(const std::string &signal_name,
                                         int domain_type,
                                         int domain_idx,
                                         const std::vector<int> &sub_signal_idx)
    {
        int result = m_active_signal.size();
        std::unique_ptr<CombinedSignal> combiner = geopm::make_unique<CombinedSignal>(agg_function(signal_name));
        register_combined_signal(result, sub_signal_idx, std::move(combiner));
        m_active_signal.emplace_back(nullptr, result);
        return result;
    }


    void PlatformIO::register_combined_signal(int signal_idx,
                                              std::vector<int> operands,
                                              std::unique_ptr<CombinedSignal> signal)
    {
        auto tmp = std::make_pair(operands, std::move(signal));
        m_combined_signal[signal_idx] = std::move(tmp);
    }

    int PlatformIO::push_control(const std::string &control_name,
                                 int domain_type,
                                 int domain_idx)
    {
        if (m_is_active) {
            throw Exception("PlatformIO::push_control(): pushing controls after read_batch() or adjust().",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        int result = -1;
        auto ctl_tup = std::make_tuple(control_name, domain_type, domain_idx);
        auto ctl_tup_it = m_existing_control.find(ctl_tup);
        if (ctl_tup_it != m_existing_control.end()) {
            result = ctl_tup_it->second;
        }
        if (result == -1) {
            auto iogroup = find_control_iogroup(control_name);
            if (iogroup != nullptr) {
                if (iogroup->control_domain_type(control_name) == domain_type) {
                    int group_control_idx = iogroup->push_control(control_name, domain_type, domain_idx);
                    result = m_active_control.size();
                    m_existing_control[ctl_tup] = result;
                    m_active_control.emplace_back(iogroup, group_control_idx);
                }
                else {
                    // Handle aggregated controls
                    result = push_control_convert_domain(control_name, domain_type, domain_idx);
                    m_existing_control[ctl_tup] = result;
                }
            }
        }
        if (result == -1) {
            throw Exception("PlatformIO::push_control(): control name \"" +
                            control_name + "\" not found",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        return result;
    }

    int PlatformIO::push_control_convert_domain(const std::string &control_name,
                                                int domain_type,
                                                int domain_idx)
    {
        int result = -1;
        int base_domain_type = control_domain_type(control_name);
        if (m_platform_topo.is_domain_within(base_domain_type, domain_type)) {
            std::set<int> cpus = m_platform_topo.domain_cpus(domain_type, domain_idx);
            std::set<int> base_domain_idx;
            for (auto it : cpus) {
                base_domain_idx.insert(m_platform_topo.domain_idx(base_domain_type, it));
            }
            std::vector<int> control_idx;
            for (auto it : base_domain_idx) {
                control_idx.push_back(push_control(control_name, base_domain_type, it));
            }
            result = m_active_control.size();
            m_combined_control.emplace(std::make_pair(result, control_idx));
            m_active_control.emplace_back(nullptr, result);
        }
        return result;
    }

    int PlatformIO::num_signal(void) const
    {
        return m_active_signal.size();
    }

    int PlatformIO::num_control(void) const
    {
        return m_active_control.size();
    }

    double PlatformIO::sample(int signal_idx)
    {
        double result = NAN;
        if (signal_idx < 0 || signal_idx >= num_signal()) {
            throw Exception("PlatformIO::sample(): signal_idx out of range",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        if (!m_is_active) {
            throw Exception("PlatformIO::sample(): read_batch() not called prior to call to sample()",
                            GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
        }
        auto &group_idx_pair = m_active_signal[signal_idx];
        if (group_idx_pair.first) {
            result = group_idx_pair.first->sample(group_idx_pair.second);
        }
        else {
            result = sample_combined(group_idx_pair.second);
        }
        return result;
    }

    double PlatformIO::sample_combined(int signal_idx)
    {
        double result = NAN;
        auto &op_func_pair = m_combined_signal.at(signal_idx);
        std::vector<int> &operand_idx = op_func_pair.first;
        auto &signal = op_func_pair.second;
        std::vector<double> operands(operand_idx.size());
        for (size_t ii = 0; ii < operands.size(); ++ii) {
            operands[ii] = sample(operand_idx[ii]);
        }
        result = signal->sample(operands);
        return result;
    }

    void PlatformIO::adjust(int control_idx,
                            double setting)
    {
        if (control_idx < 0 || control_idx >= num_control()) {
            throw Exception("PlatformIO::adjust(): control_idx out of range",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        if (std::isnan(setting)) {
            throw Exception("PlatformIO::adjust(): setting is NAN",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        auto &group_idx_pair = m_active_control[control_idx];
        if (nullptr == group_idx_pair.first) {
            auto &sub_controls = m_combined_control.at(control_idx);
            for (auto idx : sub_controls) {
                adjust(idx, setting);
            }
        }
        else {
            group_idx_pair.first->adjust(group_idx_pair.second, setting);
        }
        m_is_active = true;
    }

    void PlatformIO::read_batch(void)
    {
        for (auto &it : m_iogroup_list) {
            it->read_batch();
        }
        m_is_active = true;
    }

    void PlatformIO::write_batch(void)
    {
        for (auto &it : m_iogroup_list) {
            it->write_batch();
        }
    }

    double PlatformIO::read_signal(const std::string &signal_name,
                                   int domain_type,
                                   int domain_idx)
    {
        auto iogroup = find_signal_iogroup(signal_name);
        if (iogroup == nullptr) {
            throw Exception("PlatformIO::read_signal(): signal name \"" + signal_name + "\" not found",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        else if (iogroup->signal_domain_type(signal_name) != domain_type) {
            throw Exception("PlatformIO::read_signal(): domain " + std::to_string(domain_type) +
                            " is not valid for signal \"" + signal_name + "\"",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        return iogroup->read_signal(signal_name, domain_type, domain_idx);
    }

    void PlatformIO::write_control(const std::string &control_name,
                                   int domain_type,
                                   int domain_idx,
                                   double setting)
    {
        auto iogroup = find_control_iogroup(control_name);
        if (iogroup == nullptr) {
            throw Exception("PlatformIO::write_control(): control name \"" + control_name + "\" not found",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        else if (iogroup->control_domain_type(control_name) != domain_type) {
            throw Exception("PlatformIO::write_control(): domain " + std::to_string(domain_type) +
                            " is not valid for control \"" + control_name + "\"",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }

        iogroup->write_control(control_name, domain_type, domain_idx, setting);
    }

    void PlatformIO::save_control(void)
    {
        m_do_restore = true;
        for (auto &it : m_iogroup_list) {
            it->save_control();
        }
    }

    void PlatformIO::restore_control(void)
    {
        if (m_do_restore) {
            for (auto &it : m_iogroup_list) {
                it->restore_control();
            }
        }
    }

    std::function<double(const std::vector<double> &)> PlatformIO::agg_function(const std::string &signal_name) const
    {
        // Special signals from PlatformIO
        /// @todo: find a better way to track signals produced by PlatformIO itself
        if (signal_name == "POWER_PACKAGE" || signal_name == "POWER_DRAM") {
            return Agg::sum;
        }
        auto iogroup = find_signal_iogroup(signal_name);
        if (iogroup == nullptr) {
            throw Exception("PlatformIO::agg_function(): unknown how to aggregate \"" + signal_name + "\"",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        return iogroup->agg_function(signal_name);
    }

    std::string PlatformIO::signal_description(const std::string &signal_name) const
    {
        /// @todo: find a better way to track signals produced by PlatformIO itself
        if (signal_name == "POWER_PACKAGE") {
            return "Average package power in watts over the last 8 samples (usually 40 ms).";
        }
        else if (signal_name == "POWER_DRAM") {
            return "Average DRAM power in watts over the last 8 samples (usually 40 ms).";
        }
        auto iogroup = find_signal_iogroup(signal_name);
        if (iogroup == nullptr) {
            throw Exception("PlatformIO::signal_description(): unknown signal \"" + signal_name + "\"",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        return iogroup->signal_description(signal_name);
    }

    std::string PlatformIO::control_description(const std::string &control_name) const
    {
        auto iogroup = find_control_iogroup(control_name);
        if (iogroup == nullptr) {
            throw Exception("PlatformIO::control_description(): unknown control \"" + control_name + "\"",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        return iogroup->control_description(control_name);
    }
}
