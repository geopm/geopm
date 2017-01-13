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

#include <string>
#include <sstream>
#include <inttypes.h>
#include <cpuid.h>

#include "geopm_plugin.h"
#include "Exception.hpp"
#include "PlatformFactory.hpp"
#include "RAPLPlatform.hpp"
#include "XeonPlatformImp.hpp"
#include "KNLPlatformImp.hpp"
#include "config.h"


void geopm_factory_register(struct geopm_factory_c *factory, geopm::Platform *platform)
{
    geopm::PlatformFactory *fact_obj = (geopm::PlatformFactory *)(factory);
    if (fact_obj == NULL) {
        throw geopm::Exception(GEOPM_ERROR_FACTORY_NULL, __FILE__, __LINE__);
    }
    fact_obj->register_platform(std::unique_ptr<geopm::Platform>(platform));
}

void geopm_factory_register(struct geopm_factory_c *factory, geopm::PlatformImp *platform)
{
    geopm::PlatformFactory *fact_obj = (geopm::PlatformFactory *)(factory);
    if (fact_obj == NULL) {
        throw geopm::Exception(GEOPM_ERROR_FACTORY_NULL, __FILE__, __LINE__);
    }
    fact_obj->register_platform(std::unique_ptr<geopm::PlatformImp>(platform));
}

int geopm_read_cpuid(void)
{
    uint32_t key = 1; //processor features
    uint32_t proc_info = 0;
    uint32_t model;
    uint32_t family;
    uint32_t ext_model;
    uint32_t ext_family;
    uint32_t ebx, ecx, edx;
    const uint32_t model_mask = 0xF0;
    const uint32_t family_mask = 0xF00;
    const uint32_t extended_model_mask = 0xF0000;
    const uint32_t extended_family_mask = 0xFF00000;

    __get_cpuid(key, &proc_info, &ebx, &ecx, &edx);

    model = (proc_info & model_mask) >> 4;
    family = (proc_info & family_mask) >> 8;
    ext_model = (proc_info & extended_model_mask) >> 16;
    ext_family = (proc_info & extended_family_mask) >> 20;

    if (family == 6) {
        model+=(ext_model << 4);
    }
    else if (family == 15) {
        model+=(ext_model << 4);
        family+=ext_family;
    }

    return ((family << 8) + model);
}


namespace geopm
{

    PlatformFactory::PlatformFactory()
    {
        // register all the platforms we know about
        geopm_plugin_load(GEOPM_PLUGIN_TYPE_PLATFORM, (struct geopm_factory_c *)this);
        geopm_plugin_load(GEOPM_PLUGIN_TYPE_PLATFORM_IMP, (struct geopm_factory_c *)this);
        register_platform(std::unique_ptr<Platform>(new RAPLPlatform()));
        register_platform(std::unique_ptr<PlatformImp>(new SNBPlatformImp()));
        register_platform(std::unique_ptr<PlatformImp>(new IVTPlatformImp()));
        register_platform(std::unique_ptr<PlatformImp>(new HSXPlatformImp()));
        register_platform(std::unique_ptr<PlatformImp>(new BDXPlatformImp()));
        register_platform(std::unique_ptr<PlatformImp>(new KNLPlatformImp()));
    }

    PlatformFactory::PlatformFactory(std::unique_ptr<Platform> platform,
                                     std::unique_ptr<PlatformImp> platform_imp)
    {
        register_platform(std::move(platform));
        register_platform(std::move(platform_imp));
    }

    PlatformFactory::~PlatformFactory()
    {
        for (auto it = platforms.rbegin(); it != platforms.rend(); ++it) {
            delete *it;
        }
        for (auto it = platform_imps.rbegin(); it != platform_imps.rend(); ++it) {
            delete *it;
        }
    }

    Platform* PlatformFactory::platform(const std::string &description, bool do_initialize)
    {
        int platform_id;
        bool is_found = false;
        Platform *result = NULL;
        platform_id = read_cpuid();
        for (auto it = platforms.begin(); it != platforms.end(); ++it) {
            if ((*it) != NULL && (*it)->is_model_supported(platform_id, description)) {
                result = (*it);
                break;
            }
        }
        for (auto it = platform_imps.begin(); it != platform_imps.end(); ++it) {
            if ((*it) != NULL && result != NULL &&
                (*it)->is_model_supported(platform_id)) {
                result->set_implementation((*it), do_initialize);
                is_found = true;
                break;
            }
        }
        if (!is_found) {
            result = NULL;
        }
        if (!result) {
            // If we get here, no acceptable platform was found
            std::ostringstream ex_str;
            ex_str << "cpuid: " << platform_id;
            throw Exception(ex_str.str(), GEOPM_ERROR_PLATFORM_UNSUPPORTED, __FILE__, __LINE__);
        }

        return result;
    }

    void PlatformFactory::register_platform(std::unique_ptr<Platform> platform)
    {
        platforms.push_back(platform.release());
    }

    void PlatformFactory::register_platform(std::unique_ptr<PlatformImp> platform_imp)
    {
        platform_imps.push_back(platform_imp.release());
    }

    int PlatformFactory::read_cpuid(void)
    {
        return geopm_read_cpuid();
    }


}
