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

#include <string>
#include <inttypes.h>
#include <cpuid.h>

#include "geopm_error.h"
#include "Exception.hpp"
#include "PlatformFactory.hpp"
#include "RAPLPlatform.hpp"
#include "IVTPlatformImp.hpp"
#include "HSXPlatformImp.hpp"

namespace geopm
{

    PlatformFactory::PlatformFactory()
    {
        // register all the platforms we know about
        IVTPlatformImp *ivb_plat_imp = new IVTPlatformImp();
        RAPLPlatform *ivb_plat = new RAPLPlatform();
        HSXPlatformImp *hsx_plat_imp = new HSXPlatformImp();
        RAPLPlatform *hsx_plat = new RAPLPlatform();
        std::unique_ptr<Platform> pplat = std::unique_ptr<Platform>(ivb_plat);
        std::unique_ptr<PlatformImp> pplat_imp = std::unique_ptr<PlatformImp>(ivb_plat_imp);
        register_platform(move(pplat), move(pplat_imp));

        pplat = std::unique_ptr<Platform>(hsx_plat);
        pplat_imp = std::unique_ptr<PlatformImp>(hsx_plat_imp);
        register_platform(move(pplat), move(pplat_imp));
    }

    PlatformFactory::PlatformFactory(std::unique_ptr<Platform> platform,
                                     std::unique_ptr<PlatformImp> platform_imp)
    {
        register_platform(move(platform), move(platform_imp));
    }

    PlatformFactory::~PlatformFactory()
    {
        for (auto it = platforms.rbegin(); it != platforms.rend(); ++it) {
            delete it->first;
            delete it->second;
        }
    }

    Platform* PlatformFactory::platform()
    {
        int platform_id;
        Platform *result = NULL;
        platform_id = read_cpuid();
        for (auto it = platforms.begin(); it != platforms.end(); ++it) {
            if (it->second != NULL &&
                it->first->model_supported(platform_id) &&
                it->second->model_supported(platform_id)) {
                it->first->set_implementation(it->second);
                result =  it->first;
                break;
            }
        }
        if (!result) {
            // If we get here, no acceptable platform was found
            throw Exception("cpuid: " + std::to_string(platform_id), GEOPM_ERROR_PLATFORM_UNSUPPORTED, __FILE__, __LINE__);
        }

        return result;
    }

    void PlatformFactory::register_platform(std::unique_ptr<Platform> platform, std::unique_ptr<PlatformImp> platform_imp)
    {
        platforms.insert(std::pair<Platform*, PlatformImp*>(platform.release(), platform_imp.release()));
    }

    int PlatformFactory::read_cpuid()
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
        ext_family = (proc_info & extended_family_mask)>> 20;

        if (family == 6) {
            model+=(ext_model << 4);
        }
        else if (family == 15) {
            model+=(ext_model << 4);
            family+=ext_family;
        }

        return ((family << 8) + model);
    }
}
