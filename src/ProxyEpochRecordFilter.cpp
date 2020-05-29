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

#include "ProxyEpochRecordFilter.hpp"
#include "Exception.hpp"

namespace geopm
{
    ProxyEpochRecordFilter::ProxyEpochRecordFilter(uint64_t region_hash,
                                                   int calls_per_epoch,
                                                   int startup_count)
        : m_proxy_hash(region_hash)
        , m_num_per_epoch(calls_per_epoch)
        , m_count(-startup_count)
    {
        if (m_proxy_hash > 1ULL <<32) {
            throw Exception("ProxyEpochRecordFilter(): Parameter region_hash is out of range",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        if (calls_per_epoch <= 0) {
            throw Exception("ProxyEpochRecordFilter(): Parameter calls_per_epoch must be greater than zero",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        if (startup_count < 0) {
            throw Exception("ProxyEpochRecordFilter(): Parameter startup_count must be greater than or equal to zero",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
    }

    std::vector<ApplicationSampler::m_record_s> ProxyEpochRecordFilter::filter(const ApplicationSampler::m_record_s &record)
    {
        std::vector<ApplicationSampler::m_record_s> result;
        if (record.event == ApplicationSampler::M_EVENT_REGION_ENTRY &&
            record.signal == m_proxy_hash) {
            if (m_count >= 0 &&
                m_count % m_num_per_epoch == 0) {
                ApplicationSampler::m_record_s epoch_event(record);
                epoch_event.event = ApplicationSampler::M_EVENT_EPOCH_COUNT;
                epoch_event.signal = 1 + m_count / m_num_per_epoch;
                result.push_back(epoch_event);
            }
            ++m_count;
        }
        else if (record.event == ApplicationSampler::M_EVENT_HINT) {
            result.push_back(record);
        }
        return result;
    }
}

