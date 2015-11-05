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

#ifndef GEOPM_PLUGIN_H_INCLUDE
#define GEOPM_PLUGIN_H_INCLUDE

#ifdef __cplusplus
extern "C" {
#endif

/* opaque structure that is a handle for a specific Factory object. */
struct geopm_factory_c;

int geopm_plugins_load(const char *func_name,
                       struct geopm_factory_c *factory);

#ifdef __cplusplus
}
#include "DeciderFactory.hpp"
#include "PlatformFactory.hpp"
#include "Exception.hpp"

static inline void geopm_decider_factory_register(struct geopm_factory_c *factory, geopm::Decider *decider)
{
    std::unique_ptr<geopm::Decider> p_dec;
    geopm::DeciderFactory *fact_obj = (geopm::DeciderFactory *)(factory);
    if (fact_obj == NULL) {
        throw geopm::Exception(GEOPM_ERROR_FACTORY_NULL, __FILE__, __LINE__);
    }
    p_dec = std::unique_ptr<geopm::Decider>(decider);
    fact_obj->register_decider(move(p_dec));
}

static inline void geopm_platform_factory_register(struct geopm_factory_c *factory, geopm::Platform *platform)
{
    std::unique_ptr<geopm::Platform> p_plat;
    geopm::PlatformFactory *fact_obj = (geopm::PlatformFactory *)(factory);
    if (fact_obj == NULL) {
        throw geopm::Exception(GEOPM_ERROR_FACTORY_NULL, __FILE__, __LINE__);
    }
    p_plat = std::unique_ptr<geopm::Platform>(platform);
    fact_obj->register_platform(move(p_plat));
}

static inline void geopm_platform_factory_register(struct geopm_factory_c *factory, geopm::PlatformImp *platform)
{
    std::unique_ptr<geopm::PlatformImp> p_plat;
    geopm::PlatformFactory *fact_obj = (geopm::PlatformFactory *)(factory);
    if (fact_obj == NULL) {
        throw geopm::Exception(GEOPM_ERROR_FACTORY_NULL, __FILE__, __LINE__);
    }
    p_plat = std::unique_ptr<geopm::PlatformImp>(platform);
    fact_obj->register_platform(move(p_plat));
}
#endif
#endif
