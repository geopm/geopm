/*
 * Copyright (c) 2015 - 2023, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "config.h"

#include "geopm/MSRIOGroup.hpp"

#include <stdlib.h>
#include <cpuid.h>
#include <cmath>
#include <sstream>
#include <algorithm>
#include <utility>
#include <iomanip>
#include <iostream>
#include <set>

#include "geopm/json11.hpp"

#include "geopm_sched.h"
#include "geopm/Exception.hpp"
#include "geopm/Agg.hpp"
#include "MSRIOImp.hpp"
#include "MSR.hpp"
#include "Signal.hpp"
#include "RawMSRSignal.hpp"
#include "MSRFieldSignal.hpp"
#include "DifferenceSignal.hpp"
#include "TimeSignal.hpp"
#include "DerivativeSignal.hpp"
#include "RatioSignal.hpp"
#include "MultiplicationSignal.hpp"
#include "Control.hpp"
#include "MSRFieldControl.hpp"
#include "DomainControl.hpp"
#include "geopm/PlatformTopo.hpp"
#include "geopm/Helper.hpp"
#include "geopm_debug.hpp"
#include "SaveControl.hpp"

using json11::Json;

namespace geopm
{
    const std::string arch_msr_json(void);
    const std::string knl_msr_json(void);
    const std::string hsx_msr_json(void);
    const std::string snb_msr_json(void);
    const std::string skx_msr_json(void);

    const std::string MSRIOGroup::M_DEFAULT_DESCRIPTION =
        "Refer to the Intel(R) 64 and IA-32 Architectures Software Developer's "
        "Manual for information about this MSR";

    const std::string MSRIOGroup::M_PLUGIN_NAME = "MSR";
    const std::string MSRIOGroup::M_NAME_PREFIX = M_PLUGIN_NAME + "::";

    // Return true if turbo ratio limits are writable in all domains that
    // report writability.  Return false otherwise. In debug builds, print a
    // warning if there is mixed writability across domains.
    static bool is_trl_writable_in_all_domains(const Json &msr_json,
                                               const PlatformTopo &topo,
                                               std::shared_ptr<MSRIO> msrio)
    {
        bool is_writable = false;
        const auto &msr_obj = msr_json.object_items();
        auto platform_info_obj_it = msr_obj.find("PLATFORM_INFO");

        if (platform_info_obj_it != msr_obj.end()) {
            auto platform_info_obj = platform_info_obj_it->second.object_items();
            auto platform_info_offset = std::stoull(
                platform_info_obj["offset"].string_value(), 0, 16);
            auto domain_type = PlatformTopo::domain_name_to_type(
                platform_info_obj["domain"].string_value());
            auto trl_mode_it =
                platform_info_obj["fields"].object_items().find("PROGRAMMABLE_RATIO_LIMITS_TURBO_MODE");

            if (trl_mode_it != platform_info_obj.end()) {
                auto begin_bit = (int)(trl_mode_it->second["begin_bit"].number_value());
                auto end_bit = (int)(trl_mode_it->second["end_bit"].number_value());
                int function = MSR::string_to_function(
                    trl_mode_it->second["function"].string_value());
                double scalar = trl_mode_it->second["scalar"].number_value();

                int num_domain = topo.num_domain(domain_type);
                int num_domain_with_writable_trl = 0;
                for (int domain_idx = 0; domain_idx < num_domain; ++domain_idx) {
                    std::set<int> cpus = topo.domain_nested(
                        GEOPM_DOMAIN_CPU, domain_type, domain_idx);
                    int cpu_idx = *(cpus.begin());
                    auto platform_info_msr = std::make_shared<RawMSRSignal>(
                        msrio, cpu_idx, platform_info_offset);
                    auto trl_mode_signal = geopm::make_unique<MSRFieldSignal>(
                        platform_info_msr, begin_bit, end_bit, function, scalar);

                    num_domain_with_writable_trl += trl_mode_signal->read() != 0;
                }

                if (num_domain_with_writable_trl == num_domain) {
                    is_writable = true;
                }
                else if (num_domain_with_writable_trl != 0) {
#ifdef GEOPM_DEBUG
                    std::cerr
                        << "Warning: <geopm> " << num_domain_with_writable_trl
                        << " out of " << num_domain
                        << " entries for PROGRAMMABLE_RATIO_LIMITS_TURBO_MODE "
                           "indicate writable turbo ratio limits; defaulting "
                           "to no writable turbo ratio limits"
                        << std::endl;
#endif
                }
            }
        }

        return is_writable;
    }

    MSRIOGroup::MSRIOGroup()
        : MSRIOGroup(platform_topo(), std::make_shared<MSRIOImp>(), cpuid(), geopm_sched_num_cpu(), nullptr)
    {

    }

    MSRIOGroup::MSRIOGroup(const PlatformTopo &topo, std::shared_ptr<MSRIO> msrio, int cpuid, int num_cpu, std::shared_ptr<SaveControl> save_control)
        : m_platform_topo(topo)
        , m_msrio(std::move(msrio))
        , m_cpuid(cpuid)
        , m_num_cpu(num_cpu)
        , m_is_active(false)
        , m_is_read(false)
        , m_time_zero(std::make_shared<geopm_time_s>(time_zero()))
        , m_time_batch(std::make_shared<double>(NAN))
        , m_rdt_info(get_rdt_info())
        , m_pmc_bit_width(get_pmc_bit_width())
        , m_derivative_window(8)
        , m_sleep_time(0.005)  // 5000 us
        , m_mock_save_ctl(save_control)
    {
        // Load available signals and controls from files
        parse_json_msrs(arch_msr_json());
        try {
            // Try to extend list of MSRs if CPUID is recognized
            parse_json_msrs(platform_data(m_cpuid));
        }
        catch (const Exception &ex) {
            // Only load architectural MSRs
#ifdef GEOPM_DEBUG
            std::cerr << "Warning: <geopm> MSRIOGroup::MSRIOGroup(): "
                      << "Unable to load CPUID specific MSRs: "
                      << ex.what() << std::endl;
#endif
        }
        auto custom_files = msr_data_files();
        for (const auto &filename : custom_files) {
            std::string data = read_file(filename);
            parse_json_msrs(data);
        }

        // HWP enable is checked via an MSR, so we cannot do this as part of
        // the initializer if we want to use read_signal to determine capabilities
        m_is_hwp_enabled = get_hwp_enabled();
        // If HWP is not enabled, prune all related signals/controls
        if (!m_is_hwp_enabled) {
            auto all_signal_names = signal_names();
            for(const auto &name : all_signal_names) {
                if (string_begins_with(name, "MSR::HWP")) {
                    m_signal_available.erase(name);
                }
            }
            auto all_control_names = control_names();
            for(const auto &name : all_control_names) {
                if (string_begins_with(name, "MSR::HWP")) {
                    m_control_available.erase(name);
                }
            }
        }

        register_frequency_signals();
        register_frequency_controls();

        register_signal_alias("CPU_TIMESTAMP_COUNTER", "MSR::TIME_STAMP_COUNTER:TIMESTAMP_COUNT");

        register_signal_alias("CPU_ENERGY", "MSR::PKG_ENERGY_STATUS:ENERGY");
        register_signal_alias("DRAM_ENERGY", "MSR::DRAM_ENERGY_STATUS:ENERGY");
        register_signal_alias("CPU_CYCLES_THREAD", "MSR::APERF:ACOUNT");
        register_signal_alias("CPU_CYCLES_REFERENCE", "MSR::MPERF:MCOUNT");
        register_signal_alias("CPU_POWER_MIN_AVAIL", "MSR::PKG_POWER_INFO:MIN_POWER");
        register_signal_alias("CPU_POWER_MAX_AVAIL", "MSR::PKG_POWER_INFO:MAX_POWER");
        register_signal_alias("CPU_POWER_LIMIT_DEFAULT", "MSR::PKG_POWER_INFO:THERMAL_SPEC_POWER");
        register_signal_alias("CPU_POWER_LIMIT_CONTROL", "MSR::PKG_POWER_LIMIT:PL1_POWER_LIMIT");
        register_signal_alias("CPU_POWER_TIME_WINDOW", "MSR::PKG_POWER_LIMIT:PL1_TIME_WINDOW");

        // CNLIOGroup provides a CNL::BOARD_ENERGY & BOARD_ENERGY high level alias.
        // If it loads after the MSRIOGroup the BOARD_ENERGY alias below will be
        // overwritten, so both MSR::BOARD_ENERGY & BOARD_ENERGY are provided
        register_signal_alias("MSR::BOARD_ENERGY", "MSR::PLATFORM_ENERGY_STATUS:ENERGY");
        register_signal_alias("BOARD_ENERGY", "MSR::PLATFORM_ENERGY_STATUS:ENERGY");
        register_signal_alias("BOARD_POWER_LIMIT_CONTROL", "MSR::PLATFORM_POWER_LIMIT:PL1_POWER_LIMIT");
        register_signal_alias("BOARD_POWER_TIME_WINDOW_CONTROL", "MSR::PLATFORM_POWER_LIMIT:PL1_TIME_WINDOW");

        register_temperature_signals();
        register_power_signals();
        register_pcnt_scalability_signals();
        register_rdt_signals();

        register_control_alias("CPU_POWER_LIMIT_CONTROL", "MSR::PKG_POWER_LIMIT:PL1_POWER_LIMIT");
        register_control_alias("CPU_POWER_TIME_WINDOW", "MSR::PKG_POWER_LIMIT:PL1_TIME_WINDOW");
        register_control_alias("BOARD_POWER_LIMIT_CONTROL", "MSR::PLATFORM_POWER_LIMIT:PL1_POWER_LIMIT");
        register_control_alias("BOARD_POWER_TIME_WINDOW_CONTROL", "MSR::PLATFORM_POWER_LIMIT:PL1_TIME_WINDOW");
    }

    void MSRIOGroup::register_frequency_signals(void)
    {
        // HWP vs P-State signals
        if (m_is_hwp_enabled){
            register_signal_alias("CPU_FREQUENCY_STATUS", "MSR::PERF_STATUS:FREQ");

            register_signal_alias("CPU_FREQUENCY_MIN_CONTROL", "MSR::HWP_REQUEST:MINIMUM_PERFORMANCE");
            register_signal_alias("CPU_FREQUENCY_MAX_CONTROL", "MSR::HWP_REQUEST:MAXIMUM_PERFORMANCE");
            register_signal_alias("CPU_FREQUENCY_DESIRED_CONTROL", "MSR::HWP_REQUEST:DESIRED_PERFORMANCE");
        }
        else {
            register_signal_alias("CPU_FREQUENCY_STATUS", "MSR::PERF_STATUS:FREQ");
            register_signal_alias("CPU_FREQUENCY_MAX_CONTROL", "MSR::PERF_CTL:FREQ");
        }

        std::string max_turbo_name;
        if (m_is_hwp_enabled)
        {
            max_turbo_name = "MSR::HWP_CAPABILITIES:HIGHEST_PERFORMANCE";
        }
        else {
            if (m_cpuid == MSRIOGroup::M_CPUID_KNL) {
                max_turbo_name = "MSR::TURBO_RATIO_LIMIT:GROUP_0_MAX_RATIO_LIMIT";
            }
            else if (m_cpuid == MSRIOGroup::M_CPUID_SNB ||
                     m_cpuid == MSRIOGroup::M_CPUID_IVT ||
                     m_cpuid == MSRIOGroup::M_CPUID_HSX ||
                     m_cpuid == MSRIOGroup::M_CPUID_BDX) {
                max_turbo_name = "MSR::TURBO_RATIO_LIMIT:MAX_RATIO_LIMIT_1CORE";
            }
            else if (m_cpuid >= MSRIOGroup::M_CPUID_SKX) {
                max_turbo_name = "MSR::TURBO_RATIO_LIMIT:MAX_RATIO_LIMIT_0";
            }
        }

        if (max_turbo_name != "") {
            register_signal_alias("CPU_FREQUENCY_MAX_AVAIL", max_turbo_name);
            set_signal_description("CPU_FREQUENCY_MAX_AVAIL", "Maximum processor frequency.");
        }

        // Uncore signals
        register_signal_alias("CPU_UNCORE_FREQUENCY_STATUS", "MSR::UNCORE_PERF_STATUS:FREQ");
        register_signal_alias("CPU_UNCORE_FREQUENCY_MIN_CONTROL", "MSR::UNCORE_RATIO_LIMIT:MIN_RATIO");
        register_signal_alias("CPU_UNCORE_FREQUENCY_MAX_CONTROL", "MSR::UNCORE_RATIO_LIMIT:MAX_RATIO");

    }

    void MSRIOGroup::register_frequency_controls(void) {
        if (m_is_hwp_enabled) {
            register_control_alias("CPU_FREQUENCY_MIN_CONTROL", "MSR::HWP_REQUEST:MINIMUM_PERFORMANCE");
            register_control_alias("CPU_FREQUENCY_MAX_CONTROL", "MSR::HWP_REQUEST:MAXIMUM_PERFORMANCE");
            register_control_alias("CPU_FREQUENCY_DESIRED_CONTROL", "MSR::HWP_REQUEST:DESIRED_PERFORMANCE");
        }
        else {
            register_control_alias("CPU_FREQUENCY_MAX_CONTROL", "MSR::PERF_CTL:FREQ");
        }

        // Uncore controls
        register_control_alias("CPU_UNCORE_FREQUENCY_MIN_CONTROL", "MSR::UNCORE_RATIO_LIMIT:MIN_RATIO");
        register_control_alias("CPU_UNCORE_FREQUENCY_MAX_CONTROL", "MSR::UNCORE_RATIO_LIMIT:MAX_RATIO");

    }

    void MSRIOGroup::set_signal_description(const std::string &name,
                                            const std::string &description)
    {
        auto it = m_signal_available.find(name);
        if (it != m_signal_available.end()) {
            // Look for "alias_for".  Preserve it if present.
            auto pos = it->second.description.find("    alias_for");
            if(pos != std::string::npos) {
                it->second.description.erase(0, pos);
            }
            it->second.description.insert(0, description + '\n');
        }
    }

    void MSRIOGroup::set_control_description(const std::string &name,
                                             const std::string &description)
    {
        auto it = m_control_available.find(name);
        if (it != m_control_available.end()) {
            it->second.description = description;
        }
    }

    void MSRIOGroup::register_temperature_signals(void)
    {
        std::string max_name = "MSR::TEMPERATURE_TARGET:PROCHOT_MIN";
        auto max_it = m_signal_available.find(max_name);
        if (max_it == m_signal_available.end()) {
            throw Exception("MSRIOGroup::register_temperature_signals(): " +
                            max_name + " not found",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        int max_domain = max_it->second.domain;
        // mapping of high-level signal name to description and
        // underlying digital readout MSR
        struct temp_data
        {
            std::string temp_name;
            std::string description;
            std::string msr_name;
        };
        std::vector<temp_data> temp_signals {
            {"CPU_CORE_TEMPERATURE", "Core temperature", "MSR::THERM_STATUS:DIGITAL_READOUT"},
            {"CPU_PACKAGE_TEMPERATURE", "Package temperature", "MSR::PACKAGE_THERM_STATUS:DIGITAL_READOUT"}
        };
        for (const auto &ts : temp_signals) {
            std::string signal_name = ts.temp_name;
            std::string msr_name = ts.msr_name;
            auto read_it = m_signal_available.find(msr_name);
            if (max_it != m_signal_available.end() &&
                read_it != m_signal_available.end()) {
                auto readings = read_it->second.signals;
                int read_domain = read_it->second.domain;
                int num_domain = m_platform_topo.num_domain(read_domain);
                GEOPM_DEBUG_ASSERT(num_domain == (int)readings.size(),
                                   "size of domain for " + msr_name +
                                   " does not match number of signals available.");
                std::vector<std::shared_ptr<Signal> > result(num_domain);
                for (int domain_idx = 0; domain_idx < num_domain; ++domain_idx) {
                    int max_idx = *m_platform_topo.domain_nested(max_domain, read_domain, domain_idx).begin();
                    auto max = max_it->second.signals[max_idx];
                    auto sub = readings[domain_idx];
                    result[domain_idx] = std::make_shared<DifferenceSignal>(max, sub);
                }
                m_signal_available[signal_name] = {result,
                                                   read_domain,
                                                   IOGroup::M_UNITS_CELSIUS,
                                                   agg_function(msr_name),
                                                   ts.description +
                                                   "\n    alias_for: Temperature derived from PROCHOT and "
                                                   + ts.msr_name,
                                                   IOGroup::M_SIGNAL_BEHAVIOR_VARIABLE,
                                                   string_format_double};
            }
        }
    }

    void MSRIOGroup::register_power_signals(void)
    {
        // register time signal; domain board
        std::string time_name = "MSR::TIME";
        std::shared_ptr<Signal> time_sig = std::make_shared<TimeSignal>(m_time_zero, m_time_batch);
        m_signal_available[time_name] = {std::vector<std::shared_ptr<Signal> >({time_sig}),
                                         GEOPM_DOMAIN_BOARD,
                                         IOGroup::M_UNITS_SECONDS,
                                         Agg::select_first,
                                         "Time in seconds used to calculate power",
                                         IOGroup::M_SIGNAL_BEHAVIOR_MONOTONE,
                                         string_format_double};

        // Mapping of high-level signal name to description and
        // underlying energy MSR.  The domain will match that of the
        // energy signal.
        struct power_data
        {
            std::string power_name;
            std::string description;
            std::string msr_name;
        };
        std::vector<power_data> power_signals {
            {"CPU_POWER",
                    "Average package power over 40 ms or 8 control loop iterations",
                    "CPU_ENERGY"},
            {"DRAM_POWER",
                    "Average DRAM power over 40 ms or 8 control loop iterations",
                    "DRAM_ENERGY"},
            {"MSR::BOARD_POWER",
                    "Average BOARD power over 40 ms or 8 control loop iterations",
                    "MSR::PLATFORM_ENERGY_STATUS:ENERGY"}
        };
        for (const auto &ps : power_signals) {
            std::string signal_name = ps.power_name;
            std::string msr_name = ps.msr_name;
            auto read_it = m_signal_available.find(msr_name);
            if (read_it != m_signal_available.end()) {
                auto readings = read_it->second.signals;
                int energy_domain = read_it->second.domain;
                int num_domain = m_platform_topo.num_domain(energy_domain);
                GEOPM_DEBUG_ASSERT(num_domain == (int)readings.size(),
                                   "size of domain for " + msr_name +
                                   " does not match number of signals available.");
                std::vector<std::shared_ptr<Signal> > result(num_domain);
                for (int domain_idx = 0; domain_idx < num_domain; ++domain_idx) {
                    auto eng = readings[domain_idx];
                    result[domain_idx] =
                        std::make_shared<DerivativeSignal>(time_sig, eng,
                                                           m_derivative_window,
                                                           m_sleep_time);
                }
                m_signal_available[signal_name] = {result,
                                                   energy_domain,
                                                   IOGroup::M_UNITS_WATTS,
                                                   agg_function(msr_name),
                                                   ps.description + "\n    alias_for: " + ps.msr_name + " rate of change",
                                                   IOGroup::M_SIGNAL_BEHAVIOR_VARIABLE,
                                                   string_format_double};
            }
        }

        // CNLIOGroup provides a CNL::BOARD_POWER & BOARD_POWER high level alias.
        // If it loads after the MSRIOGroup the BOARD_POWER alias below will be
        // overwritten, so both MSR::BOARD_POWER & BOARD_POWER are provided
        register_signal_alias("BOARD_POWER", "MSR::BOARD_POWER");
    }

    void MSRIOGroup::register_pcnt_scalability_signals(void)
    {
        // Tracking for intermediate signals that will not
        // be exposed to the user but will be used to
        // generate user visible signals
        std::map<std::string, signal_info> signal_hidden;

        // MSR::TIME is a board level signal as defined in register_power_signals;
        auto time_it = m_signal_available.find("MSR::TIME");
        if (time_it == m_signal_available.end()) {
            throw Exception("MSRIOGroup::" + std::string(__func__) + "(): MSR::TIME signal unavailable",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        std::shared_ptr<Signal> time_sig = time_it->second.signals.at(0);

        int derivative_window = 8;
        double sleep_time = 0.005;  // 5000 us

        // Mapping of high-level signal name to description and
        // underlying CNT MSRs.  The domain will match that of the
        // CNT signals.
        struct cnt_data
        {
            std::string cnt_name;
            std::string description;
            std::string msr_name;
        };

        std::vector<cnt_data> cnt_signals {
            {"MSR::PCNT_RATE",
                    "Average cpu pcnt rate over 8 control loop iterations (40ms if using geopmread)",
                    "MSR::PPERF:PCNT"},
            {"MSR::ACNT_RATE",
                    "Average cpu acnt rate over 8 control loop iterations (40ms if using geopmread)",
                    "MSR::APERF:ACNT"}
        };

        // This block is taking the derivative of the MSR::PPERF:PCNT signal
        // and the MSR::APERF:ACNT signal.  Later the ratio of PCNT Rate over
        // ACNT Rate will be calculated to provide the CPU Scalability signal.
        // The ratio of delta PCNT over delta ACNT may be used instead, however
        // in testing this resulted in a noisy signal, so the derivative class
        // is used.
        for (const auto &ps : cnt_signals) {
            std::string signal_name = ps.cnt_name;
            std::string msr_name = ps.msr_name;
            auto read_it = m_signal_available.find(msr_name);
            if (read_it != m_signal_available.end()) {
                auto readings = read_it->second.signals;
                int cnt_domain = read_it->second.domain;
                int num_domain = m_platform_topo.num_domain(cnt_domain);
                GEOPM_DEBUG_ASSERT(num_domain == (int)readings.size(),
                                   "size of domain for " + msr_name +
                                   " does not match number of signals available.");
                std::vector<std::shared_ptr<Signal> > result(num_domain);
                for (int domain_idx = 0; domain_idx < num_domain; ++domain_idx) {
                    auto dt_cnt = readings[domain_idx];
                    // The derivative signal is being used here due to
                    // signal noise when sampled at faster cadences, similar
                    // to the energy signal.
                    result[domain_idx] =
                        std::make_shared<DerivativeSignal>(time_sig, dt_cnt,
                                                           derivative_window,
                                                           sleep_time);
                }

                // Store the PCNT_RATE and ACNT_RATE in a data structure that is not
                // m_signal_available so that the end user is not exposed to intermediary
                // signals
                signal_hidden[signal_name] = {result,
                                              cnt_domain,
                                              IOGroup::M_UNITS_HERTZ,
                                              Agg::average,
                                              ps.description + "\n    alias_for: " + ps.msr_name + " rate of change",
                                              IOGroup::M_SIGNAL_BEHAVIOR_VARIABLE,
                                              string_format_double};
            }
        }

        // This block provides the ratio of PCNT_RATE over ACNT_RATE.
        std::string signal_name = "MSR::CPU_SCALABILITY_RATIO";
        std::string msr_name = "MSR::PCNT_RATE";
        auto read_it = signal_hidden.find(msr_name);
        if (read_it != signal_hidden.end()) {
            auto readings = read_it->second.signals;
            int cnt_domain = read_it->second.domain;
            int num_domain = m_platform_topo.num_domain(cnt_domain);
            GEOPM_DEBUG_ASSERT(num_domain == (int)readings.size(),
                               "size of domain for " + msr_name +
                               " does not match number of signals available.");
            std::vector<std::shared_ptr<Signal> > result(num_domain);
            for (int domain_idx = 0; domain_idx < num_domain; ++domain_idx) {
                auto numer_it = signal_hidden.find("MSR::PCNT_RATE");
                auto denom_it = signal_hidden.find("MSR::ACNT_RATE");
                if (numer_it != signal_hidden.end() &&
                    denom_it != signal_hidden.end()){
                    auto numerators = numer_it->second.signals;
                    auto numerator = numerators[domain_idx];
                    auto denominators = denom_it->second.signals;
                    auto denominator = denominators[domain_idx];

                    result[domain_idx] =
                        std::make_shared<RatioSignal>(numerator, denominator);
                }
            }

            m_signal_available[signal_name] = {result,
                                               cnt_domain,
                                               IOGroup::M_UNITS_NONE,
                                               Agg::average,
                                               "Measure of CPU Scalability as determined "
                                               "by the derivative of PCNT divided by the "
                                               "derivative of ACNT over 8 samples",
                                               IOGroup::M_SIGNAL_BEHAVIOR_VARIABLE,
                                               string_format_double};
        }
    }

    void MSRIOGroup::register_rdt_signals(void)
    {
        // It may be necessary to wrap the initial rdt() call in this check
        int domain = signal_domain_type("MSR::MISC_ENABLE:LIMIT_CPUID_MAXVAL");
        int num_domain = m_platform_topo.num_domain(domain);
        bool disable = false;
        for (int dom_idx = 0; dom_idx < num_domain; ++dom_idx) {
            disable |= (bool)read_signal("MSR::MISC_ENABLE:LIMIT_CPUID_MAXVAL", domain, dom_idx);
        }
        if (disable || !m_rdt_info.rdt_support) {
            return;
        }

        std::string msr_name = "MSR::QM_CTR:RM_DATA";
        std::string signal_name = "MSR::QM_CTR_SCALED";
        std::string description = "Resource Monitor Data converted to bytes";

        auto read_it = m_signal_available.find(msr_name);
        if (read_it != m_signal_available.end()) {
            auto readings = read_it->second.signals;
            int ctr_domain = read_it->second.domain;
            num_domain = m_platform_topo.num_domain(ctr_domain);
            GEOPM_DEBUG_ASSERT(num_domain == (int)readings.size(),
                               "size of domain for " + msr_name +
                               " does not match number of signals available.");
            std::vector<std::shared_ptr<Signal> > result(num_domain);
            for (int domain_idx = 0; domain_idx < num_domain; ++domain_idx) {
                auto ctr = readings[domain_idx];
                result[domain_idx] =
                    std::make_shared<MultiplicationSignal>(ctr, (double)m_rdt_info.mbm_scalar);
            }
            m_signal_available[signal_name] = {result,
                                               ctr_domain,
                                               IOGroup::M_UNITS_NONE,
                                               agg_function(msr_name),
                                               description + "\n    alias_for: " + msr_name + " multiplied by " +
                                               std::to_string(m_rdt_info.mbm_scalar) + " (provided by cpuid)",
                                               IOGroup::M_SIGNAL_BEHAVIOR_VARIABLE,
                                               string_format_double};
        }


        // MSR::TIME is a board level signal as defined in register_power_signals;
        auto time_it = m_signal_available.find("MSR::TIME");
        if (time_it == m_signal_available.end()) {
            throw Exception("MSRIOGroup::" + std::string(__func__) + "(): MSR::TIME signal unavailable",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        std::shared_ptr<Signal> time_sig = time_it->second.signals.at(0);

        msr_name = "MSR::QM_CTR_SCALED";
        signal_name = "MSR::QM_CTR_SCALED_RATE";
        description = "Resource Monitor Data converted to bytes/second";
        read_it = m_signal_available.find(msr_name);
        if (read_it != m_signal_available.end()) {
            auto readings = read_it->second.signals;
            int ctr_domain = read_it->second.domain;
            num_domain = m_platform_topo.num_domain(ctr_domain);
            GEOPM_DEBUG_ASSERT(num_domain == (int)readings.size(),
                               "size of domain for " + msr_name +
                               " does not match number of signals available.");
            std::vector<std::shared_ptr<Signal> > result(num_domain);
            for (int domain_idx = 0; domain_idx < num_domain; ++domain_idx) {
                auto ctr = readings[domain_idx];
                result[domain_idx] =
                    std::make_shared<DerivativeSignal>(time_sig, ctr,
                                                       m_derivative_window,
                                                       m_sleep_time);
            }
            m_signal_available[signal_name] = {result,
                                               ctr_domain,
                                               IOGroup::M_UNITS_NONE,
                                               agg_function(msr_name),
                                               description + "\n    alias_for: " + msr_name + " rate of change",
                                               IOGroup::M_SIGNAL_BEHAVIOR_VARIABLE,
                                               string_format_double};
        }
    }


    std::set<std::string> MSRIOGroup::signal_names(void) const
    {
        std::set<std::string> result;
        for (const auto &sv : m_signal_available) {
            result.insert(sv.first);
        }
        return result;
    }

    std::set<std::string> MSRIOGroup::control_names(void) const
    {
        std::set<std::string> result;
        for (const auto &sv : m_control_available) {
            result.insert(sv.first);
        }
        return result;
    }

    bool MSRIOGroup::is_valid_signal(const std::string &signal_name) const
    {
        return m_signal_available.find(signal_name) != m_signal_available.end();
    }

    bool MSRIOGroup::is_valid_control(const std::string &control_name) const
    {
        return m_control_available.find(control_name) != m_control_available.end();
    }

    int MSRIOGroup::signal_domain_type(const std::string &signal_name) const
    {
        int result = GEOPM_DOMAIN_INVALID;
        auto it = m_signal_available.find(signal_name);
        if (it != m_signal_available.end()) {
            result = it->second.domain;
        }
        return result;
    }

    int MSRIOGroup::control_domain_type(const std::string &control_name) const
    {
        int result = GEOPM_DOMAIN_INVALID;
        auto it = m_control_available.find(control_name);
        if (it != m_control_available.end()) {
            result = it->second.domain;
        }
        return result;
    }

    int MSRIOGroup::push_signal(const std::string &signal_name, int domain_type, int domain_idx)
    {
        if (m_is_active) {
            throw Exception("MSRIOGroup::push_signal(): cannot push a signal after read_batch() or adjust() has been called.",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        if (!is_valid_signal(signal_name)) {
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
        int result = -1;
        bool is_found = false;

        GEOPM_DEBUG_ASSERT(m_signal_available.at(signal_name).signals.size() == (size_t)m_platform_topo.num_domain(domain_type),
                           "Signal " + signal_name + " not correctly scoped to number of domains.");
        std::shared_ptr<Signal> signal = m_signal_available.at(signal_name).signals[domain_idx];
        // Check if signal was already pushed
        for (size_t ii = 0; !is_found && ii < m_signal_pushed.size(); ++ii) {
            GEOPM_DEBUG_ASSERT(m_signal_pushed[ii] != nullptr,
                               "NULL Signal pointer was saved in active signals");
            // same location means this signal or its alias was already pushed
            if (m_signal_pushed[ii].get() == signal.get()) {
                result = ii;
                is_found = true;
            }
        }
        if (!is_found) {
            // If not pushed, add to pushed signals and configure for batch reads
            result = m_signal_pushed.size();
            m_signal_pushed.push_back(signal);
            signal->setup_batch();
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

        if (!is_valid_control(control_name)) {
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
        int result = -1;
        bool is_found = false;
        GEOPM_DEBUG_ASSERT(m_control_available.at(control_name).controls.size() == (size_t)m_platform_topo.num_domain(domain_type),
                           "Control " + control_name + " not correctly scoped to number of domains.");
        std::shared_ptr<Control> control = m_control_available.at(control_name).controls[domain_idx];
        // Check if control was already pushed
        for (size_t ii = 0; !is_found && ii < m_control_pushed.size(); ++ii) {
            GEOPM_DEBUG_ASSERT(m_control_pushed[ii] != nullptr,
                               "NULL Control pointer was saved in active controls");
            if (m_control_pushed[ii].get() == control.get()) {
                result = ii;
                is_found = true;
            }
        }
        if (control_name == "CPU_POWER_LIMIT_CONTROL") {
            write_control("MSR::PKG_POWER_LIMIT:PL1_LIMIT_ENABLE", domain_type, domain_idx, 1.0);
        }
        else if (control_name == "BOARD_POWER_LIMIT_CONTROL") {
            write_control("MSR::PLATFORM_POWER_LIMIT:PL1_LIMIT_ENABLE", domain_type, domain_idx, 1.0);
        }

        if (!is_found) {
            result = m_control_pushed.size();
            m_control_pushed.push_back(control);
            control->setup_batch();
            m_is_adjusted.push_back(false);
        }
        return result;
    }

    void MSRIOGroup::read_batch(void)
    {
        if (m_signal_pushed.size() != 0) {
            m_msrio->read_batch();
        }
        // update timesignal value
        *m_time_batch = geopm_time_since(m_time_zero.get());

        m_is_read = true;
        m_is_active = true;
    }

    void MSRIOGroup::write_batch(void)
    {
        if (m_control_pushed.size() != 0) {
            if (std::any_of(m_is_adjusted.begin(), m_is_adjusted.end(), [](bool it) {return !it;})) {
                throw Exception("MSRIOGroup::write_batch() called before all controls were adjusted",
                                GEOPM_ERROR_INVALID, __FILE__, __LINE__);
            }
            m_msrio->write_batch();
        }
        m_is_active = true;
    }

    double MSRIOGroup::sample(int signal_idx)
    {
        if (signal_idx < 0 || signal_idx >= (int)m_signal_pushed.size()) {
            throw Exception("MSRIOGroup::sample(): signal_idx out of range",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }

        if (!m_is_read) {
            throw Exception("MSRIOGroup::sample() called before signal was read.",
                            GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
        }
        return m_signal_pushed[signal_idx]->sample();
    }

    void MSRIOGroup::adjust(int control_idx, double setting)
    {
        if (control_idx < 0 || (unsigned)control_idx >= m_control_pushed.size()) {
            throw Exception("MSRIOGroup::adjust(): control_idx out of range",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        m_control_pushed[control_idx]->adjust(setting);
        m_is_adjusted[control_idx] = true;
    }

    double MSRIOGroup::read_signal(const std::string &signal_name, int domain_type, int domain_idx)
    {
        if (!is_valid_signal(signal_name)) {
            throw Exception("MSRIOGroup::read_signal(): signal name \"" +
                            signal_name + "\" not found",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        if (domain_type != signal_domain_type(signal_name)) {
            throw Exception("MSRIOGroup::read_signal(): domain_type requested does not match the domain of the signal (" + signal_name + ").",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        if (domain_idx < 0 || domain_idx >= m_platform_topo.num_domain(domain_type)) {
            throw Exception("MSRIOGroup::read_signal(): domain_idx out of range",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        std::shared_ptr<Signal> ref_sig = m_signal_available.at(signal_name).signals[domain_idx];
        return ref_sig->read();
    }

    void MSRIOGroup::write_control(const std::string &control_name, int domain_type, int domain_idx, double setting)
    {
        check_control(control_name);

        if (!is_valid_control(control_name)) {
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

        if (control_name == "CPU_POWER_LIMIT_CONTROL") {
            write_control("MSR::PKG_POWER_LIMIT:PL1_LIMIT_ENABLE", domain_type, domain_idx, 1.0);
        }
        else if (control_name == "BOARD_POWER_LIMIT_CONTROL") {
            write_control("MSR::PLATFORM_POWER_LIMIT:PL1_LIMIT_ENABLE", domain_type, domain_idx, 1.0);
        }
        std::shared_ptr<Control> control = m_control_available.at(control_name).controls[domain_idx];
        control->write(setting);
    }

    void MSRIOGroup::save_control(void)
    {
        std::vector<std::string> unallowed_controls;
        for (auto &ctl : m_control_available) {
            try {
                for (auto &dom_ctl : ctl.second.controls) {
                    dom_ctl->save();
                    dom_ctl->restore();
                }
            }
            catch (const Exception &) {
                unallowed_controls.push_back(ctl.first);
            }
        }
        for (auto &ctl : unallowed_controls) {
            m_control_available.erase(ctl);
        }
    }

    void MSRIOGroup::restore_control(void)
    {
        for (auto &ctl : m_control_available) {
            for (auto &dom_ctl : ctl.second.controls) {
                dom_ctl->restore();
            }
        }
    }

    int MSRIOGroup::cpuid(void)
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

    bool MSRIOGroup::get_hwp_enabled(void)
    {
        uint32_t leaf = 6; //thermal and power management features
        uint32_t eax, ebx, ecx, edx;
        const uint32_t hwp_mask = 0x80;
        bool supported, enabled = false;
        __get_cpuid(leaf, &eax, &ebx, &ecx, &edx);

        supported = (bool)((eax & hwp_mask) >> 7);

        if (supported) {
            std::string signal_name = "MSR::PM_ENABLE:HWP_ENABLE";
            int domain = signal_domain_type(signal_name);
            int num_domain = m_platform_topo.num_domain(domain);
            double pkg_enable = 0.0;
            for (int dom_idx = 0; dom_idx < num_domain; ++dom_idx) {
                try {
                    pkg_enable += read_signal(signal_name, domain, dom_idx);
                }
                catch (const geopm::Exception &ex) {
                    if (ex.err_value() != GEOPM_ERROR_MSR_READ) {
                        throw;
                    }
                    break;
                }
            }
            if (pkg_enable == 1.0 * num_domain) {
                enabled = true;
            }
            else {
#ifdef GEOPM_DEBUG
                std::cerr << "Warning: <geopm> MSRIOGroup::" << std::string(__func__)
                          << "(): Intel Hardware Performance states are not supported.  "
                          << "Using legacy P-States for signal and control aliases." << std::endl;
#endif
            }
        }

        return enabled;
    }

    MSRIOGroup::rdt_info MSRIOGroup::get_rdt_info(void)
    {
        uint32_t leaf, subleaf = 0;
        uint32_t eax, ebx, ecx, edx = 0;
        bool supported = false;
        uint32_t max, scale = 0;

        leaf = 0x0F;
        subleaf = 0;

        __cpuid_count(leaf, subleaf, eax, ebx, ecx, edx);
        supported = (bool)((edx >> 1) & 1);
        max = ebx;

        if (supported == true) {
            subleaf = 1;
            eax = 0;
            ebx = 0;
            ecx = 0;
            edx = 0;
            __cpuid_count(leaf, subleaf, eax, ebx, ecx, edx);
            scale = ebx;
        }

        rdt_info rdt = {
            .rdt_support = supported,
            .rmid_bit_width = (uint32_t)ceil(log2((double)max + 1)),
            .mbm_scalar = scale
        };

        return rdt;
    }

    uint32_t MSRIOGroup::get_pmc_bit_width(void)
    {
        uint32_t leaf, subleaf = 0;
        uint32_t eax, ebx, ecx, edx = 0;

        leaf = 0x0A;
        subleaf = 0;

        __cpuid_count(leaf, subleaf, eax, ebx, ecx, edx);

        // SDM vol 3b, section 18 specifies where to find how many PMC bits are
        // available
        return (eax >> 16) & 0xff;
    }

    void MSRIOGroup::register_signal_alias(const std::string &signal_name,
                                           const std::string &msr_name_field)
    {
        if (m_signal_available.find(signal_name) != m_signal_available.end()) {
            throw Exception("MSRIOGroup::register_signal_alias(): signal_name " + signal_name +
                            " was previously registered.",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        auto it = m_signal_available.find(msr_name_field);
        if (it == m_signal_available.end()) {
            // skip adding an alias if underlying signal is not found
            return;
        }
        // copy signal info but append to description
        m_signal_available[signal_name] = it->second;
        m_signal_available[signal_name].description =
            m_signal_available[msr_name_field].description + '\n' + "    alias_for: " + msr_name_field;
    }

    void MSRIOGroup::register_control_alias(const std::string &control_name,
                                            const std::string &msr_name_field)
    {
        if (m_control_available.find(control_name) != m_control_available.end()) {
            throw Exception("MSRIOGroup::register_control_alias(): control_name " + control_name +
                            " was previously registered.",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        auto it = m_control_available.find(msr_name_field);
        if (it == m_control_available.end()) {
            // skip adding an alias if underlying control is not found
            return;
        }
        // copy control info but append to description
        m_control_available[control_name] = it->second;
        m_control_available[control_name].description =
            m_control_available[msr_name_field].description + '\n' + "    alias_for: " + msr_name_field;
    }

    std::string MSRIOGroup::plugin_name(void)
    {
        return M_PLUGIN_NAME;
    }

    std::string MSRIOGroup::name(void) const
    {
        return plugin_name();
    }

    std::unique_ptr<IOGroup> MSRIOGroup::make_plugin(void)
    {
        return geopm::make_unique<MSRIOGroup>();
    }

    std::function<double(const std::vector<double> &)> MSRIOGroup::agg_function(const std::string &signal_name) const
    {
        if (!is_valid_signal(signal_name)) {
            throw Exception("MSRIOGroup::agg_function(): signal_name " + signal_name +
                            " not valid for MSRIOGroup",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        return m_signal_available.at(signal_name).agg_function;
    }

    std::function<std::string(double)> MSRIOGroup::format_function(const std::string &signal_name) const
    {
        if (!is_valid_signal(signal_name)) {
            throw Exception("MSRIOGroup::format_function(): signal_name " + signal_name +
                            " not valid for MSRIOGroup",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }

        std::function<std::string(double)> result = string_format_double;
        auto it = m_signal_available.find(signal_name);
        if (it != m_signal_available.end()) {
            result = it->second.format_function;
        }
#ifdef GEOPM_DEBUG
        else {
            throw Exception("MSRIOGroup::format_function(): signal valid but not found in map",
                            GEOPM_ERROR_LOGIC, __FILE__, __LINE__);
        }
#endif
        return result;
    }

    std::string MSRIOGroup::signal_description(const std::string &signal_name) const
    {
        if (!is_valid_signal(signal_name)) {
            throw Exception("MSRIOGroup::signal_description(): signal_name " + signal_name +
                            " not valid for MSRIOGroup",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        std::string result = "Invalid signal description: no description found.";
        auto it = m_signal_available.find(signal_name);
        if (it != m_signal_available.end()) {
            result =  "    description: " + it->second.description + '\n'; // Includes alias_for if applicable
            result += "    units: " + IOGroup::units_to_string(it->second.units) + '\n';
            result += "    aggregation: " + Agg::function_to_name(it->second.agg_function) + '\n';
            result += "    domain: " + m_platform_topo.domain_type_to_name(it->second.domain) + '\n';
            result += "    iogroup: MSRIOGroup";
        }
#ifdef GEOPM_DEBUG
        else {
            throw Exception("MSRIOGroup::signal_description(): signal valid but not found in map",
                            GEOPM_ERROR_LOGIC, __FILE__, __LINE__);
        }
#endif
        return result;
    }

    std::string MSRIOGroup::control_description(const std::string &control_name) const
    {
        if (!is_valid_control(control_name)) {
            throw Exception("MSRIOGroup::control_description(): control_name " + control_name +
                            " not valid for MSRIOGroup",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        std::string result = "Invalid control description: no description found.";
        auto it = m_control_available.find(control_name);
        if (it != m_control_available.end()) {
            result =  "    description: " + it->second.description + '\n'; // Includes alias_for if applicable
            result += "    units: " + IOGroup::units_to_string(it->second.units) + '\n';
            result += "    domain: " + m_platform_topo.domain_type_to_name(it->second.domain) + '\n';
            result += "    iogroup: MSRIOGroup";
        }
#ifdef GEOPM_DEBUG
        else {
            throw Exception("MSRIOGroup::control_description(): control valid but not found in map",
                            GEOPM_ERROR_LOGIC, __FILE__, __LINE__);
        }
#endif
        return result;
    }

    int MSRIOGroup::signal_behavior(const std::string &signal_name) const
    {
        if (!is_valid_signal(signal_name)) {
            throw Exception("MSRIOGroup::signal_behavior(): signal_name " + signal_name +
                            " not valid for MSRIOGroup",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        int result = -1;
        auto it = m_signal_available.find(signal_name);
        if (it != m_signal_available.end()) {
            result = it->second.behavior;
        }
#ifdef GEOPM_DEBUG
        else {
            throw Exception("MSRIOGroup::signal_behavior(): signal valid but not found in map",
                            GEOPM_ERROR_LOGIC, __FILE__, __LINE__);
        }
#endif
        return result;
    }

    void MSRIOGroup::save_control(const std::string &save_path)
    {
        std::shared_ptr<SaveControl> save_ctl = m_mock_save_ctl;
        if (save_ctl == nullptr) {
            save_ctl = SaveControl::make_unique(*this);
        }
        save_ctl->write_json(save_path);
    }

    void MSRIOGroup::restore_control(const std::string &save_path)
    {
        std::shared_ptr<SaveControl> save_ctl = m_mock_save_ctl;
        if (save_ctl == nullptr) {
            save_ctl = SaveControl::make_unique(geopm::read_file(save_path));
        }
        save_ctl->restore(*this);
    }

    std::string MSRIOGroup::platform_data(int cpu_id)
    {
        std::string platform_msrs;
        if (cpu_id == MSRIOGroup::M_CPUID_KNL) {
            platform_msrs = knl_msr_json();
        }
        else if (cpu_id == MSRIOGroup::M_CPUID_HSX ||
                 cpu_id == MSRIOGroup::M_CPUID_BDX) {
            platform_msrs = hsx_msr_json();
        }
        else if (cpu_id == MSRIOGroup::M_CPUID_SNB ||
                 cpu_id == MSRIOGroup::M_CPUID_IVT) {
            platform_msrs = snb_msr_json();
        }
        else if (cpu_id == MSRIOGroup::M_CPUID_SKX ||
                 cpu_id == MSRIOGroup::M_CPUID_ICX) {
            platform_msrs = skx_msr_json();
        }
        else if (cpu_id >= MSRIOGroup::M_CPUID_ICX) {
#ifdef GEOPM_DEBUG
            std::cerr << "Warning: <geopm> New/Untested CPUID detected; Defaulting to SKX MSRs"
                      << std::endl;
#endif
            platform_msrs = skx_msr_json();
        }
        else {
            throw Exception("MSRIOGroup: Unsupported CPUID",
                            GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
        }
        return platform_msrs;
    }

    std::set<std::string> MSRIOGroup::msr_data_files(void)
    {
        std::set<std::string> data_files;
        // search path for additional json files to parse
        std::string env_plugin_path(geopm::get_env("GEOPM_PLUGIN_PATH"));
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
                    data_files.insert(filename);
                }
            }
        }
        return data_files;
    }

    void MSRIOGroup::check_control(const std::string &control_name)
    {
        static const std::set<std::string> FREQ_CONTROL_SET {
            "CPU_POWER_LIMIT_CONTROL",
            "MSR::PKG_POWER_LIMIT:PL1_POWER_LIMIT",
            "CPU_FREQUENCY_MAX_CONTROL",
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
                if (scaling_governor != "performance" && scaling_governor != "userspace") {
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
                          << "The \"acpi-cpufreq\" driver and \"performance\" or \"userspace\" governor are required when setting "
                          << "CPU frequency or power limits with GEOPM.  Other Linux power settings, including the intel_pstate driver,"
                          << "may overwrite GEOPM controls for frequency and power limits." << std::endl;
                    }
            do_check_governor = false;
        }

        static const std::set<std::string> POWER_CONTROL_SET {
            "CPU_POWER_LIMIT_CONTROL",
            "MSR::PKG_POWER_LIMIT:PL1_POWER_LIMIT"};
        static bool do_check_rapl_lock = true;
        static std::string rapl_error = "Unable to control power when RAPL lock bit is set."
                                        "  Check BIOS settings to ensure RAPL is enabled.";

        if (do_check_rapl_lock &&
            POWER_CONTROL_SET.find(control_name) != POWER_CONTROL_SET.end()) {
            check_control_lock("MSR::PKG_POWER_LIMIT:LOCK",
                               rapl_error);

            do_check_rapl_lock = false;
        }

        static const std::set<std::string> PLATFORM_POWER_CONTROL_SET {
            "BOARD_POWER_LIMIT_CONTROL",
            "MSR::PLATFORM_POWER_LIMIT:PL1_POWER_LIMIT"};
        static bool do_check_platform_rapl_lock = true;
        static std::string platform_rapl_error = "Unable to control platform power when PLATFORM RAPL lock bit is set."
                                                 "  Check BIOS settings to ensure RAPL is enabled.";

        if (do_check_platform_rapl_lock &&
            PLATFORM_POWER_CONTROL_SET.find(control_name) != PLATFORM_POWER_CONTROL_SET.end()) {
            check_control_lock("MSR::PLATFORM_POWER_LIMIT:LOCK",
                               platform_rapl_error);

            do_check_platform_rapl_lock = false;
        }
    }

    void MSRIOGroup::check_control_lock(const std::string &lock_name, const std::string &error) {
        bool lock = false;
        int domain = signal_domain_type(lock_name);
        int num_domain = m_platform_topo.num_domain(domain);
        for (int dom_idx = 0; dom_idx < num_domain; ++dom_idx) {
            lock |= (bool)read_signal(lock_name, domain, dom_idx);
        }
        if (lock) {
            throw Exception("MSRIOGroup::" + std::string(__func__) + "(): " +
                            error, GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
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

    void check_expected_key_values(const Json &root,
                                   const std::map<std::string, json_checker> &required_key_map,
                                   const std::map<std::string, json_checker> &optional_key_map,
                                   const std::string &loc_message)
    {
        auto items = root.object_items();
        // check for extra keys
        for (const auto &obj : items) {
            if (required_key_map.find(obj.first) == required_key_map.end() &&
                optional_key_map.find(obj.first) == optional_key_map.end()) {
                throw Exception("MSRIOGroup::" + std::string(__func__) + "(): unexpected key \"" + obj.first + "\" found " + loc_message,
                                GEOPM_ERROR_INVALID, __FILE__, __LINE__);
            }
        }
        // check for required keys
        for (const auto &key_check : required_key_map) {
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
        // check optional keys
        for (const auto &key_check : optional_key_map) {
            std::string key = key_check.first;
            json_checker key_param = key_check.second;
            if (items.find(key) != items.end()) {
                Json obj = root[key];
                if (obj.type() != key_param.json_type || !key_param.is_valid(obj)) {
                    throw Exception("MSRIOGroup::" + std::string(__func__) + "(): \"" + key + "\" " + key_param.message + " " + loc_message,
                                    GEOPM_ERROR_INVALID, __FILE__, __LINE__);
                }
            }
        }
    }

    bool MSRIOGroup::json_check_null_func(const Json &obj)
    {
        return true;
    };

    bool MSRIOGroup::json_check_is_hex_string(const Json &obj)
    {
        return (obj.string_value().find("0x") == 0);
    };

    bool MSRIOGroup::json_check_is_valid_offset(const Json &obj)
    {
        return json_check_is_hex_string(obj) &&
               std::stoull(obj.string_value(), 0, 16) != 0ull;
    };

    bool MSRIOGroup::json_check_is_valid_domain(const Json &domain)
    {
        try {
            PlatformTopo::domain_name_to_type(domain.string_value());
        }
        catch (const Exception &ex) {
            return false;
        }
        return true;
    };

    bool MSRIOGroup::json_check_is_integer(const Json &num) {
        return ((double)((int)(num.number_value())) == num.number_value());
    };

    bool MSRIOGroup::json_check_is_valid_aggregation(const Json &obj)
    {
        try {
            Agg::name_to_function(obj.string_value());
        }
        catch (const Exception &ex) {
            return false;
        }
        return true;
    };

    void MSRIOGroup::check_top_level(const Json &root)
    {
        std::map<std::string, json_checker> top_level_keys = {
            {"msrs", {Json::OBJECT, json_check_null_func, "must be an object"}}
        };
        check_expected_key_values(root, top_level_keys, {}, "at top level");
    }

    void MSRIOGroup::check_msr_root(const Json &msr_root, const std::string &msr_name)
    {
        if (msr_root.type() != Json::OBJECT) {
            throw Exception("MSRIOGroup::" + std::string(__func__) + "(): data for msr \"" + msr_name + "\" must be an object",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        // expected keys for msr
        std::map<std::string, json_checker> msr_keys = {
                {"offset", {Json::STRING, json_check_is_valid_offset, "must be a hex string and non-zero"}},
                {"domain", {Json::STRING, json_check_is_valid_domain, "must be a valid domain string"}},
                {"fields", {Json::OBJECT, json_check_null_func, "must be an object"}}
        };
        check_expected_key_values(msr_root, msr_keys, {}, "in msr \"" + msr_name + "\"");
    }

    void MSRIOGroup::check_msr_field(const Json &msr_field,
                                     const std::string &msr_name,
                                     const std::string &field_name)
    {
        if (msr_field.type() != Json::OBJECT) {
            throw Exception("MSRIOGroup::" + std::string(__func__) + "(): \"" + field_name + "\" field within msr \"" + msr_name + "\" must be an object",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }

        // expected keys for bitfield
        std::map<std::string, json_checker> field_checker {
            {"begin_bit", {Json::NUMBER, json_check_is_integer, "must be an integer"}},
            {"end_bit", {Json::NUMBER, json_check_is_integer, "must be an integer"}},
            {"function", {Json::STRING, json_check_null_func, "must be a valid function string"}},
            {"units", {Json::STRING, json_check_null_func, "must be a string"}},
            {"scalar", {Json::NUMBER, json_check_null_func, "must be a number"}},
            {"writeable", {Json::BOOL, json_check_null_func, "must be a bool"}},
            {"behavior", {Json::STRING, json_check_null_func, "must be a valid behavior string"}},
            {"aggregation", {Json::STRING, json_check_is_valid_aggregation, "must be a valid aggregation function name"}}
        };
        std::map<std::string, json_checker> optional_field_checker {
            {"description", {Json::STRING, json_check_null_func, "must be a string"}}
        };
        check_expected_key_values(msr_field, field_checker, optional_field_checker,
                                  "in \"" + msr_name + ":" + field_name + "\"");
    }

    void MSRIOGroup::add_raw_msr_signal(const std::string &msr_name, int domain_type,
                                        uint64_t msr_offset)
    {
        std::string raw_msr_signal_name = M_NAME_PREFIX + msr_name + "#";
        int num_domain = m_platform_topo.num_domain(domain_type);
#ifdef GEOPM_DEBUG
        if (num_domain == 0) {
            std::cerr << "Warning: <geopm> no components in domain for MSR "
                      << msr_name << "; signals will not be available" << std::endl;
        }
#endif
        std::vector<std::shared_ptr<Signal> > result;
        for (int domain_idx = 0; domain_idx < num_domain; ++domain_idx) {
            // get index of a single representative CPU for this domain
            std::set<int> cpus = m_platform_topo.domain_nested(GEOPM_DOMAIN_CPU,
                                                               domain_type, domain_idx);
            int cpu_idx = *(cpus.begin());
            std::shared_ptr<Signal> raw_msr =
                std::make_shared<RawMSRSignal>(m_msrio, cpu_idx, msr_offset);
            result.push_back(raw_msr);
        }
        m_signal_available[raw_msr_signal_name] = {
            .signals = result,
            .domain = domain_type,
            .units = IOGroup::M_UNITS_NONE,
            .agg_function = Agg::select_first,
            .description = M_DEFAULT_DESCRIPTION,
            .behavior = IOGroup::M_SIGNAL_BEHAVIOR_LABEL,
            .format_function = string_format_raw64,
        };
    }

    void MSRIOGroup::add_msr_field_signal(const std::string &msr_name,
                                          const std::string &msr_field_name,
                                          int domain_type,
                                          int begin_bit, int end_bit,
                                          int function, double scalar, int units,
                                          const std::string &agg_function,
                                          const std::string &description,
                                          int behavior,
                                          const std::function<std::string(double)> &format_function)
    {
        std::string raw_msr_signal_name = M_NAME_PREFIX + msr_name + "#";
        int num_domain = m_platform_topo.num_domain(domain_type);
        std::vector<std::shared_ptr<Signal> > result_field_signal;
        for (int domain_idx = 0; domain_idx < num_domain; ++domain_idx) {
            auto raw_msr = m_signal_available.at(raw_msr_signal_name).signals[domain_idx];
            std::shared_ptr<Signal> field_signal =
                geopm::make_unique<MSRFieldSignal>(raw_msr, begin_bit, end_bit,
                                                   function, scalar);
            result_field_signal.push_back(field_signal);
        }
        m_signal_available[msr_field_name] = {
            .signals = result_field_signal,
            .domain = domain_type,
            .units = units,
            .agg_function = Agg::name_to_function(agg_function),
            .description = description,
            .behavior = behavior,
            .format_function = format_function,
        };
    }

    void MSRIOGroup::add_msr_field_control(const std::string &msr_field_name,
                                           int domain_type,
                                           uint64_t msr_offset,
                                           int begin_bit, int end_bit,
                                           int function, double scalar, int units,
                                           const std::string &description)
    {
        int num_domain = m_platform_topo.num_domain(domain_type);
        std::vector<std::shared_ptr<Control> > result_field_control;
        for (int domain_idx = 0; domain_idx < num_domain; ++domain_idx) {
            std::vector<std::shared_ptr<Control> > cpu_controls;
            std::set<int> cpus = m_platform_topo.domain_nested(GEOPM_DOMAIN_CPU,
                                                               domain_type, domain_idx);
            for (auto cpu_idx : cpus) {
                cpu_controls.push_back(std::make_shared<MSRFieldControl>(
                    m_msrio, cpu_idx, msr_offset,
                    begin_bit, end_bit, function,
                    scalar));
            }
            result_field_control.push_back(std::make_shared<DomainControl>(cpu_controls));
        }
        m_control_available[msr_field_name] = {
            .controls = result_field_control,
            .domain = domain_type,
            .units = units,
            .description = description,
        };
    }


    void MSRIOGroup::parse_json_msrs(const std::string &str)
    {
        std::string err;
        Json root = Json::parse(str, err);
        if (!err.empty() || !root.is_object()) {
            throw Exception("MSRIOGroup::" + std::string(__func__) + "(): detected a malformed json string: " + err,
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }

        check_top_level(root);

        auto msr_obj = root["msrs"].object_items();
        bool is_trl_writable = false;
        try {
            is_trl_writable = is_trl_writable_in_all_domains(root["msrs"], m_platform_topo, m_msrio);
        }
        catch (const Exception &ex) {
#ifdef GEOPM_DEBUG
            std::cerr << "Warning: <geopm> MSRIOGroup::" << std::string(__func__)
                      << "(): Unable to check TRL via PLATFORM_INFO: "
                      << ex.what() << std::endl;
#endif
        }

        for (const auto &msr : msr_obj) {
            std::string msr_name = msr.first;
            Json msr_root = msr.second;
            check_msr_root(msr_root, msr_name);

            auto msr_data = msr_root.object_items();
            uint64_t msr_offset = std::stoull(msr_data["offset"].string_value(), 0, 16);
            int domain_type = PlatformTopo::domain_name_to_type(msr_data["domain"].string_value());

            add_raw_msr_signal(msr_name, domain_type, msr_offset);

            /// Validate fields within MSR
            auto fields_obj = msr_data["fields"].object_items();
            for (const auto &field : fields_obj) {
                std::string field_name = field.first;
                Json field_root = field.second;
                std::string msr_field_name = msr_name + ":" + field_name;
                std::string sig_ctl_name = M_NAME_PREFIX + msr_field_name;

                check_msr_field(field_root, msr_name, field_name);

                // field is valid, add to list
                auto field_data = field.second.object_items();
                int begin_bit = (int)(field_data["begin_bit"].number_value());
                int end_bit = (int)(field_data["end_bit"].number_value());
                int function = MSR::string_to_function(field_data["function"].string_value());
                double scalar = field_data["scalar"].number_value();
                int units = IOGroup::string_to_units(field_data["units"].string_value());
                bool is_control = field_data["writeable"].bool_value();
                int behavior = IOGroup::string_to_behavior(field_data["behavior"].string_value());
                std::string agg_function = field_data["aggregation"].string_value();
                // optional fields
                std::string description = M_DEFAULT_DESCRIPTION;
                if (field_data.find("description") != field_data.end()) {
                    description = field_data["description"].string_value();
                }

                if (m_rdt_info.rdt_support && (msr_field_name == "QM_EVTSEL:RMID" || msr_field_name == "PQR_ASSOC:RMID")) {
                    if((int)m_rdt_info.rmid_bit_width > (end_bit - begin_bit + 1)) {
                        std::ostringstream except;
                        except << "MSRIOGroup::" << __func__ << "(): CPUID RMID bit width "
                               << m_rdt_info.rmid_bit_width << " is greater than the MSR provided RMID bit width "
                               << (end_bit - begin_bit + 1);
                        throw Exception(except.str(), GEOPM_ERROR_INVALID, __FILE__,
                                        __LINE__);
                    }
                }

                std::function<std::string(double)> format_function = string_format_double;
                if (IOGroup::M_UNITS_NONE == units) {
                    format_function = string_format_integer;
                }

                if (string_begins_with(msr_field_name, "IA32_PMC") &&
                    string_ends_with(msr_field_name, ":PERFCTR")) {
                    if (m_pmc_bit_width > 0) {
                        end_bit = begin_bit + m_pmc_bit_width - 1;
                    }
#ifdef GEOPM_DEBUG
                    else {
                        std::cerr << "Warning: <geopm> CPUID specified 0 bits for "
                                  << msr_field_name << "; using the default width: "
                                  << (end_bit - begin_bit + 1) << std::endl;
                    }
#endif
                }

                if (is_trl_writable &&
                    string_begins_with(msr_field_name,
                                       "TURBO_RATIO_LIMIT:MAX_RATIO_LIMIT_")) {
                    is_control = true;
                    behavior = IOGroup::M_SIGNAL_BEHAVIOR_VARIABLE;
                }

                add_msr_field_signal(msr_name, sig_ctl_name, domain_type,
                                     begin_bit, end_bit, function, scalar, units,
                                     agg_function, description, behavior, format_function);
                if (is_control) {
                    add_msr_field_control(sig_ctl_name, domain_type, msr_offset,
                                          begin_bit, end_bit, function, scalar, units,
                                          description);
                }
            }
        }
    }

    void MSRIOGroup::parse_json_msrs_allowlist(const std::string &str,
                                               std::map<uint64_t, std::pair<uint64_t, std::string> > &allowlist_data)
    {
        std::string err;
        Json root = Json::parse(str, err);
        if (!err.empty() || !root.is_object()) {
            throw Exception("MSRIOGroup::" + std::string(__func__) + "(): detected a malformed json string: " + err,
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }

        check_top_level(root);

        auto msr_obj = root["msrs"].object_items();
        for (const auto &msr : msr_obj) {
            std::string msr_name = msr.first;
            Json msr_root = msr.second;

            check_msr_root(msr_root, msr_name);

            auto msr_data = msr.second.object_items();
            uint64_t msr_offset = std::stoull(msr_data["offset"].string_value(), 0, 16);
            auto fields_obj = msr_data["fields"].object_items();
            uint64_t combined_write_mask = 0;
            for (const auto &field : fields_obj) {
                std::string field_name = field.first;
                Json field_root = field.second;
                std::string msr_field_name = msr_name + ":" + field_name;

                check_msr_field(field_root, msr_name, field_name);

                auto field_data = field.second.object_items();
                int begin_bit = (int)(field_data["begin_bit"].number_value());
                int end_bit = (int)(field_data["end_bit"].number_value());
                bool is_control = field_data["writeable"].bool_value();
                if (is_control) {
                    combined_write_mask |= (((1ULL << (end_bit - begin_bit + 1)) - 1) << begin_bit);
                }
            }
            allowlist_data[msr_offset] =
                std::pair<uint64_t, std::string>(combined_write_mask, msr_name);
        }
    }

    std::string MSRIOGroup::format_allowlist(const std::map<uint64_t, std::pair<uint64_t, std::string> > &allowlist_data)
    {
        std::map<uint64_t, std::string> offset_result_map;
        for (const auto &kv : allowlist_data) {
            uint64_t msr_offset = kv.first;
            uint64_t write_mask = kv.second.first;
            std::string msr_name = kv.second.second;
            std::ostringstream line;
            line << std::setfill('0') << std::hex
                 << "0x" << std::setw(8) << msr_offset << "   "
                 << "0x" << std::setw(16) << write_mask << "   # "
                 <<  "\"" << msr_name << "\"\n";
            offset_result_map[msr_offset] = line.str();
        }
        std::ostringstream result;
        result << "# MSR        Write Mask           # Comment\n";
        for (const auto &it : offset_result_map) {
            result << it.second;
        }
        return result.str();
    }

    std::string MSRIOGroup::msr_allowlist(int cpuid)
    {
        std::map<uint64_t, std::pair<uint64_t, std::string> > allowlist_data;
        parse_json_msrs_allowlist(arch_msr_json(), allowlist_data);
        try {
            parse_json_msrs_allowlist(platform_data(cpuid), allowlist_data);
        }
        catch (const Exception &ex) {
            // Write only architectural MSRs
        }
        auto custom = msr_data_files();
        for (const auto &filename : custom) {
            parse_json_msrs_allowlist(read_file(filename), allowlist_data);
        }
        return format_allowlist(allowlist_data);
    }

}
