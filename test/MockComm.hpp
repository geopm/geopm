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

#include "Comm.hpp"

class MockComm : public geopm::IComm {
    public:
        MOCK_CONST_METHOD0(split,
            IComm *(void));
        MOCK_CONST_METHOD2(split,
            IComm *(int color, int key));
        MOCK_CONST_METHOD2(split,
            IComm *(const std::string &tag, int split_type));
        MOCK_CONST_METHOD3(split,
            IComm *(std::vector<int> dimensions, std::vector<int> periods, bool is_reorder));
        MOCK_CONST_METHOD1(comm_supported,
            bool (const std::string &description));
        MOCK_CONST_METHOD1(cart_rank,
            int (const std::vector<int> &coords));
        MOCK_CONST_METHOD0(rank,
            int (void));
        MOCK_CONST_METHOD0(num_rank,
            int (void));
        MOCK_CONST_METHOD2(dimension_create,
            void (int num_nodes, std::vector<int> &dimension));
        MOCK_METHOD1(free_mem,
            void (void *base));
        MOCK_METHOD2(alloc_mem,
            void (size_t size, void **base));
        MOCK_METHOD2(window_create,
            size_t (size_t size, void *base));
        MOCK_METHOD1(window_destroy,
            void (size_t window_id));
        MOCK_CONST_METHOD4(window_lock,
            void (size_t window_id, bool isExclusive, int rank, int assert));
        MOCK_CONST_METHOD2(window_unlock,
            void (size_t window_id, int rank));
        MOCK_CONST_METHOD2(coordinate,
            void (int rank, std::vector<int> &coord));
        MOCK_CONST_METHOD0(barrier,
            void (void));
        MOCK_CONST_METHOD3(broadcast,
            void (void *buffer, size_t size, int root));
        MOCK_CONST_METHOD1(test,
            bool (bool is_true));
        MOCK_CONST_METHOD4(reduce_max,
            void (double *sendbuf, double *recvbuf, size_t count, int root));
        MOCK_CONST_METHOD5(gather,
            void (const void *send_buf, size_t send_size, void *recv_buf,
                size_t recv_size, int root));
        MOCK_CONST_METHOD6(gatherv,
            void (const void *send_buf, size_t send_size, void *recv_buf,
                const std::vector<size_t> &recv_sizes, const std::vector<off_t> &rank_offset, int root));
        MOCK_CONST_METHOD5(window_put,
            void (const void *send_buf, size_t send_size, int rank, off_t disp, size_t window_id));
};
