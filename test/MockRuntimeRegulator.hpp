/*
 * Copyright (c) 2015, 2016, 2017, 2018, 2019 Intel Corporation
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

#ifndef MOCKRUNTIMEREGULATOR_HPP_INCLUDE
#define MOCKRUNTIMEREGULATOR_HPP_INCLUDE

#include "gtest/gtest.h"
#include "gmock/gmock.h"

#include "RuntimeRegulator.hpp"

class MockRuntimeRegulator : public geopm::IRuntimeRegulator
{
    public:
        MOCK_METHOD2(record_entry,
                     void(int rank, struct geopm_time_s entry_time));
        MOCK_METHOD2(record_exit,
                     void(int rank, struct geopm_time_s exit_time));
        MOCK_METHOD1(insert_runtime_signal,
                     void(std::vector<struct geopm_telemetry_message_s> &telemetry));
        MOCK_CONST_METHOD0(per_rank_last_runtime,
                     std::vector<double>());
        MOCK_CONST_METHOD0(per_rank_total_runtime,
                     std::vector<double>());
        MOCK_CONST_METHOD0(per_rank_count,
                     std::vector<size_t>());
};

#endif
