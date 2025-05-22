/*
 * Copyright (c) 2015 - 2025 Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "DGEMMModelRegion.hpp"

#include <iostream>
#include <cmath>

#include "geopm_prof.h"
#include "geopm_hint.h"
#include "geopm/Exception.hpp"
#include "geopm/Profile.hpp"
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
#ifdef GEOPM_ENABLE_OMPT
#pragma omp parallel for
#endif
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
        , m_matrix_m_size(4096)
        , m_matrix_n_size(1024)
        , m_matrix_k_size(2048)
        , m_pad_size(geopm::hardware_destructive_interference_size)
        , m_num_warmup(4)
    {
        m_name = "dgemm";
        m_do_imbalance = do_imbalance;
        m_do_progress = do_progress;
        m_do_unmarked = do_unmarked;
        ModelRegion::region(GEOPM_REGION_HINT_COMPUTE);
        big_o(big_o_in);
        warmup();
    }

    DGEMMModelRegion::~DGEMMModelRegion()
    {
        cleanup();
    }

    void DGEMMModelRegion::cleanup(void)
    {
        if (m_matrix_a) {
            free(m_matrix_a);
            m_matrix_a = NULL;
        }
        if (m_matrix_b) {
            free(m_matrix_b);
            m_matrix_b = NULL;
        }
        if (m_matrix_c) {
            free(m_matrix_c);
            m_matrix_c = NULL;
        }
    }

    void DGEMMModelRegion::num_progress_updates(double big_o_in)
    {
        m_num_progress_updates = (uint64_t)(100.0 * big_o_in);
        (void)geopm_tprof_init(m_num_progress_updates);
    }

    void DGEMMModelRegion::big_o(double big_o_in)
    {
        if (m_big_o && m_big_o != big_o_in) {
            cleanup();
        }
        geopm_prof_region("geopm_dgemm_model_region_startup", GEOPM_REGION_HINT_IGNORE, &m_start_rid);
        geopm_prof_enter(m_start_rid);

        num_progress_updates(big_o_in);

        if (big_o_in && m_big_o != big_o_in) {
            // Allocate A: M x K
            size_t mem_size_a = sizeof(double) * (m_matrix_m_size + m_pad_size) * m_matrix_k_size;
            int err = posix_memalign((void **)&m_matrix_a, m_pad_size, mem_size_a);
            if (err) {
                throw Exception("DGEMMModelRegion::big_o(): posix_memalign() failed",
                                err, __FILE__, __LINE__);
            }
#ifdef GEOPM_ENABLE_OMPT
#pragma omp parallel for
#endif
            for (size_t i = 0; i < mem_size_a / sizeof(double); ++i) {
                m_matrix_a[i] = 2.0 * i;
            }
            // Allocate B: K x N
            size_t mem_size_b = sizeof(double) * (m_matrix_k_size + m_pad_size) * m_matrix_n_size;
            err = posix_memalign((void **)&m_matrix_b, m_pad_size, mem_size_b);
            if (err) {
                free(m_matrix_a);
                m_matrix_a = NULL;
                throw Exception("DGEMMModelRegion::big_o(): posix_memalign() failed",
                                err, __FILE__, __LINE__);
            }
#ifdef GEOPM_ENABLE_OMPT
#pragma omp parallel for
#endif
            for (size_t i = 0; i < mem_size_b / sizeof(double); ++i) {
                m_matrix_b[i] = 3.0 * i;
            }
            // Allocate C: M x N
            size_t mem_size_c = sizeof(double) * (m_matrix_m_size + m_pad_size) * m_matrix_n_size;
            err = posix_memalign((void **)&m_matrix_c, m_pad_size, mem_size_c);
            if (err) {
                free(m_matrix_b);
                free(m_matrix_a);
                m_matrix_b = NULL;
                m_matrix_a = NULL;
                throw Exception("DGEMMModelRegion::big_o(): posix_memalign() failed",
                                err, __FILE__, __LINE__);
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
        // DGEMM: C = alpha*A*B + beta*C
        // A: M x K, B: K x N, C: M x N
        int M = m_matrix_m_size;
        int N = m_matrix_n_size;
        int K = m_matrix_k_size;
        int LDA = m_matrix_m_size + m_pad_size / sizeof(double); // leading dimension of A: M
        int LDB = m_matrix_k_size + m_pad_size / sizeof(double); // leading dimension of B: K
        int LDC = m_matrix_m_size + m_pad_size / sizeof(double); // leading dimension of C: M
        double alpha = 2.0;
        double beta = 3.0;
        char transa = 'n';
        char transb = 'n';
        if (m_big_o != 0.0) {
            if (m_verbosity) {
                std::cout << "Executing DGEMM (M=" << m_matrix_m_size
                          << ", N=" << m_matrix_n_size
                          << ", K=" << m_matrix_k_size << ")" << std::endl;
            }
            ModelRegion::region_enter();
            for (uint64_t i = 0; i < m_num_progress_updates; ++i) {
                ModelRegion::loop_enter(i);

                dgemm(&transa, &transb, &M, &N, &K, &alpha, m_matrix_a, &LDA,
                      m_matrix_b, &LDB, &beta, m_matrix_c, &LDC);

                ModelRegion::loop_exit();
            }
            ModelRegion::region_exit();
        }
    }
}
