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

#include "RecordFilter.hpp"
#include "ProxyEpochRecordFilter.hpp"
#include "Helper.hpp"
#include "Exception.hpp"

namespace geopm
{
    std::unique_ptr<RecordFilter> RecordFilter::make_unique(const std::string &name)
    {
        std::unique_ptr<RecordFilter> result;
        if (string_begins_with(name, "proxy_epoch")) {
            uint64_t region_hash;
            int calls_per_epoch = 1;
            int startup_count = 0;
            std::vector<std::string> split_name = string_split(name, ",");
            if (split_name.size() <= 1ULL) {
                throw Exception("RecordFilter::make_unique(): proxy_epoch type requires a hash, e.g. proxy_epoch,0x1234abcd",
                                GEOPM_ERROR_INVALID, __FILE__, __LINE__);
            }
            else {
                if (split_name[0] != "proxy_epoch") {
                    throw Exception("RecordFilter::make_unique(): Expected name of the form \"proxy_epoch,<HASH>[,<CALLS>[,<STARTUP>]]\", got: " + name,
                                    GEOPM_ERROR_INVALID, __FILE__, __LINE__);
                }
                region_hash = std::stoull(split_name[1]);
                if (split_name.size() > 2ULL) {
                    calls_per_epoch = std::stoi(split_name[2]);
                }
                if (split_name.size() > 3ULL) {
                    startup_count = std::stoi(split_name[3]);
                }
                result = geopm::make_unique<ProxyEpochRecordFilter>(region_hash,
                                                                    calls_per_epoch,
                                                                    startup_count);
            }

        }
        else {
            throw Exception("RecordFilter::make_unique(): unable to parse name: " + name,
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        return result;
    }
}

