/*
 * Copyright (c) 2015 - 2022, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef MOCKSYSFSDRIVER_HPP_INCLUDE
#define MOCKSYSFSDRIVER_HPP_INCLUDE

#include "SysfsDriver.hpp"

#include "gmock/gmock.h"

class MockSysfsDriver : public geopm::SysfsDriver
{
    public:
        MOCK_METHOD(int, domain_type,
                    (const std::string &name),
                    (const, override));
        MOCK_METHOD(std::string, attribute_path,
                    (const std::string &name, int domain_idx),
                    (override));
        MOCK_METHOD(std::function<double(const std::string &)>, signal_parse,
                    (const std::string &signal_name),
                    (const, override));
        MOCK_METHOD(std::function<std::string(double)>, control_gen,
                    (const std::string &control_name),
                    (const, override));
        MOCK_METHOD(std::string, driver,
                    (),
                    (const, override));
        MOCK_METHOD((std::map<std::string, SysfsDriver::properties_s>), properties,
                    (),
                    (const, override));
};

#endif /* MOCKSYSFSDRIVER_HPP_INCLUDE */
