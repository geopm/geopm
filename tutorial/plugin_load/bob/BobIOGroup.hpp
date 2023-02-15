/*
 * Copyright (c) 2015 - 2023, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef BOBIOGROUP_HPP_INCLUDE
#define BOBIOGROUP_HPP_INCLUDE

#include <map>
#include <string>

#include "geopm/PluginFactory.hpp"
#include "geopm/IOGroup.hpp"

class BobIOGroup : public geopm::IOGroup
{
    public:
        BobIOGroup();
        virtual ~BobIOGroup() = default;
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
        enum m_signal_type_e {
            M_SIGNAL_BAR,
            M_SIGNAL_BAZ,
            M_SIGNAL_TIME,
            M_NUM_SIGNAL
        };
        std::map<std::string, int> m_signal_idx_map;
};

#endif
