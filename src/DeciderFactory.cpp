/*
 * Copyright (c) 2015, 2016, Intel Corporation
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

#include <inttypes.h>
#include <cpuid.h>
#include <string>
#include <memory>

#include "config.h"
#include "geopm_error.h"
#include "geopm_plugin.h"
#include "Exception.hpp"
#include "Decider.hpp"
#include "DeciderFactory.hpp"


void geopm_factory_register(struct geopm_factory_c *factory, geopm::Decider *decider)
{
    std::unique_ptr<geopm::Decider> p_dec;
    geopm::DeciderFactory *fact_obj = (geopm::DeciderFactory *)(factory);
    if (fact_obj == NULL) {
        throw geopm::Exception(GEOPM_ERROR_FACTORY_NULL, __FILE__, __LINE__);
    }
    p_dec = std::unique_ptr<geopm::Decider>(decider);
    fact_obj->register_decider(move(p_dec));
}

namespace geopm
{

    DeciderFactory::DeciderFactory()
    {
        // register all the deciders we know about
        geopm_plugin_load(GEOPM_PLUGIN_TYPE_DECIDER, (struct geopm_factory_c *)this);
    }

    DeciderFactory::DeciderFactory(std::unique_ptr<Decider> decider)
    {
        register_decider(move(decider));
    }

    DeciderFactory::~DeciderFactory()
    {
        for (auto it = deciders.rbegin(); it != deciders.rend(); ++it) {
            delete *it;
        }
        deciders.clear();
    }

    Decider* DeciderFactory::decider(const std::string &description)
    {
        Decider *result = NULL;
        for (auto it = deciders.begin(); it != deciders.end(); ++it) {
            if (*it != NULL &&
                (*it)->decider_supported(description)) {
                result =  *it;
                break;
            }
        }
        if (!result) {
            // If we get here, no acceptable decider was found
            throw Exception("decider: " + description, GEOPM_ERROR_DECIDER_UNSUPPORTED, __FILE__, __LINE__);
        }

        return result;
    }

    void DeciderFactory::register_decider(std::unique_ptr<Decider> decider)
    {
        deciders.push_back(decider.release());
    }
}
