/*
 * Copyright (c) 2015 - 2021, Intel Corporation
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
                           std::shared_ptr<ServiceProxy> service_proxy);
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
            std::shared_ptr<BatchClient> m_batch_server;
            std::vector<double> m_batch_samples;
            std::vector<double> m_batch_settings;
    };
}

#endif
