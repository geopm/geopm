/*
 * Copyright (c) 2015 - 2023, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef EXAMPLEIOGROUP_HPP_INCLUDE
#define EXAMPLEIOGROUP_HPP_INCLUDE

#include <map>
#include <vector>
#include <string>
#include <memory>

#include "geopm/PluginFactory.hpp"
#include "geopm/IOGroup.hpp"

namespace geopm
{
    class PlatformTopo;
}

/// @brief IOGroup that provides a signals for user and idle CPU time, and
///        a control for writing to standard output.
class ExampleIOGroup : public geopm::IOGroup
{
    public:
        ExampleIOGroup();
        virtual ~ExampleIOGroup() = default;
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
        void save_control(const std::string &save_path) override;
        void restore_control(void) override;
        void restore_control(const std::string &save_path) override;
        std::function<double(const std::vector<double> &)> agg_function(const std::string &signal_name) const override;
        std::function<std::string(double)> format_function(const std::string &signal_name) const override;
        std::string signal_description(const std::string &signal_name) const;
        std::string control_description(const std::string &control_name) const;
        int signal_behavior(const std::string &signal_name) const override;
        std::string name(void) const override;
        static std::string plugin_name(void);
        static std::unique_ptr<geopm::IOGroup> make_plugin(void);
    private:
        std::vector<std::string> parse_proc_stat(void);
        enum m_signal_type_e {
            M_SIGNAL_USER_TIME,
            M_SIGNAL_NICE_TIME,
            M_SIGNAL_SYSTEM_TIME,
            M_SIGNAL_IDLE_TIME,
            M_NUM_SIGNAL
        };
        enum m_control_type_e {
            M_CONTROL_STDOUT,
            M_CONTROL_STDERR,
            M_NUM_CONTROL
        };
        const geopm::PlatformTopo &m_platform_topo;
        /// Whether any signal has been pushed
        bool m_do_batch_read;
        /// Whether read_batch() has been called at least once
        bool m_is_batch_read;
        std::map<std::string, int> m_signal_idx_map;
        std::map<std::string, int> m_control_idx_map;
        std::vector<bool> m_do_read;
        std::vector<bool> m_do_write;
        std::vector<std::string> m_signal_value;
        std::vector<std::string> m_control_value;
};

#endif
