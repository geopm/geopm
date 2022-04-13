/*
 * Copyright (c) 2015 - 2022, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef MOCKSSTIOCTL_HPP_INCLUDE
#define MOCKSSTIOCTL_HPP_INCLUDE

#include "gmock/gmock.h"

#include "SSTIoctl.hpp"

class MockSSTIoctl : public geopm::SSTIoctl
{
    public:
        virtual ~MockSSTIoctl() = default;

        MOCK_METHOD(int, version, (geopm::sst_version_s *version), (override));
        MOCK_METHOD(int, get_cpu_id, (geopm::sst_cpu_map_interface_batch_s *cpu_batch), (override));
        MOCK_METHOD(int, mbox, (geopm::sst_mbox_interface_batch_s *mbox_batch), (override));
        MOCK_METHOD(int, mmio, (geopm::sst_mmio_interface_batch_s *mmio_batch), (override));
};

#endif
