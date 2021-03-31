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

#ifndef MOCKMSRIO_HPP_INCLUDE
#define MOCKMSRIO_HPP_INCLUDE

#include "gmock/gmock.h"

#include "MSRIO.hpp"

class MockMSRIO : public geopm::MSRIO
{
    public:
        MOCK_METHOD2(read_msr,
                     uint64_t(int cpu_idx, uint64_t offset));
        MOCK_METHOD4(write_msr,
                     void(int cpu_idx, uint64_t offset, uint64_t raw_value, uint64_t write_mask));
        MOCK_METHOD2(add_read,
                     int(int cpu_idx, uint64_t offset));
        MOCK_METHOD0(read_batch,
                     void(void));
        MOCK_CONST_METHOD1(sample,
                           uint64_t(int batch_idx));
        MOCK_METHOD2(add_write,
                     int(int cpu_idx, uint64_t offset));
        MOCK_METHOD3(adjust,
                     void(int batch_idx, uint64_t value, uint64_t write_mask));
        MOCK_METHOD0(write_batch,
                     void(void));
};

#endif
