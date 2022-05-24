/*
 * Copyright (c) 2015 - 2022, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "config.h"

#include <string>

namespace geopm
{
    const std::string skx_msr_json(void)
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
                    "writeable": false
                },
                "PROGRAMMABLE_RATIO_LIMITS_TURBO_MODE": {
                    "begin_bit": 28,
                    "end_bit":   28,
                    "function":  "scale",
                    "units":     "none",
                    "scalar":    1,
                    "behavior":  "constant",
                    "writeable": false,
                    "description": "Indicates whether the MSR::TURBO_RATIO_LIMIT:* signals are also available as controls."
                },
                "PROGRAMMABLE_TDP_LIMITS_TURBO_MODE": {
                    "begin_bit": 29,
                    "end_bit":   29,
                    "function":  "scale",
                    "units":     "none",
                    "scalar":    1,
                    "behavior":  "constant",
                    "writeable": false
                },
                "PROGRAMMABLE_TCC_ACTIVATION_OFFSET": {
                    "begin_bit": 30,
                    "end_bit":   30,
                    "function":  "scale",
                    "units":     "none",
                    "scalar":    1,
                    "behavior":  "constant",
                    "writeable": false
                },
                "MAX_EFFICIENCY_RATIO": {
                    "begin_bit": 40,
                    "end_bit":   47,
                    "function":  "scale",
                    "units":     "hertz",
                    "scalar":    1e8,
                    "behavior":  "constant",
                    "writeable": false
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
                    "description": "Target operating frequency of the CPU based on the control register."
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
                    "aggregation": "expect_same"
                },
                "TCC_ACTIVE_OFFSET": {
                    "begin_bit": 24,
                    "end_bit":   27,
                    "function":  "scale",
                    "units":     "celsius",
                    "scalar":    1.0,
                    "behavior":  "constant",
                    "writeable": false
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
                    "description": "Maximum turbo frequency with up to MSR::TURBO_RATIO_LIMIT_CORES:NUMCORE_0 active cores."
                },
                "MAX_RATIO_LIMIT_1": {
                    "begin_bit": 8,
                    "end_bit":   15,
                    "function":  "scale",
                    "units":     "hertz",
                    "scalar":    1e8,
                    "behavior":  "constant",
                    "writeable": false,
                    "description": "Maximum turbo frequency with more than MSR::TURBO_RATIO_LIMIT_CORES:NUMCORE_0 and up to MSR::TURBO_RATIO_LIMIT_CORES:NUMCORE_1 active cores."
                },
                "MAX_RATIO_LIMIT_2": {
                    "begin_bit": 16,
                    "end_bit":   23,
                    "function":  "scale",
                    "units":     "hertz",
                    "scalar":    1e8,
                    "behavior":  "constant",
                    "writeable": false,
                    "description": "Maximum turbo frequency with more than MSR::TURBO_RATIO_LIMIT_CORES:NUMCORE_1 and up to MSR::TURBO_RATIO_LIMIT_CORES:NUMCORE_2 active cores."
                },
                "MAX_RATIO_LIMIT_3": {
                    "begin_bit": 24,
                    "end_bit":   31,
                    "function":  "scale",
                    "units":     "hertz",
                    "scalar":    1e8,
                    "behavior":  "constant",
                    "writeable": false,
                    "description": "Maximum turbo frequency with more than MSR::TURBO_RATIO_LIMIT_CORES:NUMCORE_2 and up to MSR::TURBO_RATIO_LIMIT_CORES:NUMCORE_3 active cores."
                },
                "MAX_RATIO_LIMIT_4": {
                    "begin_bit": 32,
                    "end_bit":   39,
                    "function":  "scale",
                    "units":     "hertz",
                    "scalar":    1e8,
                    "behavior":  "constant",
                    "writeable": false,
                    "description": "Maximum turbo frequency with more than MSR::TURBO_RATIO_LIMIT_CORES:NUMCORE_3 and up to MSR::TURBO_RATIO_LIMIT_CORES:NUMCORE_4 active cores."
                },
                "MAX_RATIO_LIMIT_5": {
                    "begin_bit": 40,
                    "end_bit":   47,
                    "function":  "scale",
                    "units":     "hertz",
                    "scalar":    1e8,
                    "behavior":  "constant",
                    "writeable": false,
                    "description": "Maximum turbo frequency with more than MSR::TURBO_RATIO_LIMIT_CORES:NUMCORE_4 and up to MSR::TURBO_RATIO_LIMIT_CORES:NUMCORE_5 active cores."
                },
                "MAX_RATIO_LIMIT_6": {
                    "begin_bit": 48,
                    "end_bit":   55,
                    "function":  "scale",
                    "units":     "hertz",
                    "scalar":    1e8,
                    "behavior":  "constant",
                    "writeable": false,
                    "description": "Maximum turbo frequency with more than MSR::TURBO_RATIO_LIMIT_CORES:NUMCORE_5 and up to MSR::TURBO_RATIO_LIMIT_CORES:NUMCORE_6 active cores."
                },
                "MAX_RATIO_LIMIT_7": {
                    "begin_bit": 56,
                    "end_bit":   63,
                    "function":  "scale",
                    "units":     "hertz",
                    "scalar":    1e8,
                    "behavior":  "constant",
                    "writeable": false,
                    "description": "Maximum turbo frequency with more than MSR::TURBO_RATIO_LIMIT_CORES:NUMCORE_6 and up to MSR::TURBO_RATIO_LIMIT_CORES:NUMCORE_7 active cores."
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
                    "description": "Maximum number of active cores for a maximum turbo frequency of MSR::TURBO_RATIO_LIMIT:MAX_RATIO_LIMIT_0."
                },
                "NUMCORE_1": {
                    "begin_bit": 8,
                    "end_bit":   15,
                    "function":  "scale",
                    "units":     "none",
                    "scalar":    1,
                    "behavior":  "constant",
                    "writeable": false,
                    "description": "Maximum number of active cores for a maximum turbo frequency of MSR::TURBO_RATIO_LIMIT:MAX_RATIO_LIMIT_1."
                },
                "NUMCORE_2": {
                    "begin_bit": 16,
                    "end_bit":   23,
                    "function":  "scale",
                    "units":     "none",
                    "scalar":    1,
                    "behavior":  "constant",
                    "writeable": false,
                    "description": "Maximum number of active cores for a maximum turbo frequency of MSR::TURBO_RATIO_LIMIT:MAX_RATIO_LIMIT_2."
                },
                "NUMCORE_3": {
                    "begin_bit": 24,
                    "end_bit":   31,
                    "function":  "scale",
                    "units":     "none",
                    "scalar":    1,
                    "behavior":  "constant",
                    "writeable": false,
                    "description": "Maximum number of active cores for a maximum turbo frequency of MSR::TURBO_RATIO_LIMIT:MAX_RATIO_LIMIT_3."
                },
                "NUMCORE_4": {
                    "begin_bit": 32,
                    "end_bit":   39,
                    "function":  "scale",
                    "units":     "none",
                    "scalar":    1,
                    "behavior":  "constant",
                    "writeable": false,
                    "description": "Maximum number of active cores for a maximum turbo frequency of MSR::TURBO_RATIO_LIMIT:MAX_RATIO_LIMIT_4."
                },
                "NUMCORE_5": {
                    "begin_bit": 40,
                    "end_bit":   47,
                    "function":  "scale",
                    "units":     "none",
                    "scalar":    1,
                    "behavior":  "constant",
                    "writeable": false,
                    "description": "Maximum number of active cores for a maximum turbo frequency of MSR::TURBO_RATIO_LIMIT:MAX_RATIO_LIMIT_5."
                },
                "NUMCORE_6": {
                    "begin_bit": 48,
                    "end_bit":   55,
                    "function":  "scale",
                    "units":     "none",
                    "scalar":    1,
                    "behavior":  "constant",
                    "writeable": false,
                    "description": "Maximum number of active cores for a maximum turbo frequency of MSR::TURBO_RATIO_LIMIT:MAX_RATIO_LIMIT_6."
                },
                "NUMCORE_7": {
                    "begin_bit": 56,
                    "end_bit":   63,
                    "function":  "scale",
                    "units":     "none",
                    "scalar":    1,
                    "behavior":  "constant",
                    "writeable": false,
                    "description": "Maximum number of active cores for a maximum turbo frequency of MSR::TURBO_RATIO_LIMIT:MAX_RATIO_LIMIT_7."
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
                    "writeable": false
                },
                "ENERGY": {
                    "begin_bit": 8,
                    "end_bit":   12,
                    "function":  "log_half",
                    "units":     "joules",
                    "scalar":    1.0,
                    "behavior":  "constant",
                    "writeable": false
                },
                "TIME": {
                    "begin_bit": 16,
                    "end_bit":   19,
                    "function":  "log_half",
                    "units":     "seconds",
                    "scalar":    1.0,
                    "behavior":  "constant",
                    "writeable": false
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
                    "description": "The average power usage limit over the time window specified in PL1_TIME_WINDOW."
                },
                "PL1_LIMIT_ENABLE": {
                    "begin_bit": 15,
                    "end_bit":   15,
                    "function":  "scale",
                    "units":     "none",
                    "scalar":    1.0,
                    "behavior":  "variable",
                    "writeable": true
                },
                "PL1_CLAMP_ENABLE": {
                    "begin_bit": 16,
                    "end_bit":   16,
                    "function":  "scale",
                    "units":     "none",
                    "scalar":    1.0,
                    "behavior":  "variable",
                    "writeable": true
                },
                "PL1_TIME_WINDOW": {
                    "begin_bit": 17,
                    "end_bit":   23,
                    "function":  "7_bit_float",
                    "units":     "seconds",
                    "scalar":    9.765625e-04,
                    "behavior":  "variable",
                    "writeable": true,
                    "description": "The time window associated with power limit 1."
                },
                "PL2_POWER_LIMIT": {
                    "begin_bit": 32,
                    "end_bit":   46,
                    "function":  "scale",
                    "units":     "watts",
                    "scalar":    1.25e-1,
                    "behavior":  "variable",
                    "writeable": true
                },
                "PL2_LIMIT_ENABLE": {
                    "begin_bit": 47,
                    "end_bit":   47,
                    "function":  "scale",
                    "units":     "none",
                    "scalar":    1.0,
                    "behavior":  "variable",
                    "writeable": true
                },
                "PL2_CLAMP_ENABLE": {
                    "begin_bit": 48,
                    "end_bit":   48,
                    "function":  "scale",
                    "units":     "none",
                    "scalar":    1.0,
                    "behavior":  "variable",
                    "writeable": true
                },
                "PL2_TIME_WINDOW": {
                    "begin_bit": 49,
                    "end_bit":   55,
                    "function":  "7_bit_float",
                    "units":     "seconds",
                    "scalar":    9.765625e-04,
                    "behavior":  "variable",
                    "writeable": true
                },
                "LOCK": {
                    "begin_bit": 63,
                    "end_bit":   63,
                    "function":  "scale",
                    "units":     "none",
                    "scalar":    1.0,
                    "behavior":  "constant",
                    "writeable": false
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
                    "writeable": false
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
                    "writeable": true
                },
                "ENABLE": {
                    "begin_bit": 15,
                    "end_bit":   15,
                    "function":  "scale",
                    "units":     "none",
                    "scalar":    1.0,
                    "behavior":  "variable",
                    "writeable": true
                },
                "TIME_WINDOW": {
                    "begin_bit": 17,
                    "end_bit":   23,
                    "function":  "7_bit_float",
                    "units":     "seconds",
                    "scalar":    9.765625e-04,
                    "behavior":  "variable",
                    "writeable": true
                },
                "LOCK": {
                    "begin_bit": 31,
                    "end_bit":   31,
                    "function":  "scale",
                    "units":     "none",
                    "scalar":    1,
                    "behavior":  "constant",
                    "writeable": false
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
                    "writeable": false
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
                    "writeable": false
                },
                "MIN_POWER": {
                    "begin_bit": 16,
                    "end_bit":   30,
                    "function":  "scale",
                    "units":     "watts",
                    "scalar":    1.25e-1,
                    "behavior":  "constant",
                    "writeable": false
                },
                "MAX_POWER": {
                    "begin_bit": 32,
                    "end_bit":   46,
                    "function":  "scale",
                    "units":     "watts",
                    "scalar":    1.25e-1,
                    "behavior":  "constant",
                    "writeable": false
                },
                "MAX_TIME_WINDOW": {
                    "begin_bit": 48,
                    "end_bit":   54,
                    "function":  "7_bit_float",
                    "units":     "seconds",
                    "scalar":    9.765625e-04,
                    "behavior":  "constant",
                    "writeable": false
                }
            }
        },
        "PPERF": {
            "offset": "0x64E",
            "domain": "cpu",
            "fields": {
                "PCNT" : {
                    "begin_bit": 0,
                    "end_bit": 47,
                    "function": "overflow",
                    "units": "none",
                    "scalar": 1.0,
                    "behavior":  "monotone",
                    "writeable": false,
                    "aggregation": "sum"
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
                    "writeable": true
                },
                "MAX_RATIO": {
                    "begin_bit": 0,
                    "end_bit":   6,
                    "function":  "scale",
                    "units":     "hertz",
                    "scalar":    1e8,
                    "behavior":  "variable",
                    "writeable": true
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
                    "writeable": true
                },
                "RMID": {
                    "begin_bit": 32,
                    "end_bit":   41,
                    "function":  "scale",
                    "units":     "none",
                    "scalar":    1.0,
                    "behavior":  "label",
                    "writeable": true
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
                    "aggregation": "sum"
                },
                "UNAVAILABLE": {
                    "begin_bit": 62,
                    "end_bit":   62,
                    "function":  "scale",
                    "units":     "none",
                    "scalar":    1.0,
                    "behavior":  "variable",
                    "writeable": false
                },
                "ERROR": {
                    "begin_bit": 63,
                    "end_bit":   63,
                    "function":  "scale",
                    "units":     "none",
                    "scalar":    1.0,
                    "behavior":  "variable",
                    "writeable": false
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
                    "writeable": true
                }
            }
        },
        "UNCORE_PERF_STATUS": {
            "offset": "0x621",
            "domain": "package",
            "fields": {
                "FREQ" : {
                    "begin_bit": 0,
                    "end_bit": 6,
                    "function": "scale",
                    "units": "hertz",
                    "scalar": 1e8,
                    "behavior":  "variable",
                    "writeable": false
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
                    "function":  "scale",
                    "units":     "none",
                    "scalar":    1.0,
                    "behavior":  "variable",
                    "writeable": false,
                    "description": "Indicates HWP enabled status.  Once enabled a system reset is required to disable."
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
        "HWP_REQUEST_PKG": {
            "offset": "0x772",
            "domain": "package",
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
                    "writeable": true,
                    "description": "An explicit performance request.  Setting to zero enables HWP Autonomous states.  Any other value effectively disables HW Autonomous selection."
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
                    "writeable": true,
                    "description": "A hint to HWP indicating the observation window for performance/frequency optimizations."
                }
            }
        },
        "HWP_INTERRUPT": {
            "offset": "0x773",
            "domain": "cpu",
            "fields": {
                "EN_GUARANTEED_PERFORMANCE_CHANGE": {
                    "begin_bit": 0,
                    "end_bit":   0,
                    "function":  "scale",
                    "units":     "none",
                    "scalar":    1.0,
                    "behavior":  "variable",
                    "aggregation": "average",
                    "writeable": true,
                    "description": "If set an interrupt will be generated on a GUARANTEED_PERFORMANCE_CHANGE."
                },
                "EN_EXCURSION_MINIMUM": {
                    "begin_bit": 1,
                    "end_bit":   1,
                    "function":  "scale",
                    "units":     "none",
                    "scalar":    1.0,
                    "behavior":  "variable",
                    "aggregation": "average",
                    "writeable": true,
                    "description": "If set an interrupt will be generated on an excursion below the minimum requested performance."
                },
                "EN_HIGHEST_CHANGE": {
                    "begin_bit": 2,
                    "end_bit":   2,
                    "function":  "scale",
                    "units":     "none",
                    "scalar":    1.0,
                    "behavior":  "variable",
                    "aggregation": "average",
                    "writeable": true,
                    "description": "if set an interrupt will be generated on an change to the highest performance value."
                },
                "EN_PECI_OVERRIDE": {
                    "begin_bit": 3,
                    "end_bit":   3,
                    "function":  "scale",
                    "units":     "none",
                    "scalar":    1.0,
                    "behavior":  "variable",
                    "aggregation": "average",
                    "writeable": true,
                    "description": "If set an interrupt will be generated when PECI override entry or exit occurs."
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
                    "writeable": true,
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
                    "writeable": true,
                    "description": "A hint to HWP indicating the observation window for performance/frequency optimizations."
                },
                "PACKAGE_CONTROL": {
                    "begin_bit": 42,
                    "end_bit":   42,
                    "function":  "scale",
                    "units":     "none",
                    "scalar":    1.0,
                    "behavior":  "variable",
                    "aggregation": "average",
                    "writeable": true,
                    "description": "If set overrides requests with the package level request."
                },
                "EPP_VALID": {
                    "begin_bit": 60,
                    "end_bit":   60,
                    "function":  "scale",
                    "units":     "none",
                    "scalar":    1.0,
                    "behavior":  "variable",
                    "aggregation": "average",
                    "writeable": true,
                    "description": "If set indicates HWP should use the related cpu MSR field value regardless of the PACKAGE_CONTROL bit setting."
                },
                "DESIRED_VALID": {
                    "begin_bit": 61,
                    "end_bit":   61,
                    "function":  "scale",
                    "units":     "none",
                    "scalar":    1.0,
                    "behavior":  "variable",
                    "aggregation": "average",
                    "writeable": true,
                    "description": "If set indicates HWP should use the related cpu MSR field value regardless of the PACKAGE_CONTROL bit setting."
                },
                "MAXIMUM_VALID": {
                    "begin_bit": 62,
                    "end_bit":   62,
                    "function":  "scale",
                    "units":     "none",
                    "scalar":    1.0,
                    "behavior":  "variable",
                    "aggregation": "average",
                    "writeable": true,
                    "description": "If set indicates HWP should use the related cpu MSR field value regardless of the PACKAGE_CONTROL bit setting."
                },
                "MINIMUM_VALID": {
                    "begin_bit": 63,
                    "end_bit":   63,
                    "function":  "scale",
                    "units":     "none",
                    "scalar":    1.0,
                    "behavior":  "variable",
                    "aggregation": "average",
                    "writeable": true,
                    "description": "If set indicates HWP should use the related cpu MSR field value regardless of the PACKAGE_CONTROL bit setting."
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
                    "function":  "scale",
                    "units":     "none",
                    "scalar":    1.0,
                    "behavior":  "variable",
                    "aggregation": "average",
                    "writeable": true,
                    "description": "Log bit indicating if a GUARANTEED_PERFORMANCE change has occured.  Software responsible to clear via write to 0."
                },
                "EXCURSION_TO_MINIMUM": {
                    "begin_bit": 2,
                    "end_bit":   2,
                    "function":  "scale",
                    "units":     "none",
                    "scalar":    1.0,
                    "behavior":  "variable",
                    "aggregation": "average",
                    "writeable": true,
                    "description": "Log bit indicating if an excursion below the minimum requested performance has occured.  Software responsible to clear via write to 0."
                },
                "HIGHEST_CHANGE": {
                    "begin_bit": 3,
                    "end_bit":   3,
                    "function":  "scale",
                    "units":     "none",
                    "scalar":    1.0,
                    "behavior":  "variable",
                    "aggregation": "average",
                    "writeable": true,
                    "description": "Log bit indicating if a HIGHEST_PEROFRMANCE change has occured.  Software responsible to clear via write to 0."
                },
                "PECI_OVERRIDE_ENTRY": {
                    "begin_bit": 4,
                    "end_bit":   4,
                    "function":  "scale",
                    "units":     "none",
                    "scalar":    1.0,
                    "behavior":  "variable",
                    "aggregation": "average",
                    "writeable": true,
                    "description": "Indicates a PECI override request that will override the HWP MSR values has started.  Software responsible to clear via write to 0"
                },
                "PECI_OVERRIDE_EXIT": {
                    "begin_bit": 5,
                    "end_bit":   5,
                    "function":  "scale",
                    "units":     "none",
                    "scalar":    1.0,
                    "behavior":  "variable",
                    "aggregation": "average",
                    "writeable": true,
                    "description": "Indicates a PECI override request that will override the HWP MSR values has ended.  Software responsible to clear via write to 0"
                }
            }
        }
    }
}
)";
        return result;
    }
}
