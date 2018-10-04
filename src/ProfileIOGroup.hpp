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

#ifndef PROFILEIOGROUP_HPP_INCLUDE
#define PROFILEIOGROUP_HPP_INCLUDE

#include <set>
#include <string>

#include "IOGroup.hpp"

namespace geopm
{
    class IProfileIOSample;
    class IProfileIORuntime;
    class IPlatformTopo;

    /// @brief IOGroup that provides signals from the application.
    class ProfileIOGroup : public IOGroup
    {
        public:
            ProfileIOGroup(std::shared_ptr<IProfileIOSample> profile_sample,
                           std::shared_ptr<IProfileIORuntime> profile_runtime);
            ProfileIOGroup(std::shared_ptr<IProfileIOSample> profile_sample,
                           std::shared_ptr<IProfileIORuntime> profile_runtime,
                           geopm::IPlatformTopo &topo);
            virtual ~ProfileIOGroup() = default;
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
            static std::string plugin_name(void);
        private:
            enum m_signal_type {
                M_SIGNAL_REGION_ID,
                M_SIGNAL_PROGRESS,
                M_SIGNAL_RUNTIME,
            };
            struct m_signal_config {
                int signal_type;
                int domain_type;
                int domain_idx;
            };

            int check_signal(const std::string &signal_name, int domain_type, int domain_idx) const;

            std::shared_ptr<IProfileIOSample> m_profile_sample;
            std::shared_ptr<IProfileIORuntime> m_profile_runtime;
            std::map<std::string, int> m_signal_idx_map;
            IPlatformTopo &m_platform_topo;
            bool m_do_read_region_id = false;
            bool m_do_read_progress = false;
            bool m_do_read_runtime = false;
            bool m_is_batch_read = false;
            std::vector<struct m_signal_config> m_active_signal;
            std::vector<uint64_t> m_per_cpu_region_id;
            std::vector<double> m_per_cpu_progress;
            std::vector<double> m_per_cpu_runtime;
            std::map<int, int> m_rid_idx; // map from runtime signal index to the region id signal it uses
    };
}

#endif
