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
#include <numeric>

#include "geopm_sched.h"
#include "geopm_message.h"
#include "geopm_hash.h"
#include "geopm.h"
#include "PlatformIO.hpp"
#include "PlatformIOInternal.hpp"
#include "PlatformTopo.hpp"
#include "MSRIOGroup.hpp"
#include "TimeIOGroup.hpp"
#include "Exception.hpp"
#include "Helper.hpp"

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
    {
        if (m_iogroup_list.size() == 0) {
            for (const auto &it : iogroup_factory().plugin_names()) {
                register_iogroup(iogroup_factory().make_plugin(it));
            }
        }
    }

    void PlatformIO::register_iogroup(std::shared_ptr<IOGroup> iogroup)
    {
        m_iogroup_list.push_back(iogroup);
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
        for (auto it = m_iogroup_list.rbegin();
             result == -1 && it != m_iogroup_list.rend();
             ++it) {
            if ((*it)->is_valid_signal(signal_name) &&
                (*it)->signal_domain_type(signal_name) == domain_type) {
                int group_signal_idx = (*it)->push_signal(signal_name, domain_type, domain_idx);
                result = m_active_signal.size();
                m_active_signal.emplace_back((*it).get(), group_signal_idx);
            }
        }
        if (result == -1 && signal_name.find("POWER") != std::string::npos) {
            result = push_signal_power(signal_name, domain_type, domain_idx);
        }
        if (result == -1) {
            result = push_signal_convert_domain(signal_name, domain_type, domain_idx);
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
            int region_id_idx = push_signal("REGION_ID#", domain_type, domain_idx);
            result = m_active_signal.size();

            register_combined_signal(result,
                                     {region_id_idx, time_idx, energy_idx},
                                     std::unique_ptr<CombinedSignal>(new PerRegionDerivativeCombinedSignal));

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
            std::set<int> cpus;
            m_platform_topo.domain_cpus(domain_type, domain_idx, cpus);
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

    void PlatformIO::push_region_signal_total(int signal_idx, int domain_type, int domain_idx)
    {
        int rid_idx = push_signal("REGION_ID#", domain_type, domain_idx);
        m_region_id_idx[signal_idx] = rid_idx;
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

    double PlatformIO::sample(int signal_idx) const
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

    double PlatformIO::sample_region_total(int signal_idx, uint64_t region_id) const
    {
        double current_value = 0.0;
        uint64_t curr_rid = geopm_signal_to_field(sample(m_region_id_idx.at(signal_idx)));
        curr_rid = geopm_region_id_unset_hint(GEOPM_MASK_REGION_HINT, curr_rid);
        auto idx = std::make_pair(signal_idx, region_id);
        if (m_region_sample_data.find(idx) != m_region_sample_data.end()) {
            const auto &data =  m_region_sample_data.at(idx);
            current_value += data.total;
            // if currently in this region, add current value to total
            if (region_id == curr_rid &&
                !isnan(data.last_entry_value)) {
                current_value += sample(signal_idx) - data.last_entry_value;
            }
        }
        return current_value;
    }

    double PlatformIO::sample_combined(int signal_idx) const
    {
        double result = NAN;
        auto &op_func_pair = m_combined_signal.at(signal_idx);
        const std::vector<int> &operand_idx = op_func_pair.first;
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
        // aggregate region totals
        for (const auto &it : m_region_id_idx) {
            double value = sample(it.first);
            uint64_t region_id = geopm_signal_to_field(sample(it.second));
            region_id = geopm_region_id_unset_hint(GEOPM_MASK_REGION_HINT, region_id);
            // first time sampling this signal
            if (m_last_region_id.find(it.first) == m_last_region_id.end()) {
                m_last_region_id[it.first] = region_id;
                // set start value for first region to be recording this signal
                m_region_sample_data[std::make_pair(it.first, region_id)].last_entry_value = value;
            }
            else {
                uint64_t last_rid = m_last_region_id[it.first];
                // region boundary
                if (region_id != last_rid) {
                    // add entry to new region
                    m_region_sample_data[std::make_pair(it.first, region_id)].last_entry_value = value;
                    // update total for previous region
                    m_region_sample_data[std::make_pair(it.first, last_rid)].total +=
                        value - m_region_sample_data.at(std::make_pair(it.first, last_rid)).last_entry_value;
                    m_last_region_id[it.first] = region_id;
                }
            }
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
                                   int domain_idx) const
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

    std::function<double(const std::vector<double> &)> PlatformIO::agg_function(std::string signal_name) const
    {
        static const std::map<std::string, std::function<double(const std::vector<double> &)> > fn_map {
            {"POWER", IPlatformIO::agg_sum},
            {"REGION_POWER", IPlatformIO::agg_sum},
            {"POWER_PACKAGE", IPlatformIO::agg_sum},
            {"POWER_DRAM", IPlatformIO::agg_sum},
            {"FREQUENCY", IPlatformIO::agg_average},
            {"RUNTIME", IPlatformIO::agg_max},
            {"REGION_RUNTIME", IPlatformIO::agg_max},
            {"REGION_PROGRESS", IPlatformIO::agg_min},
            {"EPOCH_RUNTIME", IPlatformIO::agg_max},
            {"ENERGY", IPlatformIO::agg_sum},
            {"REGION_ENERGY", IPlatformIO::agg_sum},
            {"ENERGY_PACKAGE", IPlatformIO::agg_sum},
            {"ENERGY_DRAM", IPlatformIO::agg_sum},
            {"EPOCH_ENERGY", IPlatformIO::agg_sum},
            {"IS_CONVERGED", IPlatformIO::agg_and},
            {"IS_UPDATED", IPlatformIO::agg_and},
            {"REGION_ID#", IPlatformIO::agg_region_id},
            {"CYCLES_THREAD", IPlatformIO::agg_average},
            {"CYCLES_REFERENCE", IPlatformIO::agg_average},
            {"TIME", IPlatformIO::agg_average}
        };
        auto it = fn_map.find(signal_name);
        if (it == fn_map.end()) {
            throw Exception("PlatformIO::agg_function(): unknown how to aggregate \"" + signal_name + "\"",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        return it->second;
    }

    double IPlatformIO::agg_sum(const std::vector<double> &operand)
    {
        double result = NAN;
        if (operand.size()) {
            result = std::accumulate(operand.begin(), operand.end(), 0.0);
        }
        return result;
    }

    double IPlatformIO::agg_average(const std::vector<double> &operand)
    {
        double result = NAN;
        if (operand.size()) {
            result = agg_sum(operand) / operand.size();
        }
        return result;
    }

    double IPlatformIO::agg_median(const std::vector<double> &operand)
    {
        double result = NAN;
        size_t num_op = operand.size();
        if (num_op) {
            size_t mid_idx = num_op / 2;
            bool is_even = ((num_op % 2) == 0);
            std::vector<double> operand_sorted(operand);
            std::sort(operand_sorted.begin(), operand_sorted.end());
            result = operand_sorted[mid_idx];
            if (is_even) {
                result += operand_sorted[mid_idx - 1];
                result /= 2.0;
            }
        }
        return result;
    }

    double IPlatformIO::agg_and(const std::vector<double> &operand)
    {
        double result = NAN;
        if (operand.size()) {
            result = std::all_of(operand.begin(), operand.end(),
                                 [](double it) {return (it != 0.0);});
        }
        return result;
    }

    double IPlatformIO::agg_or(const std::vector<double> &operand)
    {
        double result = NAN;
        if (operand.size()) {
            result = std::any_of(operand.begin(), operand.end(),
                                 [](double it) {return (it != 0.0);});
        }
        return result;
    }

    double IPlatformIO::agg_region_id(const std::vector<double> &operand)
    {
        uint64_t common_rid = GEOPM_REGION_ID_UNDEFINED;
        if (operand.size()) {
            for (const auto &it : operand) {
                uint64_t it_rid = geopm_signal_to_field(it);
                if (it_rid != GEOPM_REGION_ID_UNDEFINED &&
                    common_rid == GEOPM_REGION_ID_UNDEFINED) {
                    common_rid = it_rid;
                }
                if (common_rid != GEOPM_REGION_ID_UNDEFINED &&
                    it_rid != GEOPM_REGION_ID_UNDEFINED &&
                    it_rid != common_rid) {
                    common_rid = GEOPM_REGION_ID_UNMARKED;
                    break;
                }
            }
        }
        return geopm_field_to_signal(common_rid);
    }

    double IPlatformIO::agg_min(const std::vector<double> &operand)
    {
        double result = NAN;
        if (operand.size()) {
            result = *std::min_element(operand.begin(), operand.end());
        }
        return result;
    }

    double IPlatformIO::agg_max(const std::vector<double> &operand)
    {
        double result = NAN;
        if (operand.size()) {
            result = *std::max_element(operand.begin(), operand.end());
        }
        return result;
    }

    double IPlatformIO::agg_stddev(const std::vector<double> &operand)
    {
        double result = NAN;
        if (operand.size() > 1) {
            double sum_squared = agg_sum(operand);
            sum_squared *= sum_squared;
            std::vector<double> operand_squared(operand);
            for (auto &it : operand_squared) {
                it *= it;
            }
            double sum_squares = agg_sum(operand_squared);
            double aa = 1.0 / (operand.size() - 1);
            double bb = aa / operand.size();
            result = std::sqrt(aa * sum_squares - bb * sum_squared);
        }
        else if (operand.size() == 1) {
            result = 0.0;
        }
        return result;
    }
}
