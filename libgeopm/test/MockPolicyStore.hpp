/*
 * Copyright (c) 2015 - 2024, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef MOCKPOLICYSTORE_HPP_INCLUDE
#define MOCKPOLICYSTORE_HPP_INCLUDE

#include "gmock/gmock.h"

#include "PolicyStore.hpp"

class MockPolicyStore : public geopm::PolicyStore
{
    public:
        MOCK_METHOD(std::vector<double>, get_best,
                    (const std::string &profile_name, const std::string &agent_name),
                    (const, override));
        MOCK_METHOD(void, set_best,
                    (const std::string &profile_name, const std::string &agent_name,
                     const std::vector<double> &policy),
                    (override));
        MOCK_METHOD(void, set_default,
                    (const std::string &agent_name, const std::vector<double> &policy),
                    (override));
};

#endif
