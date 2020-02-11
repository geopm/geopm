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
#include "ReduceModelRegion.hpp"

#include <iostream>
#include <vector>
#include <mpi.h>

#include "Exception.hpp"

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
