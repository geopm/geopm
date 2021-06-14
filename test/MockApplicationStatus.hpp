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

#ifndef MOCKAPPLICATIONSTATUS_HPP_INCLUDE
#define MOCKAPPLICATIONSTATUS_HPP_INCLUDE

#include "gmock/gmock.h"

#include "ApplicationStatus.hpp"

class MockApplicationStatus : public geopm::ApplicationStatus
{
    public:
        MOCK_METHOD(void, set_hint, (int cpu_idx, uint64_t hints), (override));
        MOCK_METHOD(uint64_t, get_hint, (int cpu_idx), (const, override));
        MOCK_METHOD(void, set_hash, (int cpu_idx, uint64_t hash, uint64_t hint),
                    (override));
        MOCK_METHOD(uint64_t, get_hash, (int cpu_idx), (const, override));
        MOCK_METHOD(void, reset_work_units, (int cpu_idx), (override));
        MOCK_METHOD(void, set_total_work_units, (int cpu_idx, int work_units),
                    (override));
        MOCK_METHOD(void, increment_work_unit, (int cpu_idx), (override));
        MOCK_METHOD(double, get_progress_cpu, (int cpu_idx), (const, override));
        MOCK_METHOD(void, set_process,
                    (const std::set<int> &cpu_idx, int process), (override));
        MOCK_METHOD(int, get_process, (int cpu_idx), (const, override));
        MOCK_METHOD(void, update_cache, (), (override));
};

#endif
