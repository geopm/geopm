/*
 * Copyright (c) 2015 - 2023, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "config.h"
#include "DGEMMModelRegion.hpp"

#include <iostream>
#include <cmath>

#include "geopm_prof.h"
#include "geopm_hint.h"
#include "geopm/Exception.hpp"
#include "Profile.hpp"
#include "geopm/Helper.hpp"

#ifdef GEOPM_ENABLE_MKL
#include <mkl.h>
#else
// Terrible DGEMM implementation should only be used if there is no
// BLAS support.  Build assumes that the Intel(R) Math Kernel Library
// is the only provider of BLAS.
static inline
void dgemm(const char *transa, const char *transb, const int *M,
           const int *N, const int *K, const double *alpha,
           const double *A, const int *LDA, const double *B,
           const int *LDB, const double *beta, double *C, const int *LDC)
{
#pragma omp parallel for
    for (int i = 0; i < *M; ++i) {
        for (int j = 0; j < *N; ++j) {
            C[i * *LDC + j] = 0;
            for (int k = 0; k < *K; ++k) {
                C[i * *LDC + j] += A[i * *LDA + j] * B[j * *LDB + k];
            }
        }
    }
}
#endif

namespace geopm
{
    DGEMMModelRegion::DGEMMModelRegion(double big_o_in,
                                       int verbosity,
                                       bool do_imbalance,
                                       bool do_progress,
                                       bool do_unmarked)
        : ModelRegion(verbosity)
        , m_matrix_a(NULL)
        , m_matrix_b(NULL)
        , m_matrix_c(NULL)
        , m_matrix_size(0)
        , m_pad_size(geopm::hardware_destructive_interference_size)
        , m_num_warmup(4)
    {
        m_name = "dgemm";
        m_do_imbalance = do_imbalance;
        m_do_progress = do_progress;
        m_do_unmarked = do_unmarked;
        int err = ModelRegion::region(GEOPM_REGION_HINT_COMPUTE);
        if (err) {
            throw Exception("DGEMMModelRegion::DGEMMModelRegion()",
                            err, __FILE__, __LINE__);
        }
        big_o(big_o_in);
        warmup();
    }

    DGEMMModelRegion::~DGEMMModelRegion()
    {
        cleanup();
    }

    void DGEMMModelRegion::cleanup(void)
    {
        free(m_matrix_c);
        free(m_matrix_b);
        free(m_matrix_a);
    }

    void DGEMMModelRegion::big_o(double big_o_in)
    {
        if (m_big_o && m_big_o != big_o_in) {
            cleanup();
        }

        geopm_prof_region("geopm_dgemm_model_region_startup", GEOPM_REGION_HINT_IGNORE, &m_start_rid);
        geopm_prof_enter(m_start_rid);

        num_progress_updates(big_o_in);

        m_matrix_size = (int)pow(4e9 * big_o_in / m_num_progress_updates, 1.0/3.0);
        if (big_o_in && m_big_o != big_o_in) {
            size_t mem_size = sizeof(double) * (m_matrix_size * (m_matrix_size + m_pad_size));
            int err = posix_memalign((void **)&m_matrix_a, m_pad_size, mem_size);
            if (!err) {
                err = posix_memalign((void **)&m_matrix_b, m_pad_size, mem_size);
            }
            if (!err) {
                err = posix_memalign((void **)&m_matrix_c, m_pad_size, mem_size);
            }
            if (err) {
                throw Exception("DGEMMModelRegion::big_o(): posix_memalign() failed",
                                err, __FILE__, __LINE__);
            }
#pragma omp parallel for
            for (size_t i = 0; i < mem_size / sizeof(double); ++i) {
                m_matrix_a[i] = 2.0 * i;
                m_matrix_b[i] = 3.0 * i;
            }
        }
        m_big_o = big_o_in;
        geopm_prof_exit(m_start_rid);
    }

    void DGEMMModelRegion::warmup(void)
    {
        geopm_prof_enter(m_start_rid);
        for (int warmup_idx = 0; warmup_idx != m_num_warmup; ++warmup_idx) {
            run();
        }
        geopm_prof_exit(m_start_rid);
    }

    void DGEMMModelRegion::run(void)
    {
        if (m_big_o != 0.0) {
            if (m_verbosity) {
                std::cout << "Executing " << m_matrix_size << " x " << m_matrix_size << " DGEMM "
                          << m_num_progress_updates << " times." << std::endl << std::flush;
            }
            ModelRegion::region_enter();
            for (uint64_t i = 0; i < m_num_progress_updates; ++i) {
                ModelRegion::loop_enter(i);

                int M = m_matrix_size;
                int N = m_matrix_size;
                int K = m_matrix_size;
                int LDA = m_matrix_size + m_pad_size / sizeof(double);
                int LDB = m_matrix_size + m_pad_size / sizeof(double);
                int LDC = m_matrix_size + m_pad_size / sizeof(double);
                double alpha = 2.0;
                double beta = 3.0;
                char transa = 'n';
                char transb = 'n';

                dgemm(&transa, &transb, &M, &N, &K, &alpha,
                      m_matrix_a, &LDA, m_matrix_b, &LDB, &beta, m_matrix_c, &LDC);

                ModelRegion::loop_exit();
            }
            ModelRegion::region_exit();
        }
    }
}
