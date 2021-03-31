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

#ifndef MOCKENDPOINT_HPP_INCLUDE
#define MOCKENDPOINT_HPP_INCLUDE

#include "gmock/gmock.h"

#include "Endpoint.hpp"

class MockEndpoint : public geopm::Endpoint
{
    public:
        MOCK_METHOD0(open,
                     void(void));
        MOCK_METHOD0(close,
                     void(void));
        MOCK_METHOD1(write_policy,
                     void(const std::vector<double> &policy));
        MOCK_METHOD1(read_sample,
                     double(std::vector<double> &sample));
        MOCK_METHOD0(get_agent,
                     std::string(void));
        MOCK_METHOD1(wait_for_agent_attach,
                    void(double timeout));
        MOCK_METHOD1(wait_for_agent_detach,
                    void(double timeout));
        MOCK_METHOD0(stop_wait_loop,
                     void(void));
        MOCK_METHOD0(reset_wait_loop,
                     void(void));
        MOCK_METHOD0(get_profile_name,
                     std::string(void));
        MOCK_METHOD0(get_hostnames,
                     std::set<std::string>(void));
};

#endif
