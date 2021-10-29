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
        MOCK_METHOD(void, append_request_s,
                    (const geopm_request_s &request), (override));
        MOCK_METHOD(bool, was_success, (), (override));
};

#endif
