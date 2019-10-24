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

#include "MSRIOGroup.hpp"

#include <cpuid.h>
#include <cmath>
#include <sstream>
#include <algorithm>
#include <utility>
#include <iomanip>
#include <iostream>

#include "contrib/json11/json11.hpp"

#include "geopm_sched.h"
#include "geopm_hash.h"
#include "Environment.hpp"
#include "Exception.hpp"
#include "Agg.hpp"
#include "MSR.hpp"
#include "MSRSignalImp.hpp"
#include "MSRControlImp.hpp"
#include "MSRIOImp.hpp"
#include "PlatformTopo.hpp"
#include "Helper.hpp"
#include "config.h"

using json11::Json;

#define GEOPM_MSR_IO_GROUP_PLUGIN_NAME "MSR"

namespace geopm
{
    const std::string arch_msr_json(void);
    const std::string knl_msr_json(void);
    const std::string hsx_msr_json(void);
    const std::string snb_msr_json(void);
    const std::string skx_msr_json(void);
    static std::vector<std::unique_ptr<MSR> > init_msr_arr(int cpu_id);

    MSRIOGroup::MSRIOGroup()
        : MSRIOGroup(platform_topo(), std::unique_ptr<MSRIO>(new MSRIOImp), cpuid(), geopm_sched_num_cpu())
    {

    }

    MSRIOGroup::MSRIOGroup(const PlatformTopo &topo, std::unique_ptr<MSRIO> msrio, int cpuid, int num_cpu)
        : m_platform_topo(topo)
        , m_num_cpu(num_cpu)
        , m_is_active(false)
        , m_is_read(false)
        , m_msrio(std::move(msrio))
        , m_cpuid(cpuid)
        , m_name_prefix(plugin_name() + "::")
        , m_per_cpu_restore(m_num_cpu)
        , m_is_fixed_enabled(false)
    {
        m_msr_arr = init_msr_arr(m_cpuid);
        for (const auto &msr_ptr : m_msr_arr) {
            m_name_msr_map.insert(std::pair<std::string, const MSR &>(msr_ptr->name(), *msr_ptr));
            for (int idx = 0; idx < msr_ptr->num_signal(); idx++) {
                register_msr_signal(m_name_prefix + msr_ptr->name() + ":" + msr_ptr->signal_name(idx));
            }
            for (int idx = 0; idx < msr_ptr->num_control(); idx++) {
                register_msr_control(m_name_prefix + msr_ptr->name() + ":" + msr_ptr->control_name(idx));
            }
            register_raw_msr_signal(msr_ptr->name(), *msr_ptr);
        }

        // Set up aggregation functions for MSRs
        // Aliases will lookup and use the agg_function for their underlying MSR
        /// @todo These functions should come from the MSR structs in msr_*.cpp
        m_func_map["MSR::PERF_STATUS:FREQ"] = Agg::average;
        m_func_map["MSR::PKG_ENERGY_STATUS:ENERGY"] = Agg::sum;
        m_func_map["MSR::DRAM_ENERGY_STATUS:ENERGY"] = Agg::sum;
        m_func_map["MSR::FIXED_CTR0:INST_RETIRED_ANY"] = Agg::sum;
        m_func_map["MSR::FIXED_CTR1:CPU_CLK_UNHALTED_THREAD"] = Agg::sum;
        m_func_map["MSR::FIXED_CTR2:CPU_CLK_UNHALTED_REF_TSC"] = Agg::sum;
        m_func_map["MSR::PKG_POWER_INFO:MIN_POWER"] = Agg::expect_same;
        m_func_map["MSR::PKG_POWER_INFO:MAX_POWER"] = Agg::expect_same;
        m_func_map["MSR::PKG_POWER_INFO:THERMAL_SPEC_POWER"] = Agg::expect_same;
        m_func_map["MSR::THERM_STATUS:DIGITAL_READOUT"] = Agg::average;
        m_func_map["MSR::TEMPERATURE_TARGET:PROCHOT_MIN"] = Agg::expect_same;

        m_signal_desc_map["MSR::PKG_POWER_INFO:THERMAL_SPEC_POWER"] = "Maximum power to stay within thermal limits (TDP)";

        m_control_desc_map["MSR::PKG_POWER_LIMIT:PL1_POWER_LIMIT"] = "Set RAPL power limit";
        m_control_desc_map["MSR::PERF_CTL:FREQ"] = "Set processor frequency";

        register_msr_signal("TIMESTAMP_COUNTER", "MSR::TIME_STAMP_COUNTER:TIMESTAMP_COUNT");
        register_msr_signal("FREQUENCY",         "MSR::PERF_STATUS:FREQ");

        std::string max_turbo_name;
        switch (m_cpuid) {
            case MSRIOGroup::M_CPUID_KNL:
                max_turbo_name = "MSR::TURBO_RATIO_LIMIT:GROUP_0_MAX_RATIO_LIMIT";
                break;
            case MSRIOGroup::M_CPUID_SNB:
            case MSRIOGroup::M_CPUID_IVT:
            case MSRIOGroup::M_CPUID_HSX:
            case MSRIOGroup::M_CPUID_BDX:
                max_turbo_name = "MSR::TURBO_RATIO_LIMIT:MAX_RATIO_LIMIT_1CORE";
                break;
            case MSRIOGroup::M_CPUID_SKX:
                max_turbo_name = "MSR::TURBO_RATIO_LIMIT:MAX_RATIO_LIMIT_0";
                break;
            default:
                throw Exception("MSRIOGroup: Unsupported CPUID",
                                GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
        }
        register_msr_signal("FREQUENCY_MAX", max_turbo_name);

        register_msr_signal("ENERGY_PACKAGE",    "MSR::PKG_ENERGY_STATUS:ENERGY");
        register_msr_signal("ENERGY_DRAM",       "MSR::DRAM_ENERGY_STATUS:ENERGY");
        register_msr_signal("INSTRUCTIONS_RETIRED", "MSR::FIXED_CTR0:INST_RETIRED_ANY");
        register_msr_signal("CYCLES_THREAD",     "MSR::FIXED_CTR1:CPU_CLK_UNHALTED_THREAD");
        register_msr_signal("CYCLES_REFERENCE",  "MSR::FIXED_CTR2:CPU_CLK_UNHALTED_REF_TSC");

        // power limit aliases
        register_msr_signal("POWER_PACKAGE_MIN", "MSR::PKG_POWER_INFO:MIN_POWER");
        register_msr_signal("POWER_PACKAGE_MAX", "MSR::PKG_POWER_INFO:MAX_POWER");
        register_msr_signal("POWER_PACKAGE_TDP", "MSR::PKG_POWER_INFO:THERMAL_SPEC_POWER");
        // power limits with sum as aggregation function
        register_msr_signal("POWER_PACKAGE_MIN_TOTAL", "MSR::PKG_POWER_INFO:MIN_POWER");
        register_msr_signal("POWER_PACKAGE_MAX_TOTAL", "MSR::PKG_POWER_INFO:MAX_POWER");
        register_msr_signal("POWER_PACKAGE_TDP_TOTAL", "MSR::PKG_POWER_INFO:THERMAL_SPEC_POWER");
        m_func_map["POWER_PACKAGE_MIN_TOTAL"] = Agg::sum;
        m_func_map["POWER_PACKAGE_MAX_TOTAL"] = Agg::sum;
        m_func_map["POWER_PACKAGE_TDP_TOTAL"] = Agg::sum;

        // @todo: have MSRIOGroup handle this combined signal instead of platformIO
        register_msr_signal("TEMPERATURE_CORE_UNDER", "MSR::THERM_STATUS:DIGITAL_READOUT");
        register_msr_signal("TEMPERATURE_PKG_UNDER", "MSR::PACKAGE_THERM_STATUS:DIGITAL_READOUT");
        register_msr_signal("TEMPERATURE_MAX", "MSR::TEMPERATURE_TARGET:PROCHOT_MIN");

        register_msr_control("POWER_PACKAGE_LIMIT", "MSR::PKG_POWER_LIMIT:PL1_POWER_LIMIT");
        register_msr_control("FREQUENCY",        "MSR::PERF_CTL:FREQ");
        register_msr_control("POWER_PACKAGE_TIME_WINDOW", "MSR::PKG_POWER_LIMIT:PL1_TIME_WINDOW");
    }

    void MSRIOGroup::register_raw_msr_signal(const std::string &msr_name, const MSR &msr_ptr)
    {
        // Insert the signal name with an empty vector into the map
        auto ins_ret = m_name_cpu_signal_map.insert(std::pair<std::string, std::vector<std::shared_ptr<MSRSignal> > >(m_name_prefix + msr_name + "#", {}));
        // Get reference to the per-cpu signal vector
        auto &cpu_signal = (*(ins_ret.first)).second;
        // Check to see if the signal name has already been registered
        if (!ins_ret.second) {
            throw Exception("MSRIOGroup::register_raw_msr_signal(): msr_name " + msr_name +
                            " was previously registered.",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        auto name_msr_it = m_name_msr_map.find(msr_name);
        if (name_msr_it == m_name_msr_map.end()) {
            throw Exception("MSRIOGroup::register_raw_msr_signal(): msr_name could not be found: " + msr_name,
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        const MSR &msr_obj = name_msr_it->second;
        cpu_signal.resize(m_num_cpu, nullptr);
        for (int cpu_idx = 0; cpu_idx < m_num_cpu; ++cpu_idx) {
            cpu_signal[cpu_idx] = std::make_shared<MSRSignalImp>(msr_obj, msr_obj.domain_type(), cpu_idx);
        }
    }

    MSRIOGroup::~MSRIOGroup()
    {

    }

    std::set<std::string> MSRIOGroup::signal_names(void) const
    {
        std::set<std::string> result;
        for (const auto &sv : m_name_cpu_signal_map) {
            result.insert(sv.first);
        }
        return result;
    }

    std::set<std::string> MSRIOGroup::control_names(void) const
    {
        std::set<std::string> result;
        for (const auto &sv : m_name_cpu_control_map) {
            result.insert(sv.first);
        }
        return result;
    }

    bool MSRIOGroup::is_valid_signal(const std::string &signal_name) const
    {
        return m_name_cpu_signal_map.find(signal_name) != m_name_cpu_signal_map.end();
    }

    bool MSRIOGroup::is_valid_control(const std::string &control_name) const
    {
        return m_name_cpu_control_map.find(control_name) != m_name_cpu_control_map.end();
    }

    int MSRIOGroup::signal_domain_type(const std::string &signal_name) const
    {
        int result = GEOPM_DOMAIN_INVALID;
        auto it = m_name_cpu_signal_map.find(signal_name);
        if (it != m_name_cpu_signal_map.end()) {
            result = it->second[0]->domain_type();
        }
        return result;
    }

    int MSRIOGroup::control_domain_type(const std::string &control_name) const
    {
        int result = GEOPM_DOMAIN_INVALID;
        auto it = m_name_cpu_control_map.find(control_name);
        if (it != m_name_cpu_control_map.end()) {
            result = it->second[0]->domain_type();
        }
        return result;
    }

    int MSRIOGroup::push_signal(const std::string &signal_name, int domain_type, int domain_idx)
    {
        if (m_is_active) {
            throw Exception("MSRIOGroup::push_signal(): cannot push a signal after read_batch() or adjust() has been called.",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        if (!m_is_fixed_enabled) {
            enable_fixed_counters();
        }
        auto ncsm_it = m_name_cpu_signal_map.find(signal_name);
        if (ncsm_it == m_name_cpu_signal_map.end()) {
            throw Exception("MSRIOGroup::push_signal(): signal name \"" +
                            signal_name + "\" not found",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        if (domain_type != signal_domain_type(signal_name)) {
            throw Exception("MSRIOGroup::push_signal(): domain_type does not match the domain of the signal.",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        if (domain_idx < 0 || domain_idx >= m_platform_topo.num_domain(domain_type)) {
            throw Exception("MSRIOGroup::push_signal(): domain_idx out of range",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        std::set<int> cpu_idx = m_platform_topo.domain_nested(GEOPM_DOMAIN_CPU,
                                                              domain_type, domain_idx);

        int result = -1;
        bool is_found = false;
        // Check if signal was already pushed
        for (size_t ii = 0; !is_found && ii < m_active_signal.size(); ++ii) {
#ifdef GEOPM_DEBUG
            if (!m_active_signal[ii]) {
                throw Exception("MSRIOGroup::push_signal(): NULL MSRSignal pointer was saved in active signals",
                                GEOPM_ERROR_LOGIC, __FILE__, __LINE__);
            }
#endif
            // signal_name may be alias, so use active signal MSR name
            std::string registered_name = ncsm_it->second[*(cpu_idx.begin())]->name();
            if (m_active_signal[ii]->name() == registered_name &&
                m_active_signal[ii]->cpu_idx() == *(cpu_idx.begin())) {
                result = ii;
                is_found = true;
            }
        }

        if (!is_found) {
            result = m_active_signal.size();
            m_active_signal.emplace_back(ncsm_it->second[*(cpu_idx.begin())]);
            const auto &msr_sig = m_active_signal[result];
#ifdef GEOPM_DEBUG
            if (!msr_sig) {
                throw Exception("MSRIOGroup::push_signal(): NULL MSRSignal pointer was saved in active signals",
                                GEOPM_ERROR_LOGIC, __FILE__, __LINE__);
            }
#endif
            uint64_t offset = msr_sig->offset();
            m_read_cpu_idx.push_back(*(cpu_idx.begin()));
            m_read_offset.push_back(offset);
        }
        return result;
    }

    int MSRIOGroup::push_control(const std::string &control_name, int domain_type, int domain_idx)
    {
        if (m_is_active) {
            throw Exception("MSRIOGroup::push_control(): cannot push a control after read_batch() or adjust() has been called.",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        check_control(control_name);

        auto nccm_it = m_name_cpu_control_map.find(control_name);
        if (nccm_it == m_name_cpu_control_map.end()) {
            throw Exception("MSRIOGroup::push_control(): control name \"" +
                            control_name + "\" not found",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        if (domain_type != control_domain_type(control_name)) {
            throw Exception("MSRIOGroup::push_control(): domain_type does not match the domain of the control.",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        if (domain_idx < 0 || domain_idx >= m_platform_topo.num_domain(domain_type)) {
            throw Exception("MSRIOGroup::push_control(): domain_idx out of range",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        std::set<int> cpu_idx = m_platform_topo.domain_nested(GEOPM_DOMAIN_CPU,
                                                              domain_type, domain_idx);
#ifdef GEOPM_DEBUG
        if (cpu_idx.size() == 0) {
            throw Exception("MSRIOGroup::push_control(): no cpus for domain",
                            GEOPM_ERROR_LOGIC, __FILE__, __LINE__);
        }
#endif
        int result = -1;
        bool is_found = false;
        // Check if control was already pushed
        for (size_t ii = 0; !is_found && ii < m_active_control.size(); ++ii) {
#ifdef GEOPM_DEBUG
            if (m_active_control[ii].size() == 0 || !m_active_control[ii][0]) {
                throw Exception("MSRIOGroup::push_control(): NULL MSRControl pointer was saved in active controls",
                                GEOPM_ERROR_LOGIC, __FILE__, __LINE__);
            }
#endif
            // control_name may be alias, so use active control MSR name
            std::string registered_name = nccm_it->second[*(cpu_idx.begin())]->name();
            if (m_active_control[ii][0]->name() == registered_name &&
                m_active_control[ii][0]->cpu_idx() == *(cpu_idx.begin())) {
                result = ii;
                is_found = true;
            }
        }

        if (!is_found) {
            result = m_active_control.size();
            m_active_control.emplace_back();
            if (control_name == "POWER_PACKAGE_LIMIT") {
                write_control("MSR::PKG_POWER_LIMIT:PL1_LIMIT_ENABLE", domain_type, domain_idx, 1.0);
                // for power only set the first cpu in the package; others are lowered
                cpu_idx = {*cpu_idx.begin()};
            }
            for (auto cpu : cpu_idx) {
                const auto &msr_ctl = nccm_it->second[cpu];
                m_active_control[result].push_back(msr_ctl);
#ifdef GEOPM_DEBUG
                if (!msr_ctl) {
                    throw Exception("MSRIOGroup::push_control(): NULL MSRControl pointer was saved in active controls",
                                    GEOPM_ERROR_LOGIC, __FILE__, __LINE__);
                }
#endif
                uint64_t offset = msr_ctl->offset();
                uint64_t mask = msr_ctl->mask();
                m_write_cpu_idx.push_back(cpu);
                m_write_offset.push_back(offset);
                m_write_mask.push_back(mask);
            }
            m_is_adjusted.push_back(false);
        }
        return result;
    }

    void MSRIOGroup::read_batch(void)
    {
        if (!m_is_active) {
            activate();
        }
        if (m_read_field.size()) {
            m_msrio->read_batch(m_read_field);
        }
        m_is_read = true;
    }

    void MSRIOGroup::write_batch(void)
    {
        if (m_active_control.size()) {
            if (std::any_of(m_is_adjusted.begin(), m_is_adjusted.end(), [](bool it) {return !it;})) {
                throw Exception("MSRIOGroup::write_batch() called before all controls were adjusted",
                                GEOPM_ERROR_INVALID, __FILE__, __LINE__);
            }
            m_msrio->write_batch(m_write_field);
        }
    }

    double MSRIOGroup::sample(int signal_idx)
    {
        if (signal_idx < 0 || signal_idx >= (int)m_active_signal.size()) {
            throw Exception("MSRIOGroup::sample(): signal_idx out of range",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }

        if (!m_is_read) {
            throw Exception("MSRIOGroup::sample() called before signal was read.",
                            GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
        }

        return m_active_signal[signal_idx]->sample();
    }

    void MSRIOGroup::adjust(int control_idx, double setting)
    {
        if (control_idx < 0 || (unsigned)control_idx >= m_active_control.size()) {
            throw Exception("MSRIOGroup::adjust(): control_idx out of range",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        if (!m_is_active) {
            activate();
        }
        for (auto &control : m_active_control[control_idx]) {
            control->adjust(setting);
        }
        m_is_adjusted[control_idx] = true;
    }

    double MSRIOGroup::read_signal(const std::string &signal_name, int domain_type, int domain_idx)
    {
        if (!m_is_fixed_enabled) {
            enable_fixed_counters();
        }
        auto ncsm_it = m_name_cpu_signal_map.find(signal_name);
        if (ncsm_it == m_name_cpu_signal_map.end()) {
            throw Exception("MSRIOGroup::read_signal(): signal name \"" +
                            signal_name + "\" not found",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        if (domain_type != signal_domain_type(signal_name)) {
            throw Exception("MSRIOGroup::read_signal(): domain_type requested does not match the domain of the signal.",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        if (domain_idx < 0 || domain_idx >= m_platform_topo.num_domain(domain_type)) {
            throw Exception("MSRIOGroup::read_signal(): domain_idx out of range",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        std::set<int> cpu_idx = m_platform_topo.domain_nested(GEOPM_DOMAIN_CPU,
                                                              domain_type, domain_idx);

        // Copy of existing signal but map own memory
        uint64_t field = 0;
        std::unique_ptr<MSRSignal> signal = ncsm_it->second[*(cpu_idx.begin())]->copy_and_remap(&field);
        uint64_t offset = signal->offset();
        field = m_msrio->read_msr(*(cpu_idx.begin()), offset);
        // @todo last value can only get updated with read batch. This means that
        // multiple calls to read_signal for a 64-bit counter will return 0
        // unless read_batch is called for those counters.
        // Alternative it to update last_value here, but that would mean
        // multiple calls to sample() could return different values if interleaved
        // with a call to read_signal().
        return signal->sample();
    }

    void MSRIOGroup::write_control(const std::string &control_name, int domain_type, int domain_idx, double setting)
    {
        check_control(control_name);
        auto nccm_it = m_name_cpu_control_map.find(control_name);
        if (nccm_it == m_name_cpu_control_map.end()) {
            throw Exception("MSRIOGroup::write_control(): control name \"" +
                            control_name + "\" not found",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        if (domain_type != control_domain_type(control_name)) {
            throw Exception("MSRIOGroup::write_control(): domain_type does not match the domain of the control.",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        if (domain_idx < 0 || domain_idx >= m_platform_topo.num_domain(domain_type)) {
            throw Exception("MSRIOGroup::write_control(): domain_idx out of range",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }

        if (control_name == "POWER_PACKAGE_LIMIT") {
            write_control("MSR::PKG_POWER_LIMIT:PL1_LIMIT_ENABLE", domain_type, domain_idx, 1.0);
        }

        std::set<int> cpu_idx = m_platform_topo.domain_nested(GEOPM_DOMAIN_CPU,
                                                              domain_type, domain_idx);
        for (auto cpu : cpu_idx) {
            // Copy of existing control but map own memory
            uint64_t field = 0;
            uint64_t mask = 0;
            std::unique_ptr<MSRControl> control = nccm_it->second[cpu]->copy_and_remap(&field, &mask);
            uint64_t offset = control->offset();
            control->adjust(setting);
            m_msrio->write_msr(cpu, offset, field, mask);
        }
    }

    void MSRIOGroup::save_control(void)
    {
        for (const auto &pair_it : m_name_cpu_control_map) {
            bool do_skip = false;
            for (const auto &ctl_ptr : pair_it.second) {
                try {
                    auto it = m_per_cpu_restore[ctl_ptr->cpu_idx()].find(ctl_ptr->offset());
                    if (it == m_per_cpu_restore[ctl_ptr->cpu_idx()].end()) {
                        struct m_restore_s restore {.value = m_msrio->read_msr(ctl_ptr->cpu_idx(),
                                                                               ctl_ptr->offset()),
                                                    .mask = ctl_ptr->mask()};
                        m_per_cpu_restore[ctl_ptr->cpu_idx()].emplace(ctl_ptr->offset(), restore);
                    }
                    else {
                        it->second.mask |= ctl_ptr->mask();
                    }
                }
                catch (const Exception &e) {
                    if (!do_skip) {
                        std::cerr << "Warning: " << e.what() << std::endl;
                    }
                    do_skip = true;
                }
            }
        }
        for (auto &map_it : m_per_cpu_restore) {
            for (auto &pair_it : map_it) {
                pair_it.second.value &= pair_it.second.mask;
            }
        }
    }

    void MSRIOGroup::restore_control(void)
    {
        int cpu_idx = 0;
        for (const auto &map_it : m_per_cpu_restore) {
            for (const auto &pair_it : map_it) {
                try {
                    m_msrio->write_msr(cpu_idx, pair_it.first,
                                       pair_it.second.value,
                                       pair_it.second.mask);
                }
                catch (const Exception &e) {
                    std::cerr << "Warning: " << e.what() << std::endl;
                }
            }
            ++cpu_idx;
        }
    }

    std::string MSRIOGroup::msr_whitelist(void) const
    {
        return msr_whitelist(m_cpuid);
    }

    std::string MSRIOGroup::msr_whitelist(int cpuid) const
    {
        std::ostringstream whitelist;
        whitelist << "# MSR        Write Mask           # Comment" << std::endl;
        whitelist << std::setfill('0') << std::hex;
        for (const auto &msr : m_msr_arr) {
            std::string msr_name = msr->name();
            uint64_t msr_offset = msr->offset();
            size_t num_signals = msr->num_signal();
            size_t num_controls = msr->num_control();
            uint64_t write_mask = 0;

            if (!num_signals && !num_controls) {
                throw Exception("MSRIOGroup::msr_whitelist(): invalid msr",
                                GEOPM_ERROR_INVALID, __FILE__, __LINE__);
            }

            if (num_controls) {
                for (size_t cidx = 0; cidx < num_controls; cidx++) {
                    uint64_t idx_field = 0, idx_mask = 0;
                    msr->control(cidx, 1, idx_field, idx_mask);
                    write_mask |= idx_mask;
                }
            }
            whitelist << "0x" << std::setw(8) << msr_offset << "   0x" << std::setw(16) << write_mask << "   # \"" << msr_name << "\"" << std::endl;
        }
        return whitelist.str();
    }

    int MSRIOGroup::cpuid(void) const
    {
        uint32_t key = 1; //processor features
        uint32_t proc_info = 0;
        uint32_t model;
        uint32_t family;
        uint32_t ext_model;
        uint32_t ext_family;
        uint32_t ebx, ecx, edx;
        const uint32_t model_mask = 0xF0;
        const uint32_t family_mask = 0xF00;
        const uint32_t extended_model_mask = 0xF0000;
        const uint32_t extended_family_mask = 0xFF00000;

        __get_cpuid(key, &proc_info, &ebx, &ecx, &edx);

        model = (proc_info & model_mask) >> 4;
        family = (proc_info & family_mask) >> 8;
        ext_model = (proc_info & extended_model_mask) >> 16;
        ext_family = (proc_info & extended_family_mask) >> 20;

        if (family == 6) {
            model+=(ext_model << 4);
        }
        else if (family == 15) {
            model+=(ext_model << 4);
            family+=ext_family;
        }

        return ((family << 8) + model);
    }

    void MSRIOGroup::activate(void)
    {
        m_msrio->config_batch(m_read_cpu_idx, m_read_offset,
                              m_write_cpu_idx, m_write_offset, m_write_mask);
        m_read_field.resize(m_read_cpu_idx.size());
        m_write_field.resize(m_write_cpu_idx.size());
        size_t msr_idx = 0;
        for (auto &msr_sig : m_active_signal) {
            const uint64_t *field_ptr = &(m_read_field[msr_idx]);
            msr_sig->map_field(field_ptr);
            ++msr_idx;
        }
        msr_idx = 0;
        for (auto &control : m_active_control) {
            for (auto &msr_ctl : control) {
                uint64_t *field_ptr = &(m_write_field[msr_idx]);
                uint64_t *mask_ptr = &(m_write_mask[msr_idx]);
                msr_ctl->map_field(field_ptr, mask_ptr);
                ++msr_idx;
            }
        }
        m_is_active = true;
    }

    void MSRIOGroup::register_msr_signal(const std::string &msr_name)
    {
        register_msr_signal(msr_name, msr_name);
    }

    void MSRIOGroup::register_msr_signal(const std::string &signal_name, const std::string &msr_name_field)
    {
        Exception ex("MSRIOGroup::register_msr_signal(): msr_name_field must be of the form \"MSR::<msr_name>:<field_name>\"",
                     GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        if (msr_name_field.compare(0, m_name_prefix.size(), m_name_prefix) != 0) {
            throw ex;
        }
        std::string name_field = msr_name_field.substr(m_name_prefix.size());
        size_t colon_pos = name_field.find(':');
        if (colon_pos == std::string::npos) {
            throw ex;
        }
        std::string msr_name(name_field.substr(0, colon_pos));
        std::string field_name(name_field.substr(colon_pos + 1));

        // Insert the signal name with an empty vector into the map
        auto ins_ret = m_name_cpu_signal_map.insert(std::pair<std::string, std::vector<std::shared_ptr<MSRSignal> > >(signal_name, {}));
        // Get reference to the per-cpu signal vector
        auto &cpu_signal = (*(ins_ret.first)).second;
        // Check to see if the signal name has already been registered
        if (!ins_ret.second) {
            throw Exception("MSRIOGroup::register_msr_signal(): signal_name " + signal_name +
                            " was previously registered.",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        cpu_signal.resize(m_num_cpu, nullptr);

        auto name_msr_it = m_name_msr_map.find(msr_name);
        if (name_msr_it == m_name_msr_map.end()) {
            throw Exception("MSRIOGroup::register_msr_signal(): msr_name could not be found: " + msr_name,
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        const MSR &msr_obj = name_msr_it->second;
        int signal_idx = msr_obj.signal_index(field_name);
        if (signal_idx == -1) {
            throw Exception("MSRIOGroup::register_msr_signal(): field_name: " + field_name + " could not be found",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }

        for (int cpu_idx = 0; cpu_idx < m_num_cpu; ++cpu_idx) {
            cpu_signal[cpu_idx] = std::make_shared<MSRSignalImp>(msr_obj, msr_obj.domain_type(), cpu_idx, signal_idx);
        }

        // Record the units in the map
        m_signal_units_map[signal_name] = msr_obj.units(signal_idx);

        // Set up aggregation for the alias
        auto func = agg_function(msr_name_field);
        m_func_map[signal_name] = func;
        // Copy description for the alias
        auto desc = signal_description(msr_name_field);
        if (signal_name != msr_name_field) {
            desc = "Alias for " + msr_name_field + ". " + desc;
        }
        m_signal_desc_map[signal_name] = desc;
    }

    void MSRIOGroup::register_msr_control(const std::string &control_name)
    {
        register_msr_control(control_name, control_name);
    }

    void MSRIOGroup::register_msr_control(const std::string &control_name, const std::string &msr_name_field)
    {
        Exception ex("MSRIOGroup::register_msr_control(): msr_name_field must be of the form \"MSR::<msr_name>:<field_name>\"",
                     GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        if (msr_name_field.compare(0, m_name_prefix.size(), m_name_prefix) != 0) {
            throw ex;
        }
        std::string name_field = msr_name_field.substr(m_name_prefix.size());
        size_t colon_pos = name_field.find(':');
        if (colon_pos == std::string::npos) {
            throw ex;
        }
        std::string msr_name(name_field.substr(0, colon_pos));
        std::string field_name(name_field.substr(colon_pos + 1));

        // Insert the control name with an empty vector into the map
        auto ins_ret = m_name_cpu_control_map.insert(std::pair<std::string, std::vector<std::shared_ptr<MSRControl> > >(control_name, {}));
        // Get reference to the per-cpu control vector
        auto &cpu_control = (*(ins_ret.first)).second;
        // Check to see if the control name has already been registered
        if (!ins_ret.second) {
            throw Exception("MSRIOGroup::register_msr_control(): control_name " + control_name +
                            " was previously registered.",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);

        }
        cpu_control.resize(m_num_cpu, nullptr);

        auto name_msr_it = m_name_msr_map.find(msr_name);
        if (name_msr_it == m_name_msr_map.end()) {
            throw Exception("MSRIOGroup::register_msr_control(): msr_name could not be found",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        const MSR &msr_obj = name_msr_it->second;
        int control_idx = msr_obj.control_index(field_name);
        if (control_idx == -1) {
            throw Exception("MSRIOGroup::register_msr_control(): field_name: " + field_name + " could not be found",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }

        for (int cpu_idx = 0; cpu_idx < m_num_cpu; ++cpu_idx) {
            cpu_control[cpu_idx] = std::make_shared<MSRControlImp>(msr_obj, msr_obj.domain_type(), cpu_idx, control_idx);
        }
        // Copy description for the alias
        auto desc = control_description(msr_name_field);
        if (control_name != msr_name_field) {
            desc = "Alias for " + msr_name_field + ". " + desc;
        }
        m_control_desc_map[control_name] = desc;
    }

    std::string MSRIOGroup::plugin_name(void)
    {
        return GEOPM_MSR_IO_GROUP_PLUGIN_NAME;
    }

    std::unique_ptr<IOGroup> MSRIOGroup::make_plugin(void)
    {
        return geopm::make_unique<MSRIOGroup>();
    }

    void MSRIOGroup::enable_fixed_counters(void)
    {
        for (int cpu_idx = 0; cpu_idx < m_num_cpu; ++cpu_idx) {
            write_control("MSR::PERF_GLOBAL_CTRL:EN_FIXED_CTR0", GEOPM_DOMAIN_CPU, cpu_idx, 1.0);
            write_control("MSR::FIXED_CTR_CTRL:EN0_OS", GEOPM_DOMAIN_CPU, cpu_idx, 1.0);
            write_control("MSR::FIXED_CTR_CTRL:EN0_USR", GEOPM_DOMAIN_CPU, cpu_idx, 1.0);
            write_control("MSR::FIXED_CTR_CTRL:EN0_PMI", GEOPM_DOMAIN_CPU, cpu_idx, 0);

            write_control("MSR::PERF_GLOBAL_CTRL:EN_FIXED_CTR1", GEOPM_DOMAIN_CPU, cpu_idx, 1.0);
            write_control("MSR::FIXED_CTR_CTRL:EN1_OS", GEOPM_DOMAIN_CPU, cpu_idx, 1.0);
            write_control("MSR::FIXED_CTR_CTRL:EN1_USR", GEOPM_DOMAIN_CPU, cpu_idx, 1.0);
            write_control("MSR::FIXED_CTR_CTRL:EN1_PMI", GEOPM_DOMAIN_CPU, cpu_idx, 0);

            write_control("MSR::PERF_GLOBAL_CTRL:EN_FIXED_CTR2", GEOPM_DOMAIN_CPU, cpu_idx, 1.0);
            write_control("MSR::FIXED_CTR_CTRL:EN2_OS", GEOPM_DOMAIN_CPU, cpu_idx, 1.0);
            write_control("MSR::FIXED_CTR_CTRL:EN2_USR", GEOPM_DOMAIN_CPU, cpu_idx, 1.0);
            write_control("MSR::FIXED_CTR_CTRL:EN2_PMI", GEOPM_DOMAIN_CPU, cpu_idx, 0);

            write_control("MSR::PERF_GLOBAL_OVF_CTRL:CLEAR_OVF_FIXED_CTR0", GEOPM_DOMAIN_CPU, cpu_idx, 0);
            write_control("MSR::PERF_GLOBAL_OVF_CTRL:CLEAR_OVF_FIXED_CTR1", GEOPM_DOMAIN_CPU, cpu_idx, 0);
            write_control("MSR::PERF_GLOBAL_OVF_CTRL:CLEAR_OVF_FIXED_CTR2", GEOPM_DOMAIN_CPU, cpu_idx, 0);
        }
        m_is_fixed_enabled = true;
    }

    std::function<double(const std::vector<double> &)> MSRIOGroup::agg_function(const std::string &signal_name) const
    {
        if (!is_valid_signal(signal_name)) {
            throw Exception("MSRIOGroup::agg_function(): signal_name " + signal_name +
                            " not valid for MSRIOGroup",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        std::function<double(const std::vector<double> &)> result = Agg::select_first;
        auto it = m_func_map.find(signal_name);
        if (it != m_func_map.end()) {
            result = it->second;
        }
        return result;
    }

    std::function<std::string(double)> MSRIOGroup::format_function(const std::string &signal_name) const
    {
        if (!is_valid_signal(signal_name)) {
            throw Exception("MSRIOGroup::format_function(): signal_name " + signal_name +
                            " not valid for MSRIOGroup",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        std::function<std::string(double)> result = string_format_double;
        if (string_ends_with(signal_name, "#")) {
            result = string_format_raw64;
        }
        else {
            auto it = m_signal_units_map.find(signal_name);
            if (it != m_signal_units_map.end()) {
                int units = it->second;
                if (MSR::M_UNITS_NONE == units) {
                    result = string_format_integer;
                }
            }
#ifdef GEOPM_DEBUG
            else {
                throw Exception("MSRIOGroup::format_function(): signal valid but not found in map",
                                GEOPM_ERROR_LOGIC, __FILE__, __LINE__);
            }
#endif
        }
        return result;
    }

    std::string MSRIOGroup::signal_description(const std::string &signal_name) const
    {
        if (!is_valid_signal(signal_name)) {
            throw Exception("MSRIOGroup::signal_description(): signal_name " + signal_name +
                            " not valid for MSRIOGroup",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        std::string result = "Refer to the Intel(R) 64 and IA-32 Architectures Software Developer's Manual for information about this MSR";
        auto it = m_signal_desc_map.find(signal_name);
        if (it != m_signal_desc_map.end()) {
            result = it->second;
        }
        return result;
    }

    std::string MSRIOGroup::control_description(const std::string &control_name) const
    {
        if (!is_valid_control(control_name)) {
            throw Exception("MSRIOGroup::control_description(): control_name " + control_name +
                            " not valid for MSRIOGroup",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        std::string result = "Refer to the Intel(R) 64 and IA-32 Architectures Software Developer's Manual for information about this MSR";
        auto it = m_control_desc_map.find(control_name);
        if (it != m_control_desc_map.end()) {
            result = it->second;
        }
        return result;
    }

    std::vector<std::unique_ptr<MSR> > init_msr_arr(int cpu_id)
    {
        std::vector<std::unique_ptr<MSR> > msr_arr = MSRIOGroup::parse_json_msrs(arch_msr_json());
        // parse arch registers first, then platform specific
        std::vector<std::unique_ptr<MSR> > msr_arr_platform;
        switch (cpu_id) {
            case MSRIOGroup::M_CPUID_KNL:
                msr_arr_platform = MSRIOGroup::parse_json_msrs(knl_msr_json());
                break;
            case MSRIOGroup::M_CPUID_HSX:
            case MSRIOGroup::M_CPUID_BDX:
                msr_arr_platform = MSRIOGroup::parse_json_msrs(hsx_msr_json());
                break;
            case MSRIOGroup::M_CPUID_SNB:
            case MSRIOGroup::M_CPUID_IVT:
                msr_arr_platform = MSRIOGroup::parse_json_msrs(snb_msr_json());
                break;
            case MSRIOGroup::M_CPUID_SKX:
                msr_arr_platform = MSRIOGroup::parse_json_msrs(skx_msr_json());
                break;
            default:
                throw Exception("MSRIOGroup: Unsupported CPUID",
                                GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
        }
        // search path for additional json files to parse
        std::string env_plugin_path = environment().plugin_path();
        std::vector<std::string> plugin_paths {GEOPM_DEFAULT_PLUGIN_PATH};
        if (!env_plugin_path.empty()) {
            std::vector<std::string> dirs = string_split(env_plugin_path, ":");
            plugin_paths.insert(plugin_paths.end(), dirs.begin(), dirs.end());
        }
        std::vector<std::unique_ptr<MSR> > msr_arr_custom;
        for (const auto &dir : plugin_paths) {
            auto files = list_directory_files(dir);
            for (const auto &file : files) {
                std::string filename = dir + "/" + file;
                if (string_begins_with(file, "msr_") && string_ends_with(file, ".json")) {
                    std::string data = read_file(filename);
                    std::vector<std::unique_ptr<MSR> > temp = MSRIOGroup::parse_json_msrs(data);
                    msr_arr_custom.insert(msr_arr_custom.begin(),
                                          std::make_move_iterator(temp.begin()),
                                          std::make_move_iterator(temp.end()));
                }
            }
        }
        msr_arr.insert(msr_arr.end(),
                       std::make_move_iterator(msr_arr_platform.begin()),
                       std::make_move_iterator(msr_arr_platform.end()));
        msr_arr.insert(msr_arr.end(),
                       std::make_move_iterator(msr_arr_custom.begin()),
                       std::make_move_iterator(msr_arr_custom.end()));
        return msr_arr;
    }

    void MSRIOGroup::check_control(const std::string &control_name)
    {
        static const std::set<std::string> FREQ_CONTROL_SET {
            "POWER_PACKAGE_LIMIT",
            "MSR::PKG_POWER_LIMIT:PL1_POWER_LIMIT",
            "FREQUENCY",
            "MSR::PERF_CTL:FREQ"};
        static bool do_check_governor = true;

        if (do_check_governor &&
            FREQ_CONTROL_SET.find(control_name) != FREQ_CONTROL_SET.end())
        {
            bool do_print_warning = false;
            std::string scaling_driver = "cpufreq-sysfs-read-error";
            std::string scaling_governor = scaling_driver;

            // Check the CPU frequency driver
            try {
                scaling_driver = geopm::read_file("/sys/devices/system/cpu/cpu0/cpufreq/scaling_driver");
                size_t cr_pos = scaling_driver.find('\n');
                scaling_driver = scaling_driver.substr(0, cr_pos);
                if (scaling_driver != "acpi-cpufreq") {
                    do_print_warning = true;
                }
            }
            catch (...) {
                do_print_warning = true;
            }

            // Check the CPU frequency governor
            try {
                scaling_governor = geopm::read_file("/sys/devices/system/cpu/cpu0/cpufreq/scaling_governor");
                size_t cr_pos = scaling_governor.find('\n');
                scaling_governor = scaling_governor.substr(0, cr_pos);
                if (scaling_governor != "performance") {
                    do_print_warning = true;
                }
            }
            catch (...) {
                do_print_warning = true;
            }
            if (do_print_warning) {
                std::cerr << "Warning: <geopm> MSRIOGroup::" << std::string(__func__)
                          << "(): Incompatible CPU frequency driver/governor detected ("
                          << scaling_driver << "/" << scaling_governor << "). "
                          << "The \"acpi-cpufreq\" driver and \"performance\" governor are required when setting "
                          << "CPU frequency or power limit with GEOPM.  Other Linux power settings, including the intel_pstate driver,"
                          << "may overwrite GEOPM controls for frequency and power limits." << std::endl;
                    }
            do_check_governor = false;
        }

        static const std::set<std::string> POWER_CONTROL_SET {
            "POWER_PACKAGE_LIMIT",
            "MSR::PKG_POWER_LIMIT:PL1_POWER_LIMIT"};
        static bool do_check_rapl_lock = true;
        if (do_check_rapl_lock &&
            POWER_CONTROL_SET.find(control_name) != POWER_CONTROL_SET.end()) {

            int domain = signal_domain_type("MSR::PKG_POWER_LIMIT:LOCK");
            int num_domain = m_platform_topo.num_domain(domain);
            double lock = 0.0;
            for (int dom_idx = 0; dom_idx < num_domain; ++dom_idx) {
                lock += read_signal("MSR::PKG_POWER_LIMIT:LOCK", domain, dom_idx);
            }
            if (lock != 0.0) {
                throw Exception("MSRIOGroup::" + std::string(__func__) + "(): " +
                                "Unable to control power when RAPL lock bit is set. " +
                                "Check BIOS settings to ensure RAPL is enabled.",
                                GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
            }
            do_check_rapl_lock = false;
        }
    }

    /// Used to validate types and values of JSON objects
    struct json_checker
    {
        // base JSON type
        Json::Type json_type;
        // additional constraints, assuming base type matches
        std::function<bool(const Json &obj)> is_valid;
        // message to use if check fails
        std::string message;
    };

    void check_expected_key_values(const Json &root, std::map<std::string, json_checker> &key_check_map,
                                   const std::string &loc_message)
    {
        auto items = root.object_items();
        // check for extra keys
        for (const auto &obj : items) {
            if (key_check_map.find(obj.first) == key_check_map.end()) {
                throw Exception("MSRIOGroup::" + std::string(__func__) + "(): unexpected key \"" + obj.first + "\" found " + loc_message,
                                GEOPM_ERROR_INVALID, __FILE__, __LINE__);
            }
        }
        // check for required keys
        for (const auto &key_check : key_check_map) {
            std::string key = key_check.first;
            json_checker key_param = key_check.second;
            if (items.find(key) == items.end()) {
                throw Exception("MSRIOGroup::" + std::string(__func__) + "(): \"" + key + "\" key is required " + loc_message,
                                GEOPM_ERROR_INVALID, __FILE__, __LINE__);
            }
            Json obj = root[key];
            if (obj.type() != key_param.json_type || !key_param.is_valid(obj)) {
                throw Exception("MSRIOGroup::" + std::string(__func__) + "(): \"" + key + "\" " + key_param.message + " " + loc_message,
                                GEOPM_ERROR_INVALID, __FILE__, __LINE__);
            }
        }
    }

    std::vector<std::unique_ptr<MSR> > MSRIOGroup::parse_json_msrs(const std::string &str)
    {
        std::vector<std::unique_ptr<MSR> > result;
        std::string err;
        Json root = Json::parse(str, err);
        if (!err.empty() || !root.is_object()) {
            throw Exception("MSRIOGroup::" + std::string(__func__) + "(): detected a malformed json string: " + err,
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }

        /// Used to validate type and value of JSON objects
        auto null_func = [](const Json &obj) { return true; };
        auto is_hex_string = [](const Json &obj) {
            return (obj.string_value().find("0x") == 0);
        };
        auto is_valid_domain = [](const Json &domain) {
            try {
                PlatformTopo::domain_name_to_type(domain.string_value());
            }
            catch (const Exception &ex) {
                return false;
            }
            return true;
        };
        auto is_integer = [](const Json &num) {
            return ((double)((int)(num.number_value())) == num.number_value());
        };
        //// Validate top level keys
        /// @todo: Validate arch value
        /// @todo: Validate cpuid value
        std::map<std::string, json_checker> top_level_keys = {
            {"msrs", {Json::OBJECT, null_func, "must be an object"}}
        };
        check_expected_key_values(root, top_level_keys, "at top level");

        /// Parse MSRs list
        auto msr_obj = root["msrs"].object_items();
        for (const auto &msr : msr_obj) {
            std::string msr_name = msr.first;
            Json msr_root = msr.second;
            if (msr_root.type() != Json::OBJECT) {
                throw Exception("MSRIOGroup::" + std::string(__func__) + "(): data for msr \"" + msr_name + "\" must be an object",
                                GEOPM_ERROR_INVALID, __FILE__, __LINE__);
            }
            // expected keys for msr
            std::map<std::string, json_checker> msr_keys = {
                {"offset", {Json::STRING, is_hex_string, "must be a hex string"}},
                {"domain", {Json::STRING, is_valid_domain, "must be a valid domain string"}},
                {"fields", {Json::OBJECT, null_func, "must be an object"}}
            };
            check_expected_key_values(msr_root, msr_keys, "in msr \"" + msr_name + "\"");

            std::vector<std::pair<std::string, struct MSR::m_encode_s> > signals;
            std::vector<std::pair<std::string, struct MSR::m_encode_s> > controls;

            /// Validate fields within MSR
            auto msr_data = msr.second.object_items();
            auto fields_obj = msr_data["fields"].object_items();
            for (const auto &field : fields_obj) {
                std::string field_name = field.first;
                Json field_root = field.second;
                if (field_root.type() != Json::OBJECT) {
                    throw Exception("MSRIOGroup::" + std::string(__func__) + "(): \"" + field_name + "\" field within msr \"" + msr_name + "\" must be an object",
                                    GEOPM_ERROR_INVALID, __FILE__, __LINE__);
                }

                // expected keys for bitfield
                std::map<std::string, json_checker> field_checker {
                    {"begin_bit", {Json::NUMBER, is_integer, "must be an integer"}},
                    {"end_bit", {Json::NUMBER, is_integer, "must be an integer"}},
                    {"function", {Json::STRING, null_func, "must be a valid function string"}},
                    {"units", {Json::STRING, null_func, "must be a string"}},
                    {"scalar", {Json::NUMBER, null_func, "must be a number"}},
                    {"writeable", {Json::BOOL, null_func, "must be a bool"}}
                };
                check_expected_key_values(field_root, field_checker, "in \"" + msr_name + ":" + field_name + "\"");

                // field is valid, add to list
                auto field_data = field.second.object_items();
                MSR::m_encode_s param {
                    .begin_bit = (int)(field_data["begin_bit"].number_value()),
                    .end_bit = (int)(field_data["end_bit"].number_value()),
                    .domain = PlatformTopo::domain_name_to_type(msr_data["domain"].string_value()),
                    .function = MSR::string_to_function(field_data["function"].string_value()),
                    .units = MSR::string_to_units(field_data["units"].string_value()),
                    .scalar = field_data["scalar"].number_value(),
                };
                signals.push_back({field_name, param});
                if (field_data["writeable"].bool_value()) {
                    controls.push_back({field_name, param});
                }
            }
            uint64_t msr_offset = std::stoull(msr_data["offset"].string_value(), 0, 16);
            if (msr_offset == 0ull) {
                throw Exception("MSRIOGroup::" + std::string(__func__) + "(): invalid offset for " + msr_name + ": " + msr_data["offset"].string_value(),
                                GEOPM_ERROR_INVALID, __FILE__, __LINE__);
            }
            result.emplace_back(MSR::make_unique(msr_name, msr_offset, signals, controls));
        }

        return result;
    }
}
