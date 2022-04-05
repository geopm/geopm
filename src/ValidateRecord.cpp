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

#include "config.h"

#include <cstdint>

#include "ValidateRecord.hpp"
#include "geopm/Exception.hpp"
#include "geopm/Helper.hpp"
#include "record.hpp"
#include "geopm_hash.h"
#include "geopm_hint.h"


namespace geopm
{
    ValidateRecord::ValidateRecord()
        : m_is_empty(true)
        , m_time(NAN)
        , m_process(-1)
        , m_epoch_count(0)
        , m_region_hash(GEOPM_REGION_HASH_INVALID)
    {

    }

    static void validate_hash(uint64_t hash)
    {
        if (hash == GEOPM_REGION_HASH_INVALID ||
            hash > UINT32_MAX) {
            throw Exception("ValidateRecord::filter(): Region hash out of bounds: " + string_format_hex(hash),
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
    }

    void ValidateRecord::check(const record_s &record)
    {
        if (m_is_empty) {
            m_time = record.time;
            m_process = record.process;
            m_epoch_count = 0;
            m_region_hash = GEOPM_REGION_HASH_INVALID;
            m_is_empty = false;
        }
        if (record.process != m_process) {
            throw Exception("ValidateRecord::filter(): Process has changed",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        if (record.time < m_time) {
            throw Exception("ValidateRecord::filter(): Time value decreased. Time=" + std::to_string(m_time),
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        m_time = record.time;
        switch (record.event) {
            case EVENT_HINT:
                if ((record.signal == 0ULL) || // check that the hint is not empty
                    (record.signal & (record.signal - 1)) || // check that only one bit is set
                    (record.signal & ~GEOPM_MASK_REGION_HINT)) { // check that the hint is in the mask
                    throw Exception("ValidateRecord::filter(): Hint out of range",
                                    GEOPM_ERROR_INVALID, __FILE__, __LINE__);
                }
                break;
            case EVENT_REGION_ENTRY:
                validate_hash(record.signal);
                if (m_region_hash != GEOPM_REGION_HASH_INVALID) {
                    throw Exception("ValidateRecord::filter(): Nested region entry detected. Region=" + string_format_hex(m_region_hash),
                                    GEOPM_ERROR_INVALID, __FILE__, __LINE__);
                }
                m_region_hash = record.signal;
                break;
            case EVENT_REGION_EXIT:
                validate_hash(record.signal);
                if (m_region_hash == GEOPM_REGION_HASH_INVALID) {
                    throw Exception("ValidateRecord::filter(): Region exit without entry Region=" + string_format_hex(m_region_hash),
                                    GEOPM_ERROR_INVALID, __FILE__, __LINE__);
                }
                if (record.signal != m_region_hash) {
                    throw Exception("ValidateRecord::filter(): Region exited differs from last region entered Current region="
                                    + string_format_hex(m_region_hash) + " Received exit for=" + string_format_hex(record.signal),
                                    GEOPM_ERROR_INVALID, __FILE__, __LINE__);
                }
                m_region_hash = GEOPM_REGION_HASH_INVALID;
                break;
            case EVENT_EPOCH_COUNT:
                if (record.signal != m_epoch_count + 1) {
                    throw Exception("ValidateRecord::filter(): Epoch count not monotone and contiguous. Current epoch="
                                    + std::to_string(m_epoch_count),
                                    GEOPM_ERROR_INVALID, __FILE__, __LINE__);
                }
                m_epoch_count = record.signal;
                break;
        }
    }
}
