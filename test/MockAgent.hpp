/*
 * Copyright (c) 2015, 2016, 2017, 2018, 2019, Intel Corporation
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

#include "Agent.hpp"
#include "PlatformIO.hpp"

class MockAgent : public geopm::Agent
{
    public:
        MOCK_METHOD3(init,
                     void(int level, const std::vector<int> &fan_in, bool is_level_root));
        MOCK_CONST_METHOD1(validate_policy,
                           void(std::vector<double> &policy));
        MOCK_METHOD2(descend,
                     bool(const std::vector<double> &in_policy,
                          std::vector<std::vector<double> >&out_policy));
        MOCK_METHOD2(ascend,
                     bool(const std::vector<std::vector<double> > &in_signal,
                          std::vector<double> &out_signal));
        MOCK_METHOD1(adjust_platform,
                     bool(const std::vector<double> &in_policy));
        MOCK_METHOD1(sample_platform,
                     bool(std::vector<double> &out_sample));
        MOCK_METHOD0(wait,
                     void(void));
        MOCK_CONST_METHOD0(report_header,
                           std::vector<std::pair<std::string, std::string> >(void));
        MOCK_CONST_METHOD0(report_node,
                           std::vector<std::pair<std::string, std::string> >(void));
        MOCK_CONST_METHOD0(report_region,
                           std::map<uint64_t, std::vector<std::pair<std::string, std::string> > >(void));
        MOCK_CONST_METHOD0(trace_names,
                           std::vector<std::string>(void));
        MOCK_METHOD1(trace_values,
                     void(std::vector<double> &values));
};

#endif
