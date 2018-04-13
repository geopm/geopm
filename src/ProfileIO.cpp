/*
 * Copyright (c) 2015, 2016, 2017, 2018, Intel Corporation
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

#include <set>

#include "ProfileIO.hpp"
#include "config.h"

namespace geopm
{
    std::map<int, int> ProfileIO::rank_to_node_local_rank(const std::vector<int> &per_cpu_rank)
    {
        std::set<int> rank_set;
        for (auto rank : per_cpu_rank) {
            if (rank != -1) {
                rank_set.insert(rank);
            }
        }
        std::map<int, int> rank_idx_map;
        int i = 0;
        for (auto rank : rank_set) {
            rank_idx_map.insert(std::pair<int, int>(rank, i));
            ++i;
        }
        return rank_idx_map;
    }

    std::vector<int> ProfileIO::rank_to_node_local_rank_per_cpu(const std::vector<int> &per_cpu_rank)
    {
        std::vector<int> result(per_cpu_rank);
        std::map<int, int> rank_idx_map = rank_to_node_local_rank(per_cpu_rank);
        for (auto &rank_it : result) {
            auto node_local_rank_it = rank_idx_map.find(rank_it);
            rank_it = node_local_rank_it->second;
        }
        return result;
    }
}
