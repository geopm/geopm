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
#include "NestedModelRegion.hpp"

#include <iostream>
#include <mpi.h>

#include "geopm.h"
#include "geopm_time.h"
#include "geopm_imbalancer.h"
#include "Exception.hpp"

namespace geopm
{
    NestedModelRegion::NestedModelRegion(double big_o_in,
                                         int verbosity,
                                         bool do_imbalance,
                                         bool do_progress,
                                         bool do_unmarked)
        : ModelRegion(verbosity)
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
            for (uint64_t i = 0 ; i < m_spin_region.m_num_progress_updates; ++i) {
                if (m_spin_region.m_do_imbalance) {
                    (void)geopm_imbalancer_enter();
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
                    (void)geopm_imbalancer_exit();
                }
            }
        }

        // Do all2all before spin region exit
        if (m_all2all_region.m_big_o != 0) {
            if (m_all2all_region.m_verbosity) {
                std::cout << "Executing " << m_all2all_region.m_num_send << " byte buffer all2all "
                          << m_all2all_region.m_num_progress_updates << " times."  << std::endl << std::flush;
            }
            for (uint64_t i = 0; i < m_all2all_region.m_num_progress_updates; ++i) {
                if (m_all2all_region.m_do_imbalance) {
                    (void)geopm_imbalancer_enter();
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
                (void)geopm_imbalancer_exit();
            }
        }

        // Do spin part deux.
        if (m_spin_region.m_big_o != 0.0) {
            if (m_spin_region.m_verbosity) {
                std::cout << "Executing " << m_spin_region.m_big_o << " second spin #2."  << std::endl << std::flush;
            }
            for (uint64_t i = 0 ; i < m_spin_region.m_num_progress_updates; ++i) {
                if (m_spin_region.m_do_imbalance) {
                    (void)geopm_imbalancer_enter();
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
                    (void)geopm_imbalancer_exit();
                }
            }
            (void)geopm_prof_exit(m_spin_region.m_region_id);
        }
    }
}
