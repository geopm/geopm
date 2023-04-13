/*
 * Copyright (c) 2015 - 2023, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
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
        , m_time({{0,0}})
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
        double delta = geopm_time_diff(&m_time, &(record.time));
        if (delta < 0) {
            throw Exception("ValidateRecord::filter(): Time value decreased. Delta=" + std::to_string(delta),
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        m_time = record.time;
        switch (record.event) {
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
