/*
 * Copyright (c) 2015 - 2023, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "config.h"

#include "record.hpp"

#include <map>
#include <utility>

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
            {EVENT_SHORT_REGION, "EVENT_SHORT_REGION"},
            {EVENT_AFFINITY, "EVENT_AFFINITY"},
            {EVENT_START_PROFILE, "EVENT_START_PROFILE"},
            {EVENT_STOP_PROFILE, "EVENT_STOP_PROFILE"},
            {EVENT_OVERHEAD, "EVENT_OVERHEAD"},
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
            {"EVENT_SHORT_REGION", EVENT_SHORT_REGION},
            {"EVENT_AFFINITY", EVENT_AFFINITY},
            {"EVENT_START_PROFILE", EVENT_START_PROFILE},
            {"EVENT_STOP_PROFILE", EVENT_STOP_PROFILE},
            {"EVENT_OVERHEAD", EVENT_OVERHEAD},
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
        static const std::string hint_names[] = {
            "UNSET",
            "UNKNOWN",
            "COMPUTE",
            "MEMORY",
            "NETWORK",
            "IO",
            "SERIAL",
            "PARALLEL",
            "IGNORE",
            "INACTIVE",
            "SPIN",
        };
        static_assert(sizeof(hint_names) / sizeof(hint_names[0]) ==
                      GEOPM_NUM_REGION_HINT,
                      "hint_names mismatch with geopm_region_hint_e");
        geopm::check_hint(hint);

        return hint_names[hint];
    }

    uint64_t hint_type(const std::string &hint_name)
    {
        static const std::pair<std::string, geopm_region_hint_e>
            hint_mapping[] = {
                std::make_pair("UNSET", GEOPM_REGION_HINT_UNSET),
                std::make_pair("UNKNOWN", GEOPM_REGION_HINT_UNKNOWN),
                std::make_pair("COMPUTE", GEOPM_REGION_HINT_COMPUTE),
                std::make_pair("MEMORY", GEOPM_REGION_HINT_MEMORY),
                std::make_pair("NETWORK", GEOPM_REGION_HINT_NETWORK),
                std::make_pair("IO", GEOPM_REGION_HINT_IO),
                std::make_pair("SERIAL", GEOPM_REGION_HINT_SERIAL),
                std::make_pair("PARALLEL", GEOPM_REGION_HINT_PARALLEL),
                std::make_pair("IGNORE", GEOPM_REGION_HINT_IGNORE),
                std::make_pair("INACTIVE", GEOPM_REGION_HINT_INACTIVE),
                std::make_pair("SPIN", GEOPM_REGION_HINT_SPIN),
            };
        static constexpr unsigned int HINT_MAP_SIZE = sizeof(hint_mapping) /
                                                      sizeof(hint_mapping[0]);
        static_assert(HINT_MAP_SIZE == GEOPM_NUM_REGION_HINT,
                      "hint_mapping mismatch with geopm_region_hint_e");
        static const std::map<std::string, uint64_t>
            result_map(hint_mapping, hint_mapping + HINT_MAP_SIZE);

        auto it = result_map.find(hint_name);
        if (it == result_map.end()) {
            throw Exception("hint_type():  Unknown hint name: " + hint_name + "\n",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        return it->second;
    }
}
