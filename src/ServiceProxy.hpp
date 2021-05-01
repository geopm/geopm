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

#ifndef SERVICEPROXY_HPP_INCLUDE
#define SERVICEPROXY_HPP_INCLUDE

#include <string>
#include <memory>
#include <vector>

struct sd_bus;
struct sd_bus_message;
struct geopm_request_s;

namespace geopm
{
    /// @brief Proxy object for the io.github.geopm D-Bus interface
    /// used to implement the ServiceIOGroup
    class ServiceProxy
    {
        public:
            struct m_signal_info_s {
                std::string name;
                std::string description;
                int domain;
                int aggregation;
                int string_format;
                int behavior;
            };

            struct m_control_info_s {
                std::string name;
                std::string description;
                int domain;
            };

            ServiceProxy() = default;
            virtual ~ServiceProxy() = default;
            std::unique_ptr<ServiceProxy> make_unique(void);
            virtual void platform_get_user_access(std::vector<std::string> &signal_names,
                                                  std::vector<std::string> &control_names) = 0;
            virtual std::vector<m_signal_info_s> platform_get_signal_info(const std::vector<std::string> &signal_names) = 0;
            virtual std::vector<m_control_info_s> platform_get_control_info(const std::vector<std::string> &control_names) = 0;
            virtual void platform_open_session(void) = 0;
            virtual void platform_close_session(void) = 0;
            virtual void platform_start_batch(const std::vector<struct geopm_request_s> &signal_config,
                                              const std::vector<struct geopm_request_s> &control_config,
                                              int &server_pid,
                                              std::string &server_key) = 0;
            virtual void platform_stop_batch(int server_pid) = 0;
            virtual double platform_read_signal(const std::string &signal_name,
                                                int domain,
                                                int domain_idx) = 0;
            virtual void platform_write_control(const std::string &control_name,
                                                int domain,
                                                int domain_idx,
                                                double setting) = 0;
    };

    class ServiceProxyImp : public ServiceProxy
    {
        public:
            ServiceProxyImp();
            virtual ~ServiceProxyImp();
            void platform_get_user_access(std::vector<std::string> &signal_names,
                                          std::vector<std::string> &control_names) override;
            std::vector<m_signal_info_s> platform_get_signal_info(const std::vector<std::string> &signal_names) override;
            std::vector<m_control_info_s> platform_get_control_info(const std::vector<std::string> &control_names) override;
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
        private:
            sd_bus_message *call_method(const std::string &method_name);
            std::vector<std::string> read_string_array(sd_bus_message *bus_message);
            struct sd_bus *m_bus;
    };
}

#endif
