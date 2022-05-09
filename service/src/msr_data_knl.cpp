/*
 * Copyright (c) 2015 - 2022, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "config.h"

#include <string>

namespace geopm
{
    const std::string knl_msr_json(void)
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
                    "behavior":  "variable",
                    "writeable": false
                },
                "PROGRAMMABLE_RATIO_LIMITS_TURBO_MODE": {
                    "begin_bit": 28,
                    "end_bit":   28,
                    "function":  "scale",
                    "units":     "none",
                    "scalar":    1,
                    "behavior":  "variable",
                    "writeable": false
                },
                "PROGRAMMABLE_TDP_LIMITS_TURBO_MODE": {
                    "begin_bit": 29,
                    "end_bit":   29,
                    "function":  "scale",
                    "units":     "none",
                    "scalar":    1,
                    "behavior":  "variable",
                    "writeable": false
                },
                "MAX_EFFICIENCY_RATIO": {
                    "begin_bit": 40,
                    "end_bit":   47,
                    "function":  "scale",
                    "units":     "hertz",
                    "scalar":    1e8,
                    "behavior":  "variable",
                    "writeable": false
                }
            }
        },
        "PERF_STATUS": {
            "offset": "0x198",
            "domain": "package",
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
            "domain": "package",
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
                    "behavior":  "variable",
                    "writeable": false,
                    "aggregation": "expect_same"
                },
                "TCC_ACTIVE_OFFSET": {
                    "begin_bit": 24,
                    "end_bit":   27,
                    "function":  "scale",
                    "units":     "celsius",
                    "scalar":    1.0,
                    "behavior":  "variable",
                    "writeable": false
                }
            }
        },
        "TURBO_RATIO_LIMIT": {
            "offset": "0x1AD",
            "domain": "package",
            "fields": {
                "GROUP_0_MAX_CORES": {
                    "begin_bit": 1,
                    "end_bit":   7,
                    "function":  "scale",
                    "units":     "none",
                    "scalar":    1.0,
                    "behavior":  "variable",
                    "writeable": false
                },
                "GROUP_0_MAX_RATIO_LIMIT": {
                    "begin_bit": 8,
                    "end_bit":   15,
                    "function":  "scale",
                    "units":     "hertz",
                    "scalar":    1e8,
                    "behavior":  "variable",
                    "writeable": false
                },
                "GROUP_1_INCREMENTAL_CORES": {
                    "begin_bit": 16,
                    "end_bit":   20,
                    "function":  "scale",
                    "units":     "none",
                    "scalar":    1.0,
                    "behavior":  "variable",
                    "writeable": false
                },
                "GROUP_1_RATIO_LIMIT_DELTA": {
                    "begin_bit": 21,
                    "end_bit":   23,
                    "function":  "scale",
                    "units":     "hertz",
                    "scalar":    1e8,
                    "behavior":  "variable",
                    "writeable": false
                },
                "GROUP_2_INCREMENTAL_CORES": {
                    "begin_bit": 24,
                    "end_bit":   28,
                    "function":  "scale",
                    "units":     "none",
                    "scalar":    1.0,
                    "behavior":  "variable",
                    "writeable": false
                },
                "GROUP_2_RATIO_LIMIT_DELTA": {
                    "begin_bit": 29,
                    "end_bit":   31,
                    "function":  "scale",
                    "units":     "hertz",
                    "scalar":    1e8,
                    "behavior":  "variable",
                    "writeable": false
                },
                "GROUP_3_INCREMENTAL_CORES": {
                    "begin_bit": 32,
                    "end_bit":   36,
                    "function":  "scale",
                    "units":     "none",
                    "scalar":    1.0,
                    "behavior":  "variable",
                    "writeable": false
                },
                "GROUP_3_RATIO_LIMIT_DELTA": {
                    "begin_bit": 37,
                    "end_bit":   39,
                    "function":  "scale",
                    "units":     "hertz",
                    "scalar":    1e8,
                    "behavior":  "variable",
                    "writeable": false
                },
                "GROUP_4_INCREMENTAL_CORES": {
                    "begin_bit": 40,
                    "end_bit":   44,
                    "function":  "scale",
                    "units":     "none",
                    "scalar":    1.0,
                    "behavior":  "variable",
                    "writeable": false
                },
                "GROUP_4_RATIO_LIMIT_DELTA": {
                    "begin_bit": 45,
                    "end_bit":   47,
                    "function":  "scale",
                    "units":     "hertz",
                    "scalar":    1e8,
                    "behavior":  "variable",
                    "writeable": false
                },
                "GROUP_5_INCREMENTAL_CORES": {
                    "begin_bit": 48,
                    "end_bit":   52,
                    "function":  "scale",
                    "units":     "none",
                    "scalar":    1.0,
                    "behavior":  "variable",
                    "writeable": false
                },
                "GROUP_5_RATIO_LIMIT_DELTA": {
                    "begin_bit": 53,
                    "end_bit":   55,
                    "function":  "scale",
                    "units":     "hertz",
                    "scalar":    1e8,
                    "behavior":  "variable",
                    "writeable": false
                },
                "GROUP_6_INCREMENTAL_CORES": {
                    "begin_bit": 56,
                    "end_bit":   60,
                    "function":  "scale",
                    "units":     "none",
                    "scalar":    1.0,
                    "behavior":  "variable",
                    "writeable": false
                },
                "GROUP_6_RATIO_LIMIT_DELTA": {
                    "begin_bit": 61,
                    "end_bit":   63,
                    "function":  "scale",
                    "units":     "hertz",
                    "scalar":    1e8,
                    "behavior":  "variable",
                    "writeable": false
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
                    "behavior":  "variable",
                    "writeable": false
                },
                "ENERGY": {
                    "begin_bit": 8,
                    "end_bit":   12,
                    "function":  "log_half",
                    "units":     "joules",
                    "scalar":    1.0,
                    "behavior":  "monotone",
                    "writeable": false
                },
                "TIME": {
                    "begin_bit": 16,
                    "end_bit":   19,
                    "function":  "log_half",
                    "units":     "seconds",
                    "scalar":    1.0,
                    "behavior":  "monotone",
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
        }
    }
}
)";
        return result;
    }
}
