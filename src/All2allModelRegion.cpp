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
#include "All2allModelRegion.hpp"

#include <mpi.h>
#include <iostream>

#include "geopm.h"
#include "geopm_time.h"
#include "Exception.hpp"


namespace geopm
{
    All2allModelRegion::All2allModelRegion(double big_o_in,
                                           int verbosity,
                                           bool do_imbalance,
                                           bool do_progress,
                                           bool do_unmarked)
        : ModelRegion(verbosity)
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
        int err = MPI_Comm_size(MPI_COMM_WORLD, &m_num_rank);
        if (err) {
            throw Exception("All2allModelRegion: MPI_Comm_size() failed",
                            err, __FILE__, __LINE__);
        }
        err = ModelRegion::region(GEOPM_REGION_HINT_UNKNOWN);
        if (!err) {
            err = MPI_Comm_rank(MPI_COMM_WORLD, &m_rank);
        }
        if (err) {
            throw Exception("All2allModelRegion::All2allModelRegion()",
                            err, __FILE__, __LINE__);
        }

        big_o(big_o_in);
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

        num_progress_updates(big_o_in);

        if (m_num_progress_updates > 1) {
            m_num_send = 1048576; //1MB
        }
        else {
            m_num_send = 10485760; //10MB
        }

        if (big_o_in && m_big_o != big_o_in) {
            int err = posix_memalign((void **)&m_send_buffer, m_align,
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
                          << m_num_progress_updates << " times."  << std::endl << std::flush;
            }
            MPI_Barrier(MPI_COMM_WORLD);
            ModelRegion::region_enter();
            for (uint64_t i = 0; i < m_num_progress_updates; ++i) {
                ModelRegion::loop_enter(i);

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
                        if (timeout > (m_big_o / m_num_progress_updates)) {
                            loop_done = 1;
                        }
                    }
                    err = MPI_Bcast((void*)&loop_done, 1, MPI_INT, 0, MPI_COMM_WORLD);
                    if (err) {
                        throw Exception("MPI_Bcast()", err, __FILE__, __LINE__);
                    }
                }

                ModelRegion::loop_exit();
            }
            ModelRegion::region_exit();
        }
    }
}
