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
        MOCK_METHOD(void, init,
                    (int level, const std::vector<int> &fan_in, bool is_level_root),
                    (override));
        MOCK_METHOD(void, validate_policy, (std::vector<double> & policy),
                    (const, override));
        MOCK_METHOD(void, split_policy,
                    (const std::vector<double> &in_policy,
                     std::vector<std::vector<double> > &out_policy),
                    (override));
        MOCK_METHOD(bool, do_send_policy, (), (const, override));
        MOCK_METHOD(void, aggregate_sample,
                    (const std::vector<std::vector<double> > &in_signal,
                     std::vector<double> &out_signal),
                    (override));
        MOCK_METHOD(bool, do_send_sample, (), (const, override));
        MOCK_METHOD(void, adjust_platform,
                    (const std::vector<double> &in_policy), (override));
        MOCK_METHOD(bool, do_write_batch, (), (const, override));
        MOCK_METHOD(void, sample_platform, (std::vector<double> & out_sample),
                    (override));
        MOCK_METHOD(void, wait, (), (override));
        MOCK_METHOD(std::vector<std::pair<std::string, std::string> >,
                    report_header, (), (const, override));
        MOCK_METHOD(std::vector<std::pair<std::string, std::string> >,
                    report_host, (), (const, override));
        MOCK_METHOD(std::map<uint64_t, std::vector<std::pair<std::string, std::string> > >,
                    report_region, (), (const, override));
        MOCK_METHOD(std::vector<std::string>, trace_names, (), (const, override));
        MOCK_METHOD(std::vector<std::function<std::string, trace_formats, (double)> >(),
                    (const, override));
        MOCK_METHOD(void, trace_values, (std::vector<double> & values), (override));
};

#endif
