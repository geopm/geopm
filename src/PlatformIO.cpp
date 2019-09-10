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

#include "PlatformIOImp.hpp"

#include <cpuid.h>
#include <iomanip>
#include <cmath>
#include <algorithm>
#include <numeric>
#include <iostream>

#include "geopm_sched.h"
#include "geopm_hash.h"
#include "geopm.h"
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
    PlatformIO &platform_io(void)
    {
        static PlatformIOImp instance;
        return instance;
    }

    PlatformIOImp::PlatformIOImp()
        : PlatformIOImp({}, platform_topo())
    {

    }

    PlatformIOImp::PlatformIOImp(std::list<std::shared_ptr<IOGroup> > iogroup_list,
                                 const PlatformTopo &topo)
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
                    std::cerr << "Warning: <geopm> Failed to load " << it << " IOGroup.  "
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

    void PlatformIOImp::register_iogroup(std::shared_ptr<IOGroup> iogroup)
    {
        if (m_do_restore) {
            throw Exception("PlatformIOImp::register_iogroup(): "
                            "IOGroup cannot be registered after a call to save_control()",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        m_iogroup_list.push_back(iogroup);
    }

    std::shared_ptr<IOGroup> PlatformIOImp::find_signal_iogroup(const std::string &signal_name) const
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

    std::shared_ptr<IOGroup> PlatformIOImp::find_control_iogroup(const std::string &control_name) const
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

    std::set<std::string> PlatformIOImp::signal_names(void) const
    {
        /// @todo better handling for signals provided by PlatformIOImp
        /// These depend on ENERGY signals and should not be available
        /// if ENERGY_PACKAGE and ENERGY_DRAM are not available.
        std::set<std::string> result {"POWER_PACKAGE", "POWER_DRAM",
                                      "TEMPERATURE_CORE", "TEMPERATURE_PACKAGE"};
        for (const auto &io_group : m_iogroup_list) {
            auto names = io_group->signal_names();
            result.insert(names.begin(), names.end());
        }
        return result;
    }

    std::set<std::string> PlatformIOImp::control_names(void) const
    {
        std::set<std::string> result;
        for (const auto &io_group : m_iogroup_list) {
            auto names = io_group->control_names();
            result.insert(names.begin(), names.end());
        }
        return result;
    }

    int PlatformIOImp::signal_domain_type(const std::string &signal_name) const
    {
        int result = GEOPM_DOMAIN_INVALID;
        bool is_found = false;
        auto iogroup = find_signal_iogroup(signal_name);
        if (iogroup != nullptr) {
            is_found = true;
            result = iogroup->signal_domain_type(signal_name);
        }
        /// @todo better handling for signals provided by PlatformIOImp
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
            throw Exception("PlatformIOImp::signal_domain_type(): signal name \"" +
                            signal_name + "\" not found",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        return result;
    }

    int PlatformIOImp::control_domain_type(const std::string &control_name) const
    {
        int result = GEOPM_DOMAIN_INVALID;
        auto iogroup = find_control_iogroup(control_name);
        if (iogroup != nullptr) {
            result = iogroup->control_domain_type(control_name);
        }
        else {
            throw Exception("PlatformIOImp::control_domain_type(): control name \"" +
                            control_name + "\" not found",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        return result;
    }

    int PlatformIOImp::push_signal(const std::string &signal_name,
                                   int domain_type,
                                   int domain_idx)
    {
        if (m_is_active) {
            throw Exception("PlatformIOImp::push_signal(): pushing signals after read_batch() or adjust().",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        if (domain_type < 0 || domain_type >= GEOPM_NUM_DOMAIN) {
            throw Exception("PlatformIOImp::push_signal(): domain_type is out of range",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        if (domain_idx < 0 || domain_idx >= m_platform_topo.num_domain(domain_type)) {
            throw Exception("PlatformIOImp::push_signal(): domain_idx is out of range",
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
            throw Exception("PlatformIOImp::push_signal(): no support for signal name \"" +
                            signal_name + "\" and domain type \"" +
                            std::to_string(domain_type) + "\"",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        return result;
    }

    int PlatformIOImp::push_signal_power(const std::string &signal_name,
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

            int time_idx = push_signal("TIME", GEOPM_DOMAIN_BOARD, 0);
            result = m_active_signal.size();

            register_combined_signal(result,
                                     {time_idx, energy_idx},
                                     std::unique_ptr<CombinedSignal>(new DerivativeCombinedSignal));

            m_active_signal.emplace_back(nullptr, result);
        }
        return result;
    }

    int PlatformIOImp::push_signal_temperature(const std::string &signal_name,
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
                                                     throw Exception("PlatformIOImp::push_signal_temperature(): expected two values to subtract for temperature.",
                                                                     GEOPM_ERROR_LOGIC, __FILE__, __LINE__);
                                                 }
#endif
                                                 return val[0] - val[1];
                                             } )));

            m_active_signal.emplace_back(nullptr, result);
        }
        return result;
    }

    int PlatformIOImp::push_signal_convert_domain(const std::string &signal_name,
                                                  int domain_type,
                                                  int domain_idx)
    {
        int result = -1;
        int native_signal_domain = signal_domain_type(signal_name);
        if (m_platform_topo.is_nested_domain(native_signal_domain, domain_type)) {
            std::set<int> base_domain_idx = m_platform_topo.domain_nested(native_signal_domain,
                                                                          domain_type, domain_idx);
            std::vector<int> signal_idx;
            for (auto it : base_domain_idx) {
                signal_idx.push_back(push_signal(signal_name, native_signal_domain, it));
            }
            result = push_combined_signal(signal_name, domain_type, domain_idx, signal_idx);
        }
        if (result == -1 &&
            m_platform_topo.is_nested_domain(domain_type, native_signal_domain)) {
            int native_dom_idx = m_platform_topo.get_outer_domain_idx(domain_type, domain_idx, native_signal_domain);
            result = push_signal(signal_name, native_signal_domain, native_dom_idx);
        }
        return result;
    }

    int PlatformIOImp::push_combined_signal(const std::string &signal_name,
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


    void PlatformIOImp::register_combined_signal(int signal_idx,
                                                 std::vector<int> operands,
                                                 std::unique_ptr<CombinedSignal> signal)
    {
        auto tmp = std::make_pair(operands, std::move(signal));
        m_combined_signal[signal_idx] = std::move(tmp);
    }

    int PlatformIOImp::push_control(const std::string &control_name,
                                    int domain_type,
                                    int domain_idx)
    {
        if (m_is_active) {
            throw Exception("PlatformIOImp::push_control(): pushing controls after read_batch() or adjust().",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        if (domain_type < 0 || domain_type >= GEOPM_NUM_DOMAIN) {
            throw Exception("PlatformIOImp::push_control(): domain_type is out of range",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        if (domain_idx < 0 || domain_idx >= m_platform_topo.num_domain(domain_type)) {
            throw Exception("PlatformIOImp::push_control(): domain_idx is out of range",
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
            throw Exception("PlatformIOImp::push_control(): control name \"" +
                            control_name + "\" not found",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        return result;
    }

    int PlatformIOImp::push_control_convert_domain(const std::string &control_name,
                                                   int domain_type,
                                                   int domain_idx)
    {
        int result = -1;
        int base_domain_type = control_domain_type(control_name);
        if (m_platform_topo.is_nested_domain(base_domain_type, domain_type)) {
            std::set<int> base_domain_idx = m_platform_topo.domain_nested(base_domain_type,
                                                                          domain_type, domain_idx);
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

    int PlatformIOImp::num_signal_pushed(void) const
    {
        return m_active_signal.size();
    }

    int PlatformIOImp::num_control_pushed(void) const
    {
        return m_active_control.size();
    }

    double PlatformIOImp::sample(int signal_idx)
    {
        double result = NAN;
        if (signal_idx < 0 || signal_idx >= num_signal_pushed()) {
            throw Exception("PlatformIOImp::sample(): signal_idx out of range",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        if (!m_is_active) {
            throw Exception("PlatformIOImp::sample(): read_batch() not called prior to call to sample()",
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

    double PlatformIOImp::sample_combined(int signal_idx)
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

    void PlatformIOImp::adjust(int control_idx,
                               double setting)
    {
        if (control_idx < 0 || control_idx >= num_control_pushed()) {
            throw Exception("PlatformIOImp::adjust(): control_idx out of range",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        if (std::isnan(setting)) {
            throw Exception("PlatformIOImp::adjust(): setting is NAN",
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

    void PlatformIOImp::read_batch(void)
    {
        for (auto &it : m_iogroup_list) {
            it->read_batch();
        }
        m_is_active = true;
    }

    void PlatformIOImp::write_batch(void)
    {
        for (auto &it : m_iogroup_list) {
            it->write_batch();
        }
    }

    double PlatformIOImp::read_signal(const std::string &signal_name,
                                      int domain_type,
                                      int domain_idx)
    {
        if (domain_type < 0 || domain_type >= GEOPM_NUM_DOMAIN) {
            throw Exception("PlatformIOImp::read_signal(): domain_type is out of range",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        if (domain_idx < 0 || domain_idx >= m_platform_topo.num_domain(domain_type)) {
            throw Exception("PlatformIOImp::read_signal(): domain_idx is out of range",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }

        double result = NAN;
        auto iogroup = find_signal_iogroup(signal_name);
        if (iogroup == nullptr) {
            throw Exception("PlatformIOImp::read_signal(): signal name \"" + signal_name + "\" not found",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        else if (iogroup->signal_domain_type(signal_name) != domain_type) {
            result = read_signal_convert_domain(signal_name, domain_type, domain_idx);
        }
        else {
            result = iogroup->read_signal(signal_name, domain_type, domain_idx);
        }
        return result;
    }

    double PlatformIOImp::read_signal_convert_domain(const std::string &signal_name,
                                                     int domain_type,
                                                     int domain_idx)
    {
        double result = NAN;
        int base_domain_type = signal_domain_type(signal_name);
        if (m_platform_topo.is_nested_domain(base_domain_type, domain_type)) {
            std::set<int> base_domain_idx = m_platform_topo.domain_nested(base_domain_type,
                                                                          domain_type, domain_idx);
            std::vector<double> values;
            for (auto idx : base_domain_idx) {
                values.push_back(read_signal(signal_name, base_domain_type, idx));
            }
            result = agg_function(signal_name)(values);
        }
        else if (m_platform_topo.is_nested_domain(domain_type, base_domain_type)) {
            int base_domain_idx = m_platform_topo.get_outer_domain_idx(domain_type, domain_idx, base_domain_type);
            result = read_signal(signal_name, base_domain_type, base_domain_idx);
        }
        else {
            throw Exception("PlatformIOImp::read_signal(): domain " + std::to_string(domain_type) +
                            " is not valid for signal \"" + signal_name + "\"",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        return result;
    }

    void PlatformIOImp::write_control(const std::string &control_name,
                                      int domain_type,
                                      int domain_idx,
                                      double setting)
    {
        if (domain_type < 0 || domain_type >= GEOPM_NUM_DOMAIN) {
            throw Exception("PlatformIOImp::write_control(): domain_type is out of range",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        if (domain_idx < 0 || domain_idx >= m_platform_topo.num_domain(domain_type)) {
            throw Exception("PlatformIOImp::write_control(): domain_idx is out of range",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }

        auto iogroup = find_control_iogroup(control_name);
        if (iogroup == nullptr) {
            throw Exception("PlatformIOImp::write_control(): control name \"" + control_name + "\" not found",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        else if (iogroup->control_domain_type(control_name) != domain_type) {
            write_control_convert_domain(control_name, domain_type, domain_idx, setting);
        }
        else {
            iogroup->write_control(control_name, domain_type, domain_idx, setting);
        }
    }

    void PlatformIOImp::write_control_convert_domain(const std::string &control_name,
                                                     int domain_type,
                                                     int domain_idx,
                                                     double setting)
    {
        int base_domain_type = control_domain_type(control_name);
        if (m_platform_topo.is_nested_domain(base_domain_type, domain_type)) {
            std::set<int> base_domain_idx = m_platform_topo.domain_nested(base_domain_type,
                                                                          domain_type, domain_idx);
            for (auto idx : base_domain_idx) {
                write_control(control_name, base_domain_type, idx, setting);
            }
        }
        else {
            throw Exception("PlatformIOImp::write_control(): domain " + std::to_string(domain_type) +
                            " is not valid for control \"" + control_name + "\"",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
    }

    void PlatformIOImp::save_control(void)
    {
        m_do_restore = true;
        for (auto &it : m_iogroup_list) {
            it->save_control();
        }
    }

    void PlatformIOImp::restore_control(void)
    {
        if (m_do_restore) {
            for (auto &it : m_iogroup_list) {
                it->restore_control();
            }
        }
    }

    std::function<double(const std::vector<double> &)> PlatformIOImp::agg_function(const std::string &signal_name) const
    {
        // Special signals from PlatformIOImp are aggregated by underlying signals
        auto iogroup = find_signal_iogroup(signal_name);
        if (iogroup == nullptr) {
            throw Exception("PlatformIOImp::agg_function(): unknown how to aggregate \"" + signal_name + "\"",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        return iogroup->agg_function(signal_name);
    }

    std::function<std::string(double)> PlatformIOImp::format_function(const std::string &signal_name) const
    {
        std::function<std::string(double)> result;
        /// @todo: find a better way to track signals produced by PlatformIOImp itself
        if (signal_name == "POWER_PACKAGE") {
            result = string_format_double;
        }
        else if (signal_name == "POWER_DRAM") {
            result = string_format_double;
        }
        else if (signal_name == "TEMPERATURE_CORE") {
            result = string_format_double;
        }
        else if (signal_name == "TEMPERATURE_PACKAGE") {
            result = string_format_double;
        }
        else {
            // PlatformIOImp forwards formatting request to underlying IOGroup
            auto iogroup = find_signal_iogroup(signal_name);
            if (iogroup == nullptr) {
               throw Exception("PlatformIOImp::format_function(): unknown how to format \"" + signal_name + "\"",
                               GEOPM_ERROR_INVALID, __FILE__, __LINE__);
            }
            result = iogroup->format_function(signal_name);
        }
        return result;
    }

    std::string PlatformIOImp::signal_description(const std::string &signal_name) const
    {
        /// @todo: find a better way to track signals produced by PlatformIOImp itself
        if (signal_name == "POWER_PACKAGE") {
            return "Average package power in watts over the last 8 samples (usually 40 ms).";
        }
        else if (signal_name == "POWER_DRAM") {
            return "Average DRAM power in watts over the last 8 samples (usually 40 ms).";
        }
        else if (signal_name == "TEMPERATURE_CORE") {
            return "Core temperaure in degrees C";
        }
        else if (signal_name == "TEMPERATURE_PACKAGE") {
            return "Package temperature in degrees C";
        }
        auto iogroup = find_signal_iogroup(signal_name);
        if (iogroup == nullptr) {
            throw Exception("PlatformIOImp::signal_description(): unknown signal \"" + signal_name + "\"",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        return iogroup->signal_description(signal_name);
    }

    std::string PlatformIOImp::control_description(const std::string &control_name) const
    {
        auto iogroup = find_control_iogroup(control_name);
        if (iogroup == nullptr) {
            throw Exception("PlatformIOImp::control_description(): unknown control \"" + control_name + "\"",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        return iogroup->control_description(control_name);
    }
}

extern "C" {

    int geopm_pio_num_signal_name(void)
    {
        int result = 0;
        try {
            result = geopm::platform_io().signal_names().size();
        }
        catch (...) {
            result = geopm::exception_handler(std::current_exception());
            result = result < 0 ? result : GEOPM_ERROR_RUNTIME;
        }
        return result;
    }

    static int geopm_pio_name_set_idx(int name_idx, size_t result_max,
                                      const std::set<std::string> name_set, char *result)
    {
        int err = 0;
        if (name_idx >= 0 &&
            (size_t)name_idx < name_set.size() &&
            result_max > 0) {
            auto ns_it = name_set.begin();
            for (int name_count = 0; name_count < name_idx; ++name_count) {
                ++ns_it;
            }
            result[result_max - 1] = '\0';
            strncpy(result, ns_it->c_str(), result_max );
            if (result[result_max - 1] != '\0') {
                err = GEOPM_ERROR_INVALID;
                result[result_max - 1] = '\0';
            }
        }
        else {
            err = GEOPM_ERROR_INVALID;
        }
        return err;
    }

    int geopm_pio_signal_name(int name_idx, size_t result_max, char *result)
    {
        int err = 0;
        if (result_max != 0) {
            result[0] = '\0';
        }
        try {
            std::set<std::string> name_set = geopm::platform_io().signal_names();
            err = geopm_pio_name_set_idx(name_idx, result_max, name_set, result);
        }
        catch (...) {
            err = geopm::exception_handler(std::current_exception());
            err = err < 0 ? err : GEOPM_ERROR_RUNTIME;
        }
        return err;
    }

    int geopm_pio_num_control_name(void)
    {
        int result = 0;
        try {
            result = geopm::platform_io().control_names().size();
        }
        catch (...) {
            result = geopm::exception_handler(std::current_exception());
            result = result < 0 ? result : GEOPM_ERROR_RUNTIME;
        }
        return result;
    }

    int geopm_pio_control_name(int name_idx, size_t result_max, char *result)
    {
        int err = 0;
        if (result_max != 0) {
            result[0] = '\0';
        }
        try {
            std::set<std::string> name_set = geopm::platform_io().control_names();
            err = geopm_pio_name_set_idx(name_idx, result_max, name_set, result);
        }
        catch (...) {
            err = geopm::exception_handler(std::current_exception());
            err = err < 0 ? err : GEOPM_ERROR_RUNTIME;
        }
        return err;
    }

    int geopm_pio_signal_domain_type(const char *signal_name)
    {
        int result = 0;
        try {
            const std::string signal_name_string(signal_name);
            result = geopm::platform_io().signal_domain_type(signal_name_string);
        }
        catch (...) {
            result = geopm::exception_handler(std::current_exception());
            result = result < 0 ? result : GEOPM_ERROR_RUNTIME;
        }
        return result;
    }

    int geopm_pio_control_domain_type(const char *control_name)
    {
        int result = 0;
        try {
            result = geopm::platform_io().control_domain_type(control_name);
        }
        catch (...) {
            result = geopm::exception_handler(std::current_exception());
            result = result < 0 ? result : GEOPM_ERROR_RUNTIME;
        }
        return result;
    }

    int geopm_pio_read_signal(const char *signal_name, int domain_type,
                              int domain_idx, double *result)
    {
        int err = 0;
        try {
            *result = geopm::platform_io().read_signal(signal_name, domain_type, domain_idx);
        }
        catch (...) {
            err = geopm::exception_handler(std::current_exception());
            err = err < 0 ? err : GEOPM_ERROR_RUNTIME;
        }
        return err;
    }

    int geopm_pio_write_control(const char *control_name, int domain_type,
                                int domain_idx, double setting)
    {
        int err = 0;
        try {
            geopm::platform_io().write_control(control_name, domain_type, domain_idx, setting);
        }
        catch (...) {
            err = geopm::exception_handler(std::current_exception());
            err = err < 0 ? err : GEOPM_ERROR_RUNTIME;
        }
        return err;
    }

    int geopm_pio_push_signal(const char *signal_name, int domain_type, int domain_idx)
    {
        int result = 0;
        try {
            result = geopm::platform_io().push_signal(signal_name, domain_type, domain_idx);
        }
        catch (...) {
            result = geopm::exception_handler(std::current_exception());
            result = result < 0 ? result : GEOPM_ERROR_RUNTIME;
        }
        return result;
    }

    int geopm_pio_push_control(const char *control_name, int domain_type, int domain_idx)
    {
        int result = 0;
        try {
            result = geopm::platform_io().push_control(control_name, domain_type, domain_idx);
        }
        catch (...) {
            result = geopm::exception_handler(std::current_exception());
            result = result < 0 ? result : GEOPM_ERROR_RUNTIME;
        }
        return result;
    }

    int geopm_pio_sample(int signal_idx, double *result)
    {
        int err = 0;
        try {
            *result = geopm::platform_io().sample(signal_idx);
        }
        catch (...) {
            err = geopm::exception_handler(std::current_exception());
            err = err < 0 ? err : GEOPM_ERROR_RUNTIME;
        }
        return err;
    }

    int geopm_pio_adjust(int control_idx, double setting)
    {
        int err = 0;
        try {
            geopm::platform_io().adjust(control_idx, setting);
        }
        catch (...) {
            err = geopm::exception_handler(std::current_exception());
            err = err < 0 ? err : GEOPM_ERROR_RUNTIME;
        }
        return err;
    }

    int geopm_pio_read_batch(void)
    {
        int err = 0;
        try {
            geopm::platform_io().read_batch();
        }
        catch (...) {
            err = geopm::exception_handler(std::current_exception());
            err = err < 0 ? err : GEOPM_ERROR_RUNTIME;
        }
        return err;
    }

    int geopm_pio_write_batch(void)
    {
        int err = 0;
        try {
            geopm::platform_io().write_batch();
        }
        catch (...) {
            err = geopm::exception_handler(std::current_exception());
            err = err < 0 ? err : GEOPM_ERROR_RUNTIME;
        }
        return err;
    }

    int geopm_pio_save_control(void)
    {
        int err = 0;
        try {
            geopm::platform_io().save_control();
        }
        catch (...) {
            err = geopm::exception_handler(std::current_exception());
            err = err < 0 ? err : GEOPM_ERROR_RUNTIME;
        }
        return err;
    }

    int geopm_pio_restore_control(void)
    {
        int err = 0;
        try {
            geopm::platform_io().restore_control();
        }
        catch (...) {
            err = geopm::exception_handler(std::current_exception());
            err = err < 0 ? err : GEOPM_ERROR_RUNTIME;
        }
        return err;
    }

    int geopm_pio_signal_description(const char *signal_name, size_t description_max,
                                     char *description)
    {
        int err = 0;
        try {
            std::string description_string = geopm::platform_io().signal_description(signal_name);
            description[description_max - 1] = '\0';
            strncpy(description, description_string.c_str(), description_max);
            if (description[description_max - 1] != '\0') {
                description[description_max - 1] = '\0';
                err = GEOPM_ERROR_INVALID;
            }
        }
        catch (...) {
            err = geopm::exception_handler(std::current_exception());
            err = err < 0 ? err : GEOPM_ERROR_RUNTIME;
        }
        return err;
    }

    int geopm_pio_control_description(const char *control_name, size_t description_max,
                                      char *description)
    {
        int err = 0;
        try {
            std::string description_string = geopm::platform_io().control_description(control_name);
            description[description_max - 1] = '\0';
            strncpy(description, description_string.c_str(), description_max);
            if (description[description_max - 1] != '\0') {
                description[description_max - 1] = '\0';
                err = GEOPM_ERROR_INVALID;
            }
        }
        catch (...) {
            err = geopm::exception_handler(std::current_exception());
            err = err < 0 ? err : GEOPM_ERROR_RUNTIME;
        }
        return err;
    }
}
