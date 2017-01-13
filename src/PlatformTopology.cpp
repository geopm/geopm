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

#include <sstream>
#include "Exception.hpp"
#include "PlatformTopology.hpp"
#include "config.h"

namespace geopm
{
    static const std::map<int, hwloc_obj_type_t> &domain_hwloc_map(void)
    {
        static const std::map<int, hwloc_obj_type_t> hwloc_map = {
#ifdef GEOPM_HWLOC_HAS_SOCKET
            {GEOPM_DOMAIN_PACKAGE, HWLOC_OBJ_SOCKET},
#else
            {GEOPM_DOMAIN_PACKAGE, HWLOC_OBJ_PACKAGE},
#endif
#ifdef GEOPM_HWLOC_HAS_L2CACHE
            {GEOPM_DOMAIN_TILE, HWLOC_OBJ_L2CACHE},
#endif
            {GEOPM_DOMAIN_PROCESS_GROUP, HWLOC_OBJ_SYSTEM},
            {GEOPM_DOMAIN_BOARD, HWLOC_OBJ_MACHINE},
            {GEOPM_DOMAIN_PACKAGE_CORE, HWLOC_OBJ_CORE},
            {GEOPM_DOMAIN_CPU, HWLOC_OBJ_PU},
            {GEOPM_DOMAIN_BOARD_MEMORY, HWLOC_OBJ_GROUP}
        };
        return hwloc_map;
    }

    PlatformTopology::PlatformTopology()
    {
        int err = hwloc_topology_init(&m_topo);
        if (err) {
            throw Exception("PlatformTopology: error returned by hwloc_topology_init()",
                            GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
        }
        err = hwloc_topology_load(m_topo);
        if (err) {
            throw Exception("PlatformTopology: error returned by hwloc_topology_load()",
                            GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
        }
    }

#ifdef HWLOC_HAS_TOPOLOGY_DUP
    PlatformTopology::PlatformTopology(const PlatformTopology &other)
    {
        int err = hwloc_topology_dup(&m_topo, other.m_topo);
        if (err) {
            throw Exception("PlatformTopology: error returned by hwloc_topology_dup()",
                            GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
        }
    }
#else
    PlatformTopology::PlatformTopology(const PlatformTopology &other)
        : PlatformTopology()
    {

    }
#endif

    PlatformTopology::~PlatformTopology()
    {
        /// @todo: This was failing internally on Catalyst. Need to debug.
        /// Thre is a ptr=malloc(0); free(ptr); in hwloc which alarmed ElectricFence.
        hwloc_topology_destroy(m_topo);
    }

    int PlatformTopology::num_domain(int domain_type) const
    {
        int result = 0;

        try {
            result = hwloc_get_nbobjs_by_type(m_topo, hwloc_domain(domain_type));
        }
        catch (Exception ex) {
            if (ex.err_value() != GEOPM_ERROR_INVALID) {
                throw ex;
            }
            if (domain_type == GEOPM_DOMAIN_TILE) {
                /// @todo: This assumes that tiles are just below
                ///        package in hwloc hierarchy.  If tiles are
                ///        at L2 cache, but processor has an L3 cache,
                ///        this may not be correct.
                int depth = hwloc_get_type_depth(m_topo, hwloc_domain(GEOPM_DOMAIN_PACKAGE)) + 1;
                result = hwloc_get_nbobjs_by_depth(m_topo, depth);
            }
            else {
                throw ex;
            }
            if (result == 0) {
                throw ex;
            }
        }
        return result;
    }

    hwloc_obj_type_t PlatformTopology::hwloc_domain(int domain_type) const
    {
        auto it = domain_hwloc_map().find(domain_type);
        if (it == domain_hwloc_map().end()) {
            std::ostringstream ex_str;
            ex_str << "PlatformTopology::hwloc_domain(): Domain type unknown: "  << domain_type;
            throw Exception(ex_str.str(), GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        return (*it).second;
    }
}
