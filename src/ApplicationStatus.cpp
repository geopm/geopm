/*
 * Copyright (c) 2015 - 2023, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "config.h"
#include "ApplicationStatus.hpp"

#include <cmath>

#include "geopm/SharedMemory.hpp"
#include "geopm/Exception.hpp"
#include "geopm_debug.hpp"
#include "geopm_hash.h"
#include "geopm_hint.h"


namespace geopm
{

    std::unique_ptr<ApplicationStatus> ApplicationStatus::make_unique(int num_cpu,
                                                                      std::shared_ptr<SharedMemory> shmem)
    {
        return geopm::make_unique<ApplicationStatusImp>(num_cpu, shmem);
    }

    size_t ApplicationStatus::buffer_size(int num_cpu)
    {
        return M_STATUS_SIZE * num_cpu;
    }

    ApplicationStatusImp::ApplicationStatusImp(int num_cpu,
                                               std::shared_ptr<SharedMemory> shmem)
        : m_num_cpu(num_cpu)
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
        m_cache.resize(m_shmem->size());
        update_cache();
    }

    void ApplicationStatusImp::set_hint(int cpu_idx, uint64_t hint)
    {
        if (cpu_idx < 0 || cpu_idx >= m_num_cpu) {
            throw Exception("ApplicationStatusImp::set_hint(): invalid CPU index: " + std::to_string(cpu_idx),
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        geopm::check_hint(hint);
        GEOPM_DEBUG_ASSERT(m_buffer != nullptr, "m_buffer not set");
        // pack hint into 32 bits for atomic write
        m_buffer[cpu_idx].hint = (uint32_t)hint;
    }

    uint64_t ApplicationStatusImp::get_hint(int cpu_idx) const
    {
        if (cpu_idx < 0 || cpu_idx >= m_num_cpu) {
            throw Exception("ApplicationStatusImp::get_hint(): invalid CPU index: " + std::to_string(cpu_idx),
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        GEOPM_DEBUG_ASSERT(m_cache.size() == buffer_size(m_num_cpu),
                           "Memory for m_cache not sized correctly");
        uint64_t result = (uint64_t)m_cache[cpu_idx].hint;
        geopm::check_hint(result);
        return result;
    }

    void ApplicationStatusImp::set_hash(int cpu_idx, uint64_t hash, uint64_t hint)
    {
        if (cpu_idx < 0 || cpu_idx >= m_num_cpu) {
            throw Exception("ApplicationStatusImp::set_hash(): invalid CPU index: " + std::to_string(cpu_idx),
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        if (((~0ULL << 32) & hash) != 0) {
            throw Exception("ApplicationStatusImp::set_hash(): invalid region hash: " + std::to_string(hash),
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        geopm::check_hint(hint);
        GEOPM_DEBUG_ASSERT(m_buffer != nullptr, "m_buffer not set");
        m_buffer[cpu_idx].hash = (uint32_t)hash;
        m_buffer[cpu_idx].hint = (uint32_t)hint;
    }

    uint64_t ApplicationStatusImp::get_hash(int cpu_idx) const
    {
        if (cpu_idx < 0 || cpu_idx >= m_num_cpu) {
            throw Exception("ApplicationStatusImp::get_hash(): invalid CPU index: " + std::to_string(cpu_idx),
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        GEOPM_DEBUG_ASSERT(m_cache.size() == buffer_size(m_num_cpu),
                           "Memory for m_cache not sized correctly");
        return m_cache[cpu_idx].hash;
    }

    void ApplicationStatusImp::reset_work_units(int cpu_idx)
    {
        if (cpu_idx < 0 || cpu_idx >= m_num_cpu) {
            throw Exception("ApplicationStatusImp::reset_work_units(): invalid CPU index: " + std::to_string(cpu_idx),
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        GEOPM_DEBUG_ASSERT(m_buffer != nullptr, "m_buffer not set");
        m_buffer[cpu_idx].total_work = 0;
        m_buffer[cpu_idx].completed_work = 0;
    }

    void ApplicationStatusImp::set_total_work_units(int cpu_idx, int work_units)
    {
        if (cpu_idx < 0 || cpu_idx >= m_num_cpu) {
            throw Exception("ApplicationStatusImp::set_total_work_units(): invalid CPU index: " + std::to_string(cpu_idx),
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        if (work_units <= 0) {
            throw Exception("ApplicationStatusImp::set_total_work_units(): invalid number of work units: " + std::to_string(work_units),
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);

        }
        GEOPM_DEBUG_ASSERT(m_buffer != nullptr, "m_buffer not set");
        // total_work non-zero gates per thread use of completed_work
        m_buffer[cpu_idx].total_work = work_units;
    }

    void ApplicationStatusImp::increment_work_unit(int cpu_idx)
    {
        if (cpu_idx < 0 || cpu_idx >= m_num_cpu) {
            throw Exception("ApplicationStatusImp::increment_work_unit(): invalid CPU index: " + std::to_string(cpu_idx),
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        GEOPM_DEBUG_ASSERT(m_buffer != nullptr, "m_buffer not set");

        if (m_buffer[cpu_idx].total_work != 0) {
            ++(m_buffer[cpu_idx].completed_work);
        }
    }

    double ApplicationStatusImp::get_progress_cpu(int cpu_idx) const
    {
        if (cpu_idx < 0 || cpu_idx >= m_num_cpu) {
            throw Exception("ApplicationStatusImp::get_progress_cpu(): invalid CPU index: " + std::to_string(cpu_idx),
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        GEOPM_DEBUG_ASSERT(m_cache.size() == buffer_size(m_num_cpu),
                           "Memory for m_cache not sized correctly");
        double result = NAN;
        int total_work = m_cache[cpu_idx].total_work;
        if (total_work != 0) {
            result = (double)m_cache[cpu_idx].completed_work / total_work;
        }
        return result;
    }

    void ApplicationStatusImp::update_cache(void)
    {
        GEOPM_DEBUG_ASSERT(m_buffer != nullptr, "m_buffer not set");
        GEOPM_DEBUG_ASSERT(m_cache.size() == buffer_size(m_num_cpu),
                           "Memory for m_cache not sized correctly");
        std::copy(m_buffer, m_buffer + m_num_cpu, m_cache.begin());
    }
}
