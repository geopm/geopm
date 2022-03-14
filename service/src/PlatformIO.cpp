/*
 * Copyright (c) 2015 - 2022, Intel Corporation
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

#include "PlatformIOImp.hpp"

#include <string.h>
#include <sys/types.h>
#include <algorithm>
#include <cmath>
#include <iostream>
#include <tuple>

#include "geopm/Agg.hpp"
#include "geopm/Exception.hpp"
#include "geopm/Helper.hpp"
#include "geopm/IOGroup.hpp"
#include "geopm/PlatformTopo.hpp"

#include "geopm_pio.h"
#include "BatchServer.hpp"
#include "CombinedSignal.hpp"
#include "ServiceIOGroup.hpp"

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
        if (m_iogroup_list.empty()) {
            for (const auto &it : IOGroup::iogroup_names()) {
                try {
                    register_iogroup(IOGroup::make_unique(it));
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

    std::vector<std::shared_ptr<IOGroup> > PlatformIOImp::find_signal_iogroup(const std::string &signal_name) const
    {
        std::vector<std::shared_ptr<IOGroup> > result;
        int native_domain = GEOPM_DOMAIN_INVALID;
        for (auto it = m_iogroup_list.rbegin();
             it != m_iogroup_list.rend();
             ++it) {
            if ((*it)->is_valid_signal(signal_name)) {
                if (result.empty()) {
                    result.push_back(*it);
                    native_domain = (*it)->signal_domain_type(signal_name);
                }
                else if ((*it)->signal_domain_type(signal_name) == native_domain) {
                    result.push_back(*it);
                }
#ifdef GEOPM_DEBUG
                else {
                    std::cerr << "Warning: <geopm> PlatformIO::find_signal_iogroup(): "
                              << "Native domain changed between IOGroups for signal name \""
                              << signal_name << "\".  Current cached domain is " << native_domain
                              << ". Ignoring IOGroup \"" << (*it)->name() << "\" at domain "
                              << (*it)->signal_domain_type(signal_name) << "."
                              << std::endl;
                }
#endif
            }
        }
        return result;
    }

    std::vector<std::shared_ptr<IOGroup> > PlatformIOImp::find_control_iogroup(const std::string &control_name) const
    {
        std::vector<std::shared_ptr<IOGroup> > result;
        int native_domain = GEOPM_DOMAIN_INVALID;
        for (auto it = m_iogroup_list.rbegin();
             it != m_iogroup_list.rend();
             ++it) {
            if ((*it)->is_valid_control(control_name)) {
                if (result.empty()) {
                    result.push_back(*it);
                    native_domain = (*it)->control_domain_type(control_name);
                }
                else if ((*it)->control_domain_type(control_name) == native_domain) {
                    result.push_back(*it);
                }
#ifdef GEOPM_DEBUG
                else {
                    std::cerr << "Warning: <geopm> PlatformIO::find_control_iogroup(): "
                              << "Native domain changed between IOGroups for control name \""
                              << control_name << "\".  Current cached domain is " << native_domain
                              << ". Ignoring IOGroup \"" << (*it)->name() << "\" at domain "
                              << (*it)->control_domain_type(control_name) << "."
                              << std::endl;
                }
#endif
            }
        }
        return result;
    }

    std::set<std::string> PlatformIOImp::signal_names(void) const
    {
        std::set<std::string> result;
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
        auto iogroups = find_signal_iogroup(signal_name);
        if (iogroups.empty()) {
            throw Exception("PlatformIOImp::signal_domain_type(): signal name \"" +
                            signal_name + "\" not found",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        return iogroups.at(0)->signal_domain_type(signal_name);
    }

    int PlatformIOImp::control_domain_type(const std::string &control_name) const
    {
        auto iogroups = find_control_iogroup(control_name);
        if (iogroups.empty()) {
            throw Exception("PlatformIOImp::control_domain_type(): control name \"" +
                            control_name + "\" not found",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        return iogroups.at(0)->control_domain_type(control_name);
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
        std::string err_msg;
        bool read_signal_ok = false;
        if (result == -1) {
            auto iogroups = find_signal_iogroup(signal_name);
            for (auto ii : iogroups) {
                if (domain_type == ii->signal_domain_type(signal_name)) {
                    try {
                        // Attempt to read before pushing to ensure batch reads will succeed
                        (void)ii->read_signal(signal_name, domain_type, domain_idx);
                        read_signal_ok = true;
                    }
                    catch (const geopm::Exception &ex) {
                        if (ex.err_value() == GEOPM_ERROR_NOT_IMPLEMENTED) {
                            // IOGroups may not support read_signal()
                            read_signal_ok = true;
                        }
                        else {
                            err_msg += std::string(ex.what()) + "\n";
                        }
                    }
                    if (read_signal_ok == true) {
                        int group_signal_idx = ii->push_signal(signal_name, domain_type, domain_idx);
                        result = m_active_signal.size();
                        m_existing_signal[sig_tup] = result;
                        m_active_signal.emplace_back(ii, group_signal_idx);
                        break;
                    }
                }
                else {
                    result = push_signal_convert_domain(signal_name, domain_type, domain_idx);
                    m_existing_signal[sig_tup] = result;
                    break;
                }
            }
        }
        if (result == -1) {
            std::string msg = "PlatformIOImp::push_signal(): no support for signal name \"" +
                              signal_name + "\" and domain type \"" +
                              std::to_string(domain_type) + "\"";
            if (err_msg.size() > 0) {
                msg += "\nThe following errors were observed:\n" + err_msg;
            }
            throw Exception(msg, GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        return result;
    }

    int PlatformIOImp::push_signal_convert_domain(const std::string &signal_name,
                                                  int domain_type,
                                                  int domain_idx)
    {
        int result = -1;
        int base_domain_type = signal_domain_type(signal_name);
        if (m_platform_topo.is_nested_domain(base_domain_type, domain_type)) {
            std::set<int> base_domain_idx = m_platform_topo.domain_nested(base_domain_type,
                                                                          domain_type, domain_idx);
            std::vector<int> signal_idx;
            for (auto it : base_domain_idx) {
                signal_idx.push_back(push_signal(signal_name, base_domain_type, it));
            }
            result = push_combined_signal(signal_name, domain_type, domain_idx, signal_idx);
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
        std::string err_msg;
        bool read_signal_ok = false;
        if (result == -1) {
            auto iogroups = find_control_iogroup(control_name);
            for (auto ii : iogroups) {
                if (ii->control_domain_type(control_name) == domain_type) {
                    int val;
                    try {
                        // Attempt to read then write the control to ensure batch writes will succeed
                        val = ii->read_signal(control_name, domain_type, domain_idx);
                        ii->write_control(control_name, domain_type, domain_idx, val);
                        read_signal_ok = true;
                    }
                    catch (const geopm::Exception &ex) {
                        if (ex.err_value() == GEOPM_ERROR_NOT_IMPLEMENTED) {
                            // IOGroups may not support read_signal()/write_control()
                            read_signal_ok = true;
                        }
                        else {
                            err_msg += std::string(ex.what()) + "\n";
                        }
                    }
                    if (read_signal_ok == true) {
                        int group_control_idx = ii->push_control(control_name, domain_type, domain_idx);
                        result = m_active_control.size();
                        m_existing_control[ctl_tup] = result;
                        m_active_control.emplace_back(ii, group_control_idx);
                        break;
                    }
                }
                else {
                    // Handle aggregated controls
                    result = push_control_convert_domain(control_name, domain_type, domain_idx);
                    m_existing_control[ctl_tup] = result;
                    break;
                }
            }
        }
        if (result == -1) {
            std::string msg = "PlatformIOImp::push_control(): no support for control name \"" +
                              control_name + "\" and domain type \"" +
                              std::to_string(domain_type) + "\"";
            if (err_msg.size() > 0) {
                msg += "\nThe following errors were observed:\n" + err_msg;
            }
            throw Exception(msg, GEOPM_ERROR_INVALID, __FILE__, __LINE__);
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
        auto iogroups = find_signal_iogroup(signal_name);
        if (iogroups.empty()) {
            throw Exception("PlatformIOImp::read_signal(): signal name \"" + signal_name + "\" not found",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }

        std::string err_msg;
        for (auto ii : iogroups) {
            if (ii->signal_domain_type(signal_name) != domain_type) {
                result = read_signal_convert_domain(signal_name, domain_type, domain_idx);
                break;
            }
            else {
                try {
                    result = ii->read_signal(signal_name, domain_type, domain_idx);
                    break;
                }
                catch (const geopm::Exception &ex) {
                    err_msg += std::string(ex.what()) + "\n";
                }
            }
        }
        if (std::isnan(result)) {
            std::string msg = "PlatformIOImp::read_signal(): no support for signal name \"" +
                              signal_name + "\" and domain type \"" +
                              std::to_string(domain_type) + "\"";
            if (err_msg.size() > 0) {
                msg += "\nThe following errors were observed:\n" + err_msg;
            }
            throw Exception(msg, GEOPM_ERROR_INVALID, __FILE__, __LINE__);
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

        auto iogroups = find_control_iogroup(control_name);
        if (iogroups.empty()) {
            throw Exception("PlatformIOImp::write_control(): control name \"" + control_name + "\" not found",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }

        std::string err_msg;
        bool write_complete = false;
        for (auto ii : iogroups) {
            if (ii->control_domain_type(control_name) != domain_type) {
                write_control_convert_domain(control_name, domain_type, domain_idx, setting);
                write_complete = true;
                break;
            }
            else {
                try {
                    ii->write_control(control_name, domain_type, domain_idx, setting);
                    write_complete = true;
                    break;
                }
                catch (const geopm::Exception &ex) {
                    err_msg += std::string(ex.what()) + "\n";
                }
            }
        }
        if (write_complete == false) {
            std::string msg = "PlatformIOImp::write_control(): no support for control name \"" +
                              control_name + "\" and domain type \"" +
                              std::to_string(domain_type) + "\"";
            if (err_msg.size() > 0) {
                msg += "\nThe following errors were observed:\n" + err_msg;
            }
            throw Exception(msg, GEOPM_ERROR_INVALID, __FILE__, __LINE__);
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
        if (!m_do_restore) {
            throw Exception("PlatformIOImp::restore_control(): Called prior to save_control()",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        for (auto &it : m_iogroup_list) {
            it->restore_control();
        }
    }

    void PlatformIOImp::save_control(const std::string &save_dir)
    {
        for (auto &it : m_iogroup_list) {
            std::string save_path = save_dir + '/' + it->name() + "-save-control.json";
            it->save_control(save_path);
        }
    }

    void PlatformIOImp::restore_control(const std::string &save_dir)
    {
        for (auto &it : m_iogroup_list) {
            std::string save_path = save_dir + '/' + it->name() + "-save-control.json";
            it->restore_control(save_path);
        }
    }

    std::function<double(const std::vector<double> &)> PlatformIOImp::agg_function(const std::string &signal_name) const
    {
        // Special signals from PlatformIOImp are aggregated by underlying signals
        auto iogroups = find_signal_iogroup(signal_name);
        if (iogroups.empty()) {
            throw Exception("PlatformIOImp::agg_function(): unknown how to aggregate \"" + signal_name + "\"",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }

        return iogroups.at(0)->agg_function(signal_name);
    }

    std::function<std::string(double)> PlatformIOImp::format_function(const std::string &signal_name) const
    {
        std::function<std::string(double)> result;
        // PlatformIOImp forwards formatting request to underlying IOGroup
        auto iogroups = find_signal_iogroup(signal_name);
        if (iogroups.empty()) {
            throw Exception("PlatformIOImp::format_function(): unknown how to format \"" + signal_name + "\"",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        return iogroups.at(0)->format_function(signal_name);
    }

    std::string PlatformIOImp::signal_description(const std::string &signal_name) const
    {
        auto iogroups = find_signal_iogroup(signal_name);
        if (iogroups.empty()) {
            throw Exception("PlatformIOImp::signal_description(): unknown signal \"" + signal_name + "\"",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        return iogroups.at(0)->signal_description(signal_name);
    }

    std::string PlatformIOImp::control_description(const std::string &control_name) const
    {
        auto iogroups = find_control_iogroup(control_name);
        if (iogroups.empty()) {
            throw Exception("PlatformIOImp::control_description(): unknown control \"" + control_name + "\"",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        return iogroups.at(0)->control_description(control_name);
    }

    int PlatformIOImp::signal_behavior(const std::string &signal_name) const
    {
        auto iogroups = find_signal_iogroup(signal_name);
        if (iogroups.empty()) {
            throw Exception("PlatformIOImp::signal_behavior(): unknown signal \"" + signal_name + "\"",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        return iogroups.at(0)->signal_behavior(signal_name);
    }

    void PlatformIOImp::start_batch_server(int client_pid,
                                           const std::vector<geopm_request_s> &signal_config,
                                           const std::vector<geopm_request_s> &control_config,
                                           int &server_pid,
                                           std::string &server_key)
    {
        if (signal_config.empty() && control_config.empty()) {
            throw Exception("PlatformIOImp::start_batch_server(): Requested a batch server, but no signals or controls were specified",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        std::shared_ptr<BatchServer> batch_server =
            BatchServer::make_unique(client_pid, signal_config, control_config);
        server_pid = batch_server->server_pid();
        server_key = batch_server->server_key();
        if (m_batch_server.find(server_pid) != m_batch_server.end()) {
            throw Exception("PlatformIOImp::start_batch_server(): Created a server with PID of existing server: " + std::to_string(server_pid),
                            GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
        }
        m_batch_server[server_pid] = batch_server;
    }

    void PlatformIOImp::stop_batch_server(int server_pid)
    {
        auto it = m_batch_server.find(server_pid);
        if (it == m_batch_server.end()) {
            throw Exception("PlatformIO::stop_batch_server(): Unknown batch server PID: " + std::to_string(server_pid),
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
#ifdef GEOPM_DEBUG
        if (!it->second->is_active()) {
            std::cerr << "Warning: <geopm> PlatformIO::stop_batch_server(): Batch server was inactive when it was stopped\n";
        }
#endif
        it->second->stop_batch();
        m_batch_server.erase(it);
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

    int geopm_pio_save_control_dir(const char *save_dir)
    {
        int err = 0;
        try {
            geopm::platform_io().save_control(save_dir);
        }
        catch (...) {
            err = geopm::exception_handler(std::current_exception());
            err = err < 0 ? err : GEOPM_ERROR_RUNTIME;
        }
        return err;
    }

    int geopm_pio_restore_control_dir(const char *save_dir)
    {
        int err = 0;
        try {
            geopm::platform_io().restore_control(save_dir);
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

    int geopm_pio_signal_info(const char *signal_name,
                              int *aggregation_type,
                              int *format_type,
                              int *behavior_type)
    {
        int err = 0;
        try {
            auto agg_func = geopm::platform_io().agg_function(signal_name);
            *aggregation_type = geopm::Agg::function_to_type(agg_func);
            auto format_func = geopm::platform_io().format_function(signal_name);
            *format_type = geopm::string_format_function_to_type(format_func);
            *behavior_type = geopm::platform_io().signal_behavior(signal_name);
        }
        catch (...) {
            err = geopm::exception_handler(std::current_exception());
            err = err < 0 ? err : GEOPM_ERROR_RUNTIME;
        }
        return err;
    }


    int geopm_pio_start_batch_server(int client_pid,
                                     int num_signal,
                                     const struct geopm_request_s *signal_config,
                                     int num_control,
                                     const struct geopm_request_s *control_config,
                                     int *server_pid,
                                     int key_size,
                                     char *server_key)
    {
        int err = 0;
        try {
            std::vector<geopm_request_s> signal_config_vec(num_signal);
            if (signal_config != nullptr) {
                std::copy(signal_config,
                          signal_config + num_signal,
                          signal_config_vec.begin());
            }
            std::vector<geopm_request_s> control_config_vec(num_control);
            if (control_config != nullptr) {
                std::copy(control_config,
                          control_config + num_control,
                          control_config_vec.begin());
            }
            std::string server_key_str;
            geopm::platform_io().start_batch_server(client_pid,
                                                    signal_config_vec,
                                                    control_config_vec,
                                                    *server_pid,
                                                    server_key_str);
            strncpy(server_key, server_key_str.c_str(), key_size);
            if (server_key[key_size - 1] != '\0') {
                server_key[key_size - 1] = '\0';
                err = GEOPM_ERROR_INVALID;
            }
        }
        catch (...) {
            err = geopm::exception_handler(std::current_exception());
            err = err < 0 ? err : GEOPM_ERROR_RUNTIME;
        }
        return err;
    }

    int geopm_pio_stop_batch_server(int server_pid)
    {
        int err = 0;
        try {
            geopm::platform_io().stop_batch_server(server_pid);
        }
        catch (...) {
            err = geopm::exception_handler(std::current_exception());
            err = err < 0 ? err : GEOPM_ERROR_RUNTIME;
        }
        return err;
    }

    int geopm_pio_format_signal(double signal,
                                int format_type,
                                size_t result_max,
                                char *result)
    {
        int err = 0;
        try {
            auto format_func = geopm::string_format_type_to_function(format_type);
            std::string result_cxx = format_func(signal);
            result[result_max - 1] = '\0';
            strncpy(result, result_cxx.c_str(), result_max);
            if (result[result_max - 1] != '\0') {
                err = GEOPM_ERROR_INVALID;
                result[result_max - 1] = '\0';
            }
        }
        catch (...) {
            err = geopm::exception_handler(std::current_exception());
            err = err < 0 ? err : GEOPM_ERROR_RUNTIME;
        }
        return err;

    }
}
