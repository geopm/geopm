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

#ifndef PROFILEIOGROUP_HPP_INCLUDE
#define PROFILEIOGROUP_HPP_INCLUDE

#include <set>
#include <string>
#include <functional>

#include "IOGroup.hpp"

namespace geopm
{
    class ApplicationSampler;
    class ProfileIOSample;
    class PlatformTopo;

    /// @brief IOGroup that provides signals from the application.
    class ProfileIOGroup : public IOGroup
    {
        public:
            ProfileIOGroup();
            ProfileIOGroup(const PlatformTopo &topo, ApplicationSampler &application_sampler);
            virtual ~ProfileIOGroup();
            std::set<std::string> signal_names(void) const override;
            std::set<std::string> control_names(void) const override;
            bool is_valid_signal(const std::string &signal_name) const override;
            bool is_valid_control(const std::string &control_name) const override;
            int signal_domain_type(const std::string &signal_name) const override;
            int control_domain_type(const std::string &control_name) const override;
            int push_signal(const std::string &signal_name, int domain_type, int domain_idx) override;
            int push_control(const std::string &control_name, int domain_type, int domain_idx) override;
            void read_batch(void) override;
            void write_batch(void) override;
            double sample(int signal_idx) override;
            void adjust(int control_idx, double setting) override;
            double read_signal(const std::string &signal_name, int domain_type, int domain_idx) override;
            void write_control(const std::string &control_name, int domain_type, int domain_idx, double setting) override;
            void save_control(void) override;
            void restore_control(void) override;
            std::function<double(const std::vector<double> &)> agg_function(const std::string &signal_name) const override;
            std::function<std::string(double)> format_function(const std::string &signal_name) const override;
            std::string signal_description(const std::string &signal_name) const override;
            std::string control_description(const std::string &control_name) const override;
            static std::string plugin_name(void);
            static std::unique_ptr<IOGroup> make_plugin(void);
            void connect(void);

        private:
            enum m_signal_type {
                M_SIGNAL_REGION_HASH,
                M_SIGNAL_REGION_HINT,
                M_SIGNAL_THREAD_PROGRESS,
                M_SIGNAL_TIME_HINT_UNSET,
                M_SIGNAL_TIME_HINT_UNKNOWN,
                M_SIGNAL_TIME_HINT_COMPUTE,
                M_SIGNAL_TIME_HINT_MEMORY,
                M_SIGNAL_TIME_HINT_NETWORK,
                M_SIGNAL_TIME_HINT_IO,
                M_SIGNAL_TIME_HINT_SERIAL,
                M_SIGNAL_TIME_HINT_PARALLEL,
                M_SIGNAL_TIME_HINT_IGNORE,
                M_NUM_SIGNAL,
            };
            struct m_signal_config {
                int signal_type;
                int domain_type;
                int domain_idx;
            };

            /// @brief Check that the signal name and domain are valid
            ///        and returns the corresponding m_signal_type for
            ///        the signal.
            int check_signal(const std::string &signal_name, int domain_type, int domain_idx) const;
            /// @brief Converts a region hash to double for use as a
            ///        signal.  GEOPM_REGION_HASH_INVALID is converted
            ///        to NAN.
            static double hash_to_signal(uint64_t hash);
            /// @brief Converts a m_signal_type for the IOGroup to the
            ///        corresponding geopm_region_hint_e defined in
            ///        geopm.h.  The signal_type must be one of the
            ///        M_SIGNAL_TIME_HINT* signals.
            static uint64_t signal_type_to_hint(int signal_type);

            ApplicationSampler &m_application_sampler;

            std::map<std::string, int> m_signal_idx_map;
            const PlatformTopo &m_platform_topo;
            const int m_num_cpu;
            std::vector<bool> m_do_read;
            bool m_is_batch_read;
            std::vector<struct m_signal_config> m_active_signal;
            std::array<std::vector<double>, M_NUM_SIGNAL> m_per_cpu_sample;
            /// map from runtime signal index to the region id signal it uses
            std::map<int, int> m_rid_idx;
            bool m_is_pushed;
    };
}

#endif
