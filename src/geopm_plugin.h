/*
 * Copyright (c) 2015, 2016, 2017, 2018, Intel Corporation
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

#ifndef GEOPM_PLUGIN_H_INCLUDE
#define GEOPM_PLUGIN_H_INCLUDE

#ifndef NAME_MAX
#define NAME_MAX 1024
#endif

#ifdef __cplusplus
extern "C" {
#endif

/*! @brief Opaque C structure that is a handle for a specific Factory
           object. */
struct geopm_factory_c;

/// @brief Structure holding plugin selection strings.
struct geopm_plugin_description_s {
    // @brief tree decider description
    char tree_decider[NAME_MAX];
    /// @brief leaf decider description
    char leaf_decider[NAME_MAX];
    /// @brief platform description
    char platform[NAME_MAX];
};


/*! @brief Declaration for function which must be defined by a plugin
           implementor which will register the plugin for the type
           specified. */
int geopm_plugin_register(int plugin_type, struct geopm_factory_c *factory, void *dl_ptr);

#ifdef __cplusplus
}

#include "Decider.hpp"
#include "Platform.hpp"
#include "PlatformImp.hpp"
#include "Comm.hpp"

void geopm_decider_plugin_register(geopm::IDecider *decider);
void geopm_comm_plugin_register(geopm::IComm *comm);

#endif
#endif
