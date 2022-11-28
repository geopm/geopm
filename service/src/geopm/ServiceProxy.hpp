/*
 * Copyright (c) 2015 - 2023, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef SERVICEPROXY_HPP_INCLUDE
#define SERVICEPROXY_HPP_INCLUDE

#include <string>
#include <memory>
#include <vector>

struct geopm_request_s;

namespace geopm
{
    class SDBus;
    class SDBusMessage;

    /// @brief Information pertaining to a particular signal supported
    ///        by PlatformIO
    struct signal_info_s {
        /// @brief Name of the signal
        std::string name;
        /// @brief Description of the signal
        std::string description;
        /// @brief Topology domain that supports the signal.  One of
        ///        the geopm_domain_e values defined in geopm_topo.h.
        int domain;
        /// @brief The signal aggregation type.  One of the the
        ///        geopm::Agg::m_type_e values defined in geopm/Agg.hpp
        int aggregation;
        /// @brief The format method to convert a signal to a string.
        ///        One of the geopm::string_format_e values defined in
        ///        geopm/Helper.hpp.
        int string_format;
        /// @brief The signal behavior type.  One of the
        ///        geopm::IOGroup::m_signal_behavior_e values defined
        ///        in geopm/IOGroup.hpp.
        int behavior;
    };

    /// @brief Information pertaining to a particular control
    ///        supported by PlatformIO
    struct control_info_s {
        /// @brief Name of the control
        std::string name;
        /// @brief Description of the control
        std::string description;
        /// @brief Topology domain that supports the signal.  One of
        ///        the geopm_domain_e values defined in geopm_topo.h.
        int domain;
    };

    /// @brief Proxy object for the io.github.geopm D-Bus interface
    ///        used to implement the ServiceIOGroup
    class ServiceProxy
    {
        public:
            /// @brief ServiceProxy constructor
            ServiceProxy() = default;
            /// @brief ServiceProxy destructor
            virtual ~ServiceProxy() = default;
            /// @brief Create a unique pointer to a ServiceProxy object
            /// @return A unique_ptr to a newly created ServiceProxy object
            static std::unique_ptr<ServiceProxy> make_unique(void);
            /// @brief Calls the PlatformGetUserAccess API defined in
            ///        the io.github.geopm D-Bus namespace.
            /// @param signal_names [out] Vector of strings containing
            ///                     all signals that the calling
            ///                     process has access to.
            /// @param control_names [out] Vector of strings
            ///                      containing all controls that the
            ///                      calling process has access to.
            virtual void platform_get_user_access(
                std::vector<std::string> &signal_names,
                std::vector<std::string> &control_names) = 0;
            /// @brief Calls the PlatformGetSignalInfo API defined in
            ///        the io.github.geopm D-Bus namespace.
            /// @param signal_names [in] Vector of strings containing
            ///                     all signal names to query.
            /// @return Vector of structures describing the queried
            ///         signals
            virtual std::vector<signal_info_s> platform_get_signal_info(
                const std::vector<std::string> &signal_names) = 0;
            /// @brief Calls the PlatformGetControlInfo API defined in
            ///        the io.github.geopm D-Bus namespace.
            /// @param control_names [in] Vector of strings containing
            ///                      all control names to query.
            /// @return Vector of structures describing the queried
            ///         controls
            virtual std::vector<control_info_s> platform_get_control_info(
                const std::vector<std::string> &control_names) = 0;
            /// @brief Calls the PlatformOpenSession API defined in
            ///        the io.github.geopm D-Bus namespace.
            virtual void platform_open_session(void) = 0;
            /// @brief Calls the PlatformCloseSession API defined in
            ///        the io.github.geopm D-Bus namespace.
            virtual void platform_close_session(void) = 0;
            /// @brief Calls the PlatformStartBatch API defined in the
            ///        io.github.geopm D-Bus namespace.
            /// @param signal_config [in] Vector of signal requests
            ///                      that will be supported by the
            ///                      batch server that is created.
            /// @param control_config [in] Vector of control requests
            ///                      that will be supported by the
            ///                      batch server that is created.
            /// @param server_pid [out] Linux PID of the server
            ///                   process created.
            /// @param server_key [out] Unique key used to connect to
            ///                   the created server.
            virtual void platform_start_batch(
                const std::vector<struct geopm_request_s> &signal_config,
                const std::vector<struct geopm_request_s> &control_config,
                int &server_pid,
                std::string &server_key) = 0;
            /// @brief Calls the PlatformStopBatch API defined in the
            ///        io.github.geopm D-Bus namespace.
            /// @param server_pid [in] The Linux PID of the batch
            ///                   server to stop.
            virtual void platform_stop_batch(int server_pid) = 0;
            /// @brief Calls the PlatformReadSignal API defined in the
            ///        io.github.geopm D-Bus namespace.
            /// @param signal_name [in] Name of the signal to read
            /// @param domain [in] Topology domain to read the signal
            ///               from.  One of the geopm_domain_e values
            ///               defined in geopm_topo.h.
            /// @param domain_idx [in] Index of the domain to read the
            ///                   signal from.
            virtual double platform_read_signal(const std::string &signal_name,
                                                int domain,
                                                int domain_idx) = 0;
            /// @brief Calls the PlatformWriteControl API defined in the
            ///        io.github.geopm D-Bus namespace.
            /// @param control_name [in] Name of the control to write
            /// @param domain [in] Topology domain to write to.  One
            ///               of the geopm_domain_e values defined in
            ///               geopm_topo.h.
            /// @param domain_idx [in] Index of the domain to write
            ///                   to.
            /// @param setting [in] Value of the control to write.
            virtual void platform_write_control(const std::string &control_name,
                                                int domain,
                                                int domain_idx,
                                                double setting) = 0;
            virtual void platform_start_profile(const std::string &profile_name) = 0;
            virtual void platform_stop_profile(void) = 0;
            virtual std::vector<int> platform_get_profile_pids(const std::string &profile_name) = 0;
    };

    class ServiceProxyImp : public ServiceProxy
    {
        public:
            ServiceProxyImp();
            ServiceProxyImp(std::shared_ptr<SDBus> bus);
            virtual ~ServiceProxyImp() = default;
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
            void platform_start_profile(const std::string &profile_name) override;
            void platform_stop_profile(void) override;
            std::vector<int> platform_get_profile_pids(const std::string &profile_name) override;
        private:
            std::vector<std::string> read_string_array(std::shared_ptr<SDBusMessage> bus_message);
            std::shared_ptr<SDBus> m_bus;
    };
}

#endif
