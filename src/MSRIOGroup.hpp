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

#ifndef MSRIOGROUP_HPP_INCLUDE
#define MSRIOGROUP_HPP_INCLUDE

#include <vector>
#include <map>
#include <memory>
#include <functional>

#include "IOGroup.hpp"
#include "geopm_time.h"

namespace geopm
{
    class MSR;
    class MSRSignal;
    class MSRControl;
    class MSRIO;
    class PlatformTopo;
    class Signal;

    /// @brief IOGroup that provides signals and controls based on MSRs.
    class MSRIOGroup : public IOGroup
    {
        public:
            enum m_cpuid_e {
                M_CPUID_SNB = 0x62D,
                M_CPUID_IVT = 0x63E,
                M_CPUID_HSX = 0x63F,
                M_CPUID_BDX = 0x64F,
                M_CPUID_KNL = 0x657,
                M_CPUID_SKX = 0x655,
            };

            MSRIOGroup();
            MSRIOGroup(const PlatformTopo &platform_topo, std::shared_ptr<MSRIO> msrio, int cpuid, int num_cpu);
            virtual ~MSRIOGroup() = default;
            std::set<std::string> signal_names(void) const override;
            std::set<std::string> control_names(void) const override;
            bool is_valid_signal(const std::string &signal_name) const override;
            bool is_valid_control(const std::string &control_name) const override;
            int signal_domain_type(const std::string &signal_name) const override;
            int control_domain_type(const std::string &control_name) const override;
            int push_signal(const std::string &signal_name,
                            int domain_type,
                            int domain_idx) override;
            int push_control(const std::string &control_name,
                             int domain_type,
                             int domain_idx) override;
            void read_batch(void) override;
            void write_batch(void) override;
            double sample(int sample_idx) override;
            void adjust(int control_idx,
                        double setting) override;
            double read_signal(const std::string &signal_name,
                               int domain_type,
                               int domain_idx) override;
            void write_control(const std::string &control_name,
                               int domain_type,
                               int domain_idx,
                               double setting) override;
            void save_control(void) override;
            void restore_control(void) override;
            std::function<double(const std::vector<double> &)> agg_function(const std::string &signal_name) const override;
            std::function<std::string(double)> format_function(const std::string &signal_name) const override;
            std::string signal_description(const std::string &signal_name) const override;
            std::string control_description(const std::string &control_name) const override;
            /// @brief Fill string with the msr-safe whitelist file contents
            ///        reflecting all known MSRs for the current platform.
            /// @return String formatted to be written to
            ///        an msr-safe whitelist file.
            std::string msr_whitelist(void) const;
            /// @brief Fill string with the msr-safe whitelist file
            ///        contents reflecting all known MSRs for the
            ///        specified platform.
            /// @param cpuid [in] The CPUID of the platform.
            /// @return String formatted to be written to an msr-safe
            ///         whitelist file.
            static std::string msr_whitelist(int cpuid);
            /// @brief Get the cpuid of the current platform.
            static int cpuid(void);
            /// @brief Register a single MSR field as a control. This
            ///        is called by init_msr().
            /// @param [in] signal_name Compound control name of form
            ///        "msr_name:field_name" where msr_name is the
            ///        name of the MSR and the field_name is the name
            ///        of the control field held in the MSR.
            void register_msr_control(const std::string &control_name);
            /// @brief Parse a JSON string and add any raw MSRs and
            ///        fields as available signals.
            void parse_json_msrs_signal(const std::string &str);
            static std::string plugin_name(void);
            static std::unique_ptr<IOGroup> make_plugin(void);
            static std::vector<std::unique_ptr<MSR> > parse_json_msrs(const std::string &str);
        private:
            /// @brief Loads architectural MSRs from the JSON file,
            ///        MSRs for the given cpuid, and user-defined MSRs
            ///        if found in the plugin path.
            void init_msrs_signal(int cpu_id);
            /// @brief Override the default aggregation function for a
            ///        signal.  If signal is not available, does
            ///        nothing.
            void set_agg_function(const std::string &name,
                                  std::function<double(const std::vector<double> &)> agg_function);
            /// @brief Override the default description for a signal.
            ///        If signal is not available, does nothing.
            void set_description(const std::string &name,
                                 const std::string &description);
            /// @brief Add support for an alias of a signal by name.
            void register_signal_alias(const std::string &signal_name, const std::string &msr_field_name);
            /// @brief Add support for temperature combined signals if underlying
            ///        signals are available.
            void register_temperature_signals(void);
            /// @brief Add support for power combined signals if underlying
            ///        signals are available.
            void register_power_signals(void);

            // All available signals: map from name to signal_info.
            // The signals vector is over the indices for the domain.
            // The signals pointers should be copied when signal is
            // pushed and used directly for read_signal.
            struct signal_info
            {
                std::vector<std::shared_ptr<Signal> > signals;
                int domain;
                int units;
                std::function<double(const std::vector<double> &)> agg_function;
                std::string description;
            };
            std::map<std::string, signal_info> m_signal_available;

            // Mapping of signal index to pushed signals.
            std::vector<std::shared_ptr<Signal> > m_signal_pushed;

            // time for derivative signals
            std::shared_ptr<geopm_time_s> m_time_zero;
            std::shared_ptr<double> m_time_batch;

            static const std::string M_DEFAULT_DESCRIPTION;

            ///////////////////////////////////
            struct m_restore_s {
                uint64_t value;
                uint64_t mask;
            };

            void register_msr_control(const std::string &control_name, const std::string &msr_field_name);
            void enable_fixed_counters(void);
            void check_control(const std::string &control_name);
            static std::string msr_whitelist(const std::vector<std::unique_ptr<MSR> > &msr_arr);

            /// @brief Configure memory for all pushed signals and controls.
            void activate(void);
            const PlatformTopo &m_platform_topo;
            int m_num_cpu;
            bool m_is_active;
            bool m_is_read;
            std::shared_ptr<MSRIO> m_msrio;
            int m_cpuid;
            std::vector<bool> m_is_adjusted;
            std::vector<std::unique_ptr<MSR> > m_msr_arr;
            // Mappings from names to all valid controls
            std::map<std::string, const MSR &> m_name_msr_map;
            std::map<std::string, std::vector<std::shared_ptr<MSRControl> > > m_name_cpu_control_map;
            // Pushed controls only
            std::vector<std::vector<std::shared_ptr<MSRControl> > > m_active_control;
            // Vectors are over MSRs for all active controls
            std::vector<uint64_t> m_write_field;
            std::vector<int> m_write_cpu_idx;
            std::vector<uint64_t> m_write_offset;
            std::vector<uint64_t> m_write_mask;
            const std::string m_name_prefix;
            std::vector<std::map<uint64_t, m_restore_s> > m_per_cpu_restore;
            bool m_is_fixed_enabled;
            std::map<std::string, std::string> m_control_desc_map;
    };
}

#endif
