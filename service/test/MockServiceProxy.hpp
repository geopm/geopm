/*
 * Copyright (c) 2015 - 2023, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef MOCKSERVICEPROXY_HPP_INCLUDE
#define MOCKSERVICEPROXY_HPP_INCLUDE

#include "gmock/gmock.h"
#include "geopm/ServiceProxy.hpp"
#include "geopm/PlatformIO.hpp"

class MockServiceProxy : public geopm::ServiceProxy
{
    public:
        MOCK_METHOD(void, platform_get_user_access,
                    (std::vector<std::string> &signal_names,
                     std::vector<std::string> &control_names), (override));
        MOCK_METHOD(std::vector<geopm::signal_info_s>, platform_get_signal_info,
                    (const std::vector<std::string> &signal_names), (override));
        MOCK_METHOD(std::vector<geopm::control_info_s>, platform_get_control_info,
                    (const std::vector<std::string> &control_names), (override));
        MOCK_METHOD(void, platform_open_session, (), (override));
        MOCK_METHOD(void, platform_close_session, (), (override));
        MOCK_METHOD(void, platform_start_batch,
                    (const std::vector<struct geopm_request_s> &signal_config,
                     const std::vector<struct geopm_request_s> &control_config,
                     int &server_pid, std::string &server_key), (override));
        MOCK_METHOD(void, platform_stop_batch, (int server_pid), (override));
        MOCK_METHOD(double, platform_read_signal,
                    (const std::string &signal_name, int domain,
                     int domain_idx), (override));
        MOCK_METHOD(void, platform_write_control,
                    (const std::string &control_name, int domain,
                     int domain_idx, double setting), (override));
        MOCK_METHOD(void, platform_start_profile, (const std::string &profile_name),
                    (override));
        MOCK_METHOD(void, platform_stop_profile, (), (override));
        MOCK_METHOD(std::vector<int>, platform_get_profile_pids,
                    (const std::string &profile_name), (override));
};

#endif
