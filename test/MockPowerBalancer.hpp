/*
 * Copyright (c) 2015, 2016, 2017, 2018, 2019, 2020, Intel Corporation
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

#ifndef MOCKPOWERBALANCER_HPP_INCLUDE
#define MOCKPOWERBALANCER_HPP_INCLUDE

#include "gmock/gmock.h"

#include "PowerBalancer.hpp"

class MockPowerBalancer : public geopm::PowerBalancer {
    public:
        MOCK_METHOD1(power_cap,
                     void(double cap));
        MOCK_CONST_METHOD0(power_cap,
                           double(void));
        MOCK_CONST_METHOD0(power_limit,
                           double(void));
        MOCK_METHOD1(power_limit_adjusted,
                     void(double limit));
        MOCK_METHOD1(is_runtime_stable,
                     bool(double measured_runtime));
        MOCK_CONST_METHOD0(runtime_sample,
                           double(void));
        MOCK_METHOD0(calculate_runtime_sample,
                     void(void));
        MOCK_METHOD1(target_runtime,
                     void(double largest_runtime));
        MOCK_METHOD1(is_target_met,
                     bool(double measured_runtime));
        MOCK_METHOD1(achieved_limit,
                     void(double achieved));
        MOCK_METHOD0(power_slack,
                     double(void));
};

#endif
