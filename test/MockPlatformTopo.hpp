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

#ifndef MOCKPLATFORMTOPO_HPP_INCLUDE
#define MOCKPLATFORMTOPO_HPP_INCLUDE

#include <memory>

#include "gmock/gmock.h"

#include "PlatformTopo.hpp"

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
