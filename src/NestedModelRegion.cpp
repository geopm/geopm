/*
 * Copyright (c) 2015 - 2022, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "config.h"
#include "NestedModelRegion.hpp"

#include <iostream>
#include <mpi.h>

#include "geopm_prof.h"
#include "geopm_time.h"
#include "geopm_imbalancer.h"
#include "geopm/Exception.hpp"

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
