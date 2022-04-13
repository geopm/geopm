/*
 * Copyright (c) 2015 - 2022, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
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
