/*
 * Copyright (c) 2015, 2016, 2017, Intel Corporation
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
#include "geopm_time.h"
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
        bool do_unmarked = (name.find("-unmarked") != std::string::npos);

        if (do_unmarked) {
            do_progress = false;
        }

        ModelRegionBase *result = NULL;
        if (name.find("sleep") == 0 &&
            (name[strlen("sleep")] == '\0' ||
             name[strlen("sleep")] == '-')) {
            result = new SleepModelRegion(big_o, verbosity, do_imbalance, do_progress, do_unmarked);
        }
        else if (name.find("spin") == 0 &&
                 (name[strlen("spin")] == '\0' ||
                  name[strlen("spin")] == '-'))  {
            result = new SpinModelRegion(big_o, verbosity, do_imbalance, do_progress, do_unmarked);
        }
        else if (name.find("dgemm") == 0 &&
                 (name[strlen("dgemm")] == '\0' ||
                  name[strlen("dgemm")] == '-'))  {
            result = new DGEMMModelRegion(big_o, verbosity, do_imbalance, do_progress, do_unmarked);
        }
        else if (name.find("stream") == 0 &&
                 (name[strlen("stream")] == '\0' ||
                  name[strlen("stream")] == '-')) {
            result = new StreamModelRegion(big_o, verbosity, do_imbalance, do_progress, do_unmarked);
        }
        else if (name.find("all2all") == 0 &&
                 (name[strlen("all2all")] == '\0' ||
                  name[strlen("all2all")] == '-')) {
            result = new All2allModelRegion(big_o, verbosity, do_imbalance, do_progress, do_unmarked);
        }
        else if (name.find("nested") == 0 &&
                 (name[strlen("nested")] == '\0' ||
                  name[strlen("nested")] == '-')) {
            result = new NestedModelRegion(big_o, verbosity, do_imbalance, do_progress, do_unmarked);
        }
        else if (name.find("ignore") == 0 &&
            (name[strlen("ignore")] == '\0' ||
             name[strlen("ignore")] == '-')) {
            result = new IgnoreModelRegion(big_o, verbosity, do_imbalance, do_progress, do_unmarked);
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
        , m_do_unmarked(false)
        , m_loop_count(1)
        , m_norm(1.0)
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

    void ModelRegionBase::loop_count(double big_o_in)
    {
        if (!m_do_progress) {
            m_loop_count = 1;
        }
        else if (big_o_in > 1.0) {
            m_loop_count = (uint64_t)(100.0 * big_o_in);
        }
        else {
            m_loop_count = 100;
        }
        m_norm = 1.0 / m_loop_count;
    }

    int ModelRegionBase::region(void)
    {
        int err = 0;
        if (!m_do_unmarked) {
            err = geopm_prof_region(m_name.c_str(), GEOPM_REGION_HINT_UNKNOWN, &m_region_id);
        }
        return err;
    }

    void ModelRegionBase::region_enter(void)
    {
        if (!m_do_unmarked) {
            (void)geopm_prof_enter(m_region_id);
        }
    }

    void ModelRegionBase::region_exit(void)
    {
        if (!m_do_unmarked) {
            (void)geopm_prof_exit(m_region_id);
        }
    }

    void ModelRegionBase::loop_enter(uint64_t iteration)
    {
        if (m_do_progress) {
            (void)geopm_prof_progress(m_region_id, iteration * m_norm);
        }
        if (m_do_imbalance) {
            (void)imbalancer_enter();
        }
    }

    void ModelRegionBase::loop_exit(void)
    {
        if (m_do_imbalance) {
            (void)imbalancer_exit();
        }
    }

    SleepModelRegion::SleepModelRegion(double big_o_in, int verbosity, bool do_imbalance, bool do_progress, bool do_unmarked)
        : ModelRegionBase(verbosity)
    {
        m_name = "sleep";
        m_do_imbalance = do_imbalance;
        m_do_progress = do_progress;
        m_do_unmarked = do_unmarked;
        big_o(big_o_in);
        int err = ModelRegionBase::region();
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
        loop_count(big_o_in);
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
            ModelRegionBase::region_enter();
            for (uint64_t i = 0 ; i < m_loop_count; ++i) {
                ModelRegionBase::loop_enter(i);
                int err;
#ifdef __APPLE__
                err = nanosleep(&m_delay, NULL);
#else
                err = clock_nanosleep(CLOCK_REALTIME, 0, &m_delay, NULL);
#endif

                if (err) {
                    throw Exception("SleepModelRegion::run()",
                                    GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
                }
                ModelRegionBase::loop_exit();
            }
            ModelRegionBase::region_exit();
        }
    }

    SpinModelRegion::SpinModelRegion(double big_o_in, int verbosity, bool do_imbalance, bool do_progress, bool do_unmarked)
        : ModelRegionBase(verbosity)
    {
        m_name = "spin";
        m_do_imbalance = do_imbalance;
        m_do_progress = do_progress;
        m_do_unmarked = do_unmarked;
        big_o(big_o_in);
        int err = ModelRegionBase::region();
        if (err) {
            throw Exception("SpinModelRegion::SpinModelRegion()",
                            err, __FILE__, __LINE__);
        }
    }

    SpinModelRegion::~SpinModelRegion()
    {

    }

    void SpinModelRegion::big_o(double big_o_in)
    {
        loop_count(big_o_in);
        m_delay = big_o_in / m_loop_count;
        m_big_o = big_o_in;
    }

    void SpinModelRegion::run(void)
    {
        if (m_big_o != 0.0) {
            if (m_verbosity) {
                std::cout << "Executing " << m_big_o << " second spin."  << std::endl << std::flush;
            }
            ModelRegionBase::region_enter();
            for (uint64_t i = 0 ; i < m_loop_count; ++i) {
                ModelRegionBase::loop_enter(i);

                double timeout = 0.0;
                struct geopm_time_s start = {{0,0}};
                struct geopm_time_s curr = {{0,0}};
                (void)geopm_time(&start);
                while (timeout < m_delay) {
                    (void)geopm_time(&curr);
                    timeout = geopm_time_diff(&start, &curr);
                }

                ModelRegionBase::loop_exit();
            }
            ModelRegionBase::region_exit();
        }
    }

    DGEMMModelRegion::DGEMMModelRegion(double big_o_in, int verbosity, bool do_imbalance, bool do_progress, bool do_unmarked)
        : ModelRegionBase(verbosity)
        , m_matrix_a(NULL)
        , m_matrix_b(NULL)
        , m_matrix_c(NULL)
        , m_matrix_size(0)
        , m_pad_size(64)
    {
        m_name = "dgemm";
        m_do_imbalance = do_imbalance;
        m_do_progress = do_progress;
        m_do_unmarked = do_unmarked;
        big_o(big_o_in);
        int err = ModelRegionBase::region();
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

        loop_count(big_o_in);

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
            for (size_t i = 0; i < mem_size / sizeof(double); ++i) {
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
            ModelRegionBase::region_enter();
            for (uint64_t i = 0; i < m_loop_count; ++i) {
                ModelRegionBase::loop_enter(i);

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

                ModelRegionBase::loop_exit();
            }
            ModelRegionBase::region_exit();
        }
    }

    StreamModelRegion::StreamModelRegion(double big_o_in, int verbosity, bool do_imbalance, bool do_progress, bool do_unmarked)
        : ModelRegionBase(verbosity)
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
        int err = ModelRegionBase::region();
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

        loop_count(big_o_in);

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
            for (size_t i = 0; i < m_array_len; i++) {
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
            ModelRegionBase::region_enter();
            for (uint64_t i = 0; i < m_loop_count; ++i) {
                ModelRegionBase::loop_enter(i);

                double scalar = 3.0;
#pragma omp parallel for
                for (size_t i = 0; i < m_array_len; ++i) {
                    m_array_a[i] = m_array_b[i] + scalar * m_array_c[i];
                }

                ModelRegionBase::loop_exit();
            }
            ModelRegionBase::region_exit();
        }
    }

    All2allModelRegion::All2allModelRegion(double big_o_in, int verbosity, bool do_imbalance, bool do_progress, bool do_unmarked)
        : ModelRegionBase(verbosity)
        , m_send_buffer(NULL)
        , m_recv_buffer(NULL)
        , m_num_send(0)
        , m_num_rank(0)
        , m_align(64)
        , m_rank(-1)
    {
        m_name = "all2all";
        m_do_imbalance = do_imbalance;
        m_do_progress = do_progress;
        m_do_unmarked = do_unmarked;
        big_o(big_o_in);
        int err = ModelRegionBase::region();
        if (!err) {
            err = MPI_Comm_rank(MPI_COMM_WORLD, &m_rank);
        }
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

        loop_count(big_o_in);

        int err = MPI_Comm_size(MPI_COMM_WORLD, &m_num_rank);
        if (err) {
            throw Exception("MPI_Comm_size()",
                            err, __FILE__, __LINE__);
        }

        if (m_loop_count > 1) {
            m_num_send = 1048576; //1MB
        }
        else {
            m_num_send = 10485760; //10MB
        }

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
            for (size_t i = 0; i < m_num_rank * m_num_send; i++) {
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
            ModelRegionBase::region_enter();
            for (uint64_t i = 0; i < m_loop_count; ++i) {
                ModelRegionBase::loop_enter(i);

                double timeout = 0.0;
                struct geopm_time_s start = {{0,0}};
                struct geopm_time_s curr = {{0,0}};
                int loop_done = 0;
                if (!m_rank) {
                    (void)geopm_time(&start);
                }
                while (!loop_done) {
                    int err = MPI_Alltoall(m_send_buffer, m_num_send, MPI_CHAR, m_recv_buffer,
                                           m_num_send, MPI_CHAR, MPI_COMM_WORLD);
                    if (err) {
                        throw Exception("MPI_Alltoall()", err, __FILE__, __LINE__);
                    }
                    if (!m_rank) {
                        (void)geopm_time(&curr);
                        timeout = geopm_time_diff(&start, &curr);
                        if (timeout > (m_big_o / m_loop_count)) {
                            loop_done = 1;
                        }
                    }
                    err = MPI_Bcast((void*)&loop_done, 1, MPI_INT, 0, MPI_COMM_WORLD);
                    if (err) {
                        throw Exception("MPI_Bcast()", err, __FILE__, __LINE__);
                    }
                }

                ModelRegionBase::loop_exit();
            }
            ModelRegionBase::region_exit();
        }
    }

    NestedModelRegion::NestedModelRegion(double big_o_in, int verbosity, bool do_imbalance, bool do_progress, bool do_unmarked)
        : ModelRegionBase(verbosity)
        , m_spin_region(big_o_in, verbosity, do_imbalance, do_progress, do_unmarked)
        , m_all2all_region(big_o_in, verbosity, do_imbalance, do_progress, do_unmarked)
    {
    }

    NestedModelRegion::~NestedModelRegion()
    {
    }

    void NestedModelRegion::big_o(double big_o_in)
    {
        m_spin_region.big_o(big_o_in);
        m_all2all_region.big_o(big_o_in);
    }

    void NestedModelRegion::run(void)
    {
        if (m_spin_region.m_big_o != 0.0 && m_all2all_region.m_big_o != 0.0) {
            (void)geopm_prof_epoch();
        }
        // Do spin
        if (m_spin_region.m_big_o != 0.0) {
            if (m_spin_region.m_verbosity) {
                std::cout << "Executing " << m_spin_region.m_big_o << " second spin."  << std::endl << std::flush;
            }
            (void)geopm_prof_enter(m_spin_region.m_region_id);
            for (uint64_t i = 0 ; i < m_spin_region.m_loop_count; ++i) {
                if (m_spin_region.m_do_imbalance) {
                    (void)imbalancer_enter();
                }

                double timeout = 0.0;
                struct geopm_time_s start = {{0,0}};
                struct geopm_time_s curr = {{0,0}};
                (void)geopm_time(&start);
                while (timeout < m_spin_region.m_delay) {
                    (void)geopm_time(&curr);
                    timeout = geopm_time_diff(&start, &curr);
                }

                if (m_spin_region.m_do_imbalance) {
                    (void)imbalancer_exit();
                }
            }
        }

        // Do all2all before spin region exit
        if (m_all2all_region.m_big_o != 0) {
            if (m_all2all_region.m_verbosity) {
                std::cout << "Executing " << m_all2all_region.m_num_send << " byte buffer all2all "
                          << m_all2all_region.m_loop_count << " times."  << std::endl << std::flush;
            }
            for (uint64_t i = 0; i < m_all2all_region.m_loop_count; ++i) {
                if (m_all2all_region.m_do_imbalance) {
                    (void)imbalancer_enter();
                }

                int err = MPI_Alltoall(m_all2all_region.m_send_buffer, m_all2all_region.m_num_send,
                                       MPI_CHAR, m_all2all_region.m_recv_buffer,
                                       m_all2all_region.m_num_send, MPI_CHAR, MPI_COMM_WORLD);
                if (err) {
                    throw Exception("MPI_Alltoall()", err, __FILE__, __LINE__);
                }
                err = MPI_Barrier(MPI_COMM_WORLD);
                if (err) {
                    throw Exception("MPI_Barrier()", err, __FILE__, __LINE__);
                }
            }

            if (m_all2all_region.m_do_imbalance) {
                (void)imbalancer_exit();
            }
        }

        // Do spin part deux.
        if (m_spin_region.m_big_o != 0.0) {
            if (m_spin_region.m_verbosity) {
                std::cout << "Executing " << m_spin_region.m_big_o << " second spin #2."  << std::endl << std::flush;
            }
            for (uint64_t i = 0 ; i < m_spin_region.m_loop_count; ++i) {
                if (m_spin_region.m_do_imbalance) {
                    (void)imbalancer_enter();
                }

                double timeout = 0.0;
                struct geopm_time_s start = {{0,0}};
                struct geopm_time_s curr = {{0,0}};
                (void)geopm_time(&start);
                while (timeout < m_spin_region.m_delay) {
                    (void)geopm_time(&curr);
                    timeout = geopm_time_diff(&start, &curr);
                }

                if (m_spin_region.m_do_imbalance) {
                    (void)imbalancer_exit();
                }
            }
            (void)geopm_prof_exit(m_spin_region.m_region_id);
        }
    }

    IgnoreModelRegion::IgnoreModelRegion(double big_o_in, int verbosity, bool do_imbalance, bool do_progress, bool do_unmarked)
        : ModelRegionBase(verbosity)
    {
        m_name = "ignore";
        m_do_imbalance = do_imbalance;
        m_do_progress = do_progress;
        m_do_unmarked = do_unmarked;
        big_o(big_o_in);
        int err = ModelRegionBase::region();
        if (err) {
            throw Exception("IgnoreModelRegion::IgnoreModelRegion()",
                            err, __FILE__, __LINE__);
        }
    }

    IgnoreModelRegion::~IgnoreModelRegion()
    {

    }

    void IgnoreModelRegion::big_o(double big_o_in)
    {
        loop_count(big_o_in);
        double seconds = big_o_in / m_loop_count;
        m_delay = {(time_t)(seconds),
                   (long)((seconds - (time_t)(seconds)) * 1E9)};

        m_big_o = big_o_in;
    }

    void IgnoreModelRegion::run(void)
    {
        if (m_big_o != 0.0) {
            if (m_verbosity) {
                std::cout << "Executing ignored " << m_big_o << " second sleep."  << std::endl << std::flush;
            }
            ModelRegionBase::region_enter();
            for (uint64_t i = 0 ; i < m_loop_count; ++i) {
                ModelRegionBase::loop_enter(i);

                int err;
#ifdef __APPLE__
                err = nanosleep(&m_delay, NULL);
#else
                err = clock_nanosleep(CLOCK_REALTIME, 0, &m_delay, NULL);
#endif

                if (err) {
                    throw Exception("IgnoreModelRegion::run()",
                                    GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
                }

                ModelRegionBase::loop_exit();
            }
            ModelRegionBase::region_exit();
        }
    }
}
