/*
 * Copyright (c) 2015, 2016, 2017, 2018, 2019, Intel Corporation
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

#ifndef REGIONAGGREGATORIMP_HPP_INCLUDE
#define REGIONAGGREGATORIMP_HPP_INCLUDE

#include <cmath>

#include <map>

#include "RegionAggregator.hpp"

namespace geopm
{
    class PlatformIO;

    class RegionAggregatorImp : public RegionAggregator
    {
        public:
            RegionAggregatorImp();
            RegionAggregatorImp(PlatformIO &platio);
            void init(void) override;
            int push_signal_total(const std::string &signal_idx,
                                  int domain_type,
                                  int domain_idx) override;
            double sample_total(int signal_idx, uint64_t region_hash) override;
            void read_batch(void) override;
            std::set<uint64_t> tracked_region_hash(void) const override;
        private:
            PlatformIO &m_platform_io;
            std::map<int, int> m_region_hash_idx;
            struct m_region_data_s
            {
                double total = 0.0;
                double last_entry_value = NAN;
            };
            // Data for each combination of signal index and region hash
            std::map<std::pair<int, uint64_t>, m_region_data_s> m_region_sample_data;
            std::map<int, uint64_t> m_last_region_hash;
            int m_epoch_count_idx;
            std::set<uint64_t> m_tracked_region_hash;
    };
}

#endif
