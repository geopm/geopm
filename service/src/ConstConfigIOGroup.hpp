/*
 * Copyright (c) 2015 - 2022, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef CONSTCONFIGIOGROUP_HPP_INCLUDE
#define CONSTCONFIGIOGROUP_HPP_INCLUDE

#include <map>
#include <string>
#include <memory>

#include "geopm/json11.hpp"

#include "geopm/IOGroup.hpp"

namespace geopm
{
    class ConstConfigIOGroup : public IOGroup
    {
        public:
            ConstConfigIOGroup();
            ConstConfigIOGroup(const std::string &user_file_path,
                               const std::string &default_file_path);
            std::set<std::string> signal_names(void) const override;
            /// @return empty set; this IOGroup does not provide any controls.
            std::set<std::string> control_names(void) const override;
            bool is_valid_signal(const std::string &signal_name) const override;
            /// @return false; this IOGroup does not provide any controls.
            bool is_valid_control(const std::string &control_name) const override;
            /// @return the domain for the provided signal if the signal is valid for this IOGroup, otherwise GEOPM_DOMAIN_INVALID.
            int signal_domain_type(const std::string &signal_name) const override;
            /// @return GEOPM_DOMAIN_INVALID; this IOGroup does not provide any controls.
            int control_domain_type(const std::string &control_name) const override;
            int push_signal(const std::string &signal_name, int domain_type, int domain_idx) override;
            /// @brief Should not be called; this IOGroup does not provide any controls.
            ///
            /// @throws geopm::Exception there are no controls supported by the ConstConfigIOGroup
            int push_control(const std::string &control_name, int domain_type, int domain_idx) override;
            /// @brief this is essentially a no-op as this IOGroup's signals are constant.
            void read_batch(void) override;
            /// @brief Does nothing; this IOGroup does not provide any controls.
            void write_batch(void) override;
            double sample(int batch_idx) override;
            /// @brief Should not be called; this IOGroup does not provide any controls.
            ///
            /// @throws geopm::Exception there are no controls supported by the ConstConfigIOGroup
            void adjust(int batch_idx, double setting) override;
            double read_signal(const std::string &signal_name, int domain_type, int domain_idx) override;
            /// @brief Should not be called; this IOGroup does not provide any controls.
            ///
            /// @throws geopm::Exception there are no controls supported by the ConstConfigIOGroup
            void write_control(const std::string &control_name, int domain_type, int domain_idx, double setting) override;
            /// @brief Does nothing; this IOGroup does not provide any controls.
            void save_control(void) override;
            /// @brief Does nothing; this IOGroup does not provide any controls.
            void restore_control(void) override;
            std::function<double(const std::vector<double> &)> agg_function(const std::string &signal_name) const override;
            std::function<std::string(double)> format_function(const std::string &signal_name) const override;
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
            std::string name(void) const override;
            static std::string plugin_name(void);
            static std::unique_ptr<IOGroup> make_plugin(void);

        private:
            struct m_signal_desc_s {
                json11::Json::Type type;
                bool required;
            };

            struct m_signal_info_s {
                int units;
                int domain;
                std::function<double(const std::vector<double> &)> agg_function;
                std::string description;
                std::vector<double> values;
            };

            struct m_signal_ref_s {
                std::shared_ptr<m_signal_info_s> signal_info;
                int domain_idx;
            };

            void parse_config_json(const std::string &config);
            static json11::Json construct_config_json_obj(const std::string &config);
            static void check_json_signal(const json11::Json::object::value_type &signal);

            static const std::string M_PLUGIN_NAME;
            static const std::string M_SIGNAL_PREFIX;
            static const std::map<std::string, m_signal_desc_s> M_SIGNAL_FIELDS;
            static const std::string M_CONFIG_PATH_ENV;
            static const std::string M_DEFAULT_CONFIG_FILE_PATH;

            std::map<std::string, std::shared_ptr<m_signal_info_s> > m_signal_available;
            std::vector<m_signal_ref_s> m_pushed_signals;
    };
}

#endif
