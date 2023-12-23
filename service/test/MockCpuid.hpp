/*
 * Copyright (c) 2015 - 2023, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef MOCKCPUID_HPP_INCLUDE
#define MOCKCPUID_HPP_INCLUDE

#include "gmock/gmock.h"

#include "Cpuid.hpp"


class MockCpuid : public geopm::Cpuid {
    public:
        MOCK_METHOD(int, cpuid, (), (override, const));
        MOCK_METHOD(bool, is_hwp_supported, (), (override, const));
        MOCK_METHOD(double, freq_sticker, (), (override, const));
        MOCK_METHOD(Cpuid::rdt_info_s, rdt_info, (), (override, const));
        MOCK_METHOD(uint32_t, pmc_bit_width, (), (override, const));
};

#endif
