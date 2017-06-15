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

#include <string.h>
#include <float.h>

#include "geopm.h"
#include "ProfileThread.hpp"
#include "Exception.hpp"

namespace geopm
{
    ProfileThread::ProfileThread(int num_thread, size_t num_iter, size_t chunk_size)
        : m_num_iter(num_iter)
        , m_num_thread(num_thread)
        , m_chunk_size(chunk_size)
        , m_stride(64 / sizeof(uint32_t)) // 64 byte cache line
        , m_progress(NULL)
        , m_norm(m_chunk_size ? m_num_thread : 1)
    {
        if (num_iter == 0 || num_thread <= 0) {
            throw Exception("ProfileThread(): invalid arguments", GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }

        // Using array with posix_memalign rather than alignas()
        int err = posix_memalign((void**)(&m_progress), 64, num_thread * m_stride * sizeof(uint32_t));
        if (err) {
            throw Exception("ProfileThread(): could not allocate m_progress vector", err, __FILE__, __LINE__);
        }
        memset(m_progress, 0, num_thread * m_stride * sizeof(uint32_t));

        // Calculate the norm: the inverse number of iterations
        // assigned to each thread
        if (chunk_size) {
            size_t num_chunk = m_num_iter / m_chunk_size;
            size_t unchunked = m_num_iter % m_chunk_size;
            size_t min_it = m_chunk_size * (num_chunk / m_num_thread);
            int last_full_thread = num_chunk % m_num_thread;
            for (int i = 0; i < m_num_thread; ++i) {
                m_norm[i] = (double) min_it;
                if (i < last_full_thread) {
                    m_norm[i] += (double) chunk_size;
                }
                else if (i == last_full_thread) {
                    m_norm[i] += (double) unchunked;
                }
                if (m_norm[i]) {
                    m_norm[i] = 1.0 / m_norm[i];
                }
            }
        }
        else {
            m_norm[0] = 1.0 / m_num_iter;
        }
    }

    ProfileThread::ProfileThread(int num_thread, size_t num_iter)
        : ProfileThread(num_thread, num_iter, 0)
    {

    }

    ProfileThread::~ProfileThread()
    {
        free(m_progress);
    }

    void ProfileThread::increment(IProfile &prof, uint64_t region_id, int thread_idx)
    {
        ++m_progress[m_stride * thread_idx];
        if (!thread_idx) {
            double result;

            if (m_chunk_size) {
                result = DBL_MAX;
                for (int j = 0; j != m_num_thread; ++j) {
                    if (m_norm[j]) {
                        double tmp = m_progress[j * m_stride] * m_norm[j];
                        result =  tmp < result ? tmp : result;
                    }
                }
            }
            else {
                result = 0.0;
                for (int j = 0; j != m_num_thread; ++j) {
                    result += m_progress[j * m_stride];
                }
                result *= m_norm[0];
            }
            prof.progress(region_id, result);
        }
    }
}
