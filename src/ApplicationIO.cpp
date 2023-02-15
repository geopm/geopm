/*
 * Copyright (c) 2015 - 2023, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "ApplicationIO.hpp"

#include <utility>

#include "geopm/Exception.hpp"
#include "ApplicationSampler.hpp"
#include "ProfileSampler.hpp"
#include "geopm/Helper.hpp"
#include "config.h"

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
