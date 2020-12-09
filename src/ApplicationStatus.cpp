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

#include "config.h"
#include "ApplicationStatus.hpp"

#include <cmath>

#include "geopm.h"
#include "SharedMemory.hpp"
#include "Helper.hpp"
#include "Exception.hpp"
#include "PlatformTopo.hpp"
#include "geopm_debug.hpp"

namespace geopm
{

    std::unique_ptr<ApplicationStatus> ApplicationStatus::make_unique(const PlatformTopo& topo,
                                                                      std::shared_ptr<SharedMemory> shmem)
    {
        return geopm::make_unique<ApplicationStatusImp>(topo, shmem);
    }

    size_t ApplicationStatus::buffer_size(int num_cpu)
    {
        return M_STATUS_SIZE * num_cpu;
    }

    ApplicationStatusImp::ApplicationStatusImp(const PlatformTopo& topo,
                                               std::shared_ptr<SharedMemory> shmem)
        : m_topo(topo)
        , m_num_cpu(m_topo.num_domain(GEOPM_DOMAIN_CPU))
        , m_shmem(shmem)
    {
        if (m_shmem == nullptr) {
            throw Exception("ApplicationStatus: shared memory pointer cannot be null",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        if (m_shmem->size() != buffer_size(m_num_cpu)) {
            throw Exception("ApplicationStatus: shared memory incorrectly sized",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        // Note: no lock; all members of the struct are 32-bits and will be
        // accessed atomically by hardware.
        m_buffer = (m_app_status_s *)m_shmem->pointer();

        // initialize shmem if all zero is not appropriate
        for (int ii = 0; ii < m_num_cpu; ++ii) {
            m_buffer[ii].total_work = -1;
            m_buffer[ii].process = -1;
        }
    }

    void ApplicationStatusImp::set_hint(int cpu_idx, uint64_t hints)
    {
        if (cpu_idx < 0 || cpu_idx >= m_num_cpu) {
            throw Exception("ApplicationStatusImp::set_hint(): invalid CPU index: " + std::to_string(cpu_idx),
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        if ((hints & ~GEOPM_MASK_REGION_HINT) != 0ULL) {
            throw Exception("ApplicationStatusImp::set_hint(): invalid hint",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        GEOPM_DEBUG_ASSERT(m_buffer != nullptr, "m_buffer not set");
        // pack hints into 32 bits for atomic write
        m_buffer[cpu_idx].hints = (uint32_t)(hints >> 32);
    }

    uint64_t ApplicationStatusImp::get_hint(int cpu_idx) const
    {
        if (cpu_idx < 0 || cpu_idx >= m_num_cpu) {
            throw Exception("ApplicationStatusImp::get_hint(): invalid CPU index: " + std::to_string(cpu_idx),
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        GEOPM_DEBUG_ASSERT(m_buffer != nullptr, "m_buffer not set");
        return (uint64_t)m_buffer[cpu_idx].hints << 32;
    }

    void ApplicationStatusImp::set_hash(int cpu_idx, uint64_t hash)
    {
        if (cpu_idx < 0 || cpu_idx >= m_num_cpu) {
            throw Exception("ApplicationStatusImp::set_hash(): invalid CPU index: " + std::to_string(cpu_idx),
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        if (((~0ULL << 32) & hash) != 0) {
            throw Exception("ApplicationStatusImp::set_hash(): invalid region hash: " + std::to_string(hash),
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        GEOPM_DEBUG_ASSERT(m_buffer != nullptr, "m_buffer not set");
        m_buffer[cpu_idx].hash = (uint32_t)hash;
    }

    uint64_t ApplicationStatusImp::get_hash(int cpu_idx) const
    {
        if (cpu_idx < 0 || cpu_idx >= m_num_cpu) {
            throw Exception("ApplicationStatusImp::get_hash(): invalid CPU index: " + std::to_string(cpu_idx),
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        GEOPM_DEBUG_ASSERT(m_buffer != nullptr, "m_buffer not set");
        return m_buffer[cpu_idx].hash;
    }

    void ApplicationStatusImp::set_total_work_units(int cpu_idx, int work_units)
    {
        if (cpu_idx < 0 || cpu_idx >= m_num_cpu) {
            throw Exception("ApplicationStatusImp::set_total_work_units(): invalid CPU index: " + std::to_string(cpu_idx),
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        GEOPM_DEBUG_ASSERT(m_buffer != nullptr, "m_buffer not set");

        if (work_units < 0) {
            throw Exception("ApplicationStatusImp::set_total_work_units(): invalid number of work units: " + std::to_string(work_units),
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);

        }
        m_buffer[cpu_idx].completed_work = 0;
        m_buffer[cpu_idx].total_work = work_units;
    }

    void ApplicationStatusImp::increment_work_unit(int cpu_idx)
    {
        if (cpu_idx < 0 || cpu_idx >= m_num_cpu) {
            throw Exception("ApplicationStatusImp::increment_work_unit(): invalid CPU index: " + std::to_string(cpu_idx),
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        GEOPM_DEBUG_ASSERT(m_buffer != nullptr, "m_buffer not set");

        // only this thread is writing, so don't need to lock
        if (m_buffer[cpu_idx].completed_work >= m_buffer[cpu_idx].total_work) {
            throw Exception("ApplicationStatusImp::increment_work_unit(): more increments than total work",
                            GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);

        }
        m_buffer[cpu_idx].completed_work += 1;
    }

    double ApplicationStatusImp::get_work_progress(int cpu_idx) const
    {
        if (cpu_idx < 0 || cpu_idx >= m_num_cpu) {
            throw Exception("ApplicationStatusImp::get_work_progress(): invalid CPU index: " + std::to_string(cpu_idx),
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        GEOPM_DEBUG_ASSERT(m_buffer != nullptr, "m_buffer not set");
        double result = NAN;
        int total_work = m_buffer[cpu_idx].total_work;
        if (total_work > 0) {
            result = (double)m_buffer[cpu_idx].completed_work / total_work;
        }
        return result;
    }

    void ApplicationStatusImp::set_process(const std::set<int> &cpu_idx, int process)
    {
        GEOPM_DEBUG_ASSERT(m_buffer != nullptr, "m_buffer not set");
        for (auto cpu : cpu_idx) {
            if (cpu < 0 || cpu >= m_num_cpu) {
                throw Exception("ApplicationStatusImp::set_process(): invalid CPU index: " + std::to_string(cpu),
                                GEOPM_ERROR_INVALID, __FILE__, __LINE__);
            }
            m_buffer[cpu].process = process;
        }
    }

    int ApplicationStatusImp::get_process(int cpu_idx) const
    {
        if (cpu_idx < 0 || cpu_idx >= m_num_cpu) {
            throw Exception("ApplicationStatusImp::get_process(): invalid CPU index: " + std::to_string(cpu_idx),
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        GEOPM_DEBUG_ASSERT(m_buffer != nullptr, "m_buffer not set");

        return m_buffer[cpu_idx].process;
    }
}
