/*
 * Copyright (c) 2015 - 2024, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "config.h"

#include <string>

namespace geopm
{
    const std::string cpufreq_sysfs_json(void)
    {
        static const std::string result = R"(
{
    "attributes": {
        "BIOS_LIMIT": {
            "attribute": "bios_limit",
            "scalar": 1e3,
            "description": "Maximum CPU frequency, limited by BIOS settings.",
            "aggregation": "average",
            "behavior": "variable",
            "format": "double",
            "units": "hertz",
            "writeable": false,
            "alias": ""
        },
        "CPUINFO_CUR_FREQ": {
            "attribute": "cpuinfo_cur_freq",
            "scalar": 1e3,
            "description": "The current operating frequency reported by the CPU hardware.",
            "aggregation": "average",
            "behavior": "variable",
            "format": "double",
            "units": "hertz",
            "writeable": false,
            "alias": ""
        },
        "CPUINFO_MAX_FREQ": {
            "attribute": "cpuinfo_max_freq",
            "scalar": 1e3,
            "description": "The maximum allowed frequency to set on the CPU.",
            "aggregation": "average",
            "behavior": "variable",
            "format": "double",
            "units": "hertz",
            "writeable": false,
            "alias": "CPU_FREQUENCY_MAX_AVAIL"
        },
        "CPUINFO_MIN_FREQ": {
            "attribute": "cpuinfo_min_freq",
            "scalar": 1e3,
            "description": "The minimum allowed frequency to set on the CPU.",
            "aggregation": "average",
            "behavior": "variable",
            "format": "double",
            "units": "hertz",
            "writeable": false,
            "alias": "CPU_FREQUENCY_MIN_AVAIL"
        },
        "CPUINFO_TRANSITION_LATENCY": {
            "attribute": "cpuinfo_transition_latency",
            "scalar": 1e-9,
            "description": "The time delay to switch from one P-State to another.",
            "aggregation": "average",
            "behavior": "variable",
            "format": "double",
            "units": "seconds",
            "writeable": false,
            "alias": ""
        },
        "SCALING_CUR_FREQ": {
            "attribute": "scaling_cur_freq",
            "scalar": 1e3,
            "description": "The current requested CPU frequency by the cpufreq scaling driver.",
            "aggregation": "average",
            "behavior": "variable",
            "format": "double",
            "units": "hertz",
            "writeable": false,
            "alias": "CPU_FREQUENCY_STATUS"
        },
        "SCALING_MAX_FREQ": {
            "attribute": "scaling_max_freq",
            "scalar": 1e3,
            "description": "The maximum frequency allowed by the cpufreq scaling driver.",
            "aggregation": "average",
            "behavior": "variable",
            "format": "double",
            "units": "hertz",
            "writeable": true,
            "alias": "CPU_FREQUENCY_MAX_CONTROL"
        },
        "SCALING_MIN_FREQ": {
            "attribute": "scaling_min_freq",
            "scalar": 1e3,
            "description": "The minimum frequency allowed by the cpufreq scaling driver.",
            "aggregation": "average",
            "behavior": "variable",
            "format": "double",
            "units": "hertz",
            "writeable": true,
            "alias": "CPU_FREQUENCY_MIN_CONTROL"
        },
        "SCALING_SETSPEED": {
            "attribute": "scaling_setspeed",
            "scalar": 1e3,
            "description": "The latest frequency request sent to the userspace scaling governor.",
            "aggregation": "average",
            "behavior": "variable",
            "format": "double",
            "units": "hertz",
            "writeable": true,
            "alias": "CPU_FREQUENCY_DESIRED_CONTROL"
        }
    }
}
        )";
        return result;
    }
}
