/*
 * Copyright (c) 2015 - 2022, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef MOCKACCELERATORTOPO_HPP_INCLUDE
#define MOCKACCELERATORTOPO_HPP_INCLUDE

#include "gmock/gmock.h"

#include "AcceleratorTopo.hpp"

class MockAcceleratorTopo : public geopm::AcceleratorTopo
{
    public:
        MOCK_METHOD(int, num_accelerator, (), (const, override));
        MOCK_METHOD(int, num_accelerator, (int), (const, override));
        MOCK_METHOD(std::set<int>, cpu_affinity_ideal, (int), (const, override));
        MOCK_METHOD(std::set<int>, cpu_affinity_ideal, (int, int), (const, override));
};

#endif
