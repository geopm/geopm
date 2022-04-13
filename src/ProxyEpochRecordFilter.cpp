/*
 * Copyright (c) 2015 - 2022, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "config.h"

#include "geopm_hash.h"

#include "ProxyEpochRecordFilter.hpp"
#include "geopm/Exception.hpp"
#include "geopm/Helper.hpp"
#include "record.hpp"

namespace geopm
{
    static uint64_t parse_region_hash(const std::string &filter_name)
    {
        uint64_t region_hash = 0ULL;
        int calls_per_epoch = 0;
        int startup_count = 0;
        ProxyEpochRecordFilter::parse_name(filter_name,
                                           region_hash,
                                           calls_per_epoch,
                                           startup_count);
        return region_hash;
    }

    static int parse_calls_per_epoch(const std::string &filter_name)
    {
        uint64_t region_hash = 0ULL;
        int calls_per_epoch = 0;
        int startup_count = 0;
        ProxyEpochRecordFilter::parse_name(filter_name,
                                           region_hash,
                                           calls_per_epoch,
                                           startup_count);
        return calls_per_epoch;
    }

    static int parse_startup_count(const std::string &filter_name)
    {
        uint64_t region_hash = 0ULL;
        int calls_per_epoch = 0;
        int startup_count = 0;
        ProxyEpochRecordFilter::parse_name(filter_name,
                                           region_hash,
                                           calls_per_epoch,
                                           startup_count);
        return startup_count;
    }

    ProxyEpochRecordFilter::ProxyEpochRecordFilter(const std::string &filter_name)
        : ProxyEpochRecordFilter(parse_region_hash(filter_name),
                                 parse_calls_per_epoch(filter_name),
                                 parse_startup_count(filter_name))
    {

    }


    void ProxyEpochRecordFilter::parse_name(const std::string &name,
                                            uint64_t &region_hash,
                                            int &calls_per_epoch,
                                            int &startup_count)
    {
        std::vector<std::string> split_name = string_split(name, ",");
        if (split_name[0] != "proxy_epoch") {
            throw Exception("RecordFilter::make_unique(): Expected name of the form \"proxy_epoch,<HASH>[,<CALLS>[,<STARTUP>]]\", got: " + name,
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        if (split_name.size() <= 1ULL) {
            throw Exception("RecordFilter::make_unique(): proxy_epoch type requires a hash, e.g. proxy_epoch,0x1234abcd",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        if (split_name[1].empty()) {
            throw Exception("RecordFilter::make_unique(): Parameter region_hash is empty",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        try {
            region_hash = std::stoull(split_name[1], nullptr, 0);
        }
        catch (const std::exception &) {
            region_hash = geopm_crc32_str(split_name[1].c_str());
        }
        calls_per_epoch = 1;
        startup_count = 0;
        if (split_name.size() > 2ULL) {
            try {
                calls_per_epoch = std::stoi(split_name[2]);
            }
            catch (const std::exception &) {
                throw Exception("RecordFilter::make_unique(): Unable to parse parameter calls_per_epoch from filter name: " + name,
                                GEOPM_ERROR_INVALID, __FILE__, __LINE__);
            }
        }
        if (split_name.size() > 3ULL) {
            try {
                startup_count = std::stoi(split_name[3]);
            }
            catch (const std::exception &) {
                throw Exception("RecordFilter::make_unique(): Unable to parse parameter startup_count from filter name: " + name,
                                GEOPM_ERROR_INVALID, __FILE__, __LINE__);
            }
        }
    }

    ProxyEpochRecordFilter::ProxyEpochRecordFilter(uint64_t region_hash,
                                                   int calls_per_epoch,
                                                   int startup_count)
        : m_proxy_hash(region_hash)
        , m_num_per_epoch(calls_per_epoch)
        , m_count(-startup_count)
    {
        // Hash is a CRC32, so check that it is 32 bits
        if (m_proxy_hash > UINT32_MAX) {
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

    std::vector<record_s> ProxyEpochRecordFilter::filter(const record_s &record)
    {
        std::vector<record_s> result;
        if (record.event != EVENT_EPOCH_COUNT) {
            result.push_back(record);
            if (record.event == EVENT_REGION_ENTRY &&
                record.signal == m_proxy_hash) {
                if (m_count >= 0 &&
                    m_count % m_num_per_epoch == 0) {
                    record_s epoch_event = record;
                    epoch_event.event = EVENT_EPOCH_COUNT;
                    epoch_event.signal = 1 + m_count / m_num_per_epoch;
                    result.push_back(epoch_event);
                }
                ++m_count;
            }
        }
        return result;
    }
}
