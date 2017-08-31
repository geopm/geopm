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

/*! @brief Enum to select the factory type for C interface. */
enum geopm_plugin_type_e {
    GEOPM_PLUGIN_TYPE_DECIDER,
    GEOPM_PLUGIN_TYPE_PLATFORM,
    GEOPM_PLUGIN_TYPE_PLATFORM_IMP,
    GEOPM_PLUGIN_TYPE_COMM,
    GEOPM_NUM_PLUGIN_TYPE
};

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

int geopm_plugin_load(int plugin_type, struct geopm_factory_c *factory);
#ifdef __cplusplus
}

namespace geopm
{
    class IDecider;
    class Platform;
    class PlatformImp;
    class IComm;
}

/// @brief Called within the implementation of geopm_plugin_register()
///        once the Decider object has been created. */
void geopm_factory_register(struct geopm_factory_c *factory, geopm::IDecider *decider, void *dl_ptr);
/// @brief Called within the implementation of geopm_plugin_register()
///        once the Platform object has been created. */
void geopm_factory_register(struct geopm_factory_c *factory, geopm::Platform *platform, void *dl_ptr);
/// @brief Called within the implementation of geopm_plugin_register()
///        once the PlatformImp object has been created. */
void geopm_factory_register(struct geopm_factory_c *factory, geopm::PlatformImp *platform, void *dl_ptr);
/// @brief Called within the implementation of geopm_plugin_register()
///        once implementations of IComm interface has been created. */
void geopm_factory_register(struct geopm_factory_c *factory, const geopm::IComm *in_comm, void *dl_ptr);
#endif
#endif
