/*
 * Copyright (c) 2015 - 2023, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef MOCKSDBUS_HPP_INCLUDE
#define MOCKSDBUS_HPP_INCLUDE

#include "gmock/gmock.h"

#include "SDBus.hpp"

class MockSDBus : public geopm::SDBus
{
    public:
        MOCK_METHOD(std::shared_ptr<geopm::SDBusMessage>, call_method,
                    (std::shared_ptr<geopm::SDBusMessage> message), (override));
        MOCK_METHOD(std::shared_ptr<geopm::SDBusMessage>, call_method,
                    (const std::string &member), (override));
        MOCK_METHOD(std::shared_ptr<geopm::SDBusMessage>, call_method,
                    (const std::string &member,
                     const std::string &arg0, int arg1, int arg2), (override));
        MOCK_METHOD(std::shared_ptr<geopm::SDBusMessage>, call_method,
                    (const std::string &member,
                     const std::string &arg0, int arg1, int arg2, double arg3),
                    (override));
        MOCK_METHOD(std::shared_ptr<geopm::SDBusMessage>, call_method,
                    (const std::string &member, int arg0),
                    (override));
        MOCK_METHOD(std::shared_ptr<geopm::SDBusMessage>, make_call_message,
                    (const std::string &member), (override));
        MOCK_METHOD(std::shared_ptr<geopm::SDBusMessage>, call_method,
                    (const std::string &member, const std::string  &arg0),
                    (override));
};

#endif
