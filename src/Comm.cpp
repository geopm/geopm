/*
 * Copyright (c) 2015 - 2023, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "config.h"

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


    std::vector<std::string> Comm::comm_names(void)
    {
        return comm_factory().plugin_names();
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
