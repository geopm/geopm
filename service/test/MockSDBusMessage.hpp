/*
 * Copyright (c) 2015 - 2023, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef MOCKSDBUSMESSAGE_HPP_INCLUDE
#define MOCKSDBUSMESSAGE_HPP_INCLUDE

#include "gmock/gmock.h"

#include "SDBusMessage.hpp"

class MockSDBusMessage : public geopm::SDBusMessage
{
    public:
        MOCK_METHOD(sd_bus_message*, get_sd_ptr, (), (override));
        MOCK_METHOD(void, enter_container, (char type,
                                            const std::string &contents),
                    (override));
        MOCK_METHOD(void, exit_container, (), (override));
        MOCK_METHOD(void, open_container, (char type,
                                            const std::string &contents),
                    (override));
        MOCK_METHOD(void, close_container, (), (override));
        MOCK_METHOD(std::string, read_string, (), (override));
        MOCK_METHOD(double, read_double, (), (override));
        MOCK_METHOD(int, read_integer, (), (override));
        MOCK_METHOD(void, append_strings,
                    (const std::vector<std::string> &write_values), (override));
        MOCK_METHOD(void, append_request,
                    (const geopm_request_s &request), (override));
        MOCK_METHOD(bool, was_success, (), (override));
};

#endif
