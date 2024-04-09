/*
 * Copyright (c) 2015 - 2024, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "config.h"
#include "StreamModelRegion.hpp"

#include <iostream>

#include "geopm_prof.h"
#include "geopm_hint.h"
#include "geopm/Exception.hpp"
#include "geopm/Profile.hpp"
#include "geopm/Helper.hpp"

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
        , m_align(geopm::hardware_destructive_interference_size)
    {
        m_name = "stream";
        m_do_imbalance = do_imbalance;
        m_do_progress = do_progress;
        m_do_unmarked = do_unmarked;
        ModelRegion::region(GEOPM_REGION_HINT_MEMORY);
        big_o(big_o_in);
    }

    StreamModelRegion::~StreamModelRegion()
    {
        cleanup();
    }

    void StreamModelRegion::cleanup(void)
    {
        free(m_array_c);
        free(m_array_b);
        free(m_array_a);
    }

    void StreamModelRegion::big_o(double big_o_in)
    {
        if (m_big_o && m_big_o != big_o_in) {
            cleanup();
        }

        uint64_t start_rid = 0;
        geopm_prof_region("geopm_stream_model_region_startup", GEOPM_REGION_HINT_IGNORE, &start_rid);
        geopm_prof_enter(start_rid);

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

        geopm_prof_exit(start_rid);
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
