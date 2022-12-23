/*
 * Copyright (c) 2015 - 2022, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef GRPCSERVICEPROXY_HPP_INCLUDE
#define GRPCSERVICEPROXY_HPP_INCLUDE

#include <string>
#include <memory>
#include <vector>
#include "geopm/ServiceProxy.hpp"

#include "geopm_service.pb.h"
#include "geopm_service.grpc.pb.h"

namespace geopm
{

    class GRPCServiceProxy : public ServiceProxy
    {
        public:
            GRPCServiceProxy();
            virtual ~GRPCServiceProxy();
            void platform_get_user_access(std::vector<std::string> &signal_names,
                                          std::vector<std::string> &control_names) override;
            std::vector<signal_info_s> platform_get_signal_info(const std::vector<std::string> &signal_names) override;
            std::vector<control_info_s> platform_get_control_info(const std::vector<std::string> &control_names) override;
            void platform_open_session(void) override;
            void platform_close_session(void) override;
            void platform_start_batch(const std::vector<struct geopm_request_s> &signal_config,
                                      const std::vector<struct geopm_request_s> &control_config,
                                      int &server_pid,
                                      std::string &server_key) override;
            void platform_stop_batch(int server_pid) override;
            double platform_read_signal(const std::string &signal_name,
                                        int domain,
                                        int domain_idx) override;
            void platform_write_control(const std::string &control_name,
                                        int domain,
                                        int domain_idx,
                                        double setting) override;
            void platform_restore_control(void) override;
            void platform_start_profile(const std::string &profile_name) override;
            void platform_stop_profile(const std::vector<std::string> &region_names) override;
            std::vector<int> platform_get_profile_pids(const std::string &profile_name) override;
            std::vector<std::string> platform_pop_profile_region_names(const std::string &profile_name) override;
            std::string topo_get_cache(void) override;
        private:
            const std::string m_grpc_socket;
            std::string m_session_key;
            int m_pidfd;
            std::shared_ptr<GEOPMPackage::GEOPMService::Stub> m_client;
            GEOPMPackage::SessionKey m_id_request;
    };
}

#endif
