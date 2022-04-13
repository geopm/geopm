/*
 * Copyright (c) 2015 - 2022, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "config.h"

#include "record.hpp"

#include <map>

#include "geopm/Exception.hpp"
#include "geopm_hint.h"

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
