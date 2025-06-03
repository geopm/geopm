/*
 * Copyright (c) 2015 - 2025 Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

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

    void StreamModelRegion::num_progress_updates(double big_o_in)
    {
        m_num_progress_updates = (uint64_t)(100.0 * big_o_in);
        if (m_num_progress_updates == 0) {
            m_num_progress_updates = 1;
        }
        (void)geopm_tprof_init(m_num_progress_updates);
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

        m_array_len = 33554432ULL; // 768 MB total allocation for three arrays
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
#ifdef GEOPM_ENABLE_OMPT
#pragma omp parallel for
#endif
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
            double scalar = 3.0;
            for (uint64_t i = 0; i < m_num_progress_updates; ++i) {
                ModelRegion::loop_enter(i);
#ifdef GEOPM_ENABLE_OMPT
#pragma omp parallel for
#endif
                for (size_t j = 0; j < m_array_len; ++j) {
                    m_array_a[j] = m_array_b[j] + scalar * m_array_c[j];
                }
                ModelRegion::loop_exit();
            }
            ModelRegion::region_exit();
        }
    }
}
