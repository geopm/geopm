/*
 * Copyright (c) 2015, 2016, Intel Corporation
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

#include <stdlib.h>
#include <iostream>
#include <time.h>
#include <math.h>
#include <string>
#include <string.h>
#include <mpi.h>
#ifdef _OPENMP
#include <omp.h>
#endif

#include "geopm.h"
#include "ModelRegion.hpp"
#include "Exception.hpp"
#include "tutorial_region.h"
#include "imbalancer.h"
#ifdef TUTORIAL_ENABLE_MKL
#include <mkl.h>
#endif

namespace geopm
{
    ModelRegionBase *model_region_factory(std::string name, double big_o, int verbosity)
    {
        bool do_imbalance = (name.find("-imbalance") != std::string::npos);
        bool do_progress = (name.find("-progress") != std::string::npos);

        ModelRegionBase *result = NULL;
        if (name.find("sleep") == 0 &&
            (name[strlen("sleep")] == '\0' ||
             name[strlen("sleep")] == '-')) {
            result = new SleepModelRegion(big_o, verbosity, do_imbalance, do_progress);
        }
        else if (name.find("dgemm") == 0 &&
                 (name[strlen("dgemm")] == '\0' ||
                  name[strlen("dgemm")] == '-'))  {
            result = new DGEMMModelRegion(big_o, verbosity, do_imbalance, do_progress);
        }
        else if (name.find("stream") == 0 &&
                 (name[strlen("stream")] == '\0' ||
                  name[strlen("stream")] == '-')) {
            result = new StreamModelRegion(big_o, verbosity, do_imbalance, do_progress);
        }
        else if (name.find("all2all") == 0 &&
                 (name[strlen("all2all")] == '\0' ||
                  name[strlen("all2all")] == '-')) {
            result = new All2allModelRegion(big_o, verbosity, do_imbalance, do_progress);
        }
        else {
            throw Exception("model_region_factory: unknown name: " + name,
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        return result;
    }

    ModelRegionBase::ModelRegionBase(int verbosity)
        : m_big_o(0.0)
        , m_verbosity(verbosity)
        , m_do_imbalance(false)
        , m_do_progress(false)
        , m_loop_count(1)
    {

    }

    ModelRegionBase::~ModelRegionBase()
    {

    }

    std::string ModelRegionBase::name(void)
    {
        return m_name;
    }

    double ModelRegionBase::big_o(void)
    {
        return m_big_o;
    }

    SleepModelRegion::SleepModelRegion(double big_o_in, int verbosity, bool do_imbalance, bool do_progress)
        : ModelRegionBase(verbosity)
    {
        m_name = "sleep";
        m_do_imbalance = do_imbalance;
        m_do_progress = do_progress;
        big_o(big_o_in);
        int err = geopm_prof_region(m_name.c_str(), GEOPM_POLICY_HINT_UNKNOWN, &m_region_id);
        if (err) {
            throw Exception("SleepModelRegion::SleepModelRegion()",
                            err, __FILE__, __LINE__);
        }
    }

    SleepModelRegion::~SleepModelRegion()
    {

    }

    void SleepModelRegion::big_o(double big_o_in)
    {
        if (!m_do_progress) {
            m_loop_count = 1;
        }
        else if (big_o_in > 10) {
            m_loop_count = (uint64_t)big_o_in;
        }
        else {
            m_loop_count = 10;
        }

        double seconds = big_o_in / m_loop_count;
        m_delay = {(time_t)(seconds),
                   (long)((seconds - (time_t)(seconds)) * 1E9)};

        m_big_o = big_o_in;
    }

    void SleepModelRegion::run(void)
    {
        if (m_big_o != 0.0) {
            if (m_verbosity) {
                std::cout << "Executing " << m_big_o << " second sleep."  << std::endl << std::flush;
            }
            double norm = 1.0 / m_loop_count;
            (void)geopm_prof_enter(m_region_id);
            for (uint64_t i = 0 ; i < m_loop_count; ++i) {
                if (m_do_progress) {
                    geopm_prof_progress(m_region_id, i * norm);
                }
                if (m_do_imbalance) {
                    (void)imbalancer_enter();
                }

                int err = clock_nanosleep(CLOCK_REALTIME, 0, &m_delay, NULL);
                if (err) {
                    throw Exception("SleepModelRegion::run()",
                                    GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
                }

                if (m_do_imbalance) {
                    (void)imbalancer_exit();
                }
            }
            (void)geopm_prof_exit(m_region_id);
        }
    }

    DGEMMModelRegion::DGEMMModelRegion(double big_o_in, int verbosity, bool do_imbalance, bool do_progress)
        : m_matrix_a(NULL)
        , m_matrix_b(NULL)
        , m_matrix_c(NULL)
        , m_matrix_size(0)
        , m_pad_size(64)
        , ModelRegionBase(verbosity)
    {
        m_name = "dgemm";
        m_do_imbalance = do_imbalance;
        m_do_progress = do_progress;
        big_o(big_o_in);
        int err = geopm_prof_region(m_name.c_str(), GEOPM_POLICY_HINT_COMPUTE, &m_region_id);
        if (err) {
            throw Exception("DGEMMModelRegion::DGEMMModelRegion()",
                            err, __FILE__, __LINE__);
        }
    }

    DGEMMModelRegion::~DGEMMModelRegion()
    {
        big_o(0);
    }

    void DGEMMModelRegion::big_o(double big_o_in)
    {
        if (m_big_o && m_big_o != big_o_in) {
            free(m_matrix_c);
            free(m_matrix_b);
            free(m_matrix_a);
        }

        if (!m_do_progress) {
            m_loop_count = 1;
        }
        else if (big_o_in > 10) {
            m_loop_count = (uint64_t)big_o_in;
        }
        else {
            m_loop_count = 10;
        }

        m_matrix_size = (int)pow(4e9 * big_o_in / m_loop_count, 1.0/3.0);
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
            for (int i = 0; i < mem_size / sizeof(double); ++i) {
                m_matrix_a[i] = 2.0 * i;
                m_matrix_b[i] = 3.0 * i;
            }
        }
        m_big_o = big_o_in;
    }

    void DGEMMModelRegion::run(void)
    {
        if (m_big_o != 0.0) {
            if (m_verbosity) {
                std::cout << "Executing " << m_matrix_size << " x " << m_matrix_size << " DGEMM "
                          << m_loop_count << " times." << std::endl << std::flush;
            }
            double norm = 1.0 / m_loop_count;
            (void)geopm_prof_enter(m_region_id);
            for (uint64_t i = 0; i < m_loop_count; ++i) {
                if (m_do_progress) {
                    geopm_prof_progress(m_region_id, i * norm);
                }
                if (m_do_imbalance) {
                    (void)imbalancer_enter();
                }

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

                if (m_do_imbalance) {
                    (void)imbalancer_exit();
                }
            }
            (void)geopm_prof_exit(m_region_id);
        }
    }

    StreamModelRegion::StreamModelRegion(double big_o_in, int verbosity, bool do_imbalance, bool do_progress)
        : m_array_a(NULL)
        , m_array_b(NULL)
        , m_array_c(NULL)
        , m_array_len(0)
        , m_align(64)
        , ModelRegionBase(verbosity)
    {
        m_name = "stream";
        m_do_imbalance = do_imbalance;
        m_do_progress = do_progress;
        big_o(big_o_in);
        int err = geopm_prof_region(m_name.c_str(), GEOPM_POLICY_HINT_MEMORY, &m_region_id);
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
        if (m_big_o && m_big_o != big_o_in) {
            free(m_array_c);
            free(m_array_b);
            free(m_array_a);
        }

        if (!m_do_progress) {
            m_loop_count = 1;
        }
        else if (big_o_in > 10) {
            m_loop_count = (uint64_t)big_o_in;
        }
        else {
            m_loop_count = 10;
        }

        m_array_len = (size_t)(5e8 * big_o_in / m_loop_count);
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
            for (int i = 0; i < m_array_len; i++) {
                m_array_a[i] = 0.0;
                m_array_b[i] = 1.0;
                m_array_c[i] = 2.0;
            }
        }
        m_big_o = big_o_in;
    }

    void StreamModelRegion::run(void)
    {
        if (m_big_o != 0.0) {
            if (m_verbosity) {
                std::cout << "Executing " << m_array_len * m_loop_count << " array length stream triadd."  << std::endl << std::flush;
            }
            double norm = 1.0 / m_loop_count;
            (void)geopm_prof_enter(m_region_id);
            for (uint64_t i = 0; i < m_loop_count; ++i) {
                if (m_do_progress) {
                    geopm_prof_progress(m_region_id, i * norm);
                }
                if (m_do_imbalance) {
                    (void)imbalancer_enter();
                }

                double scalar = 3.0;
#pragma omp parallel for
                for (int i = 0; i < m_array_len; ++i) {
                    m_array_a[i] = m_array_b[i] + scalar * m_array_c[i];
                }
                if (m_do_imbalance) {
                    (void)imbalancer_exit();
                }
            }
            (void)geopm_prof_exit(m_region_id);
        }
    }

    All2allModelRegion::All2allModelRegion(double big_o_in, int verbosity, bool do_imbalance, bool do_progress)
        : m_send_buffer(NULL)
        , m_recv_buffer(NULL)
        , m_num_send(0)
        , m_num_rank(0)
        , m_align(64)
        , ModelRegionBase(verbosity)
    {
        m_name = "all2all";
        m_do_imbalance = do_imbalance;
        m_do_progress = do_progress;
        big_o(big_o_in);
        int err = geopm_prof_region(m_name.c_str(), GEOPM_POLICY_HINT_NETWORK, &m_region_id);
        if (err) {
            throw Exception("All2allModelRegion::All2allModelRegion()",
                            err, __FILE__, __LINE__);
        }
    }

    All2allModelRegion::~All2allModelRegion()
    {
        big_o(0);
    }

    void All2allModelRegion::big_o(double big_o_in)
    {
        if (m_big_o && m_big_o != big_o_in) {
            free(m_recv_buffer);
            free(m_send_buffer);
        }

        if (!m_do_progress) {
            m_loop_count = 1;
        }
        else if (big_o_in > 10) {
            m_loop_count = (uint64_t)big_o_in;
        }
        else {
            m_loop_count = 10;
        }

        int err = MPI_Comm_size(MPI_COMM_WORLD, &m_num_rank);
        if (err) {
            throw Exception("MPI_Comm_size()",
                            err, __FILE__, __LINE__);
        }
        m_num_send = (size_t)pow(2.0, 16 * big_o_in / m_loop_count - m_num_rank / 128.0);
        m_num_send = m_num_send ? m_num_send : 1;

        if (big_o_in && m_big_o != big_o_in) {
            err = posix_memalign((void **)&m_send_buffer, m_align,
                                 m_num_rank * m_num_send * sizeof(char));
            if (!err) {
                err = posix_memalign((void **)&m_recv_buffer, m_align,
                                     m_num_rank * m_num_send * sizeof(char));
            }
            if (err) {
                throw Exception("All2allModelRegion::big_o(): posix_memalign() failed",
                                err, __FILE__, __LINE__);
            }
#pragma omp parallel for
            for (int i = 0; i < m_num_rank * m_num_send; i++) {
                m_send_buffer[i] = (char)(i);
            }
        }
        m_big_o = big_o_in;
    }

    void All2allModelRegion::run(void)
    {
        if (m_big_o != 0) {
            if (m_verbosity) {
                std::cout << "Executing " << m_num_send << " byte buffer all2all "
                          << m_loop_count << " times."  << std::endl << std::flush;
            }
            double norm = 1.0 / m_loop_count;
            (void)geopm_prof_enter(m_region_id);
            for (uint64_t i = 0; i < m_loop_count; ++i) {
                if (m_do_progress) {
                    geopm_prof_progress(m_region_id, i * norm);
                }
                if (m_do_imbalance) {
                    (void)imbalancer_enter();
                }

                int err = MPI_Alltoall(m_send_buffer, m_num_send, MPI_CHAR, m_recv_buffer,
                                       m_num_send, MPI_CHAR, MPI_COMM_WORLD);
                if (err) {
                    throw Exception("MPI_Alltoall()", err, __FILE__, __LINE__);
                }
                err = MPI_Barrier(MPI_COMM_WORLD);
                if (err) {
                    throw Exception("MPI_Barrier()", err, __FILE__, __LINE__);
                }
            }

            if (m_do_imbalance) {
                (void)imbalancer_exit();
            }
            (void)geopm_prof_exit(m_region_id);
        }
    }
}
