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

#include <cpuid.h>
#include <iomanip>
#include <cmath>
#include <algorithm>

#include "geopm_sched.h"
#include "PlatformIO.hpp"
#include "PlatformIOInternal.hpp"
#include "PlatformTopo.hpp"
#include "MSRIOGroup.hpp"
#include "TimeIOGroup.hpp"

#include "Exception.hpp"

#include "config.h"

namespace geopm
{
    IPlatformIO &platform_io(void)
    {
        static PlatformIO instance;
        return instance;
    }

    PlatformIO::PlatformIO()
        : m_is_active(false)
    {
         for (const auto &it : iogroup_factory().plugin_names()) {
             register_iogroup(iogroup_factory().make_plugin(it));
         }
    }

    PlatformIO::PlatformIO(std::list<std::unique_ptr<IOGroup> > iogroup_list)
        : m_is_active(false)
        , m_iogroup_list(std::move(iogroup_list))
    {

    }

    PlatformIO::~PlatformIO()
    {

    }

    void PlatformIO::register_iogroup(std::unique_ptr<IOGroup> iogroup)
    {
        m_iogroup_list.push_back(std::move(iogroup));
    }

    int PlatformIO::signal_domain_type(const std::string &signal_name) const
    {
        int result = PlatformTopo::M_DOMAIN_INVALID;
        bool is_found = false;
        for (auto it = m_iogroup_list.rbegin();
             !is_found && it != m_iogroup_list.rend();
             ++it) {
            if ((*it)->is_valid_signal(signal_name)) {
                result = (*it)->signal_domain_type(signal_name);
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
        bool is_found = false;
        for (auto it = m_iogroup_list.rbegin();
             !is_found && it != m_iogroup_list.rend();
             ++it) {
            if ((*it)->is_valid_control(control_name)) {
                result = (*it)->control_domain_type(control_name);
                is_found = true;
            }
        }
        if (!is_found) {
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
        bool is_found = false;
        for (auto it = m_iogroup_list.rbegin();
             !is_found && it != m_iogroup_list.rend();
             ++it) {
            if ((*it)->is_valid_signal(signal_name)) {
                int group_signal_idx = (*it)->push_signal(signal_name, domain_type, domain_idx);
                result = m_active_signal.size();
                m_active_signal.emplace_back((*it).get(), group_signal_idx);
                is_found = true;
            }
        }

        if (!is_found && (signal_name == "POWER_PACKAGE" || signal_name == "POWER_DRAM")) {
            int energy_idx = -1;
            if (signal_name == "POWER_PACKAGE") {
                energy_idx = push_signal("ENERGY_PACKAGE", domain_type, domain_idx);
            }
            else if (signal_name == "POWER_DRAM") {
                energy_idx = push_signal("ENERGY_DRAM", domain_type, domain_idx);
            }

            int time_idx = push_signal("TIME", domain_type, domain_idx);
            int region_id_idx = push_signal("REGION_ID#", domain_type, domain_idx);
            result = m_active_signal.size();

            register_combined_signal(result,
                                     {region_id_idx, time_idx, energy_idx},
                                     std::unique_ptr<CombinedSignal>(new PerRegionDerivativeCombinedSignal));

            m_active_signal.emplace_back(nullptr, result);
            is_found = true;
        }

        if (result == -1) {
            throw Exception("PlatformIO::push_signal(): signal name \"" + signal_name + "\" not found",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
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
        bool is_found = false;
        for (auto it = m_iogroup_list.rbegin();
             !is_found && it != m_iogroup_list.rend();
             ++it) {
            if ((*it)->is_valid_control(control_name)) {
                int group_control_idx = (*it)->push_control(control_name, domain_type, domain_idx);
                result = m_active_control.size();
                m_active_control.emplace_back((*it).get(), group_control_idx);
                is_found = true;
            }
        }
        if (result == -1) {
            throw Exception("PlatformIO::push_control(): control name \"" +
                            control_name + "\" not found",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
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
        auto &group_idx_pair = m_active_control[control_idx];
        group_idx_pair.first->adjust(group_idx_pair.second, setting);
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
        double result = 0.0;
        bool is_found = false;
        for (auto it = m_iogroup_list.rbegin();
             !is_found && it != m_iogroup_list.rend();
             ++it) {
            if ((*it)->is_valid_signal(signal_name)) {
                result = (*it)->read_signal(signal_name, domain_type, domain_idx);
                is_found = true;
            }
        }
        if (!is_found) {
            throw Exception("PlatformIO::read_signal(): signal name \"" + signal_name + "\" not found",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        return result;
    }

    void PlatformIO::write_control(const std::string &control_name,
                                   int domain_type,
                                   int domain_idx,
                                   double setting)
    {
        bool is_found = false;
        for (auto it = m_iogroup_list.rbegin();
             !is_found && it != m_iogroup_list.rend();
             ++it) {
            if ((*it)->is_valid_control(control_name)) {
                (*it)->write_control(control_name, domain_type, domain_idx, setting);
                is_found = true;
            }
        }
        if (!is_found) {
            throw Exception("PlatformIO::write_control(): control name \"" + control_name + "\" not found",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
    }
}
