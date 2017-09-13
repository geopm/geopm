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

namespace geopm
{
    // Forward declaration
    class Request;

    /// @brief Abstract base class for interprocess communication in geopm
    class Comm
    {
        public:
            /// @brief Constructor for global communicator
            Comm();
            /// @brief Constructor for splitting a communictor into a Cartesian grid
            Comm(const Comm *in_comm, std::vector<int> dimension, std::vector<bool> is_periodic, bool is_reorder);
            /// @brief Constructor for splitting a Cartesian grid into a sub-grid
            Comm(const Comm *in_comm, std::vector<bool> is_remain);
            /// @brief Split a communicator based on color, order ranks based on key
            Comm(const Comm *in_comm, int color, int key);
            /// @brief Default destructor
            virtual ~Comm();

            // Enum lookup
            /// @brief String to enum mapping for buffer type
            virtual int buffer_type(std::string type) = 0;
            /// @brief String to enum mapping for operation
            virtual int operation_type(std::string type) = 0;

            // Introspection
            /// @brief Process rank within communicator
            virtual int rank(void) = 0;
            /// @brief Number of ranks in the communicator
            virtual int num_rank(void) = 0;
            /// @brief Dimension of Cartesian grid (returns 1 for non-Cartesian communicators
            virtual int num_dimension(void) = 0;
            /// @brief Populate vector of optimal dimensions given the number of ranks the communicator
            virtual void dimension_create(std::vector<int> &dimension) = 0;
            /// @brief Coordinate in Cartesian grid for specified rank
            virtual void coordinate(int rank, std::vector<int> &coord) = 0;

            // Point to point communication
            /// @brief Blocking message send
            virtual void send(void *buffer, int count, int buffer_type, int dest, int tag) = 0;
            /// @brief Non-blocking message send
            virtual void send(void *buffer, int count, int buffer_type, int dest, int tag, Request *request) = 0;
            /// @brief Non-blocking message send for ready reciever
            virtual void send(void *buffer, int count, int buffer_type, int dest, int tag, Request *request, bool is_ready) = 0;
            /// @brief Blocking message recieve
            virtual void recieve(void *buffer, int count, int buffer_type, int source, int tag) = 0;
            /// @brief Non-blocking message recieve
            virtual void recieve(void *buffer, int count, int buffer_type, int source, int tag, Request *request) = 0;

            // Collective communication
            /// @brief Barrier for all ranks
            virtual void barrier(void) = 0;
            /// @brief Broadcast a message to all ranks
            virtual void broadcast(void *buffer, int count, int buffer_type, int root) = 0;
            /// @brief Reduce distributed messages across all ranks using specified operation, store result on all ranks
            virtual void reduce(const void *sendbuf, void *recvbuf, int count, int buffer_type, int operation) = 0;
            /// @brief Reduce distributed messages across all ranks using specified operation, store result only on root
            virtual void reduce(const void *sendbuf, void *recvbuf, int count, int buffer_type, int operation, int root) = 0;
    };

    /// @brief Abstract base class for handling non-blocking request status for Comm
    class Request
    {
        public:
            /// @brief Default constructor
            Request();
            /// @brief Default destructor
            virtual ~Request();
            /// @brief Cancel a pending non-blocking request
            virtual void cancel(void) = 0;
            /// @brief Check if a non-blocking request has completed
            virtual bool is_complete(void) = 0;
    };
}
