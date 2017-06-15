/*
 * Copyright (c) 2016, Intel Corporation
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
#ifdef __APPLE__
#define _DARWIN_C_SOURCE
#include <sys/types.h>
#include <sys/sysctl.h>
#endif

#include <string.h>
#include <float.h>
#include <unistd.h>

#include "ProfileThread.hpp"
#include "Exception.hpp"

namespace geopm
{
    ProfileThreadTable::ProfileThreadTable(size_t buffer_size, void *buffer)
        : m_buffer((uint32_t *)buffer)
        , m_num_cpu(num_cpu())
        , m_stride(64 / sizeof(uint32_t))
    {
        if (buffer_size < 64 * m_num_cpu) {
            throw Exception("ProfileThreadTable: provided buffer too small",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
    }

    ProfileThreadTable::ProfileThreadTable(const ProfileThreadTable &other)
        : m_buffer(other.m_buffer)
        , m_num_cpu(other.m_num_cpu)
        , m_stride(other.m_stride)
        , m_is_enabled(true)
    {

    }

    ProfileThreadTable::~ProfileThreadTable()
    {

    }

    int ProfileThreadTable::num_cpu(void)
    {
        return m_num_cpu;
    }

    int ProfileThreadTable::num_cpu_s(void)
    {
#ifdef _SC_NPROCESSORS_ONLN
        uint32_t result = sysconf(_SC_NPROCESSORS_ONLN);
#else
        uint32_t result = 1;
        size_t len = sizeof(result);
        sysctl((int[2]) {CTL_HW, HW_NCPU}, 2, &result, &len, NULL, 0);
#endif
        return result;
    }

    void ProfileThreadTable::enable(bool is_enabled)
    {
        m_is_enabled = is_enabled;
    }

    void ProfileThreadTable::reset(const std::vector<uint32_t> &num_work_unit)
    {
        if (!m_is_enabled) {
            return;
        }
        for (uint32_t cpu = 0; cpu < m_num_cpu; ++cpu) {
            m_buffer[cpu * m_stride] = 0;
            m_buffer[cpu * m_stride + 1] = num_work_unit[cpu];
        }
    }

    void ProfileThreadTable::reset(int num_thread, size_t num_iter, size_t chunk_size)
    {
        if (!m_is_enabled) {
            return;
        }
        std::vector<uint32_t> num_work_unit(m_num_cpu);
        std::fill(num_work_unit.begin(), num_work_unit.end(), 0);

        size_t num_chunk = num_iter / chunk_size;
        size_t unchunked = num_iter % chunk_size;
        size_t min_unit = chunk_size * (num_chunk / num_thread);
        int last_full_thread = num_chunk % num_thread;
        for (int i = 0; i < num_thread; ++i) {
            num_work_unit[i] = min_unit;
            if (i < last_full_thread) {
                num_work_unit[i] += chunk_size;
            }
            else if (i == last_full_thread) {
                num_work_unit[i] += unchunked;
            }
        }
        reset(num_work_unit);
    }

    void ProfileThreadTable::reset(int num_thread, size_t num_iter)
    {
        if (!m_is_enabled) {
            return;
        }
        std::vector<uint32_t> num_work_unit(m_num_cpu);
        std::fill(num_work_unit.begin(), num_work_unit.end(), num_iter / num_thread);
        for (int i = 0; i < (int)(num_iter % num_thread); ++i) {
            ++num_work_unit[i];
        }
        reset(num_work_unit);
    }

    void ProfileThreadTable::increment(void)
    {
        if (!m_is_enabled) {
            return;
        }
        ++m_buffer[cpu_idx() * m_stride];
    }

    void ProfileThreadTable::dump(std::vector<double> &progress)
    {
        double numer;
        uint32_t denom;
        for (uint32_t cpu = 0; cpu < m_num_cpu; ++cpu) {
            numer = (double)m_buffer[cpu * m_stride];
            denom = m_buffer[cpu * m_stride + 1];
            progress[cpu] = denom ? numer / denom : -1.0;
        }
    }

    int ProfileThreadTable::cpu_idx(void)
    {
        static thread_local int result = -1;
        if (result == -1) {
            result = sched_getcpu();
            if (result >= num_cpu_s()) {
                throw Exception("Number of online CPUs is less than or equal to the value returned by sched_getcpu()",
                                GEOPM_ERROR_LOGIC, __FILE__, __LINE__);
            }
        }
        return result;
    }
}
