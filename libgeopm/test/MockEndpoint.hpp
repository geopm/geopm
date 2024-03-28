/*
 * Copyright (c) 2015 - 2024, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef MOCKENDPOINT_HPP_INCLUDE
#define MOCKENDPOINT_HPP_INCLUDE

#include "gmock/gmock.h"

#include "Endpoint.hpp"

class MockEndpoint : public geopm::Endpoint
{
    public:
        MOCK_METHOD(void, open, (), (override));
        MOCK_METHOD(void, close, (), (override));
        MOCK_METHOD(void, write_policy, (const std::vector<double> &policy),
                    (override));
        MOCK_METHOD(double, read_sample, (std::vector<double> & sample), (override));
        MOCK_METHOD(std::string, get_agent, (), (override));
        MOCK_METHOD(void, wait_for_agent_attach, (double timeout), (override));
        MOCK_METHOD(void, wait_for_agent_detach, (double timeout), (override));
        MOCK_METHOD(void, stop_wait_loop, (), (override));
        MOCK_METHOD(void, reset_wait_loop, (), (override));
        MOCK_METHOD(std::string, get_profile_name, (), (override));
        MOCK_METHOD(std::set<std::string>, get_hostnames, (), (override));
};

#endif
