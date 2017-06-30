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
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <string.h>
#include <float.h>
#include <unistd.h>

#include "geopm_sched.h"
#include "ProfileThread.hpp"
#include "Exception.hpp"

namespace geopm
{
    ProfileThreadTable::ProfileThreadTable(size_t buffer_size, void *buffer)
        : m_buffer((uint32_t *)buffer)
        , m_num_cpu(geopm_sched_num_cpu())
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

    void ProfileThreadTable::enable(bool is_enabled)
    {
        m_is_enabled = is_enabled;
    }

    void ProfileThreadTable::init(const uint32_t num_work_unit)
    {
        if (!m_is_enabled) {
            return;
        }
        m_buffer[cpu_idx() * m_stride] = 0;
        m_buffer[cpu_idx() * m_stride + 1] = num_work_unit;
    }

    void ProfileThreadTable::init(int num_thread, int thread_idx, size_t num_iter, size_t chunk_size)
    {
        if (!m_is_enabled) {
            return;
        }
        std::vector<uint32_t> num_work_unit(num_thread);
        std::fill(num_work_unit.begin(), num_work_unit.end(), 0);

        size_t num_chunk = num_iter / chunk_size;
        size_t unchunked = num_iter % chunk_size;
        size_t min_unit = chunk_size * (num_chunk / num_thread);
        int last_full_thread = num_chunk % num_thread;
        for (int thread_idx = 0; thread_idx < num_thread; ++thread_idx) {
            num_work_unit[thread_idx] = min_unit;
            if (thread_idx < last_full_thread) {
                num_work_unit[thread_idx] += chunk_size;
            }
            else if (thread_idx == last_full_thread) {
                num_work_unit[thread_idx] += unchunked;
            }
        }
        init(num_work_unit[thread_idx]);
    }

    void ProfileThreadTable::init(int num_thread, int thread_idx, size_t num_iter)
    {
        if (!m_is_enabled) {
            return;
        }
        std::vector<uint32_t> num_work_unit(num_thread);
        std::fill(num_work_unit.begin(), num_work_unit.end(), num_iter / num_thread);
        for (int thread_idx = 0; thread_idx < (int)(num_iter % num_thread); ++thread_idx) {
            ++num_work_unit[thread_idx];
        }
        init(num_work_unit[thread_idx]);
    }

    void ProfileThreadTable::post(void)
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
            result = geopm_sched_get_cpu();
            if (result >= geopm_sched_num_cpu()) {
                throw Exception("ProfileThreadTable::cpu_idx(): Number of online CPUs is less than or equal to the value returned by sched_getcpu()",
                                GEOPM_ERROR_LOGIC, __FILE__, __LINE__);
            }
        }
        return result;
    }
}
