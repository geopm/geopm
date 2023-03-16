/*
 * Copyright (c) 2015 - 2023, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "config.h"
#include "All2allModelRegion.hpp"

#include <stdlib.h>
#include <mpi.h>
#include <iostream>

#include "geopm_hint.h"
#include "geopm_time.h"
#include "geopm/Exception.hpp"
#include "geopm/Helper.hpp"

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
        , m_align(geopm::hardware_destructive_interference_size)
        , m_rank(-1)
    {
        m_name = "all2all";
        m_do_imbalance = do_imbalance;
        m_do_progress = do_progress;
        m_do_unmarked = do_unmarked;
        int err = 0;
        if (!getenv("GEOPMBENCH_NO_MPI")) {
            err = MPI_Comm_size(MPI_COMM_WORLD, &m_num_rank);
            if (err) {
                throw Exception("All2allModelRegion: MPI_Comm_size() failed",
                                err, __FILE__, __LINE__);
            }
        }
        err = ModelRegion::region(GEOPM_REGION_HINT_UNKNOWN);
        if (!err && !getenv("GEOPMBENCH_NO_MPI")) {
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
        cleanup();
    }

    void All2allModelRegion::cleanup(void)
    {
        free(m_recv_buffer);
        free(m_send_buffer);
    }

    void All2allModelRegion::big_o(double big_o_in)
    {
        if (m_big_o && m_big_o != big_o_in) {
            cleanup();
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
            if (!getenv("GEOPMBENCH_NO_MPI")) {
                MPI_Barrier(MPI_COMM_WORLD);
            }
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
                    int err = 0;
                    if (!getenv("GEOPMBENCH_NO_MPI")) {
                        err = MPI_Alltoall(m_send_buffer, m_num_send, MPI_CHAR, m_recv_buffer,
                                           m_num_send, MPI_CHAR, MPI_COMM_WORLD);
                        if (err) {
                            throw Exception("MPI_Alltoall()", err, __FILE__, __LINE__);
                        }
                    }
                    if (!m_rank) {
                        (void)geopm_time(&curr);
                        timeout = geopm_time_diff(&start, &curr);
                        if (timeout > (m_big_o / m_num_progress_updates)) {
                            loop_done = 1;
                        }
                    }
                    if (!getenv("GEOPMBENCH_NO_MPI")) {
                        err = MPI_Bcast((void*)&loop_done, 1, MPI_INT, 0, MPI_COMM_WORLD);
                        if (err) {
                            throw Exception("MPI_Bcast()", err, __FILE__, __LINE__);
                        }
                    }
                }

                ModelRegion::loop_exit();
            }
            ModelRegion::region_exit();
        }
    }
}
