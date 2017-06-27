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

#include <inttypes.h>
#include <cpuid.h>
#include <string>
#include <sstream>
#include <dlfcn.h>

#include "geopm_plugin.h"
#include "Exception.hpp"
#include "Decider.hpp"
#include "DeciderFactory.hpp"
#include "StaticPolicyDecider.hpp"
#include "config.h"

void geopm_factory_register(struct geopm_factory_c *factory, geopm::IDecider *decider, void *dl_ptr)
{
    geopm::DeciderFactory *fact_obj = (geopm::DeciderFactory *)(factory);
    if (fact_obj == NULL) {
        throw geopm::Exception(GEOPM_ERROR_FACTORY_NULL, __FILE__, __LINE__);
    }
    fact_obj->register_decider(decider, dl_ptr);
}

namespace geopm
{

    DeciderFactory::DeciderFactory()
    {
        // register all the deciders we know about
        geopm_plugin_load(GEOPM_PLUGIN_TYPE_DECIDER, (struct geopm_factory_c *)this);
        register_decider(new StaticPolicyDecider(), NULL);
    }

    DeciderFactory::DeciderFactory(IDecider *decider)
    {
        register_decider(decider, NULL);
    }

    DeciderFactory::~DeciderFactory()
    {
        for (auto it = m_decider_list.rbegin(); it != m_decider_list.rend(); ++it) {
            delete *it;
        }

        for (auto it = m_dl_ptr_list.rbegin(); it != m_dl_ptr_list.rend(); ++it) {
            dlclose(*it);
        }
    }

    IDecider* DeciderFactory::decider(const std::string &description)
    {
        IDecider *result = NULL;
        for (auto it = m_decider_list.begin(); it != m_decider_list.end(); ++it) {
            if (*it != NULL &&
                (*it)->decider_supported(description)) {
                result = (*it)->clone();
                break;
            }
        }
        if (!result) {
            // If we get here, no acceptable decider was found
            std::ostringstream ex_str;
            ex_str << "decider: " << description;
            throw Exception(ex_str.str(), GEOPM_ERROR_DECIDER_UNSUPPORTED, __FILE__, __LINE__);
        }

        return result;
    }

    void DeciderFactory::register_decider(IDecider *decider, void *dl_ptr)
    {
        m_decider_list.push_back(decider);
        if (dl_ptr) {
            m_dl_ptr_list.push_back(dl_ptr);
        }
    }
}
