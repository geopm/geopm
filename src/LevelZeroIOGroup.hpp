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

#ifndef LEVELZEROIOGROUP_HPP_INCLUDE
#define LEVELZEROIOGROUP_HPP_INCLUDE

#include <map>
#include <vector>
#include <string>
#include <memory>
#include <functional>

#include "IOGroup.hpp"
#include "LevelZeroSignal.hpp"

namespace geopm
{
    class PlatformTopo;
    class LevelZeroDevicePool;

    /// @brief IOGroup that provides signals and controls for Accelerators
    class LevelZeroIOGroup : public IOGroup
    {
        public:
            LevelZeroIOGroup();
            LevelZeroIOGroup(const PlatformTopo &platform_topo, const LevelZeroDevicePool &device_pool);
            virtual ~LevelZeroIOGroup() = default;
            std::set<std::string> signal_names(void) const override;
            std::set<std::string> control_names(void) const override;
            bool is_valid_signal(const std::string &signal_name) const override;
            bool is_valid_control(const std::string &control_name) const override;
            int signal_domain_type(const std::string &signal_name) const override;
            int control_domain_type(const std::string &control_name) const override;
            int push_signal(const std::string &signal_name, int domain_type, int domain_idx)  override;
            int push_control(const std::string &control_name, int domain_type, int domain_idx) override;
            void read_batch(void) override;
            void write_batch(void) override;
            double sample(int batch_idx) override;
            void adjust(int batch_idx, double setting) override;
            double read_signal(const std::string &signal_name, int domain_type, int domain_idx) override;
            void write_control(const std::string &control_name, int domain_type, int domain_idx, double setting) override;
            void save_control(void) override;
            void restore_control(void) override;
            std::function<double(const std::vector<double> &)> agg_function(const std::string &signal_name) const override;
            std::function<std::string(double)> format_function(const std::string &signal_name) const override;
            std::string signal_description(const std::string &signal_name) const override;
            std::string control_description(const std::string &control_name) const override;
            int signal_behavior(const std::string &signal_name) const override;
            static std::string plugin_name(void);
            static std::unique_ptr<IOGroup> make_plugin(void);
        private:
            void init(void);

            void register_derivative_signals(void);
            void register_signal_alias(const std::string &alias_name, const std::string &signal_name);
            void register_control_alias(const std::string &alias_name, const std::string &control_name);

            const PlatformTopo &m_platform_topo;
            const LevelZeroDevicePool &m_levelzero_device_pool;
            bool m_is_batch_read;
            std::vector<uint64_t> m_initial_freq_range_max_gpu_limit;
            std::vector<uint64_t> m_initial_freq_range_min_gpu_limit;
            std::vector<uint64_t> m_initial_power_limit_sustained;
            std::vector<uint64_t> m_initial_power_limit_enabled_sustained;
            std::vector<uint64_t> m_initial_power_limit_interval_sustained;
            std::vector<uint64_t> m_initial_power_limit_burst;
            std::vector<uint64_t> m_initial_power_limit_enabled_burst;

            struct control_s
            {
                double m_setting;
                bool m_is_adjusted;
            };

            struct signal_info {
                std::string m_description;
                int domain_type;
                std::function<double(const std::vector<double> &)> m_agg_function;
                std::function<std::string(double)> m_format_function;
                std::vector<std::shared_ptr<Signal> > m_signals;
                std::function<double (unsigned int)> m_devpool_func;
                double scalar;
            };

            struct control_info {
                std::string m_description;
                std::vector<std::shared_ptr<control_s> > controls;
                int domain_type;
                std::function<double(const std::vector<double> &)> m_agg_function;
                std::function<std::string(double)> m_format_function;
            };

            std::map<std::string, signal_info> m_signal_available;
            std::map<std::string, control_info> m_control_available;
            std::vector<std::shared_ptr<Signal> > m_signal_pushed;
            std::vector<std::shared_ptr<control_s> > m_control_pushed;
    };
}
#endif
