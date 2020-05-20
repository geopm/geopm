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
#include "EpochRuntimeRegulator.hpp"
#include "PlatformIO.hpp"
#include "PlatformTopo.hpp"
#include "ProfileSampler.hpp"
#include "ProfileThread.hpp"
#include "ProfileIOSample.hpp"
#include "ProfileIOGroup.hpp"
#include "Helper.hpp"
#include "config.h"

#ifdef GEOPM_HAS_XMMINTRIN
#include <xmmintrin.h>
#endif

namespace geopm
{
    constexpr size_t ApplicationIOImp::M_SHMEM_REGION_SIZE;

    ApplicationIOImp::ApplicationIOImp(const std::string &shm_key)
        : ApplicationIOImp(shm_key,
                           ApplicationSampler::application_sampler(),
                           platform_io(), platform_topo())
    {

    }

    ApplicationIOImp::ApplicationIOImp(const std::string &shm_key,
                                       ApplicationSampler &application_sampler,
                                       PlatformIO &platform_io,
                                       const PlatformTopo &platform_topo)
        : m_platform_io(platform_io)
        , m_platform_topo(platform_topo)
        , m_thread_progress(m_platform_topo.num_domain(GEOPM_DOMAIN_CPU))
        , m_is_connected(false)
        , m_application_sampler(application_sampler)
    {
        if (m_application_sampler.get_io_sample() == nullptr) {
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
            m_prof_sample.resize(m_application_sampler.get_sampler()->capacity());

            if (m_application_sampler.get_io_sample() == nullptr) {
                auto rank_per_node = m_application_sampler.get_sampler()->rank_per_node();
                auto epoch_regulator = std::make_shared<EpochRuntimeRegulatorImp>(rank_per_node, m_platform_io, m_platform_topo);
                m_application_sampler.set_regulator(epoch_regulator);

                auto profile_io_sample = std::make_shared<ProfileIOSampleImp>();
                m_application_sampler.set_io_sample(profile_io_sample);

                platform_io().get_profileio()->connect();
            }
            m_is_connected = true;
        }
    }

    void ApplicationIOImp::controller_ready(void)
    {
        m_application_sampler.get_regulator()->init_unmarked_region();
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
        bool result = m_application_sampler.get_sampler()->do_shutdown();
        if (result) {
            m_application_sampler.get_io_sample()->finalize_unmarked_region();
        }
        return result;
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

    double ApplicationIOImp::total_region_runtime(uint64_t region_id) const
    {
#ifdef GEOPM_DEBUG
        if (!m_is_connected) {
            throw Exception("ApplicationIOImp::" + std::string(__func__) +
                            " called before connect().",
                            GEOPM_ERROR_LOGIC, __FILE__, __LINE__);
        }
#endif
        double result = 0.0;
        try {
            result = m_application_sampler.get_regulator()->total_region_runtime(region_id);
        }
        catch (const Exception &ex) {
        }
        return result;
    }

    double ApplicationIOImp::total_region_runtime_mpi(uint64_t region_id) const
    {
#ifdef GEOPM_DEBUG
        if (!m_is_connected) {
            throw Exception("ApplicationIOImp::" + std::string(__func__) +
                            " called before connect().",
                            GEOPM_ERROR_LOGIC, __FILE__, __LINE__);
        }
#endif
        double result = 0.0;
        try {
            result = m_application_sampler.get_regulator()->total_region_runtime_mpi(region_id);
        }
        catch (const Exception &ex) {
        }
        return result;
    }

    double ApplicationIOImp::total_epoch_runtime(void) const
    {
#ifdef GEOPM_DEBUG
        if (!m_is_connected) {
            throw Exception("ApplicationIOImp::" + std::string(__func__) +
                            " called before connect().",
                            GEOPM_ERROR_LOGIC, __FILE__, __LINE__);
        }
#endif
        return m_application_sampler.get_regulator()->total_epoch_runtime();
    }

    double ApplicationIOImp::total_epoch_runtime_network(void) const
    {
#ifdef GEOPM_DEBUG
        if (!m_is_connected) {
            throw Exception("ApplicationIOImp::" + std::string(__func__) +
                            " called before connect().",
                            GEOPM_ERROR_LOGIC, __FILE__, __LINE__);
        }
#endif
        return m_application_sampler.get_regulator()->total_epoch_runtime_network();
    }

    double ApplicationIOImp::total_epoch_energy_pkg(void) const
    {
#ifdef GEOPM_DEBUG
        if (!m_is_connected) {
            throw Exception("ApplicationIOImp::" + std::string(__func__) +
                            " called before connect().",
                            GEOPM_ERROR_LOGIC, __FILE__, __LINE__);
        }
#endif
        return m_application_sampler.get_regulator()->total_epoch_energy_pkg();
    }

    double ApplicationIOImp::total_epoch_energy_dram(void) const
    {
#ifdef GEOPM_DEBUG
        if (!m_is_connected) {
            throw Exception("ApplicationIOImp::" + std::string(__func__) +
                            " called before connect().",
                            GEOPM_ERROR_LOGIC, __FILE__, __LINE__);
        }
#endif
        return m_application_sampler.get_regulator()->total_epoch_energy_dram();
    }

    double ApplicationIOImp::total_app_runtime_mpi(void) const
    {
#ifdef GEOPM_DEBUG
        if (!m_is_connected) {
            throw Exception("ApplicationIOImp::" + std::string(__func__) +
                            " called before connect().",
                            GEOPM_ERROR_LOGIC, __FILE__, __LINE__);
        }
#endif
        return m_application_sampler.get_regulator()->total_app_runtime_mpi();
    }

    double ApplicationIOImp::total_app_runtime_ignore(void) const
    {
#ifdef GEOPM_DEBUG
        if (!m_is_connected) {
            throw Exception("ApplicationIOImp::" + std::string(__func__) +
                            " called before connect().",
                            GEOPM_ERROR_LOGIC, __FILE__, __LINE__);
        }
#endif
        return m_application_sampler.get_regulator()->total_app_runtime_ignore();
    }

    int ApplicationIOImp::total_epoch_count(void) const
    {
#ifdef GEOPM_DEBUG
        if (!m_is_connected) {
            throw Exception("ApplicationIOImp::" + std::string(__func__) +
                            " called before connect().",
                            GEOPM_ERROR_LOGIC, __FILE__, __LINE__);
        }
#endif
        return m_application_sampler.get_regulator()->total_epoch_count();
    }

    double ApplicationIOImp::total_epoch_runtime_ignore(void) const
    {
#ifdef GEOPM_DEBUG
        if (!m_is_connected) {
            throw Exception("ApplicationIOImp::" + std::string(__func__) +
                            " called before connect().",
                            GEOPM_ERROR_LOGIC, __FILE__, __LINE__);
        }
#endif
        return m_application_sampler.get_regulator()->total_epoch_runtime_ignore();
    }

    int ApplicationIOImp::total_count(uint64_t region_id) const
    {
#ifdef GEOPM_DEBUG
        if (!m_is_connected) {
            throw Exception("ApplicationIOImp::" + std::string(__func__) +
                            " called before connect().",
                            GEOPM_ERROR_LOGIC, __FILE__, __LINE__);
        }
#endif
        double result = 0.0;
        try {
            result = m_application_sampler.get_regulator()->total_count(region_id);
        }
        catch (const Exception &ex) {
        }
        return result;
    }

    void ApplicationIOImp::update(std::shared_ptr<Comm> comm)
    {
#ifdef GEOPM_DEBUG
        if (!m_is_connected) {
            throw Exception("ApplicationIOImp::" + std::string(__func__) +
                            " called before connect().",
                            GEOPM_ERROR_LOGIC, __FILE__, __LINE__);
        }
#endif
        size_t length = 0;
        m_application_sampler.get_sampler()->sample(m_prof_sample, length, comm);
        m_application_sampler.update_records();
        m_application_sampler.get_io_sample()->update(m_prof_sample.cbegin(), m_prof_sample.cbegin() + length);
        m_application_sampler.get_sampler()->tprof_table()->dump(m_thread_progress);
        m_application_sampler.get_io_sample()->update_thread(m_thread_progress);
    }

    std::list<geopm_region_info_s> ApplicationIOImp::region_info(void) const
    {
#ifdef GEOPM_DEBUG
        if (!m_is_connected) {
            throw Exception("ApplicationIOImp::" + std::string(__func__) +
                            " called before connect().",
                            GEOPM_ERROR_LOGIC, __FILE__, __LINE__);
        }
#endif
        return m_application_sampler.get_regulator()->region_info();
    }

    void ApplicationIOImp::clear_region_info(void)
    {
        m_application_sampler.get_regulator()->clear_region_info();
    }

    void ApplicationIOImp::abort(void)
    {
        m_application_sampler.get_sampler()->abort();
    }
}
