// THIS IS A GENERATED FILE, DO NOT MODIFY, INSTEAD MODIFY: docs/json_data/msr_data_spr.json
// AND RERUN update_data.sh
/*
 * Copyright (c) 2015 - 2024, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "config.h"

#include <string>

namespace geopm
{
    const std::string spr_msr_json(void)
    {
        static const std::string result = R"(
{
    "msrs": {
        "PLATFORM_INFO": {
            "offset": "0xCE",
            "domain": "package",
            "fields": {
                "MAX_NON_TURBO_RATIO": {
                    "begin_bit": 8,
                    "end_bit":   15,
                    "function":  "scale",
                    "units":     "hertz",
                    "scalar":    1e8,
                    "behavior":  "constant",
                    "writeable": false,
                    "description": "The processor's maximum non-turbo frequency.",
                    "aggregation": "expect_same"
                },
                "PROGRAMMABLE_RATIO_LIMITS_TURBO_MODE": {
                    "begin_bit": 28,
                    "end_bit":   28,
                    "function":  "logic",
                    "units":     "none",
                    "scalar":    1,
                    "behavior":  "constant",
                    "writeable": false,
                    "description": "Indicates whether the MSR::TURBO_RATIO_LIMIT:* signals are also available as controls.",
                    "aggregation": "expect_same"
                },
                "PROGRAMMABLE_TDP_LIMITS_TURBO_MODE": {
                    "begin_bit": 29,
                    "end_bit":   29,
                    "function":  "logic",
                    "units":     "none",
                    "scalar":    1,
                    "behavior":  "constant",
                    "writeable": false,
                    "description": "Indicates whether this platform supports programmable TDP limits for turbo mode.",
                    "aggregation": "expect_same"
                },
                "PROGRAMMABLE_TCC_ACTIVATION_OFFSET": {
                    "begin_bit": 30,
                    "end_bit":   30,
                    "function":  "logic",
                    "units":     "none",
                    "scalar":    1,
                    "behavior":  "constant",
                    "writeable": false,
                    "description": "Indicates whether the platform permits writes to MSR::TEMPERATURE_TARGET:TCC_ACTIVE_OFFSET.",
                    "aggregation": "expect_same"
                },
                "MAX_EFFICIENCY_RATIO": {
                    "begin_bit": 40,
                    "end_bit":   47,
                    "function":  "scale",
                    "units":     "hertz",
                    "scalar":    1e8,
                    "behavior":  "constant",
                    "writeable": false,
                    "description": "The minimum operating frequency of the processor.",
                    "aggregation": "expect_same"
                }
            }
        },
        "PERF_STATUS": {
            "offset": "0x198",
            "domain": "cpu",
            "fields": {
                "FREQ": {
                    "begin_bit": 8,
                    "end_bit":   15,
                    "function":  "scale",
                    "units":     "hertz",
                    "scalar":    1e8,
                    "behavior":  "variable",
                    "writeable": false,
                    "aggregation": "average",
                    "description": "The current operating frequency of the CPU."
                }
            }
        },
        "PERF_CTL": {
            "offset": "0x199",
            "domain": "core",
            "fields": {
                "FREQ": {
                    "begin_bit": 8,
                    "end_bit":   15,
                    "function":  "scale",
                    "units":     "hertz",
                    "scalar":    1e8,
                    "behavior":  "variable",
                    "writeable": true,
                    "description": "Target operating frequency of the CPU based on the control register. Note: when querying at a higher domain, if NaN is returned, query at its native domain.",
                    "aggregation": "average"
                }
            }
        },
        "TEMPERATURE_TARGET": {
            "offset": "0x1A2",
            "domain": "core",
            "fields": {
                "PROCHOT_MIN": {
                    "begin_bit": 16,
                    "end_bit":   23,
                    "function":  "scale",
                    "units":     "celsius",
                    "scalar":    1.0,
                    "behavior":  "constant",
                    "writeable": false,
                    "aggregation": "expect_same",
                    "description": "The lowest temperature considered a high temperature. Measured temperatures at or above this value will generate a PROCHOT event."
                },
                "TCC_ACTIVE_OFFSET": {
                    "begin_bit": 24,
                    "end_bit":   27,
                    "function":  "scale",
                    "units":     "celsius",
                    "scalar":    1.0,
                    "behavior":  "constant",
                    "writeable": false,
                    "description": "An offset to subtract from MSR::TEMPERATURE_TARGET:PROCHOT_MIN as the cutoff to generate a PROCHOT event.",
                    "aggregation": "expect_same"
                }
            }
        },
        "TURBO_RATIO_LIMIT": {
            "offset": "0x1AD",
            "domain": "package",
            "fields": {
                "MAX_RATIO_LIMIT_0": {
                    "begin_bit": 0,
                    "end_bit":   7,
                    "function":  "scale",
                    "units":     "hertz",
                    "scalar":    1e8,
                    "behavior":  "constant",
                    "writeable": false,
                    "description": "Maximum turbo frequency with up to MSR::TURBO_RATIO_LIMIT_CORES:NUMCORE_0 active cores.",
                    "aggregation": "expect_same"
                },
                "MAX_RATIO_LIMIT_1": {
                    "begin_bit": 8,
                    "end_bit":   15,
                    "function":  "scale",
                    "units":     "hertz",
                    "scalar":    1e8,
                    "behavior":  "constant",
                    "writeable": false,
                    "description": "Maximum turbo frequency with more than MSR::TURBO_RATIO_LIMIT_CORES:NUMCORE_0 and up to MSR::TURBO_RATIO_LIMIT_CORES:NUMCORE_1 active cores.",
                    "aggregation": "expect_same"
                },
                "MAX_RATIO_LIMIT_2": {
                    "begin_bit": 16,
                    "end_bit":   23,
                    "function":  "scale",
                    "units":     "hertz",
                    "scalar":    1e8,
                    "behavior":  "constant",
                    "writeable": false,
                    "description": "Maximum turbo frequency with more than MSR::TURBO_RATIO_LIMIT_CORES:NUMCORE_1 and up to MSR::TURBO_RATIO_LIMIT_CORES:NUMCORE_2 active cores.",
                    "aggregation": "expect_same"
                },
                "MAX_RATIO_LIMIT_3": {
                    "begin_bit": 24,
                    "end_bit":   31,
                    "function":  "scale",
                    "units":     "hertz",
                    "scalar":    1e8,
                    "behavior":  "constant",
                    "writeable": false,
                    "description": "Maximum turbo frequency with more than MSR::TURBO_RATIO_LIMIT_CORES:NUMCORE_2 and up to MSR::TURBO_RATIO_LIMIT_CORES:NUMCORE_3 active cores.",
                    "aggregation": "expect_same"
                },
                "MAX_RATIO_LIMIT_4": {
                    "begin_bit": 32,
                    "end_bit":   39,
                    "function":  "scale",
                    "units":     "hertz",
                    "scalar":    1e8,
                    "behavior":  "constant",
                    "writeable": false,
                    "description": "Maximum turbo frequency with more than MSR::TURBO_RATIO_LIMIT_CORES:NUMCORE_3 and up to MSR::TURBO_RATIO_LIMIT_CORES:NUMCORE_4 active cores.",
                    "aggregation": "expect_same"
                },
                "MAX_RATIO_LIMIT_5": {
                    "begin_bit": 40,
                    "end_bit":   47,
                    "function":  "scale",
                    "units":     "hertz",
                    "scalar":    1e8,
                    "behavior":  "constant",
                    "writeable": false,
                    "description": "Maximum turbo frequency with more than MSR::TURBO_RATIO_LIMIT_CORES:NUMCORE_4 and up to MSR::TURBO_RATIO_LIMIT_CORES:NUMCORE_5 active cores.",
                    "aggregation": "expect_same"
                },
                "MAX_RATIO_LIMIT_6": {
                    "begin_bit": 48,
                    "end_bit":   55,
                    "function":  "scale",
                    "units":     "hertz",
                    "scalar":    1e8,
                    "behavior":  "constant",
                    "writeable": false,
                    "description": "Maximum turbo frequency with more than MSR::TURBO_RATIO_LIMIT_CORES:NUMCORE_5 and up to MSR::TURBO_RATIO_LIMIT_CORES:NUMCORE_6 active cores.",
                    "aggregation": "expect_same"
                },
                "MAX_RATIO_LIMIT_7": {
                    "begin_bit": 56,
                    "end_bit":   63,
                    "function":  "scale",
                    "units":     "hertz",
                    "scalar":    1e8,
                    "behavior":  "constant",
                    "writeable": false,
                    "description": "Maximum turbo frequency with more than MSR::TURBO_RATIO_LIMIT_CORES:NUMCORE_6 and up to MSR::TURBO_RATIO_LIMIT_CORES:NUMCORE_7 active cores.",
                    "aggregation": "expect_same"
                }
            }
        },
        "TURBO_RATIO_LIMIT_CORES": {
            "offset": "0x1AE",
            "domain": "package",
            "fields": {
                "NUMCORE_0": {
                    "begin_bit": 0,
                    "end_bit":   7,
                    "function":  "scale",
                    "units":     "none",
                    "scalar":    1,
                    "behavior":  "constant",
                    "writeable": false,
                    "description": "Maximum number of active cores for a maximum turbo frequency of MSR::TURBO_RATIO_LIMIT:MAX_RATIO_LIMIT_0.",
                    "aggregation": "expect_same"
                },
                "NUMCORE_1": {
                    "begin_bit": 8,
                    "end_bit":   15,
                    "function":  "scale",
                    "units":     "none",
                    "scalar":    1,
                    "behavior":  "constant",
                    "writeable": false,
                    "description": "Maximum number of active cores for a maximum turbo frequency of MSR::TURBO_RATIO_LIMIT:MAX_RATIO_LIMIT_1.",
                    "aggregation": "expect_same"
                },
                "NUMCORE_2": {
                    "begin_bit": 16,
                    "end_bit":   23,
                    "function":  "scale",
                    "units":     "none",
                    "scalar":    1,
                    "behavior":  "constant",
                    "writeable": false,
                    "description": "Maximum number of active cores for a maximum turbo frequency of MSR::TURBO_RATIO_LIMIT:MAX_RATIO_LIMIT_2.",
                    "aggregation": "expect_same"
                },
                "NUMCORE_3": {
                    "begin_bit": 24,
                    "end_bit":   31,
                    "function":  "scale",
                    "units":     "none",
                    "scalar":    1,
                    "behavior":  "constant",
                    "writeable": false,
                    "description": "Maximum number of active cores for a maximum turbo frequency of MSR::TURBO_RATIO_LIMIT:MAX_RATIO_LIMIT_3.",
                    "aggregation": "expect_same"
                },
                "NUMCORE_4": {
                    "begin_bit": 32,
                    "end_bit":   39,
                    "function":  "scale",
                    "units":     "none",
                    "scalar":    1,
                    "behavior":  "constant",
                    "writeable": false,
                    "description": "Maximum number of active cores for a maximum turbo frequency of MSR::TURBO_RATIO_LIMIT:MAX_RATIO_LIMIT_4.",
                    "aggregation": "expect_same"
                },
                "NUMCORE_5": {
                    "begin_bit": 40,
                    "end_bit":   47,
                    "function":  "scale",
                    "units":     "none",
                    "scalar":    1,
                    "behavior":  "constant",
                    "writeable": false,
                    "description": "Maximum number of active cores for a maximum turbo frequency of MSR::TURBO_RATIO_LIMIT:MAX_RATIO_LIMIT_5.",
                    "aggregation": "expect_same"
                },
                "NUMCORE_6": {
                    "begin_bit": 48,
                    "end_bit":   55,
                    "function":  "scale",
                    "units":     "none",
                    "scalar":    1,
                    "behavior":  "constant",
                    "writeable": false,
                    "description": "Maximum number of active cores for a maximum turbo frequency of MSR::TURBO_RATIO_LIMIT:MAX_RATIO_LIMIT_6.",
                    "aggregation": "expect_same"
                },
                "NUMCORE_7": {
                    "begin_bit": 56,
                    "end_bit":   63,
                    "function":  "scale",
                    "units":     "none",
                    "scalar":    1,
                    "behavior":  "constant",
                    "writeable": false,
                    "description": "Maximum number of active cores for a maximum turbo frequency of MSR::TURBO_RATIO_LIMIT:MAX_RATIO_LIMIT_7.",
                    "aggregation": "expect_same"
                }
            }
        },
        "RAPL_POWER_UNIT": {
            "offset": "0x606",
            "domain": "package",
            "fields": {
                "POWER": {
                    "begin_bit": 0,
                    "end_bit":   3,
                    "function":  "log_half",
                    "units":     "watts",
                    "scalar":    1.0,
                    "behavior":  "constant",
                    "writeable": false,
                    "description": "The resolution of RAPL power interfaces.",
                    "aggregation": "expect_same"
                },
                "ENERGY": {
                    "begin_bit": 8,
                    "end_bit":   12,
                    "function":  "log_half",
                    "units":     "joules",
                    "scalar":    1.0,
                    "behavior":  "constant",
                    "writeable": false,
                    "description": "The resolution of RAPL energy interfaces.",
                    "aggregation": "expect_same"
                },
                "TIME": {
                    "begin_bit": 16,
                    "end_bit":   19,
                    "function":  "log_half",
                    "units":     "seconds",
                    "scalar":    1.0,
                    "behavior":  "constant",
                    "writeable": false,
                    "description": "The resolution of RAPL time interfaces.",
                    "aggregation": "expect_same"
                }
            }
        },
        "PKG_POWER_LIMIT": {
            "offset": "0x610",
            "domain": "package",
            "fields": {
                "PL1_POWER_LIMIT": {
                    "begin_bit": 0,
                    "end_bit":   14,
                    "function":  "scale",
                    "units":     "watts",
                    "scalar":    1.25e-1,
                    "behavior":  "variable",
                    "writeable": true,
                    "description": "The average power usage limit over the time window specified in PL1_TIME_WINDOW.",
                    "aggregation": "sum"
                },
                "PL1_LIMIT_ENABLE": {
                    "begin_bit": 15,
                    "end_bit":   15,
                    "function":  "logic",
                    "units":     "none",
                    "scalar":    1.0,
                    "behavior":  "variable",
                    "writeable": true,
                    "description": "Enable the limit specified in PL1_POWER_LIMIT. When reading at a higher level domain than its native domain, it aggregates as the count of all such bits that have been set.",
                    "aggregation": "sum"
                },
                "PL1_CLAMP_ENABLE": {
                    "begin_bit": 16,
                    "end_bit":   16,
                    "function":  "logic",
                    "units":     "none",
                    "scalar":    1.0,
                    "behavior":  "variable",
                    "writeable": true,
                    "description": "Allow processor cores to go below the requested P-State or T-State to achieve the requested PL1_POWER_LIMIT. When reading at a higher level domain than its native domain, it aggregates as the count of all such bits that have been set.",
                    "aggregation": "sum"
                },
                "PL1_TIME_WINDOW": {
                    "begin_bit": 17,
                    "end_bit":   23,
                    "function":  "7_bit_float",
                    "units":     "seconds",
                    "scalar":    9.765625e-04,
                    "behavior":  "variable",
                    "writeable": true,
                    "description": "The time window associated with power limit 1.",
                    "aggregation": "expect_same"
                },
                "PL2_POWER_LIMIT": {
                    "begin_bit": 32,
                    "end_bit":   46,
                    "function":  "scale",
                    "units":     "watts",
                    "scalar":    1.25e-1,
                    "behavior":  "variable",
                    "writeable": true,
                    "description": "The average power usage limit over the time window specified in PL2_TIME_WINDOW.",
                    "aggregation": "sum"
                },
                "PL2_LIMIT_ENABLE": {
                    "begin_bit": 47,
                    "end_bit":   47,
                    "function":  "logic",
                    "units":     "none",
                    "scalar":    1.0,
                    "behavior":  "variable",
                    "writeable": true,
                    "description": "Enable the limit specified in PL2_POWER_LIMIT. When reading at a higher level domain than its native domain, it aggregates as the count of all such bits that have been set.",
                    "aggregation": "sum"
                },
                "PL2_CLAMP_ENABLE": {
                    "begin_bit": 48,
                    "end_bit":   48,
                    "function":  "logic",
                    "units":     "none",
                    "scalar":    1.0,
                    "behavior":  "variable",
                    "writeable": true,
                    "description": "Allow processor cores to go below the requested P-State or T-State to achieve the requested PL2_POWER_LIMIT. When reading at a higher level domain than its native domain, it aggregates as the count of all such bits that have been set.",
                    "aggregation": "sum"
                },
                "PL2_TIME_WINDOW": {
                    "begin_bit": 49,
                    "end_bit":   55,
                    "function":  "7_bit_float",
                    "units":     "seconds",
                    "scalar":    9.765625e-04,
                    "behavior":  "variable",
                    "writeable": true,
                    "description": "The time window associated with power limit 2.",
                    "aggregation": "expect_same"
                },
                "LOCK": {
                    "begin_bit": 63,
                    "end_bit":   63,
                    "function":  "logic",
                    "units":     "none",
                    "scalar":    1.0,
                    "behavior":  "constant",
                    "writeable": false,
                    "description": "Ignore any changes to PL1 and PL2 configuration in PKG_POWER_LIMIT until the next reset.",
                    "aggregation": "expect_same"
                }
            }
        },
        "PKG_ENERGY_STATUS": {
            "offset": "0x611",
            "domain": "package",
            "fields": {
                "ENERGY": {
                    "begin_bit": 0,
                    "end_bit":   31,
                    "function":  "overflow",
                    "units":     "joules",
                    "scalar":    6.103515625e-05,
                    "behavior":  "monotone",
                    "writeable": false,
                    "aggregation": "sum",
                    "description": "An increasing meter of energy consumed by the package over time.  It will reset periodically due to roll-over."
                }
            }
        },
        "PKG_POWER_INFO": {
            "offset": "0x614",
            "domain": "package",
            "fields": {
                "THERMAL_SPEC_POWER": {
                    "begin_bit": 0,
                    "end_bit":   14,
                    "function":  "scale",
                    "units":     "watts",
                    "scalar":    1.25e-1,
                    "behavior":  "constant",
                    "writeable": false,
                    "aggregation": "sum",
                    "description": "Maximum power to stay within the thermal limits based on the design (TDP)."
                },
                "MIN_POWER": {
                    "begin_bit": 16,
                    "end_bit":   30,
                    "function":  "scale",
                    "units":     "watts",
                    "scalar":    1.25e-1,
                    "behavior":  "constant",
                    "writeable": false,
                    "aggregation": "sum",
                    "description": "The minimum power limit based on the electrical specification."
                },
                "MAX_POWER": {
                    "begin_bit": 32,
                    "end_bit":   46,
                    "function":  "scale",
                    "units":     "watts",
                    "scalar":    1.25e-1,
                    "behavior":  "constant",
                    "writeable": false,
                    "aggregation": "sum",
                    "description": "The maximum power limit based on the electrical specification."
                },
                "MAX_TIME_WINDOW": {
                    "begin_bit": 48,
                    "end_bit":   54,
                    "function":  "7_bit_float",
                    "units":     "seconds",
                    "scalar":    9.765625e-04,
                    "behavior":  "constant",
                    "writeable": false,
                    "description": "The maximum time accepted in MSR::PKG_POWER_LIMIT:PL1_TIME_WINDOW and MSR::PKG_POWER_LIMIT:PL2_TIME_WINDOW.",
                    "aggregation": "expect_same"
                }
            }
        },
        "DRAM_POWER_LIMIT": {
            "offset": "0x618",
            "domain": "memory",
            "fields": {
                "POWER_LIMIT": {
                    "begin_bit": 0,
                    "end_bit":   14,
                    "function":  "scale",
                    "units":     "watts",
                    "scalar":    1.25e-1,
                    "behavior":  "variable",
                    "writeable": true,
                    "description": "The average DRAM power usage limit over the time window specified in TIME_WINDOW.",
                    "aggregation": "sum"
                },
                "ENABLE": {
                    "begin_bit": 15,
                    "end_bit":   15,
                    "function":  "logic",
                    "units":     "none",
                    "scalar":    1.0,
                    "behavior":  "variable",
                    "writeable": true,
                    "description": "Enable the limit specified in POWER_LIMIT. When reading at a higher level domain than its native domain, it aggregates as the count of all such bits that have been set.",
                    "aggregation": "sum"
                },
                "TIME_WINDOW": {
                    "begin_bit": 17,
                    "end_bit":   23,
                    "function":  "7_bit_float",
                    "units":     "seconds",
                    "scalar":    9.765625e-04,
                    "behavior":  "variable",
                    "writeable": true,
                    "description": "The time window associated with the DRAM power limit.",
                    "aggregation": "expect_same"
                },
                "LOCK": {
                    "begin_bit": 31,
                    "end_bit":   31,
                    "function":  "logic",
                    "units":     "none",
                    "scalar":    1,
                    "behavior":  "constant",
                    "writeable": false,
                    "description": "Ignore any changes to configuration in DRAM_POWER_LIMIT until the next reset.",
                    "aggregation": "expect_same"
                }
            }
        },
        "DRAM_ENERGY_STATUS": {
            "offset": "0x619",
            "domain": "package",
            "fields": {
                "ENERGY": {
                    "begin_bit": 0,
                    "end_bit":   31,
                    "function":  "overflow",
                    "units":     "joules",
                    "scalar":    1.5258789063e-05,
                    "behavior":  "monotone",
                    "writeable": false,
                    "aggregation": "sum",
                    "description": "An increasing meter of energy consumed by the DRAM over time.  It will reset periodically due to roll-over."
                }
            }
        },
        "DRAM_PERF_STATUS": {
            "offset": "0x61B",
            "domain": "memory",
            "fields": {
                "THROTTLE_TIME": {
                    "begin_bit": 0,
                    "end_bit":   31,
                    "function":  "scale",
                    "units":     "seconds",
                    "scalar":    9.765625e-04,
                    "behavior":  "monotone",
                    "writeable": false,
                    "description": "The amount of time that the package was throttled below the requested frequency due to MSR::DRAM_POWER_LIMIT:POWER_LIMIT.",
                    "aggregation": "sum"
                }
            }
        },
        "DRAM_POWER_INFO": {
            "offset": "0x61C",
            "domain": "memory",
            "fields": {
                "THERMAL_SPEC_POWER": {
                    "begin_bit": 0,
                    "end_bit":   14,
                    "function":  "scale",
                    "units":     "watts",
                    "scalar":    1.25e-1,
                    "behavior":  "constant",
                    "writeable": false,
                    "description": "Maximum DRAM power to stay within the thermal limits based on the design.",
                    "aggregation": "sum"
                },
                "MIN_POWER": {
                    "begin_bit": 16,
                    "end_bit":   30,
                    "function":  "scale",
                    "units":     "watts",
                    "scalar":    1.25e-1,
                    "behavior":  "constant",
                    "writeable": false,
                    "description": "The minimum DRAM power limit based on the electrical specification.",
                    "aggregation": "sum"
                },
                "MAX_POWER": {
                    "begin_bit": 32,
                    "end_bit":   46,
                    "function":  "scale",
                    "units":     "watts",
                    "scalar":    1.25e-1,
                    "behavior":  "constant",
                    "writeable": false,
                    "description": "The maximum DRAM power limit based on the electrical specification.",
                    "aggregation": "sum"
                },
                "MAX_TIME_WINDOW": {
                    "begin_bit": 48,
                    "end_bit":   54,
                    "function":  "7_bit_float",
                    "units":     "seconds",
                    "scalar":    9.765625e-04,
                    "behavior":  "constant",
                    "writeable": false,
                    "description": "The maximum value accepted in MSR::DRAM_POWER_LIMIT:TIME_WINDOW.",
                    "aggregation": "expect_same"
                }
            }
        },
        "PPERF": {
            "offset": "0x64E",
            "domain": "cpu",
            "fields": {
                "PCNT": {
                    "begin_bit": 0,
                    "end_bit":   47,
                    "function":  "overflow",
                    "units":     "none",
                    "scalar":    1.0,
                    "behavior":  "monotone",
                    "writeable": false,
                    "aggregation": "sum",
                    "description": "A filtered counter of MSR::APERF:ACNT that only increments for cycles the hardware expects are productive toward instruction execution. This counter cannot measure processor performance when the CPU is inactive."
                }
            }
        },
        "UNCORE_RATIO_LIMIT": {
            "offset": "0x620",
            "domain": "package",
            "fields": {
                "MIN_RATIO": {
                    "begin_bit": 8,
                    "end_bit":   14,
                    "function":  "scale",
                    "units":     "hertz",
                    "scalar":    1e8,
                    "behavior":  "variable",
                    "writeable": true,
                    "description": "A lower limit for uncore frequency control. Note: when querying at a higher domain, if NaN is returned, query at its native domain.",
                    "aggregation": "expect_same"
                },
                "MAX_RATIO": {
                    "begin_bit": 0,
                    "end_bit":   6,
                    "function":  "scale",
                    "units":     "hertz",
                    "scalar":    1e8,
                    "behavior":  "variable",
                    "writeable": true,
                    "description": "An upper limit for uncore frequency control. Note: when querying at a higher domain, if NaN is returned, query at its native domain.",
                    "aggregation": "expect_same"
                }
            }
        },
        "QM_EVTSEL": {
            "offset": "0xC8D",
            "domain": "package",
            "fields": {
                "EVENT_ID": {
                    "begin_bit": 0,
                    "end_bit":   7,
                    "function":  "scale",
                    "units":     "none",
                    "scalar":    1.0,
                    "behavior":  "label",
                    "writeable": true,
                    "description": "Set an event code to choose which resource is monitored in MSR::QM_CTR:RM_DATA. Refer to the Intel(R) 64 and IA-32 Architectures Software Developer's Manual for more information about how to use this MSR with Cache Monitoring Technology and Memory Bandwidth Monitoring. Event counts are accumulated in MSR::QM_CTR::RM_DATA.",
                    "aggregation": "expect_same"
                },
                "RMID": {
                    "begin_bit": 32,
                    "end_bit":   41,
                    "function":  "scale",
                    "units":     "none",
                    "scalar":    1.0,
                    "behavior":  "label",
                    "writeable": true,
                    "description": "Specify which resource monitoring identifier (RMID) must be active to update MSR::QM_CTR:RM_DATA. Associate RMIDs with CPUs by writing to MSR::PQR_ASSOC:RMID.",
                    "aggregation": "expect_same"
                }
            }
        },
        "QM_CTR": {
            "offset": "0xC8E",
            "domain": "package",
            "fields": {
                "RM_DATA": {
                    "begin_bit": 0,
                    "end_bit":   23,
                    "function":  "overflow",
                    "units":     "none",
                    "scalar":    1.0,
                    "behavior":  "variable",
                    "writeable": false,
                    "aggregation": "sum",
                    "description": "The raw counted value for the MSR::QM_EVTSEL:* configuration. Configurations that report bandwidth metrics report a raw value based on an implementation-specific counter. If reading a bandwidth metric, read the QM_CTR_SCALED alias instead."
                },
                "UNAVAILABLE": {
                    "begin_bit": 62,
                    "end_bit":   62,
                    "function":  "logic",
                    "units":     "none",
                    "scalar":    1.0,
                    "behavior":  "variable",
                    "writeable": false,
                    "description": "Indicates that no monitoring data is available, and MSR::QM_CTR:RM_DATA does not contain valid data. When reading at a higher level domain than its native domain, it aggregates as the count of all such bits that have been set.",
                    "aggregation": "sum"
                },
                "ERROR": {
                    "begin_bit": 63,
                    "end_bit":   63,
                    "function":  "logic",
                    "units":     "none",
                    "scalar":    1.0,
                    "behavior":  "variable",
                    "writeable": false,
                    "description": "Indicates an unsupported configuration in MSR::QM_EVTSEL:*, and that MSR::QM_CTR:RM_DATA does not contain valid data. When reading at a higher level domain than its native domain, it aggregates as the count of all such bits that have been set.",
                    "aggregation": "sum"
                }
            }
        },
        "PQR_ASSOC": {
            "offset": "0xC8F",
            "domain": "cpu",
            "fields": {
                "RMID": {
                    "begin_bit": 0,
                    "end_bit":   9,
                    "function":  "scale",
                    "units":     "none",
                    "scalar":    1.0,
                    "behavior":  "variable",
                    "writeable": true,
                    "description": "The resource monitoring identifier (RMID) currently associated with this CPU. Multiple CPUs are permitted to map to the same RMID. RMID-based resource monitoring interfaces track each monitored resource by a CPU package, RMID pair.",
                    "aggregation": "expect_same"
                }
            }
        },
        "UNCORE_PERF_STATUS": {
            "offset": "0x621",
            "domain": "package",
            "fields": {
                "FREQ": {
                    "begin_bit": 0,
                    "end_bit":   6,
                    "function":  "scale",
                    "units":     "hertz",
                    "scalar":    1e8,
                    "behavior":  "variable",
                    "writeable": false,
                    "description": "The current uncore frequency.",
                    "aggregation": "average"
                }
            }
        },
        "MISC_FEATURE_CONTROL": {
            "offset": "0x1a4",
            "domain": "cpu",
            "fields": {
                "L2_HW_PREFETCHER_DISABLE": {
                    "begin_bit": 0,
                    "end_bit":   0,
                    "function":  "logic",
                    "units":     "none",
                    "scalar":    1,
                    "behavior":  "variable",
                    "aggregation": "sum",
                    "writeable": true,
                    "description": "Disable for the L2 hardware prefetcher. When reading at a higher level domain than its native domain, it aggregates as the count of all such bits that have been set."
                },
                "L2_ADJACENT_PREFETCHER_DISABLE": {
                    "begin_bit": 1,
                    "end_bit":   1,
                    "function":  "logic",
                    "units":     "none",
                    "scalar":    1,
                    "behavior":  "variable",
                    "aggregation": "sum",
                    "writeable": true,
                    "description": "Disable for the L2 adjacent cache line prefetcher When reading at a higher level domain than its native domain, it aggregates as the count of all such bits that have been set."
                },
                "DCU_HW_PREFETCHER_DISABLE": {
                    "begin_bit": 2,
                    "end_bit":   2,
                    "function":  "logic",
                    "units":     "none",
                    "scalar":    1,
                    "behavior":  "variable",
                    "aggregation": "sum",
                    "writeable": true,
                    "description": "Disable for the L1 data cache prefetcher When reading at a higher level domain than its native domain, it aggregates as the count of all such bits that have been set."
                },
                "DCU_IP_PREFETCHER_DISABLE": {
                    "begin_bit": 3,
                    "end_bit":   3,
                    "function":  "logic",
                    "units":     "none",
                    "scalar":    1,
                    "behavior":  "variable",
                    "aggregation": "sum",
                    "writeable": true,
                    "description": "Disable for the L1 data cache instruction pointer prefetcher When reading at a higher level domain than its native domain, it aggregates as the count of all such bits that have been set."
                }
            }
        },
        "PM_ENABLE": {
            "offset": "0x770",
            "domain": "package",
            "fields": {
                "HWP_ENABLE": {
                    "begin_bit": 0,
                    "end_bit":   0,
                    "function":  "logic",
                    "units":     "none",
                    "scalar":    1.0,
                    "behavior":  "variable",
                    "writeable": false,
                    "description": "Indicates HWP enabled status.  Once enabled a system reset is required to disable. When reading at a higher level domain than its native domain, it aggregates as the count of all such bits that have been set.",
                    "aggregation": "sum"
                }
            }
        },
        "HWP_CAPABILITIES": {
            "offset": "0x771",
            "domain": "package",
            "fields": {
                "HIGHEST_PERFORMANCE": {
                    "begin_bit": 0,
                    "end_bit":   7,
                    "function":  "scale",
                    "units":     "hertz",
                    "scalar":    1e8,
                    "behavior":  "variable",
                    "aggregation": "expect_same",
                    "writeable": false,
                    "description": "Maximum non-guaranteed performance level when using HWP."
                },
                "GUARANTEED_PERFORMANCE": {
                    "begin_bit": 8,
                    "end_bit":   15,
                    "function":  "scale",
                    "units":     "hertz",
                    "scalar":    1e8,
                    "behavior":  "variable",
                    "aggregation": "expect_same",
                    "writeable": false,
                    "description": "Current guaranteed performance level.  This may change dynamically based on various system constraints."
                },
                "MOST_EFFICIENT_PERFORMANCE": {
                    "begin_bit": 16,
                    "end_bit":   23,
                    "function":  "scale",
                    "units":     "hertz",
                    "scalar":    1e8,
                    "behavior":  "variable",
                    "aggregation": "expect_same",
                    "writeable": false,
                    "description": "Current value of the most efficient performance level.  May change dynamically."
                },
                "LOWEST_PERFORMANCE": {
                    "begin_bit": 24,
                    "end_bit":   31,
                    "function":  "scale",
                    "units":     "hertz",
                    "scalar":    1e8,
                    "behavior":  "variable",
                    "aggregation": "expect_same",
                    "writeable": false,
                    "description": "Minimum performance level when using HWP."
                }
            }
        },
        "HWP_REQUEST": {
            "offset": "0x774",
            "domain": "cpu",
            "fields": {
                "MINIMUM_PERFORMANCE": {
                    "begin_bit": 0,
                    "end_bit":   7,
                    "function":  "scale",
                    "units":     "hertz",
                    "scalar":    1e8,
                    "behavior":  "variable",
                    "aggregation": "average",
                    "writeable": true,
                    "description": "A hint to HWP on the minimum performance level required."
                },
                "MAXIMUM_PERFORMANCE": {
                    "begin_bit": 8,
                    "end_bit":   15,
                    "function":  "scale",
                    "units":     "hertz",
                    "scalar":    1e8,
                    "behavior":  "variable",
                    "aggregation": "average",
                    "writeable": true,
                    "description": "A hint to HWP on the maximum performance level required."
                },
                "DESIRED_PERFORMANCE": {
                    "begin_bit": 16,
                    "end_bit":   23,
                    "function":  "scale",
                    "units":     "hertz",
                    "scalar":    1e8,
                    "behavior":  "variable",
                    "aggregation": "average",
                    "writeable": false,
                    "description": "An explicit performance request.  Setting to zero enables HWP Autonomous states.  Any other value effectively disables HWP Autonomous selection."
                },
                "ENERGY_PERFORMANCE_PREFERENCE": {
                    "begin_bit": 24,
                    "end_bit":   31,
                    "function":  "scale",
                    "units":     "none",
                    "scalar":    1.0,
                    "behavior":  "variable",
                    "aggregation": "average",
                    "writeable": true,
                    "description": "Influences rate of performance increase/decrease.  0x00 = performance, 0xFF = energy efficiency."
                },
                "ACTIVITY_WINDOW": {
                    "begin_bit": 32,
                    "end_bit":   41,
                    "function":  "scale",
                    "units":     "none",
                    "scalar":    1.0,
                    "behavior":  "variable",
                    "aggregation": "average",
                    "writeable": false,
                    "description": "A hint to HWP indicating the observation window for performance/frequency optimizations."
                },
                "PACKAGE_CONTROL": {
                    "begin_bit": 42,
                    "end_bit":   42,
                    "function":  "logic",
                    "units":     "none",
                    "scalar":    1.0,
                    "behavior":  "variable",
                    "aggregation": "sum",
                    "writeable": true,
                    "description": "If set overrides requests with the package level request. When reading at a higher level domain than its native domain, it aggregates as the count of all such bits that have been set."
                },
                "EPP_VALID": {
                    "begin_bit": 60,
                    "end_bit":   60,
                    "function":  "logic",
                    "units":     "none",
                    "scalar":    1.0,
                    "behavior":  "variable",
                    "aggregation": "sum",
                    "writeable": true,
                    "description": "If set indicates HWP should use the related cpu MSR field value regardless of the PACKAGE_CONTROL bit setting. When reading at a higher level domain than its native domain, it aggregates as the count of all such bits that have been set."
                },
                "DESIRED_VALID": {
                    "begin_bit": 61,
                    "end_bit":   61,
                    "function":  "logic",
                    "units":     "none",
                    "scalar":    1.0,
                    "behavior":  "variable",
                    "aggregation": "sum",
                    "writeable": true,
                    "description": "If set indicates HWP should use the related cpu MSR field value regardless of the PACKAGE_CONTROL bit setting. When reading at a higher level domain than its native domain, it aggregates as the count of all such bits that have been set."
                },
                "MAXIMUM_VALID": {
                    "begin_bit": 62,
                    "end_bit":   62,
                    "function":  "logic",
                    "units":     "none",
                    "scalar":    1.0,
                    "behavior":  "variable",
                    "aggregation": "sum",
                    "writeable": true,
                    "description": "If set indicates HWP should use the related cpu MSR field value regardless of the PACKAGE_CONTROL bit setting. When reading at a higher level domain than its native domain, it aggregates as the count of all such bits that have been set."
                },
                "MINIMUM_VALID": {
                    "begin_bit": 63,
                    "end_bit":   63,
                    "function":  "logic",
                    "units":     "none",
                    "scalar":    1.0,
                    "behavior":  "variable",
                    "aggregation": "sum",
                    "writeable": true,
                    "description": "If set indicates HWP should use the related cpu MSR field value regardless of the PACKAGE_CONTROL bit setting. When reading at a higher level domain than its native domain, it aggregates as the count of all such bits that have been set."
                }
            }
        },
        "HWP_STATUS": {
            "offset": "0x777",
            "domain": "cpu",
            "fields": {
                "GUARANTEED_PERFORMANCE_CHANGE": {
                    "begin_bit": 0,
                    "end_bit":   0,
                    "function":  "logic",
                    "units":     "none",
                    "scalar":    1.0,
                    "behavior":  "variable",
                    "aggregation": "sum",
                    "writeable": true,
                    "description": "Log bit indicating if a GUARANTEED_PERFORMANCE change has occurred.  Software responsible to clear via write to 0. When reading at a higher level domain than its native domain, it aggregates as the count of all such bits that have been set."
                },
                "EXCURSION_TO_MINIMUM": {
                    "begin_bit": 2,
                    "end_bit":   2,
                    "function":  "logic",
                    "units":     "none",
                    "scalar":    1.0,
                    "behavior":  "variable",
                    "aggregation": "sum",
                    "writeable": true,
                    "description": "Log bit indicating if an excursion below the minimum requested performance has occurred.  Software responsible to clear via write to 0. When reading at a higher level domain than its native domain, it aggregates as the count of all such bits that have been set."
                },
                "HIGHEST_CHANGE": {
                    "begin_bit": 3,
                    "end_bit":   3,
                    "function":  "logic",
                    "units":     "none",
                    "scalar":    1.0,
                    "behavior":  "variable",
                    "aggregation": "sum",
                    "writeable": true,
                    "description": "Log bit indicating if a HIGHEST_PEROFRMANCE change has occurred.  Software responsible to clear via write to 0. When reading at a higher level domain than its native domain, it aggregates as the count of all such bits that have been set."
                },
                "PECI_OVERRIDE_ENTRY": {
                    "begin_bit": 4,
                    "end_bit":   4,
                    "function":  "logic",
                    "units":     "none",
                    "scalar":    1.0,
                    "behavior":  "variable",
                    "aggregation": "sum",
                    "writeable": true,
                    "description": "Indicates a PECI override request that will override the HWP MSR values has started.  Software responsible to clear via write to 0 When reading at a higher level domain than its native domain, it aggregates as the count of all such bits that have been set."
                },
                "PECI_OVERRIDE_EXIT": {
                    "begin_bit": 5,
                    "end_bit":   5,
                    "function":  "logic",
                    "units":     "none",
                    "scalar":    1.0,
                    "behavior":  "variable",
                    "aggregation": "sum",
                    "writeable": true,
                    "description": "Indicates a PECI override request that will override the HWP MSR values has ended.  Software responsible to clear via write to 0 When reading at a higher level domain than its native domain, it aggregates as the count of all such bits that have been set."
                }
            }
        },
        "PLATFORM_RAPL_PERF_STATUS": {
            "offset": "0x666",
            "domain": "board",
            "fields": {
                "PWR_LIMIT_THROTTLE_CTR" : {
                    "begin_bit": 0,
                    "end_bit":   31,
                    "function":  "overflow",
                    "units":     "none",
                    "scalar":    1,
                    "behavior":  "monotone",
                    "writeable": false,
                    "aggregation": "expect_same",
                    "description": "An increasing count of throttle events related to PLATFORM_RAPL"
                }
            }
        },
        "PLATFORM_ENERGY_STATUS": {
            "offset": "0x64D",
            "domain": "board",
            "fields": {
                "ENERGY" : {
                    "begin_bit": 0,
                    "end_bit":   31,
                    "function":  "overflow",
                    "units":     "joules",
                    "scalar":    1,
                    "behavior":  "monotone",
                    "writeable": false,
                    "aggregation": "expect_same",
                    "description": "An increasing meter of energy in Joules (U32.0) consumed by the board over time."
                }
            }
        },
        "PLATFORM_POWER_LIMIT": {
            "offset": "0x65C",
            "domain": "board",
            "fields": {
                "PL1_POWER_LIMIT" : {
                    "begin_bit": 0,
                    "end_bit":   16,
                    "function":  "scale",
                    "units":     "watts",
                    "scalar":    1.25e-1,
                    "behavior":  "variable",
                    "writeable": true,
                    "aggregation": "expect_same",
                    "description": "The average board power usage limit over the time window specified in the board PL1_TIME_WINDOW."
                },
                "PL1_LIMIT_ENABLE" : {
                    "begin_bit": 17,
                    "end_bit":   17,
                    "function":  "logic",
                    "units":     "none",
                    "scalar":    1.0,
                    "behavior":  "variable",
                    "writeable": true,
                    "aggregation": "expect_same",
                    "description": "Enable the limit specified in board PL1_POWER_LIMIT."
                },
                "PL1_CLAMP_ENABLE" : {
                    "begin_bit": 18,
                    "end_bit":   18,
                    "function":  "logic",
                    "units":     "none",
                    "scalar":    1.0,
                    "behavior":  "variable",
                    "writeable": true,
                    "aggregation": "expect_same",
                    "description": "Allow hardware to go below the requested P-State to achieve the requested board PL1_POWER_LIMIT."
                },
                "PL1_TIME_WINDOW": {
                    "begin_bit": 19,
                    "end_bit":   25,
                    "function":  "scale",
                    "units":     "seconds",
                    "scalar":    9.765625e-04,
                    "behavior":  "variable",
                    "writeable": true,
                    "description": "The time window associated with the board PL1_POWER_LIMIT",
                    "aggregation": "expect_same"
                },
                "LOCK": {
                    "begin_bit": 63,
                    "end_bit":   63,
                    "function":  "logic",
                    "units":     "none",
                    "scalar":    1.0,
                    "behavior":  "constant",
                    "writeable": false,
                    "description": "Ignore any changes to PL1 and PL2 configuration in PLATFORM_POWER_LIMIT until the next reset.",
                    "aggregation": "expect_same"
                }
            }
        }
    }
}
        )";
        return result;
    }
}
