/*
 * Copyright (c) 2015 - 2023, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef ENVIRONMENT_HPP_INCLUDE
#define ENVIRONMENT_HPP_INCLUDE

#include <string>
#include <vector>
#include <utility>
#include <map>
#include <set>
#include <memory>


namespace geopm
{
    /// @brief Environment class encapsulates all functionality related to
    /// dealing with runtime environment variables.
    class Environment
    {
        public:
            /// @brief Enum for controller launch methods
            ///
            /// The return value from pmpi_ctl() is one of these.
            enum m_ctl_e {
                M_CTL_NONE,
                M_CTL_PROCESS,
                M_CTL_PTHREAD,
            };

            Environment() = default;
            virtual ~Environment() = default;
            virtual std::string report(void) const = 0;
            virtual std::string comm(void) const = 0;
            virtual std::string policy(void) const = 0;
            virtual std::string endpoint(void) const = 0;
            virtual std::string trace(void) const = 0;
            virtual std::string trace_profile(void) const = 0;
            virtual std::string trace_endpoint_policy(void) const = 0;
            virtual std::string profile(void) const = 0;
            virtual std::string frequency_map(void) const = 0;
            virtual std::string agent(void) const = 0;
            virtual std::vector<std::pair<std::string, int> > trace_signals(void) const = 0;
            virtual std::vector<std::pair<std::string, int> > report_signals(void) const = 0;
            virtual int max_fan_out(void) const = 0;
            virtual int pmpi_ctl(void) const = 0;
            virtual bool do_policy(void) const = 0;
            virtual bool do_endpoint(void) const = 0;
            virtual bool do_trace(void) const = 0;
            virtual bool do_trace_profile(void) const = 0;
            virtual bool do_trace_endpoint_policy(void) const = 0;
            virtual bool do_profile(void) const = 0;
            virtual int timeout(void) const = 0;
            virtual bool do_ompt(void) const = 0;
            virtual std::string default_config_path(void) const = 0;
            virtual std::string override_config_path(void) const = 0;
            virtual std::string record_filter(void) const = 0;
            virtual bool do_record_filter(void) const = 0;
            virtual bool do_debug_attach_all(void) const = 0;
            virtual bool do_debug_attach_one(void) const = 0;
            virtual bool do_init_control(void) const = 0;
            virtual int debug_attach_process(void) const = 0;
            virtual std::string init_control(void) const = 0;
            virtual double period(double default_period) const = 0;
            virtual int num_proc(void) const = 0;
            static std::map<std::string, std::string> parse_environment_file(const std::string &env_file_path);
    };

    class PlatformIO;

    class EnvironmentImp : public Environment
    {
        public:
            EnvironmentImp();
            EnvironmentImp(const std::string &default_settings_path,
                           const std::string &override_settings_path,
                           const PlatformIO *platform_io);
            virtual ~EnvironmentImp() = default;
            std::string report(void) const override;
            std::string comm(void) const override;
            std::string policy(void) const override;
            std::string endpoint(void) const override;
            std::string trace(void) const override;
            std::string trace_profile(void) const override;
            std::string trace_endpoint_policy(void) const override;
            std::string profile(void) const override;
            std::string frequency_map(void) const override;
            std::string agent(void) const override;
            std::vector<std::pair<std::string, int> > trace_signals(void) const override;
            std::vector<std::pair<std::string, int> > report_signals(void) const override;
            std::vector<std::pair<std::string, int> > signal_parser(std::string environment_variable_contents) const;
            int max_fan_out(void) const override;
            int pmpi_ctl(void) const override;
            bool do_policy(void) const override;
            bool do_endpoint(void) const override;
            bool do_trace(void) const override;
            bool do_trace_profile(void) const override;
            bool do_trace_endpoint_policy(void) const override;
            bool do_profile() const override;
            int timeout(void) const override;
            static std::set<std::string> get_all_vars(void);
            bool do_ompt(void) const override;
            std::string default_config_path(void) const override;
            std::string override_config_path(void) const override;
            static void parse_environment_file(const std::string &settings_path,
                                               const std::set<std::string> &all_names,
                                               const std::set<std::string> &user_defined_names,
                                               std::map<std::string, std::string> &name_value_map);
            std::string record_filter(void) const override;
            bool do_record_filter(void) const override;
            bool do_debug_attach_all(void) const override;
            bool do_debug_attach_one(void) const override;
            bool do_init_control(void) const override;
            int debug_attach_process(void) const override;
            std::string init_control(void) const override;
            double period(double default_period) const override;
            int num_proc(void) const override;
        protected:
            void parse_environment(void);
            bool is_set(const std::string &env_var) const;
            std::string lookup(const std::string &env_var) const;
            const std::set<std::string> m_all_names;
            std::set<std::string> m_user_defined_names;
            std::map<std::string, std::string> m_name_value_map;
            const std::string m_default_config_path;
            const std::string m_override_config_path;
            // Pointer used here to avoid calling the singleton too
            // early as the Environment is used in the top of
            // geopm_pmpi_init(). Do *NOT* delete this pointer.
            mutable const PlatformIO *m_platform_io;
    };

    const Environment &environment(void);
}
#endif
