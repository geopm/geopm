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

#include "record.hpp"

#include <map>

#include "Exception.hpp"
#include "geopm.h"

namespace geopm
{
    std::string event_name(int event_type)
    {
        static const std::map<int, std::string> event_names {
            {EVENT_REGION_ENTRY, "REGION_ENTRY"},
            {EVENT_REGION_EXIT, "REGION_EXIT"},
            {EVENT_EPOCH_COUNT, "EPOCH_COUNT"},
            {EVENT_HINT, "HINT"}
        };
        auto it = event_names.find(event_type);
        if (it == event_names.end()) {
            throw geopm::Exception("unsupported event type: " + std::to_string(event_type),
                                   GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        return it->second;
    }

    int event_type(const std::string &event_name)
    {
        static const std::map<std::string, int> event_types {
            {"REGION_ENTRY", EVENT_REGION_ENTRY},
            {"REGION_EXIT", EVENT_REGION_EXIT},
            {"EPOCH_COUNT", EVENT_EPOCH_COUNT},
            {"HINT", EVENT_HINT}
        };
        auto it = event_types.find(event_name);
        if (it == event_types.end()) {
            throw geopm::Exception("invalid event type string: " + event_name,
                                   GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        return it->second;
    }

    std::string hint_name(uint64_t hint)
    {
        static const std::map<uint64_t, std::string> result_map {
            {GEOPM_REGION_HINT_UNKNOWN, "UNKNOWN"},
            {GEOPM_REGION_HINT_COMPUTE, "COMPUTE"},
            {GEOPM_REGION_HINT_MEMORY, "MEMORY"},
            {GEOPM_REGION_HINT_NETWORK, "NETWORK"},
            {GEOPM_REGION_HINT_IO, "IO"},
            {GEOPM_REGION_HINT_SERIAL, "SERIAL"},
            {GEOPM_REGION_HINT_PARALLEL, "PARALLEL"},
            {GEOPM_REGION_HINT_IGNORE, "IGNORE"},
        };
        auto it = result_map.find(hint);
        if (it == result_map.end()) {
            throw Exception("hint_hame(): Invalid hint",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        return it->second;
    }

    uint64_t hint_type(const std::string &hint_name)
    {
        static const std::map<std::string, uint64_t> result_map {
            {"UNKNOWN", GEOPM_REGION_HINT_UNKNOWN},
            {"COMPUTE", GEOPM_REGION_HINT_COMPUTE},
            {"MEMORY", GEOPM_REGION_HINT_MEMORY},
            {"NETWORK", GEOPM_REGION_HINT_NETWORK},
            {"IO", GEOPM_REGION_HINT_IO},
            {"SERIAL", GEOPM_REGION_HINT_SERIAL},
            {"PARALLEL", GEOPM_REGION_HINT_PARALLEL},
            {"IGNORE", GEOPM_REGION_HINT_IGNORE},
        };
        auto it = result_map.find(hint_name);
        if (it == result_map.end()) {
            throw Exception("hint_type():  Unknown hint name: " + hint_name + "\n",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        return it->second;
    }
}
