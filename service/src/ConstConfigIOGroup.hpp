/*
 * Copyright (c) 2015 - 2022, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef CONSTCONFIGIOGROUP_HPP_INCLUDE
#define CONSTCONFIGIOGROUP_HPP_INCLUDE

#include <map>
#include <string>

#include "geopm/IOGroup.hpp"

namespace geopm
{
    class ConstConfigIOGroup : public IOGroup
    {
        public:
            ConstConfigIOGroup();
            ConstConfigIOGroup(const std::string &config);
            /// @return the list of signal names provided by this IOGroup.
            std::set<std::string> signal_names(void) const override;
            /// @return empty set; this IOGroup does not provide any controls.
            std::set<std::string> control_names(void) const override;
            /// @return whether the given signal_name is supported by the
            ///         ConstConfigIOGroup for the current configuration.
            bool is_valid_signal(const std::string &signal_name) const override;
            /// @return false; this IOGroup does not provide any controls.
            bool is_valid_control(const std::string &control_name) const override;
            /// @return the domain for the provided signal if the signal is valid for this IOGroup, otherwise GEOPM_DOMAIN_INVALID.
            int signal_domain_type(const std::string &signal_name) const override;
            /// @return GEOPM_DOMAIN_INVALID; this IOGroup does not provide any controls.
            int control_domain_type(const std::string &control_name) const override;
            /// @param signal_name Adds the signal specified to the list of signals to be read during read_batch()
            ///
            /// @param domain_type must be one of the PlatformTopo::m_domain_e enum values
            ///
            /// @param domain_idx
            ///
            /// @throws geopm::Exception
            ///         if signal_name is not valid
            ///         if domain_type is not valid
            ///         if domain_idx is not valid (out of range)
            int push_signal(const std::string &signal_name, int domain_type, int domain_idx) override;
            /// @brief Should not be called; this IOGroup does not provide any controls.
            ///
            /// @throws geopm::Exception there are no controls supported by the ConstConfigIOGroup
            int push_control(const std::string &control_name, int domain_type, int domain_idx) override;
            /// @brief this is essentially a no-op as this IOGroup's signals are constant.
            void read_batch(void) override;
            /// @brief Does nothing; this IOGroup does not provide any controls.
            void write_batch(void) override;
            /// @param batch_idx Specifies a signal_idx returned from push_signal()
            ///
            /// @return the value of the signal specified by a signal_idx returned from push_signal().
            double sample(int batch_idx) override;
            /// @brief Should not be called; this IOGroup does not provide any controls.
            ///
            /// @throws geopm::Exception there are no controls supported by the ConstConfigIOGroup
            void adjust(int batch_idx, double setting) override;
            /// @param signal_name Specifies the name of the signal whose value you want to get.
            ///
            /// @param domain_type This must be == GEOPM_DOMAIN_BOARD
            ///
            /// @param domain_idx is ignored
            ///
            /// @throws geopm::Exception
            ///         if signal_name is not valid
            ///         if domain_type is not GEOPM_DOMAIN_BOARD
            ///
            /// @return the stored value for the given signal_name.
            double read_signal(const std::string &signal_name, int domain_type, int domain_idx) override;
            /// @brief Should not be called; this IOGroup does not provide any controls.
            ///
            /// @throws geopm::Exception there are no controls supported by the ConstConfigIOGroup
            void write_control(const std::string &control_name, int domain_type, int domain_idx, double setting) override;
            /// @brief Does nothing; this IOGroup does not provide any controls.
            void save_control(void) override;
            /// @brief Does nothing; this IOGroup does not provide any controls.
            void restore_control(void) override;
            /// @return  a function that should be used when aggregating the given signal.
            ///
            /// @see geopm::Agg
            std::function<double(const std::vector<double> &)> agg_function(const std::string &signal_name) const override;
            /// @return  a function that should be used when formatting the given signal.
            ///
            /// @see geopm::Agg
            std::function<std::string(double)> format_function(const std::string &signal_name) const override;
            /// @return a string description for signal_name, if defined.
            std::string signal_description(const std::string &signal_name) const override;
            /// @throws geopm::Exception there are no controls supported by the ConstConfigIOGroup
            std::string control_description(const std::string &control_name) const override;
            /// @return M_SIGNAL_BEHAVIOR_CONSTANT 
            int signal_behavior(const std::string &signal_name) const override;
            /// @brief Does nothing; this IOGroup does not provide any controls.
            ///
            /// @param save_path this argument is ignored
            void save_control(const std::string &save_path) override;
            /// @brief Does nothing; this IOGroup does not provide any controls.
            ///
            /// @param save_path this argument is ignored
            void restore_control(const std::string &save_path) override;
            /// @return the name of the IOGroup
            std::string name(void) const override;
            /// @return the name of the plugin to use when this plugin is
            ///         registered with the IOGroup factory
            ///
            /// @see geopm::PluginFactory
            static std::string plugin_name(void);
            /// @return a pointer to a new ConstConfigIOGroup object
            ///
            /// @see geopm::PluginFactory
            static std::unique_ptr<IOGroup> make_plugin(void);

        private:
            void parse_config_json(const std::string &config);

            struct m_signal_info_s {
                double value;
                int units;
                std::function<double(const std::vector<double> &)> agg_function;
                std::string description;
            };

            std::map<std::string, m_signal_info_s> m_signal_available;
    };
}

#endif
