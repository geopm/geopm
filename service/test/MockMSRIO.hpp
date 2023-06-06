/*
 * Copyright (c) 2015 - 2023, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef MOCKMSRIO_HPP_INCLUDE
#define MOCKMSRIO_HPP_INCLUDE

#include "gmock/gmock.h"

#include "MSRIO.hpp"

class MockMSRIO : public geopm::MSRIO
{
    public:
        MOCK_METHOD(uint64_t, read_msr, (int cpu_idx, uint64_t offset), (override));
        MOCK_METHOD(void, write_msr,
                    (int cpu_idx, uint64_t offset, uint64_t raw_value, uint64_t write_mask),
                    (override));
        MOCK_METHOD(int, create_batch_context, (), (override));
        MOCK_METHOD(int, add_read, (int cpu_idx, uint64_t offset), (override));
        MOCK_METHOD(int, add_read, (int cpu_idx, uint64_t offset, int batch_ctx), (override));
        MOCK_METHOD(void, read_batch, (), (override));
        MOCK_METHOD(void, read_batch, (int batch_ctx), (override));
        MOCK_METHOD(uint64_t, sample, (int batch_idx), (const, override));
        MOCK_METHOD(uint64_t, sample, (int batch_idx, int batch_ctx), (const, override));
        MOCK_METHOD(int, add_write, (int cpu_idx, uint64_t offset), (override));
        MOCK_METHOD(int, add_write, (int cpu_idx, uint64_t offset, int batch_ctx), (override));
        MOCK_METHOD(void, adjust,
                    (int batch_idx, uint64_t value, uint64_t write_mask), (override));
        MOCK_METHOD(void, adjust,
                    (int batch_idx, uint64_t value, uint64_t write_mask, int batch_ctx), (override));
        MOCK_METHOD(void, write_batch, (), (override));
        MOCK_METHOD(void, write_batch, (int batch_ctx), (override));
};

#endif
