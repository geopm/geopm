/*
 * Copyright (c) 2015, Intel Corporation
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

#ifndef TOPOLOGY_HPP_INCLUDE
#define TOPOLOGY_HPP_INCLUDE

#include <vector>
#include <hwloc.h>

namespace geopm
{
    enum domain_type_e {
        // Group of MPI processes used for control
        GEOPM_DOMAIN_PROCESS_GROUP = HWLOC_OBJ_SYSTEM,
        // Coherent memory domain
        GEOPM_DOMAIN_BOARD = HWLOC_OBJ_MACHINE,
        // Single processor package
        GEOPM_DOMAIN_PACKAGE = HWLOC_OBJ_NODE,
        // All CPU's within a package
        GEOPM_DOMAIN_PACKAGE_CORE = HWLOC_OBJ_SOCKET,
        // Everything on package other than the cores
        GEOPM_DOMAIN_PACKAGE_UNCORE = HWLOC_OBJ_CACHE,
        // Single processing unit
        GEOPM_DOMAIN_CPU = HWLOC_OBJ_PU,
        // Standard off package DIMM (DRAM or NAND)
        GEOPM_DOMAIN_BOARD_MEMORY = HWLOC_OBJ_GROUP,
        // On package memory (MCDRAM)
        GEOPM_DOMAIN_PACKAGE_MEMORY = HWLOC_OBJ_TYPE_MAX,
        // Network interface controller
        GEOPM_DOMAIN_NIC = HWLOC_OBJ_TYPE_MAX + 1,
        // Software defined grouping of tiles
        GEOPM_DOMAIN_TILE_GROUP = HWLOC_OBJ_TYPE_MAX + 2,
        // Group of CPU's that share a cache
        GEOPM_DOMAIN_TILE = HWLOC_OBJ_TYPE_MAX + 3,
    };

    class PlatformTopology
    {
        public:
            PlatformTopology();
            ~PlatformTopology();
            int num_domain(int domain_type) const;
            void domain_by_type(int type, std::vector<hwloc_obj_t> &domain) const;
        private:
            hwloc_topology_t m_topo;
    };
}

#endif
