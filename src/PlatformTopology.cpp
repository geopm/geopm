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

#include <stdexcept>

#include "PlatformTopology.hpp"

namespace geopm
{
    PlatformTopology::PlatformTopology()
    {
        hwloc_topology_init(&m_topo);
        hwloc_topology_load(m_topo);
    }

    PlatformTopology::~PlatformTopology()
    {
        hwloc_topology_destroy(m_topo);
    }

    int PlatformTopology::num_domain(int domain_type) const
    {
        int result;
        if (domain_type < HWLOC_OBJ_TYPE_MAX) {
            result = hwloc_get_nbobjs_by_type(m_topo, (hwloc_obj_type_t)domain_type);
        }
        else {
            throw(std::invalid_argument("Type index out of boungs.  PlatformTopology supports hwloc defined objects only.\n"));
        }
        return result;
    }

    void PlatformTopology::domain_by_type(int domain_type, std::vector<hwloc_obj_t> &domain) const
    {
        hwloc_obj_t dom;

        if (!domain.empty()) {
            domain.clear();
        }

        if (domain_type < HWLOC_OBJ_TYPE_MAX) {
            dom = hwloc_get_next_obj_by_type(m_topo, (hwloc_obj_type_t)domain_type, NULL);
            while (dom) {
                domain.push_back(dom);
                dom = hwloc_get_next_obj_by_type(m_topo, (hwloc_obj_type_t)domain_type, dom);
            }
        }
        else {
            throw(std::invalid_argument("Type index out of boungs.  PlatformTopology supports hwloc defined objects only.\n"));
        }
    }
}
