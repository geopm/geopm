/*
 * Copyright (c) 2015, 2016, 2017, 2018, Intel Corporation
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

#include "ApplicationIO.hpp"
#include "PlatformIO.hpp"
#include "PlatformTopo.hpp"
#include "ProfileSampler.hpp"
#include "KprofileIOSample.hpp"
#include "KprofileIOGroup.hpp"
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
                        platform_io(),
                        platform_topo(),
                        geopm::make_unique<ProfileSampler>(M_SHMEM_REGION_SIZE),
                        nullptr)
    {

    }

    ApplicationIO::ApplicationIO(const std::string &shm_key,
                                 IPlatformIO &platform_io,
                                 IPlatformTopo &platform_topo,
                                 std::unique_ptr<IProfileSampler> sampler,
                                 std::shared_ptr<IKprofileIOSample> pio_sample)
        : m_platform_io(platform_io)
        , m_platform_topo(platform_topo)
        , m_sampler(std::move(sampler))
        , m_profile_io_sample(pio_sample)
        , m_do_shutdown(false)
        , m_is_connected(false)
        , m_rank_per_node(-1)
    {
        connect();

        m_start_energy = current_energy();
    }

    ApplicationIO::~ApplicationIO()
    {

    }

    double ApplicationIO::current_energy(void) const
    {
        double package_energy = 0.0;
        for (int domain_idx = 0; domain_idx < m_platform_topo.num_domain(IPlatformTopo::M_DOMAIN_PACKAGE); ++domain_idx) {
            package_energy += m_platform_io.read_signal("ENERGY_PACKAGE", IPlatformTopo::M_DOMAIN_PACKAGE, domain_idx);
        }
        double dram_energy = 0.0;
        for (int domain_idx = 0; domain_idx < m_platform_topo.num_domain(IPlatformTopo::M_DOMAIN_BOARD_MEMORY); ++domain_idx) {
            dram_energy += m_platform_io.read_signal("ENERGY_DRAM", IPlatformTopo::M_DOMAIN_BOARD_MEMORY, domain_idx);
        }
        return package_energy + dram_energy;
    }

    void ApplicationIO::connect(void)
    {
        if (!m_is_connected) {
            m_sampler->initialize();
            m_rank_per_node = m_sampler->rank_per_node();
            m_prof_sample.resize(m_sampler->capacity());
            std::vector<int> cpu_rank = m_sampler->cpu_rank();
            if (m_profile_io_sample == nullptr) {
                m_profile_io_sample = std::make_shared<KprofileIOSample>(cpu_rank);
                platform_io().register_iogroup(geopm::make_unique<KprofileIOGroup>(m_profile_io_sample));
            }
            m_is_connected = true;
        }
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
        return m_sampler->do_shutdown();
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
        return m_profile_io_sample->total_region_runtime(region_id);
    }

    double ApplicationIO::total_region_mpi_runtime(uint64_t region_id) const
    {
#ifdef GEOPM_DEBUG
        if (!m_is_connected) {
            throw Exception("ApplicationIO::" + std::string(__func__) +
                            " called before connect().",
                            GEOPM_ERROR_LOGIC, __FILE__, __LINE__);
        }
#endif
        return m_profile_io_sample->total_region_mpi_time(region_id);
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
        return m_profile_io_sample->total_epoch_runtime();
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

    double ApplicationIO::total_app_mpi_runtime(void) const
    {
#ifdef GEOPM_DEBUG
        if (!m_is_connected) {
            throw Exception("ApplicationIO::" + std::string(__func__) +
                            " called before connect().",
                            GEOPM_ERROR_LOGIC, __FILE__, __LINE__);
        }
#endif
        return m_profile_io_sample->total_app_mpi_time();
    }

    double ApplicationIO::total_app_energy(void) const
    {
#ifdef GEOPM_DEBUG
        if (!m_is_connected) {
            throw Exception("ApplicationIO::" + std::string(__func__) +
                            " called before connect().",
                            GEOPM_ERROR_LOGIC, __FILE__, __LINE__);
        }
#endif
        return current_energy() - m_start_energy;
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
        return m_profile_io_sample->total_count(region_id);
    }

    void ApplicationIO::update(std::shared_ptr<IComm> comm)
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
    }
}
