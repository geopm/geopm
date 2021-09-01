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

#ifndef MOCKSSTIO_HPP_INCLUDE
#define MOCKSSTIO_HPP_INCLUDE

#include "gmock/gmock.h"

#include "SSTIO.hpp"

class MockSSTIO : public geopm::SSTIO
{
    public:
        virtual ~MockSSTIO() = default;
        MOCK_METHOD(int, add_mbox_read,
                    (uint32_t cpu_index, uint16_t command, uint16_t subcommand,
                     uint32_t subcommand_arg),
                    (override));
        MOCK_METHOD(int, add_mbox_write,
                    (uint32_t cpu_index, uint16_t command, uint16_t subcommand,
                     uint32_t interface_parameter, uint16_t read_subcommand,
                     uint32_t read_interface_parameter, uint32_t read_mask),
                    (override));
        MOCK_METHOD(int, add_mmio_read,
                    (uint32_t cpu_index, uint16_t register_offset), (override));
        MOCK_METHOD(int, add_mmio_write,
                    (uint32_t cpu_index, uint16_t register_offset,
                     uint32_t register_value, uint32_t read_mask),
                    (override));
        MOCK_METHOD(void, read_batch, (), (override));
        MOCK_METHOD(uint64_t, sample, (int batch_idx), (const, override));
        MOCK_METHOD(void, write_batch, (), (override));
        MOCK_METHOD(uint32_t, read_mbox_once,
                    (uint32_t cpu_index, uint16_t command, uint16_t subcommand,
                     uint32_t subcommand_arg),
                    (override));
        MOCK_METHOD(void, write_mbox_once,
                    (uint32_t cpu_index, uint16_t command, uint16_t subcommand,
                     uint32_t interface_parameter, uint16_t read_subcommand,
                     uint32_t read_interface_parameter, uint32_t read_mask,
                     uint64_t write_value, uint64_t write_mask),
                    (override));
        MOCK_METHOD(uint32_t, read_mmio_once,
                    (uint32_t cpu_index, uint16_t register_offset), (override));
        MOCK_METHOD(void, write_mmio_once,
                    (uint32_t cpu_index, uint16_t register_offset, uint32_t register_value,
                     uint32_t read_mask, uint64_t write_value, uint64_t write_mask),
                    (override));
        MOCK_METHOD(void, adjust,
                    (int batch_idx, uint64_t write_value, uint64_t write_mask),
                    (override));
        MOCK_METHOD(uint32_t, get_punit_from_cpu, (uint32_t cpu_index), (override));
};

#endif
