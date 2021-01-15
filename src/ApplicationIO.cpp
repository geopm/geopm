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

#include "ApplicationIO.hpp"

#include <utility>

#include "Exception.hpp"
#include "ApplicationSampler.hpp"
#include "ProfileSampler.hpp"
#include "Helper.hpp"
#include "config.h"

#ifdef GEOPM_HAS_XMMINTRIN
#include <xmmintrin.h>
#endif

namespace geopm
{
    constexpr size_t ApplicationIOImp::M_SHMEM_REGION_SIZE;

    ApplicationIOImp::ApplicationIOImp()
        : ApplicationIOImp(ApplicationSampler::application_sampler())
    {

    }

    ApplicationIOImp::ApplicationIOImp(ApplicationSampler &application_sampler)
        : m_is_connected(false)
        , m_application_sampler(application_sampler)
    {
        if (m_application_sampler.get_sampler() == nullptr) {
            auto sampler = std::make_shared<ProfileSamplerImp>(M_SHMEM_REGION_SIZE);
            m_application_sampler.set_sampler(sampler);
        }
    }

    ApplicationIOImp::~ApplicationIOImp()
    {

    }

    void ApplicationIOImp::connect(void)
    {
        if (!m_is_connected) {
            m_application_sampler.get_sampler()->initialize();
            m_is_connected = true;
        }
    }

    void ApplicationIOImp::controller_ready(void)
    {
        m_application_sampler.get_sampler()->controller_ready();
    }

    bool ApplicationIOImp::do_shutdown(void) const
    {
#ifdef GEOPM_DEBUG
        if (!m_is_connected) {
            throw Exception("ApplicationIOImp::" + std::string(__func__) +
                            " called before connect().",
                            GEOPM_ERROR_LOGIC, __FILE__, __LINE__);
        }
#endif
        return m_application_sampler.get_sampler()->do_shutdown();
    }

    std::string ApplicationIOImp::report_name(void) const
    {
#ifdef GEOPM_DEBUG
        if (!m_is_connected) {
            throw Exception("ApplicationIOImp::" + std::string(__func__) +
                            " called before connect().",
                            GEOPM_ERROR_LOGIC, __FILE__, __LINE__);
        }
#endif
        return m_application_sampler.get_sampler()->report_name();
    }

    std::string ApplicationIOImp::profile_name(void) const
    {
#ifdef GEOPM_DEBUG
        if (!m_is_connected) {
            throw Exception("ApplicationIOImp::" + std::string(__func__) +
                            " called before connect().",
                            GEOPM_ERROR_LOGIC, __FILE__, __LINE__);
        }
#endif
        return m_application_sampler.get_sampler()->profile_name();
    }

    std::set<std::string> ApplicationIOImp::region_name_set(void) const
    {
#ifdef GEOPM_DEBUG
        if (!m_is_connected) {
            throw Exception("ApplicationIOImp::" + std::string(__func__) +
                            " called before connect().",
                            GEOPM_ERROR_LOGIC, __FILE__, __LINE__);
        }
#endif
        return m_application_sampler.get_sampler()->name_set();
    }

    void ApplicationIOImp::abort(void)
    {
        m_application_sampler.get_sampler()->abort();
    }
}
