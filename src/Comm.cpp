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

#include "Comm.hpp"

#include <inttypes.h>
#include <cpuid.h>
#include <string>
#include <sstream>
#include <dlfcn.h>
#include <list>
#include <mutex>
#include <Environment.hpp>
#include <geopm_plugin.hpp>
#include "config.h"
#ifdef GEOPM_ENABLE_MPI
#include "MPIComm.hpp"
#endif

namespace geopm
{
    const std::string Comm::M_PLUGIN_PREFIX = "libgeopmcomm_";

    CommFactory::CommFactory()
    {
#ifdef GEOPM_ENABLE_MPI
        register_plugin(geopm::MPIComm::plugin_name(),
                        geopm::MPIComm::make_plugin);
#endif
    }

    CommFactory &comm_factory(void)
    {
        static CommFactory instance;
        static bool is_once = true;
        static std::once_flag flag;
        if (is_once) {
            is_once = false;
            std::call_once(flag, plugin_load, Comm::M_PLUGIN_PREFIX);
        }
        return instance;
    }

    std::unique_ptr<Comm> Comm::make_unique(const std::string &comm_name)
    {
        return comm_factory().make_plugin(comm_name);
    }

    std::unique_ptr<Comm> Comm::make_unique(void)
    {
        return comm_factory().make_plugin(environment().comm());
    }
}
