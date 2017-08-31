/*
 * Copyright (c) 2015, 2016, 2017, Intel Corporation
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

#ifndef PLATFORMTOPOLOGY_HPP_INCLUDE
#define PLATFORMTOPOLOGY_HPP_INCLUDE

#include <vector>
#include <map>
#include <hwloc.h>

namespace geopm
{
    /// @brief Platform resource types.
    enum geopm_domain_type_e {
        /// @brief Group of MPI processes used for control
        GEOPM_DOMAIN_PROCESS_GROUP,
        /// @brief Coherent memory domain
        GEOPM_DOMAIN_BOARD,
        /// @brief Single processor package
        GEOPM_DOMAIN_PACKAGE,
        /// @brief All CPU's within a package
        GEOPM_DOMAIN_PACKAGE_CORE,
        /// @brief Everything on package other than the cores
        GEOPM_DOMAIN_PACKAGE_UNCORE,
        /// @brief Single processing unit
        GEOPM_DOMAIN_CPU,
        /// @brief Standard off package DIMM (DRAM or NAND)
        GEOPM_DOMAIN_BOARD_MEMORY,
        /// @brief On package memory (MCDRAM)
        GEOPM_DOMAIN_PACKAGE_MEMORY,
        /// @brief Network interface controller
        GEOPM_DOMAIN_NIC,
        /// @brief Software defined grouping of tiles
        GEOPM_DOMAIN_TILE_GROUP,
        /// @brief Group of CPU's that share a cache
        GEOPM_DOMAIN_TILE,
    };

    /// @brief This class is a wrapper around hwloc. It holds the topology of
    /// hardware resources of the platform.
    class IPlatformTopology
    {
        public:
            IPlatformTopology() {}
            IPlatformTopology(const IPlatformTopology &other) {}
            virtual ~IPlatformTopology() {}
            /// @brief Retrieve the count of a specific hwloc resource type.
            /// @param [in] domain_type Enum of type domain_type_e representing the
            /// type of resource to query.
            /// @return Count of the specified resource type.
            virtual int num_domain(int domain_type) const = 0;
    };

    class PlatformTopology : public IPlatformTopology
    {
        public:
            /// @brief Default constructor initializes and builds
            /// the hwloc tree.
            PlatformTopology();
            PlatformTopology(const PlatformTopology &other);
            /// @brief Default destructor destroys the hwloc tree.
            virtual ~PlatformTopology();
            virtual int num_domain(int domain_type) const;
        private:
            virtual hwloc_obj_type_t hwloc_domain(int domain_type) const;
            /// @brief Holds the hwloc topology tree.
            hwloc_topology_t m_topo;
    };
}

#endif
