/*
 * Copyright (c) 2015, 2016, 2017, 2018, 2019, 2020, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef LEVELZEROIOGROUP_HPP_INCLUDE
#define LEVELZEROIOGROUP_HPP_INCLUDE

#include <map>
#include <vector>
#include <string>
#include <memory>
#include <functional>

#include "geopm/IOGroup.hpp"
#include "LevelZeroSignal.hpp"

namespace geopm
{
    class PlatformTopo;
    class LevelZeroDevicePool;
    class SaveControl;

    /// @brief IOGroup that provides signals and controls for GPUs
    class LevelZeroIOGroup : public IOGroup
    {
        public:
            LevelZeroIOGroup();
            LevelZeroIOGroup(const PlatformTopo &platform_topo,
                             const LevelZeroDevicePool &device_pool,
                             std::shared_ptr<SaveControl> save_control_test);
            virtual ~LevelZeroIOGroup() = default;
            std::set<std::string> signal_names(void) const override;
            std::set<std::string> control_names(void) const override;
            bool is_valid_signal(const std::string &signal_name) const override;
            bool is_valid_control(const std::string &control_name) const override;
            int signal_domain_type(const std::string &signal_name) const override;
            int control_domain_type(const std::string &control_name) const override;
            int push_signal(const std::string &signal_name, int domain_type,
                            int domain_idx)  override;
            int push_control(const std::string &control_name, int domain_type,
                             int domain_idx) override;
            void read_batch(void) override;
            void write_batch(void) override;
            double sample(int batch_idx) override;
            void adjust(int batch_idx, double setting) override;
            double read_signal(const std::string &signal_name, int domain_type,
                               int domain_idx) override;
            void write_control(const std::string &control_name, int domain_type,
                               int domain_idx, double setting) override;
            void save_control(void) override;
            void restore_control(void) override;
            std::function<double(const std::vector<double> &)> agg_function(
                                 const std::string &signal_name) const override;
            std::function<std::string(double)> format_function(
                                 const std::string &signal_name) const override;
            std::string signal_description(const std::string
                                           &signal_name) const override;
            std::string control_description(const std::string
                                            &control_name) const override;
            int signal_behavior(const std::string &signal_name) const override;
            void save_control(const std::string &save_path) override;
            void restore_control(const std::string &save_path) override;
            std::string name(void) const override;
            static std::string plugin_name(void);
            static std::unique_ptr<IOGroup> make_plugin(void);
        private:
            void init(void);

            void register_derivative_signals(void);
            void register_signal_alias(const std::string &alias_name,
                                       const std::string &signal_name);
            void register_control_alias(const std::string &alias_name,
                                        const std::string &control_name);

            struct control_s {
                double m_setting;
                bool m_is_adjusted;
            };

            struct signal_info {
                std::string m_description;
                int m_domain_type;
                std::function<double(const std::vector<double> &)> m_agg_function;
                int behavior;
                std::function<std::string(double)> m_format_function;
                std::vector<std::shared_ptr<Signal> > m_signals;
                std::function<double (unsigned int)> m_devpool_func;
                double m_scalar;
            };

            struct control_info {
                std::string m_description;
                std::vector<std::shared_ptr<control_s> > m_controls;
                int m_domain_type;
                std::function<double(const std::vector<double> &)> m_agg_function;
                std::function<std::string(double)> m_format_function;
            };

            struct derivative_signal_info
            {
                std::string m_description;
                std::string m_base_name;
                std::string m_time_name;
                int m_behavior;
            };

            static const std::string M_PLUGIN_NAME;
            static const std::string M_NAME_PREFIX;
            const PlatformTopo &m_platform_topo;
            const LevelZeroDevicePool &m_levelzero_device_pool;
            bool m_is_batch_read;

            std::map<std::string, signal_info> m_signal_available;
            std::map<std::string, control_info> m_control_available;
            std::vector<std::shared_ptr<Signal> > m_signal_pushed;
            std::vector<std::shared_ptr<control_s> > m_control_pushed;
            const std::set<std::string> m_special_signal_set;
            std::map<std::string, derivative_signal_info> m_derivative_signal_map;
            std::set<int> m_derivative_signal_pushed_set;

            //GEOPM Domain indexed
            std::vector<std::pair<double,double> > m_frequency_range;

            std::shared_ptr<SaveControl> m_mock_save_ctl;
    };
}
#endif
