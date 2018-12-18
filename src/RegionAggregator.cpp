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

#include "geopm.h"
#include "geopm_internal.h"
#include "geopm_region_id.h"
#include "geopm_hash.h"
#include "RegionAggregator.hpp"
#include "PlatformIO.hpp"
#include "PlatformTopo.hpp"
#include "Exception.hpp"
#include "config.h"

namespace geopm
{
    RegionAggregator::RegionAggregator()
        : RegionAggregator(platform_io())
    {

    }

    RegionAggregator::RegionAggregator(IPlatformIO &platio)
        : m_platform_io(platio)
        , m_last_epoch_count(-1)
    {

    }

    void RegionAggregator::init(void)
    {
        m_epoch_count_idx = m_platform_io.push_signal("EPOCH_COUNT", IPlatformTopo::M_DOMAIN_BOARD, 0);
    }

    int RegionAggregator::push_signal_total(const std::string &signal_name,
                                            int domain_type,
                                            int domain_idx)
    {
        int signal_idx = m_platform_io.push_signal(signal_name, domain_type, domain_idx);
        m_region_hash_idx[signal_idx] = m_platform_io.push_signal("REGION_HASH", domain_type, domain_idx);
        return signal_idx;
    }

    double RegionAggregator::sample_total(int signal_idx, uint64_t region_hash)
    {
        if (signal_idx < 0) {
            throw Exception("RegionAggregator::sample_total(): Invalid signal index",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        if (m_region_hash_idx.find(signal_idx) == m_region_hash_idx.end()) {
            throw Exception("RegionAggregator::sample_total(): Cannot call sample_total "
                            "for signal index not pushed with push_signal_total.",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        double current_value = 0.0;
        uint64_t curr_hash = m_platform_io.sample(m_region_hash_idx.at(signal_idx));
        m_tracked_region_hash.insert(curr_hash);
        // Look up the data for this combination of signal and region ID
        auto idx = std::make_pair(signal_idx, region_hash);
        auto data_it = m_region_sample_data.find(idx);
        if (data_it != m_region_sample_data.end()) {
            const auto &data = data_it->second;
            current_value += data.total;
            // if currently in this region, add current value to total
            if (region_hash == curr_hash &&
                !std::isnan(data.last_entry_value)) {
                current_value += m_platform_io.sample(signal_idx) - data.last_entry_value;
            }
        }
        return current_value;
    }

    void RegionAggregator::read_batch(void)
    {
        for (const auto &it : m_region_hash_idx) {
            double value = m_platform_io.sample(it.first);
            const uint64_t region_hash = geopm_signal_to_field(m_platform_io.sample(it.second));
            m_tracked_region_hash.insert(region_hash);
            // first time sampling this signal
            if (m_last_region_hash.find(it.first) == m_last_region_hash.end()) {
                m_last_region_hash[it.first] = region_hash;
                // set start value for first region to be recording this signal
                m_region_sample_data[std::make_pair(it.first, region_hash)].last_entry_value = value;
            }
            else {
                const uint64_t last_hash = m_last_region_hash[it.first];
                // region boundary
                if (region_hash != last_hash) {
                    // add entry to new region
                    m_region_sample_data[std::make_pair(it.first, region_hash)].last_entry_value = value;
                    // update total for previous region
                    double prev_total = value - m_region_sample_data.at(std::make_pair(it.first, last_hash)).last_entry_value;
                    m_region_sample_data[std::make_pair(it.first, last_hash)].total += prev_total;
                    // update epoch
                    double curr_epoch_count = m_platform_io.sample(m_epoch_count_idx);
                    if (curr_epoch_count != m_last_epoch_count) {
                        m_region_sample_data[std::make_pair(it.first, GEOPM_HASH_REGION_EPOCH)].total += prev_total;
                    }
                    m_last_region_hash[it.first] = region_hash;
                }
            }
        }
    }

    std::set<uint64_t> RegionAggregator::tracked_region_hash(void) const
    {
        return m_tracked_region_hash;
    }
}
