/*
 * Copyright (c) 2015 - 2022, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef MOCKGPUTOPO_HPP_INCLUDE
#define MOCKGPUTOPO_HPP_INCLUDE

#include "gmock/gmock.h"

#include "GPUTopo.hpp"

class MockGPUTopo : public geopm::GPUTopo
{
    public:
        MOCK_METHOD(int, num_gpu, (), (const, override));
        MOCK_METHOD(int, num_gpu, (int), (const, override));
        MOCK_METHOD(std::set<int>, cpu_affinity_ideal, (int), (const, override));
        MOCK_METHOD(std::set<int>, cpu_affinity_ideal, (int, int), (const, override));
};

#endif
