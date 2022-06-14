/*
 * Copyright (c) 2015 - 2022, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "config.h"
#include "ReduceModelRegion.hpp"

#include <iostream>
#include <vector>
#include <mpi.h>

#include "geopm/Exception.hpp"

namespace geopm
{
    ReduceModelRegion::ReduceModelRegion(double big_o_in,
                                         int verbosity,
                                         bool do_imbalance,
                                         bool do_progress,
                                         bool do_unmarked)
        : ModelRegion(verbosity)
    {
        big_o(big_o_in);
    }

    void ReduceModelRegion::big_o(double big_o)
    {
        m_num_elem = 67108864 * big_o; // 64 MiB per big_o
        m_send_buffer.resize(m_num_elem);
        m_recv_buffer.resize(m_num_elem);
        std::fill(m_send_buffer.begin(), m_send_buffer.end(), 1.0);
        std::fill(m_recv_buffer.begin(), m_recv_buffer.end(), 0.0);
    }

    void ReduceModelRegion::run(void)
    {
        int num_rank = 0;
        int err = MPI_Comm_size(MPI_COMM_WORLD, &num_rank);
        if (err) {
            throw Exception("MPI_Comm_size", err, __FILE__, __LINE__);
        }
        if (m_verbosity != 0) {
            std::cout << "Executing reduce\n";
        }
        err = MPI_Allreduce(m_send_buffer.data(), m_recv_buffer.data(),
                            m_num_elem, MPI_DOUBLE, MPI_SUM, MPI_COMM_WORLD);
        if (err) {
            throw Exception("MPI_Allreduce", err, __FILE__, __LINE__);
        }
    }
}
