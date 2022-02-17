/*
 * Copyright (c) 2015 - 2022, Intel Corporation
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

#ifndef MOCKCOMM_HPP_INCLUDE
#define MOCKCOMM_HPP_INCLUDE

#include "gmock/gmock.h"

#include "Comm.hpp"

class MockComm : public geopm::Comm
{
    public:
        MOCK_METHOD(std::shared_ptr<Comm>, split, (), (const, override));
        MOCK_METHOD(std::shared_ptr<Comm>, split, (int color, int key),
                    (const, override));
        MOCK_METHOD(std::shared_ptr<Comm>, split,
                    (const std::string &tag, int split_type), (const, override));
        MOCK_METHOD(std::shared_ptr<Comm>, split,
                    (std::vector<int> dimensions, std::vector<int> periods, bool is_reorder),
                    (const, override));
        MOCK_METHOD(std::shared_ptr<Comm>, split_cart,
                    (std::vector<int> dimensions), (const, override));
        MOCK_METHOD(bool, comm_supported, (const std::string &description),
                    (const, override));
        MOCK_METHOD(int, cart_rank, (const std::vector<int> &coords),
                    (const, override));
        MOCK_METHOD(int, rank, (), (const, override));
        MOCK_METHOD(int, num_rank, (), (const, override));
        MOCK_METHOD(void, dimension_create,
                    (int num_nodes, std::vector<int> &dimension), (const, override));
        MOCK_METHOD(void, free_mem, (void *base), (override));
        MOCK_METHOD(void, alloc_mem, (size_t size, void **base), (override));
        MOCK_METHOD(size_t, window_create, (size_t size, void *base), (override));
        MOCK_METHOD(void, window_destroy, (size_t window_id), (override));
        MOCK_METHOD(void, window_lock,
                    (size_t window_id, bool isExclusive, int rank, int assert),
                    (const, override));
        MOCK_METHOD(void, window_unlock, (size_t window_id, int rank),
                    (const, override));
        MOCK_METHOD(void, coordinate, (int rank, std::vector<int> &coord),
                    (const, override));
        MOCK_METHOD(std::vector<int>, coordinate, (int rank), (const, override));
        MOCK_METHOD(void, barrier, (), (const, override));
        MOCK_METHOD(void, broadcast, (void *buffer, size_t size, int root),
                    (const, override));
        MOCK_METHOD(bool, test, (bool is_true), (const, override));
        MOCK_METHOD(void, reduce_max,
                    (double *send_buf, double *recv_buf, size_t count, int root),
                    (const, override));
        MOCK_METHOD(void, gather,
                    (const void *send_buf, size_t send_size, void *recv_buf,
                     size_t recv_size, int root),
                    (const, override));
        MOCK_METHOD(void, gatherv,
                    (const void *send_buf, size_t send_size, void *recv_buf,
                     const std::vector<size_t> &recv_sizes,
                     const std::vector<off_t> &rank_offset, int root),
                    (const, override));
        MOCK_METHOD(void, window_put,
                    (const void *send_buf, size_t send_size, int rank,
                     off_t disp, size_t window_id),
                    (const, override));
        MOCK_METHOD(void, tear_down, (), (override));
};

#endif
