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

#ifndef MSRIOGROUP_HPP_INCLUDE
#define MSRIOGROUP_HPP_INCLUDE

#include <vector>
#include <map>
#include <memory>
#include <functional>

#include "IOGroup.hpp"

namespace geopm
{
    class IMSR;
    class MSRSignal;
    class MSRControl;
    class IMSRIO;
    class IPlatformTopo;

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
            MSRIOGroup(IPlatformTopo &platform_topo, std::unique_ptr<IMSRIO> msrio, int cpuid, int num_cpu);
            virtual ~MSRIOGroup();
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
            std::string msr_whitelist(int cpuid) const;
            /// @brief Get the cpuid of the current platform.
            int cpuid(void) const;
            /// @brief Register a single MSR field as a signal. This
            ///        is called by init_msr().
            /// @param [in] signal_name Compound signal name of form
            ///        "msr_name:field_name" where msr_name is the
            ///        name of the MSR and the field_name is the name
            ///        of the signal field held in the MSR.
            void register_msr_signal(const std::string &signal_name);
            /// @brief Register a single MSR field as a control. This
            ///        is called by init_msr().
            /// @param [in] signal_name Compound control name of form
            ///        "msr_name:field_name" where msr_name is the
            ///        name of the MSR and the field_name is the name
            ///        of the control field held in the MSR.
            void register_msr_control(const std::string &control_name);
            static std::string plugin_name(void);
            static std::unique_ptr<IOGroup> make_plugin(void);
        private:
            struct m_restore_s {
                uint64_t value;
                uint64_t mask;
            };
            void register_msr_signal(const std::string &signal_name, const std::string &msr_field_name);
            void register_msr_control(const std::string &control_name, const std::string &msr_field_name);
            void register_raw_msr_signal(const std::string &msr_name, const IMSR &msr_ptr);
            void enable_fixed_counters(void);
            void check_control(const std::string &control_name);

            /// @brief Configure memory for all pushed signals and controls.
            void activate(void);
            IPlatformTopo &m_platform_topo;
            int m_num_cpu;
            bool m_is_active;
            bool m_is_read;
            std::unique_ptr<IMSRIO> m_msrio;
            int m_cpuid;
            std::vector<bool> m_is_adjusted;
            // Mappings from names to all valid signals and controls
            std::map<std::string, const IMSR &> m_name_msr_map;
            std::map<std::string, std::vector<MSRSignal *> > m_name_cpu_signal_map;
            std::map<std::string, std::vector<MSRControl *> > m_name_cpu_control_map;
            // Pushed signals and controls only
            std::vector<MSRSignal *> m_active_signal;
            std::vector<std::vector<MSRControl *> > m_active_control;
            // Vectors are over MSRs for all active signals
            std::vector<uint64_t> m_read_field;
            std::vector<int> m_read_cpu_idx;
            std::vector<uint64_t> m_read_offset;
            // Vectors are over MSRs for all active controls
            std::vector<uint64_t> m_write_field;
            std::vector<int> m_write_cpu_idx;
            std::vector<uint64_t> m_write_offset;
            std::vector<uint64_t> m_write_mask;
            const std::string m_name_prefix;
            std::vector<std::map<uint64_t, m_restore_s> > m_per_cpu_restore;
            bool m_is_fixed_enabled;
            std::map<std::string, std::function<double(const std::vector<double> &)> > m_func_map;
            std::map<std::string, std::string> m_signal_desc_map;
            std::map<std::string, std::string> m_control_desc_map;
    };
}

#endif
