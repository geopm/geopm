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

#ifndef MOCKAGENT_HPP_INCLUDE
#define MOCKAGENT_HPP_INCLUDE

#include "gmock/gmock.h"

#include "Agent.hpp"
#include "PlatformIO.hpp"

class MockAgent : public geopm::Agent
{
    public:
        MOCK_METHOD3(init,
                     void(int level, const std::vector<int> &fan_in, bool is_level_root));
        MOCK_CONST_METHOD1(validate_policy,
                           void(std::vector<double> &policy));
        MOCK_METHOD2(split_policy,
                     void(const std::vector<double> &in_policy,
                          std::vector<std::vector<double> >&out_policy));
        MOCK_CONST_METHOD0(do_send_policy,
                           bool(void));
        MOCK_METHOD2(aggregate_sample,
                     void(const std::vector<std::vector<double> > &in_signal,
                          std::vector<double> &out_signal));
        MOCK_CONST_METHOD0(do_send_sample,
                           bool(void));
        MOCK_METHOD1(adjust_platform,
                     void(const std::vector<double> &in_policy));
        MOCK_CONST_METHOD0(do_write_batch,
                           bool(void));
        MOCK_METHOD1(sample_platform,
                     void(std::vector<double> &out_sample));
        MOCK_METHOD0(wait,
                     void(void));
        MOCK_CONST_METHOD0(report_header,
                           std::vector<std::pair<std::string, std::string> >(void));
        MOCK_CONST_METHOD0(report_host,
                           std::vector<std::pair<std::string, std::string> >(void));
        MOCK_CONST_METHOD0(report_region,
                           std::map<uint64_t, std::vector<std::pair<std::string, std::string> > >(void));
        MOCK_CONST_METHOD0(trace_names,
                           std::vector<std::string>(void));
        MOCK_CONST_METHOD0(trace_formats,
                           std::vector<std::function<std::string(double)> >(void));
        MOCK_METHOD1(trace_values,
                     void(std::vector<double> &values));
};

#endif
