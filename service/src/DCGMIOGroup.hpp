/*
 * Copyright (c) 2015 - 2022, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef DCGMIOGROUP_HPP_INCLUDE
#define DCGMIOGROUP_HPP_INCLUDE

#include <map>
#include <vector>
#include <string>
#include <memory>

#include "geopm/IOGroup.hpp"

namespace geopm
{
    class PlatformTopo;
    class DCGMDevicePool;

    /// @brief IOGroup that provides signals and controls for DCGM GPUs
    class DCGMIOGroup : public IOGroup
    {
        public:
            DCGMIOGroup();
            DCGMIOGroup(const PlatformTopo &platform_topo, DCGMDevicePool &device_pool);
            ~DCGMIOGroup();
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
            void save_control(const std::string &save_path) override;
            void restore_control(const std::string &save_path) override;
            std::string name(void) const override;
            static std::string plugin_name(void);
            static std::unique_ptr<geopm::IOGroup> make_plugin(void);
        private:
            void register_signal_alias(const std::string &alias_name, const std::string &signal_name);
            void register_control_alias(const std::string &alias_name, const std::string &control_name);

            const PlatformTopo &m_platform_topo;
            DCGMDevicePool &m_dcgm_device_pool;
            bool m_is_batch_read;

            struct signal_s {
                double m_value;
                bool m_do_read;
            };

            struct control_s {
                double m_setting;
                bool m_is_adjusted;
            };

            struct signal_info {
                std::string m_description;
                std::vector<std::shared_ptr<signal_s> > m_signals;
                std::function<double (unsigned int)> m_devpool_func;
                std::function<double(const std::vector<double> &)> m_agg_function;
                std::function<std::string(double)> m_format_function;
            };

            struct control_info {
                std::string m_description;
                std::vector<std::shared_ptr<control_s> > m_controls;
                std::function<double(const std::vector<double> &)> m_agg_function;
                std::function<std::string(double)> m_format_function;
            };

            std::map<std::string, signal_info> m_signal_available;
            std::map<std::string, control_info> m_control_available;
            std::vector<std::shared_ptr<signal_s> > m_signal_pushed;
            std::vector<std::shared_ptr<control_s> > m_control_pushed;
    };
}
#endif
