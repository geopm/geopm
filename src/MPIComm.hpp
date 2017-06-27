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

#ifndef MPICOMM_HPP_INCLUDE
#define MPICOMM_HPP_INCLUDE

#include <set>
#include <vector>
#include <string>
#include "Comm.hpp"
#ifndef GEOPM_TEST
#include <mpi.h>
#endif

namespace geopm
{
    class MPIComm : public IComm
    {
        public:
            MPIComm();
            MPIComm(MPI_Comm comm);
            MPIComm(const MPIComm *in_comm);
            MPIComm(const MPIComm *in_comm, std::vector<int> dimension, std::vector<int> periods, bool is_reorder);
            MPIComm(const MPIComm *in_comm, int color, int key);
            MPIComm(const MPIComm *in_comm, std::string tag,  bool &is_ctl);
            MPIComm(const MPIComm *in_comm, std::string tag);
            MPIComm(const MPIComm *in_comm, std::string tag, int split_type);
            virtual ~MPIComm();

            virtual bool comm_supported(const std::string &description) const;

            virtual int cart_rank(std::vector<int> coords) const;
            virtual int rank(void) const;
            virtual int num_rank(void) const;
            virtual void dimension_create(int num_nodes, std::vector<int> &dimension) const;
            virtual void alloc_mem(size_t size, void **base);
            virtual void free_mem(void *base);
            virtual size_t window_create(size_t size, void *base);
            virtual void window_destroy(size_t window_id);
            virtual void coordinate(int rank, std::vector<int> &coord) const;
            virtual void window_lock(size_t window_id, bool isExclusive, int rank, int assert) const;
            virtual void window_unlock(size_t window_id, int rank) const;
            virtual void barrier(void) const;
            virtual void broadcast(void *buffer, size_t size, int root) const;
            virtual bool test(bool is_true) const;
            virtual void reduce_max(double *sendbuf, double *recvbuf, size_t count, int root) const;
            virtual void gather(const void *send_buf, size_t send_size, void *recv_buf,
                    size_t recv_size, int root) const;
            virtual void gatherv(const void *send_buf, size_t send_size, void *recv_buf,
                    const std::vector<size_t> &recv_sizes, const std::vector<off_t> &rank_offset, int root) const;
            virtual void window_put(const void *send_buf, size_t send_size, int rank, off_t disp, size_t window_id) const;
        protected:
            void check_window(size_t window_id) const;
            bool is_valid() const;
            MPI_Comm m_comm;
            size_t m_maxdims;
            std::set<size_t> m_windows;
            const std::string m_description;
    };
}

#endif
