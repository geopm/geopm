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
#include "StreamModelRegion.hpp"

#include <iostream>

#include "geopm.h"
#include "Exception.hpp"
#include "Profile.hpp"

namespace geopm
{
    StreamModelRegion::StreamModelRegion(double big_o_in,
                                         int verbosity,
                                         bool do_imbalance,
                                         bool do_progress,
                                         bool do_unmarked)
        : ModelRegion(verbosity)
        , m_array_a(NULL)
        , m_array_b(NULL)
        , m_array_c(NULL)
        , m_array_len(0)
        , m_align(64)
    {
        m_name = "stream";
        m_do_imbalance = do_imbalance;
        m_do_progress = do_progress;
        m_do_unmarked = do_unmarked;
        big_o(big_o_in);
        int err = ModelRegion::region(GEOPM_REGION_HINT_MEMORY);
        if (err) {
            throw Exception("StreamModelRegion::StreamModelRegion()",
                            err, __FILE__, __LINE__);
        }
    }

    StreamModelRegion::~StreamModelRegion()
    {
        big_o(0);
    }

    void StreamModelRegion::big_o(double big_o_in)
    {
        geopm::Profile &prof = geopm::Profile::default_profile();
        uint64_t start_rid = prof.region("geopm_stream_model_region_startup", GEOPM_REGION_HINT_IGNORE);
        prof.enter(start_rid);

        if (m_big_o && m_big_o != big_o_in) {
            free(m_array_c);
            free(m_array_b);
            free(m_array_a);
        }

        num_progress_updates(big_o_in);

        m_array_len = (size_t)(5e8 * big_o_in);
        if (big_o_in && m_big_o != big_o_in) {
            int err = posix_memalign((void **)&m_array_a, m_align, m_array_len * sizeof(double));
            if (!err) {
                err = posix_memalign((void **)&m_array_b, m_align, m_array_len * sizeof(double));
            }
            if (!err) {
                err = posix_memalign((void **)&m_array_c, m_align, m_array_len * sizeof(double));
            }
            if (err) {
                throw Exception("StreamModelRegion::big_o(): posix_memalign() failed",
                                err, __FILE__, __LINE__);
            }
#pragma omp parallel for
            for (size_t i = 0; i < m_array_len; i++) {
                m_array_a[i] = 0.0;
                m_array_b[i] = 1.0;
                m_array_c[i] = 2.0;
            }
        }
        m_big_o = big_o_in;

        prof.exit(start_rid);
    }

    void StreamModelRegion::run(void)
    {
        if (m_big_o != 0.0) {
            if (m_verbosity) {
                std::cout << "Executing " << m_array_len * m_num_progress_updates << " array length stream triadd."  << std::endl << std::flush;
            }
            ModelRegion::region_enter();
            size_t block_size = m_array_len / m_num_progress_updates;
            double scalar = 3.0;
            for (uint64_t i = 0; i < m_num_progress_updates; ++i) {
                ModelRegion::loop_enter(i);
#pragma omp parallel for
                for (size_t j = 0; j < block_size; ++j) {
                    m_array_a[i * block_size + j] = m_array_b[i * block_size + j] + scalar * m_array_c[i * block_size + j];
                }

                ModelRegion::loop_exit();
            }
            size_t remainder = m_array_len;
            if (block_size != 0) {
                remainder = m_array_len % block_size;
            }
            for (uint64_t j = 0; j < remainder; ++j) {
                m_array_a[m_num_progress_updates * block_size + j] = m_array_b[m_num_progress_updates * block_size + j] + scalar * m_array_c[m_num_progress_updates * block_size + j];
            }
            ModelRegion::region_exit();
        }
    }
}
