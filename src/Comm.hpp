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

#ifndef COMM_HPP_INCLUDE
#define COMM_HPP_INCLUDE

#include <vector>
#include <string>

namespace geopm
{
    extern const char *MPICOMM_DESCRIPTION;
    /// @brief Abstract base class for interprocess communication in geopm
    class IComm
    {
        public:
            enum m_comm_split_type_e {
                M_COMM_SPLIT_TYPE_CTL,
                M_COMM_SPLIT_TYPE_PPN1,
                M_COMM_SPLIT_TYPE_SHARED,
                M_NUM_COMM_SPLIT_TYPE
            };

            enum m_split_color_e {
                M_SPLIT_COLOR_UNDEFINED = -16,
            };

            /// @brief Constructor for global communicator
            IComm() {}
            IComm(const IComm *in_comm) {}
            /// @brief Default destructor
            virtual ~IComm() {}

            virtual bool comm_supported(const std::string &description) const = 0;

            // Introspection
            /// @brief Process rank within communicator
            virtual int cart_rank(const std::vector<int> &coords) const = 0;
            virtual int rank(void) const = 0;
            /// @brief Number of ranks in the communicator
            virtual int num_rank(void) const = 0;
            /// @brief Dimension of Cartesian grid (returns 1 for non-Cartesian communicators
            /// @brief Populate vector of optimal dimensions given the number of ranks the communicator
            virtual void dimension_create(int num_nodes, std::vector<int> &dimension) const = 0;
            /// @brief Free memory that was allocated for message passing and RMA
            virtual void free_mem(void *base) = 0;
            /// @brief Allocate memory for message passing and RMA
            virtual void alloc_mem(size_t size, void **base) = 0;
            /// @brief Create window for message passing and RMA
            virtual size_t window_create(size_t size, void *base) = 0;
            /// @brief Destroy window for message passing and RMA
            virtual void window_destroy(size_t window_id) = 0;
            /// @brief Begin epoch for message passing and RMA
            virtual void window_lock(size_t window_id, bool is_exclusive, int rank, int assert) const = 0;
            /// @brief End epoch for message passing and RMA
            virtual void window_unlock(size_t window_id, int rank) const = 0;
            /// @brief Coordinate in Cartesian grid for specified rank
            virtual void coordinate(int rank, std::vector<int> &coord) const = 0;

            // Collective communication
            /// @brief Barrier for all ranks
            virtual void barrier(void) const = 0;
            /// @brief Broadcast a message to all ranks
            virtual void broadcast(void *buffer, size_t size, int root) const = 0;
            virtual bool test(bool is_true) const = 0;
            /// @brief Reduce distributed messages across all ranks using specified operation, store result on all ranks
            virtual void reduce_max(double *sendbuf, double *recvbuf, size_t count, int root) const = 0;
            /// @brief Gather bytes from all processes
            virtual void gather(const void *send_buf, size_t send_size, void *recv_buf,
                    size_t recv_size, int root) const = 0;
            /// @brief Gather bytes into specified location from all processes
            virtual void gatherv(const void *send_buf, size_t send_size, void *recv_buf,
                    const std::vector<size_t> &recv_sizes, const std::vector<off_t> &rank_offset, int root) const = 0;
            /// @brief Perform message passing or RMA
            virtual void window_put(const void *send_buf, size_t send_size, int rank, off_t disp, size_t window_id) const = 0;
    };

    IComm* geopm_get_comm(const std::string &description);
    IComm* geopm_get_comm(const IComm *in_comm);
    IComm* geopm_get_comm(const IComm *in_comm, int color, int key);
    IComm* geopm_get_comm(const IComm *in_comm, const std::string &tag, int split_type);
    IComm* geopm_get_comm(const IComm *in_comm, std::vector<int> dimensions, std::vector<int> periods, bool is_reorder);
}

#endif
