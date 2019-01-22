/*
 * Copyright (c) 2015, 2016, 2017, 2018, 2019 Intel Corporation
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

#include <utility>

#include "Exception.hpp"
#include "ApplicationIO.hpp"
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
    constexpr size_t ApplicationIO::M_SHMEM_REGION_SIZE;

    ApplicationIO::ApplicationIO(const std::string &shm_key)
        : ApplicationIO(shm_key,
                        geopm::make_unique<ProfileSampler>(M_SHMEM_REGION_SIZE),
                        nullptr, nullptr,
                        platform_io(), platform_topo())
    {

    }

    ApplicationIO::ApplicationIO(const std::string &shm_key,
                                 std::unique_ptr<IProfileSampler> sampler,
                                 std::shared_ptr<IProfileIOSample> pio_sample,
                                 std::unique_ptr<IEpochRuntimeRegulator> epoch_regulator,
                                 IPlatformIO &platform_io,
                                 IPlatformTopo &platform_topo)
        : m_sampler(std::move(sampler))
        , m_profile_io_sample(pio_sample)
        , m_platform_io(platform_io)
        , m_platform_topo(platform_topo)
        , m_thread_progress(m_platform_topo.num_domain(PlatformTopo::M_DOMAIN_CPU))
        , m_is_connected(false)
        , m_rank_per_node(-1)
        , m_epoch_regulator(std::move(epoch_regulator))
        , m_start_energy_pkg(NAN)
        , m_start_energy_dram(NAN)
    {
    }

    ApplicationIO::~ApplicationIO()
    {

    }

    void ApplicationIO::connect(void)
    {
        if (!m_is_connected) {
            m_sampler->initialize();
            m_rank_per_node = m_sampler->rank_per_node();
            m_prof_sample.resize(m_sampler->capacity());
            std::vector<int> cpu_rank = m_sampler->cpu_rank();
            if (m_profile_io_sample == nullptr) {
                m_epoch_regulator = geopm::make_unique<EpochRuntimeRegulator>(m_rank_per_node, m_platform_io, m_platform_topo);
                m_epoch_regulator->init_unmarked_region();
                m_profile_io_sample = std::make_shared<ProfileIOSample>(cpu_rank, *m_epoch_regulator);
                platform_io().register_iogroup(geopm::make_unique<ProfileIOGroup>(m_profile_io_sample, *m_epoch_regulator));
            }
            m_is_connected = true;

            m_start_energy_pkg = current_energy_pkg();
            m_start_energy_dram = current_energy_dram();
        }
    }

    void ApplicationIO::controller_ready(void)
    {
        m_sampler->controller_ready();
    }

    bool ApplicationIO::do_shutdown(void) const
    {
#ifdef GEOPM_DEBUG
        if (!m_is_connected) {
            throw Exception("ApplicationIO::" + std::string(__func__) +
                            " called before connect().",
                            GEOPM_ERROR_LOGIC, __FILE__, __LINE__);
        }
#endif
        bool result = m_sampler->do_shutdown();
        if (result) {
            m_profile_io_sample->finalize_unmarked_region();
        }
        return result;
    }

    std::string ApplicationIO::report_name(void) const
    {
#ifdef GEOPM_DEBUG
        if (!m_is_connected) {
            throw Exception("ApplicationIO::" + std::string(__func__) +
                            " called before connect().",
                            GEOPM_ERROR_LOGIC, __FILE__, __LINE__);
        }
#endif
        return m_sampler->report_name();
    }

    std::string ApplicationIO::profile_name(void) const
    {
#ifdef GEOPM_DEBUG
        if (!m_is_connected) {
            throw Exception("ApplicationIO::" + std::string(__func__) +
                            " called before connect().",
                            GEOPM_ERROR_LOGIC, __FILE__, __LINE__);
        }
#endif
        return m_sampler->profile_name();
    }

    std::set<std::string> ApplicationIO::region_name_set(void) const
    {
#ifdef GEOPM_DEBUG
        if (!m_is_connected) {
            throw Exception("ApplicationIO::" + std::string(__func__) +
                            " called before connect().",
                            GEOPM_ERROR_LOGIC, __FILE__, __LINE__);
        }
#endif
        return m_sampler->name_set();
    }

    double ApplicationIO::total_region_runtime(uint64_t region_id) const
    {
#ifdef GEOPM_DEBUG
        if (!m_is_connected) {
            throw Exception("ApplicationIO::" + std::string(__func__) +
                            " called before connect().",
                            GEOPM_ERROR_LOGIC, __FILE__, __LINE__);
        }
#endif
        double result = 0.0;
        try {
            result = m_epoch_regulator->total_region_runtime(region_id);
        }
        catch (const Exception &ex) {
        }
        return result;
    }

    double ApplicationIO::total_region_runtime_mpi(uint64_t region_id) const
    {
#ifdef GEOPM_DEBUG
        if (!m_is_connected) {
            throw Exception("ApplicationIO::" + std::string(__func__) +
                            " called before connect().",
                            GEOPM_ERROR_LOGIC, __FILE__, __LINE__);
        }
#endif
        double result = 0.0;
        try {
            result = m_epoch_regulator->total_region_runtime_mpi(region_id);
        }
        catch (const Exception &ex) {
        }
        return result;
    }

    double ApplicationIO::total_epoch_runtime(void) const
    {
#ifdef GEOPM_DEBUG
        if (!m_is_connected) {
            throw Exception("ApplicationIO::" + std::string(__func__) +
                            " called before connect().",
                            GEOPM_ERROR_LOGIC, __FILE__, __LINE__);
        }
#endif
        return m_epoch_regulator->total_epoch_runtime();
    }

    double ApplicationIO::total_epoch_runtime_mpi(void) const
    {
#ifdef GEOPM_DEBUG
        if (!m_is_connected) {
            throw Exception("ApplicationIO::" + std::string(__func__) +
                            " called before connect().",
                            GEOPM_ERROR_LOGIC, __FILE__, __LINE__);
        }
#endif
        return m_epoch_regulator->total_epoch_runtime_mpi();
    }

    double ApplicationIO::total_epoch_energy_pkg(void) const
    {
#ifdef GEOPM_DEBUG
        if (!m_is_connected) {
            throw Exception("ApplicationIO::" + std::string(__func__) +
                            " called before connect().",
                            GEOPM_ERROR_LOGIC, __FILE__, __LINE__);
        }
#endif
        return m_epoch_regulator->total_epoch_energy_pkg();
    }

    double ApplicationIO::total_epoch_energy_dram(void) const
    {
#ifdef GEOPM_DEBUG
        if (!m_is_connected) {
            throw Exception("ApplicationIO::" + std::string(__func__) +
                            " called before connect().",
                            GEOPM_ERROR_LOGIC, __FILE__, __LINE__);
        }
#endif
        return m_epoch_regulator->total_epoch_energy_dram();
    }

    double ApplicationIO::total_app_runtime(void) const
    {
#ifdef GEOPM_DEBUG
        if (!m_is_connected) {
            throw Exception("ApplicationIO::" + std::string(__func__) +
                            " called before connect().",
                            GEOPM_ERROR_LOGIC, __FILE__, __LINE__);
        }
#endif
        return m_profile_io_sample->total_app_runtime();
    }

    double ApplicationIO::current_energy_pkg(void) const
    {
        double energy = 0.0;
        int num_package = m_platform_topo.num_domain(IPlatformTopo::M_DOMAIN_PACKAGE);
        for (int pkg = 0; pkg < num_package; ++pkg) {
            energy += m_platform_io.read_signal("ENERGY_PACKAGE", IPlatformTopo::M_DOMAIN_PACKAGE, pkg);
        }
        return energy;
   }

    double ApplicationIO::current_energy_dram(void) const
    {
        double energy = 0.0;
        int num_dram = m_platform_topo.num_domain(IPlatformTopo::M_DOMAIN_BOARD_MEMORY);
        for (int dram = 0; dram < num_dram; ++dram) {
            energy += m_platform_io.read_signal("ENERGY_DRAM", IPlatformTopo::M_DOMAIN_BOARD_MEMORY, dram);
        }
        return energy;
    }

    double ApplicationIO::total_app_energy_pkg(void) const
    {
#ifdef GEOPM_DEBUG
        if (!m_is_connected) {
            throw Exception("ApplicationIO::" + std::string(__func__) +
                            " called before connect().",
                            GEOPM_ERROR_LOGIC, __FILE__, __LINE__);
        }
#endif
        return current_energy_pkg() - m_start_energy_pkg;
    }

    double ApplicationIO::total_app_energy_dram(void) const
    {
#ifdef GEOPM_DEBUG
        if (!m_is_connected) {
            throw Exception("ApplicationIO::" + std::string(__func__) +
                            " called before connect().",
                            GEOPM_ERROR_LOGIC, __FILE__, __LINE__);
        }
#endif
        return current_energy_dram() - m_start_energy_dram;
    }

    double ApplicationIO::total_app_runtime_mpi(void) const
    {
#ifdef GEOPM_DEBUG
        if (!m_is_connected) {
            throw Exception("ApplicationIO::" + std::string(__func__) +
                            " called before connect().",
                            GEOPM_ERROR_LOGIC, __FILE__, __LINE__);
        }
#endif
        return m_epoch_regulator->total_app_runtime_mpi();
    }

    double ApplicationIO::total_app_runtime_ignore(void) const
    {
#ifdef GEOPM_DEBUG
        if (!m_is_connected) {
            throw Exception("ApplicationIO::" + std::string(__func__) +
                            " called before connect().",
                            GEOPM_ERROR_LOGIC, __FILE__, __LINE__);
        }
#endif
        return m_epoch_regulator->total_app_runtime_ignore();
    }

    double ApplicationIO::total_epoch_runtime_ignore(void) const
    {
#ifdef GEOPM_DEBUG
        if (!m_is_connected) {
            throw Exception("ApplicationIO::" + std::string(__func__) +
                            " called before connect().",
                            GEOPM_ERROR_LOGIC, __FILE__, __LINE__);
        }
#endif
        return m_epoch_regulator->total_epoch_runtime_ignore();
    }

    int ApplicationIO::total_count(uint64_t region_id) const
    {
#ifdef GEOPM_DEBUG
        if (!m_is_connected) {
            throw Exception("ApplicationIO::" + std::string(__func__) +
                            " called before connect().",
                            GEOPM_ERROR_LOGIC, __FILE__, __LINE__);
        }
#endif
        double result = 0.0;
        try {
            result = m_epoch_regulator->total_count(region_id);
        }
        catch (const Exception &ex) {
        }
        return result;
    }

    void ApplicationIO::update(std::shared_ptr<Comm> comm)
    {
#ifdef GEOPM_DEBUG
        if (!m_is_connected) {
            throw Exception("ApplicationIO::" + std::string(__func__) +
                            " called before connect().",
                            GEOPM_ERROR_LOGIC, __FILE__, __LINE__);
        }
#endif
        size_t length = 0;
        m_sampler->sample(m_prof_sample, length, comm);
        m_profile_io_sample->update(m_prof_sample.cbegin(), m_prof_sample.cbegin() + length);
        m_sampler->tprof_table()->dump(m_thread_progress);
        m_profile_io_sample->update_thread(m_thread_progress);
    }

    std::list<geopm_region_info_s> ApplicationIO::region_info(void) const
    {
#ifdef GEOPM_DEBUG
        if (!m_is_connected) {
            throw Exception("ApplicationIO::" + std::string(__func__) +
                            " called before connect().",
                            GEOPM_ERROR_LOGIC, __FILE__, __LINE__);
        }
#endif
        return m_epoch_regulator->region_info();
    }

    void ApplicationIO::clear_region_info(void)
    {
        m_epoch_regulator->clear_region_info();
    }

    void ApplicationIO::abort(void)
    {
        m_sampler->abort();
    }
}
