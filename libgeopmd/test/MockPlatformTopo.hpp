/*
 * Copyright (c) 2015 - 2024, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef MOCKPLATFORMTOPO_HPP_INCLUDE
#define MOCKPLATFORMTOPO_HPP_INCLUDE

#include <memory>

#include "gmock/gmock.h"

#include "geopm/PlatformTopo.hpp"

class MockPlatformTopo : public geopm::PlatformTopo
{
    public:
        MOCK_METHOD(int, num_domain, (int domain_type), (const, override));
        MOCK_METHOD(int, domain_idx, (int domain_type, int cpu_idx), (const, override));
        MOCK_METHOD(bool, is_nested_domain,
                    (int inner_domain, int outer_domain), (const, override));
        MOCK_METHOD(std::set<int>, domain_nested,
                    (int inner_domain, int outer_domain, int outer_idx),
                    (const, override));
};

/// Create a MockPlatformTopo and set up expectations for the system hierarchy.
/// Counts for each input component are for the whole board.
std::shared_ptr<MockPlatformTopo> make_topo(int num_package, int num_core, int num_cpu);

#endif
