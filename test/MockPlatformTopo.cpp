/*
 * Copyright (c) 2015, 2016, 2017, 2018, 2019, 2020, Intel Corporation
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
#include <iostream>
#include "MockPlatformTopo.hpp"

#include "Exception.hpp"

using geopm::Exception;
using ::testing::Return;

std::shared_ptr<MockPlatformTopo> make_topo(int num_package, int num_core, int num_cpu)
{
    if (num_core % num_package != 0 || num_cpu % num_core != 0) {
        throw Exception("Cannot make MockPlatformTopo unless packages/cores/cpus divide evenly.",
                        GEOPM_ERROR_INVALID, __FILE__, __LINE__);
    }

    std::shared_ptr<MockPlatformTopo> topo = std::make_shared<MockPlatformTopo>();

    // expectations for num_domain
    ON_CALL(*topo, num_domain(GEOPM_DOMAIN_BOARD))
        .WillByDefault(Return(1));
    ON_CALL(*topo, num_domain(GEOPM_DOMAIN_PACKAGE))
        .WillByDefault(Return(num_package));
    ON_CALL(*topo, num_domain(GEOPM_DOMAIN_BOARD_MEMORY))
        .WillByDefault(Return(num_package));
    ON_CALL(*topo, num_domain(GEOPM_DOMAIN_CORE))
        .WillByDefault(Return(num_core));
    ON_CALL(*topo, num_domain(GEOPM_DOMAIN_CPU))
        .WillByDefault(Return(num_cpu));

    // expectations for is_nested_domain
    ON_CALL(*topo, is_nested_domain(GEOPM_DOMAIN_CPU, GEOPM_DOMAIN_BOARD))
        .WillByDefault(Return(true));
    ON_CALL(*topo, is_nested_domain(GEOPM_DOMAIN_CPU, GEOPM_DOMAIN_BOARD_MEMORY))
        .WillByDefault(Return(true));
    ON_CALL(*topo, is_nested_domain(GEOPM_DOMAIN_CPU, GEOPM_DOMAIN_PACKAGE))
        .WillByDefault(Return(true));
    ON_CALL(*topo, is_nested_domain(GEOPM_DOMAIN_CPU, GEOPM_DOMAIN_CORE))
        .WillByDefault(Return(true));
    ON_CALL(*topo, is_nested_domain(GEOPM_DOMAIN_CORE, GEOPM_DOMAIN_BOARD))
        .WillByDefault(Return(true));
    ON_CALL(*topo, is_nested_domain(GEOPM_DOMAIN_CORE, GEOPM_DOMAIN_PACKAGE))
        .WillByDefault(Return(true));
    ON_CALL(*topo, is_nested_domain(GEOPM_DOMAIN_PACKAGE, GEOPM_DOMAIN_BOARD))
        .WillByDefault(Return(true));
    ON_CALL(*topo, is_nested_domain(GEOPM_DOMAIN_BOARD_MEMORY, GEOPM_DOMAIN_BOARD))
        .WillByDefault(Return(true));

    // expectations for domain_nested
    std::vector<std::set<int> > package_cores(num_package);
    std::vector<std::set<int> > package_cpus(num_package);
    std::vector<std::set<int> > core_cpus(num_core);
    std::set<int> all_pkgs;
    std::set<int> all_cores;
    std::set<int> all_cpus;
    for (int package_idx = 0; package_idx < num_package; ++package_idx) {
        all_pkgs.insert(package_idx);
    }
    int core_per_package = num_core / num_package;
    for (int core_idx = 0; core_idx < num_core; ++core_idx) {
        package_cores[core_idx / core_per_package].insert(core_idx);
        all_cores.insert(core_idx);
    }
    for (int cpu_idx = 0; cpu_idx < num_cpu; ++cpu_idx) {
        int core_idx = cpu_idx % num_core;
        package_cpus[core_idx / core_per_package].insert(cpu_idx);
        core_cpus[core_idx].insert(cpu_idx);
        all_cpus.insert(cpu_idx);
    }

    ON_CALL(*topo, domain_nested(GEOPM_DOMAIN_CPU, GEOPM_DOMAIN_BOARD, 0))
        .WillByDefault(Return(all_cpus));
    ON_CALL(*topo, domain_nested(GEOPM_DOMAIN_CORE, GEOPM_DOMAIN_BOARD, 0))
        .WillByDefault(Return(all_cores));
    ON_CALL(*topo, domain_nested(GEOPM_DOMAIN_PACKAGE, GEOPM_DOMAIN_BOARD, 0))
        .WillByDefault(Return(all_pkgs));
    // for now assume board_memory is the same as package
    ON_CALL(*topo, domain_nested(GEOPM_DOMAIN_BOARD_MEMORY, GEOPM_DOMAIN_BOARD, 0))
        .WillByDefault(Return(all_pkgs));

    for (int package_idx = 0; package_idx < num_package; ++package_idx) {
        ON_CALL(*topo, domain_nested(GEOPM_DOMAIN_CPU, GEOPM_DOMAIN_PACKAGE, package_idx))
            .WillByDefault(Return(package_cpus[package_idx]));
        ON_CALL(*topo, domain_nested(GEOPM_DOMAIN_CORE, GEOPM_DOMAIN_PACKAGE, package_idx))
            .WillByDefault(Return(package_cores[package_idx]));
    }
    for (int core_idx = 0; core_idx < num_core; ++core_idx) {
        ON_CALL(*topo, domain_nested(GEOPM_DOMAIN_CPU, GEOPM_DOMAIN_CORE, core_idx))
            .WillByDefault(Return(core_cpus[core_idx]));
    }
    for (int cpu_idx = 0; cpu_idx < num_cpu; ++cpu_idx) {
        ON_CALL(*topo, domain_nested(GEOPM_DOMAIN_CPU, GEOPM_DOMAIN_CPU, cpu_idx))
            .WillByDefault(Return(std::set<int>{cpu_idx}));
    }

    return topo;
}
