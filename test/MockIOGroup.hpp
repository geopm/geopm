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

#ifndef MOCKIOGROUP_HPP_INCLUDE
#define MOCKIOGROUP_HPP_INCLUDE

#include "gmock/gmock.h"

#include "IOGroup.hpp"

class MockIOGroup : public geopm::IOGroup
{
    public:
        MOCK_CONST_METHOD0(signal_names,
                           std::set<std::string>(void));
        MOCK_CONST_METHOD0(control_names,
                           std::set<std::string>(void));
        MOCK_CONST_METHOD1(is_valid_signal,
                           bool (const std::string &signal_name));
        MOCK_CONST_METHOD1(is_valid_control,
                           bool (const std::string &control_name));
        MOCK_CONST_METHOD1(signal_domain_type,
                           int (const std::string &signal_name));
        MOCK_CONST_METHOD1(control_domain_type,
                           int (const std::string &control_name));
        MOCK_METHOD3(push_signal,
                     int (const std::string &signal_name, int domain_type, int domain_idx));
        MOCK_METHOD3(push_control,
                     int (const std::string &control_name, int domain_type, int domain_idx));
        MOCK_METHOD0(read_batch,
                     void (void));
        MOCK_METHOD0(write_batch,
                     void (void));
        MOCK_METHOD1(sample,
                     double (int sample_idx));
        MOCK_METHOD2(adjust,
                     void (int control_idx, double setting));
        MOCK_METHOD3(read_signal,
                     double (const std::string &signal_name, int domain_type, int domain_idx));
        MOCK_METHOD4(write_control,
                     void (const std::string &control_name, int domain_type, int domain_idx, double setting));
        MOCK_METHOD0(save_control,
                     void(void));
        MOCK_METHOD0(restore_control,
                     void(void));
        MOCK_CONST_METHOD1(agg_function,
                           std::function<double(const std::vector<double> &)> (const std::string &signal_name));
        MOCK_CONST_METHOD1(format_function,
                           std::function<std::string(double)> (const std::string &signal_name));
        MOCK_CONST_METHOD1(signal_description,
                           std::string(const std::string &signal_name));
        MOCK_CONST_METHOD1(control_description,
                           std::string(const std::string &control_name));
        MOCK_CONST_METHOD1(signal_behavior,
                           int(const std::string &signal_name));
};

#endif
