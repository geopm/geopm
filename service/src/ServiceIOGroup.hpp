/*
 * Copyright (c) 2015 - 2023, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef SERVICEIOGROUP_HPP_INCLUDE
#define SERVICEIOGROUP_HPP_INCLUDE

#include <map>

#include "geopm/IOGroup.hpp"

struct geopm_request_s;

namespace geopm
{
    class PlatformTopo;
    class ServiceProxy;
    class BatchClient;
    struct signal_info_s;
    struct control_info_s;

    /// @brief IOGroup that uses DBus interface to access geopmd
    ///        provided signals and controls.  This IOGroup is not
    ///        loaded by a server side PlatformIO object.
    class ServiceIOGroup : public IOGroup
    {
        public:
            ServiceIOGroup();
            ServiceIOGroup(const PlatformTopo &platform_topo,
                           std::shared_ptr<ServiceProxy> service_proxy,
                           std::shared_ptr<BatchClient> batch_client_mock);
            virtual ~ServiceIOGroup();
            std::set<std::string> signal_names(void) const override;
            std::set<std::string> control_names(void) const override;
            bool is_valid_signal(const std::string &signal_name) const override;
            bool is_valid_control(const std::string &control_name) const override;
            int signal_domain_type(const std::string &signal_name) const override;
            int control_domain_type(const std::string &control_name) const override;
            int push_signal(const std::string &signal_name,
                            int domain_type,
                            int domain_idx) override;
            int push_control(const std::string &control_name,
                             int domain_type,
                             int domain_idx) override;
            void read_batch(void) override;
            void write_batch(void) override;
            double sample(int sample_idx) override;
            void adjust(int control_idx,
                        double setting) override;
            double read_signal(const std::string &signal_name,
                               int domain_type,
                               int domain_idx) override;
            void write_control(const std::string &control_name,
                               int domain_type,
                               int domain_idx,
                               double setting) override;
            // NOTE: This IOGroup will not directlly implement a
            //       save/restore since it is a proxy.  Creating this
            //       IOGroup will start a session with the service,
            //       and save and restore will be managed by the
            //       geopmd service for every session that is opened.
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
            static std::unique_ptr<IOGroup> make_plugin(void);
        private:
            void init_batch_server(void);
            static const std::string M_PLUGIN_NAME;
            static std::map<std::string, signal_info_s> service_signal_info(std::shared_ptr<ServiceProxy> service_proxy);
            static std::map<std::string, control_info_s> service_control_info(std::shared_ptr<ServiceProxy> service_proxy);
            static std::string strip_plugin_name(const std::string &name);
            const PlatformTopo &m_platform_topo;
            std::shared_ptr<ServiceProxy> m_service_proxy;
            std::map<std::string, signal_info_s> m_signal_info;
            std::map<std::string, control_info_s> m_control_info;
            std::vector<geopm_request_s> m_signal_requests;
            std::vector<geopm_request_s> m_control_requests;
            std::shared_ptr<BatchClient> m_batch_client;
            std::vector<double> m_batch_samples;
            std::vector<double> m_batch_settings;
            int m_session_pid;
            bool m_is_batch_active;
    };
}

#endif
