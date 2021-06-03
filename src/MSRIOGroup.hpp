/*
 * Copyright (c) 2015 - 2021, Intel Corporation
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

#include "contrib/json11/json11.hpp"

#include "IOGroup.hpp"
#include "geopm_time.h"

namespace geopm
{
    class MSRIO;
    class PlatformTopo;
    class Signal;
    class Control;

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
                M_CPUID_ICX = 0x66A,
            };

            MSRIOGroup();
            MSRIOGroup(const PlatformTopo &platform_topo,
                       std::shared_ptr<MSRIO> msrio,
                       int cpuid,
                       int num_cpu);
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
            int signal_behavior(const std::string &signal_name) const override;

            /// @brief Parse a JSON string and add any raw MSRs and
            ///        fields as available signals and controls.
            void parse_json_msrs(const std::string &str);
            /// @brief Fill string with the msr-safe allowlist file
            ///        contents reflecting all known MSRs for the
            ///        specified platform.
            /// @param cpuid [in] The CPUID of the platform.
            /// @return String formatted to be written to an msr-safe
            ///         allowlist file.
            static std::string msr_allowlist(int cpuid);
            /// @brief Get the cpuid of the current platform.
            static int cpuid(void);
            static std::string plugin_name(void);
            static std::unique_ptr<IOGroup> make_plugin(void);
        private:
            /// @brief Parse the given JSON string and update the
            ///        allowlist data map.
            static void parse_json_msrs_allowlist(const std::string &str,
                                                  std::map<uint64_t, std::pair<uint64_t, std::string> > &allowlist_data);
            /// @brief Format a string with the msr-safe allowlist file contents
            ///        reflecting all known MSRs for the current platform.
            /// @param [in] allowlist_data Map from MSR offset to
            ///        write mask and name.
            /// @return String formatted to be written to
            ///        an msr-safe allowlist file.
            static std::string format_allowlist(const std::map<uint64_t, std::pair<uint64_t, std::string> > &allowlist_data);

            /// @brief Return the JSON string for the MSR data
            ///        associated with the given cpuid.
            static std::string platform_data(int cpu_id);
            /// @brief Returns the filenames for user-defined MSRs if
            ///        found in the plugin path.
            static std::set<std::string> msr_data_files(void);
            /// @brief Override the default description for a signal.
            ///        If signal is not available, does nothing.
            void set_signal_description(const std::string &name,
                                        const std::string &description);
            /// @brief Override the default description for a signal.
            ///        If signal is not available, does nothing.
            void set_control_description(const std::string &name,
                                        const std::string &description);
            /// @brief Add support for an alias of a signal by name.
            void register_signal_alias(const std::string &signal_name, const std::string &msr_field_name);
            /// @brief Add support for an alias of a control by name.
            void register_control_alias(const std::string &control_name, const std::string &msr_field_name);
            /// @brief Add support for temperature combined signals if underlying
            ///        signals are available.
            void register_temperature_signals(void);
            /// @brief Add support for power combined signals if underlying
            ///        signals are available.
            void register_power_signals(void);
            /// @brief Add support for frequency signal aliases if underlying
            ///        signals are available.
            void register_frequency_signals(void);
            /// @brief Add support for frequency control aliases if underlying
            ///        controls are available.
            void register_frequency_controls(void);
            /// @brief Write to enable bits for all fixed counters.
            void enable_fixed_counters(void);
            /// @brief Check system configuration and warn if it ma
            ///        interfere with the given control.
            void check_control(const std::string &control_name);

            /// Helpers for JSON parsing
            static void check_top_level(const json11::Json &root);
            static void check_msr_root(const json11::Json &msr_root,
                                       const std::string &msr_name);
            static void check_msr_field(const json11::Json &msr_field,
                                        const std::string &msr_name,
                                        const std::string &msr_field_name);
            // Used to validate type and value of JSON objects
            static bool json_check_null_func(const json11::Json &obj);
            static bool json_check_is_hex_string(const json11::Json &obj);
            static bool json_check_is_valid_offset(const json11::Json &obj);
            static bool json_check_is_valid_domain(const json11::Json &domain);
            static bool json_check_is_integer(const json11::Json &num);
            static bool json_check_is_valid_aggregation(const json11::Json &obj);
            // Add raw MSR as an available signal
            void add_raw_msr_signal(const std::string &msr_name, int domain_type,
                                    uint64_t msr_offset);
            // Add a bitfield of an MSR as an available signal
            void add_msr_field_signal(const std::string &msr_name,
                                      const std::string &msr_field_name,
                                      int domain_type,
                                      int begin_bit, int end_bit,
                                      int function, double scalar, int units,
                                      const std::string &aggregation,
                                      const std::string &description,
                                      int behavior);
            // Add a bitfield of an MSR as an available control
            void add_msr_field_control(const std::string &msr_field_name,
                                       int domain_type,
                                       uint64_t msr_offset,
                                       int begin_bit, int end_bit,
                                       int function, double scalar, int units,
                                       const std::string &description);

            static const std::string M_DEFAULT_DESCRIPTION;
            static const std::string M_PLUGIN_NAME;
            static const std::string M_NAME_PREFIX;
            const PlatformTopo &m_platform_topo;
            std::shared_ptr<MSRIO> m_msrio;
            int m_cpuid;
            int m_num_cpu;
            bool m_is_active;
            bool m_is_read;
            bool m_is_fixed_enabled;
            std::vector<bool> m_is_adjusted;

            // time for derivative signals
            std::shared_ptr<geopm_time_s> m_time_zero;
            std::shared_ptr<double> m_time_batch;

            bool is_hwp_enabled(void);
            bool m_hwp_is_enabled;

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
                int behavior;
            };
            std::map<std::string, signal_info> m_signal_available;

            struct control_info
            {
                std::vector<std::shared_ptr<Control> > controls;
                int domain;
                int units;
                std::string description;
            };
            std::map<std::string, control_info> m_control_available;

            // Mapping of signal index to pushed signals.
            std::vector<std::shared_ptr<Signal> > m_signal_pushed;
            // Mapping of control index to pushed controls
            std::vector<std::shared_ptr<Control> > m_control_pushed;
    };
}

#endif
