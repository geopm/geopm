/*
 * Copyright (c) 2015, 2016, 2017, 2018, 2019, Intel Corporation
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
                    "writeable": false
                },
                "PROGRAMMABLE_RATIO_LIMITS_TURBO_MODE": {
                    "begin_bit": 28,
                    "end_bit":   28,
                    "function":  "scale",
                    "units":     "none",
                    "scalar":    1,
                    "writeable": false
                },
                "PROGRAMMABLE_TDP_LIMITS_TURBO_MODE": {
                    "begin_bit": 29,
                    "end_bit":   29,
                    "function":  "scale",
                    "units":     "none",
                    "scalar":    1,
                    "writeable": false
                },
                "PROGRAMMABLE_TCC_ACTIVATION_OFFSET": {
                    "begin_bit": 30,
                    "end_bit":   30,
                    "function":  "scale",
                    "units":     "none",
                    "scalar":    1,
                    "writeable": false
                },
                "MAX_EFFICIENCY_RATIO": {
                    "begin_bit": 40,
                    "end_bit":   47,
                    "function":  "scale",
                    "units":     "hertz",
                    "scalar":    1e8,
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
                    "writeable": false
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
                    "writeable": true
                },
                "TURBO_DISENGAGE": {
                    "begin_bit": 32,
                    "end_bit":   32,
                    "function":  "scale",
                    "units":     "none",
                    "scalar":    1.0,
                    "writeable": true
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
                    "writeable": false
                },
                "TCC_ACTIVE_OFFSET": {
                    "begin_bit": 24,
                    "end_bit":   27,
                    "function":  "scale",
                    "units":     "celsius",
                    "scalar":    1.0,
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
                    "writeable": false
                },
                "MAX_RATIO_LIMIT_1": {
                    "begin_bit": 8,
                    "end_bit":   15,
                    "function":  "scale",
                    "units":     "hertz",
                    "scalar":    1e8,
                    "writeable": false
                },
                "MAX_RATIO_LIMIT_2": {
                    "begin_bit": 16,
                    "end_bit":   23,
                    "function":  "scale",
                    "units":     "hertz",
                    "scalar":    1e8,
                    "writeable": false
                },
                "MAX_RATIO_LIMIT_3": {
                    "begin_bit": 24,
                    "end_bit":   31,
                    "function":  "scale",
                    "units":     "hertz",
                    "scalar":    1e8,
                    "writeable": false
                },
                "MAX_RATIO_LIMIT_4": {
                    "begin_bit": 32,
                    "end_bit":   39,
                    "function":  "scale",
                    "units":     "hertz",
                    "scalar":    1e8,
                    "writeable": false
                },
                "MAX_RATIO_LIMIT_5": {
                    "begin_bit": 40,
                    "end_bit":   47,
                    "function":  "scale",
                    "units":     "hertz",
                    "scalar":    1e8,
                    "writeable": false
                },
                "MAX_RATIO_LIMIT_6": {
                    "begin_bit": 48,
                    "end_bit":   55,
                    "function":  "scale",
                    "units":     "hertz",
                    "scalar":    1e8,
                    "writeable": false
                },
                "MAX_RATIO_LIMIT_7": {
                    "begin_bit": 56,
                    "end_bit":   63,
                    "function":  "scale",
                    "units":     "hertz",
                    "scalar":    1e8,
                    "writeable": false
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
                    "writeable": false
                },
                "NUMCORE_1": {
                    "begin_bit": 8,
                    "end_bit":   15,
                    "function":  "scale",
                    "units":     "none",
                    "scalar":    1,
                    "writeable": false
                },
                "NUMCORE_2": {
                    "begin_bit": 16,
                    "end_bit":   23,
                    "function":  "scale",
                    "units":     "none",
                    "scalar":    1e8,
                    "writeable": false
                },
                "NUMCORE_3": {
                    "begin_bit": 24,
                    "end_bit":   31,
                    "function":  "scale",
                    "units":     "none",
                    "scalar":    1,
                    "writeable": false
                },
                "NUMCORE_4": {
                    "begin_bit": 32,
                    "end_bit":   39,
                    "function":  "scale",
                    "units":     "none",
                    "scalar":    1,
                    "writeable": false
                },
                "NUMCORE_5": {
                    "begin_bit": 40,
                    "end_bit":   47,
                    "function":  "scale",
                    "units":     "none",
                    "scalar":    1,
                    "writeable": false
                },
                "NUMCORE_6": {
                    "begin_bit": 48,
                    "end_bit":   55,
                    "function":  "scale",
                    "units":     "none",
                    "scalar":    1,
                    "writeable": false
                },
                "NUMCORE_7": {
                    "begin_bit": 56,
                    "end_bit":   63,
                    "function":  "scale",
                    "units":     "none",
                    "scalar":    1,
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
                    "writeable": false
                },
                "ENERGY": {
                    "begin_bit": 8,
                    "end_bit":   12,
                    "function":  "log_half",
                    "units":     "joules",
                    "scalar":    1.0,
                    "writeable": false
                },
                "TIME": {
                    "begin_bit": 16,
                    "end_bit":   19,
                    "function":  "log_half",
                    "units":     "seconds",
                    "scalar":    1.0,
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
                    "writeable": true
                },
                "PL1_LIMIT_ENABLE": {
                    "begin_bit": 15,
                    "end_bit":   15,
                    "function":  "scale",
                    "units":     "none",
                    "scalar":    1.0,
                    "writeable": true
                },
                "PL1_CLAMP_ENABLE": {
                    "begin_bit": 16,
                    "end_bit":   16,
                    "function":  "scale",
                    "units":     "none",
                    "scalar":    1.0,
                    "writeable": true
                },
                "PL1_TIME_WINDOW": {
                    "begin_bit": 17,
                    "end_bit":   23,
                    "function":  "7_bit_float",
                    "units":     "seconds",
                    "scalar":    9.765625e-04,
                    "writeable": true
                },
                "PL2_POWER_LIMIT": {
                    "begin_bit": 32,
                    "end_bit":   46,
                    "function":  "scale",
                    "units":     "watts",
                    "scalar":    1.25e-1,
                    "writeable": true
                },
                "PL2_LIMIT_ENABLE": {
                    "begin_bit": 47,
                    "end_bit":   47,
                    "function":  "scale",
                    "units":     "none",
                    "scalar":    1.0,
                    "writeable": true
                },
                "PL2_CLAMP_ENABLE": {
                    "begin_bit": 48,
                    "end_bit":   48,
                    "function":  "scale",
                    "units":     "none",
                    "scalar":    1.0,
                    "writeable": true
                },
                "PL2_TIME_WINDOW": {
                    "begin_bit": 49,
                    "end_bit":   55,
                    "function":  "7_bit_float",
                    "units":     "seconds",
                    "scalar":    9.765625e-04,
                    "writeable": true
                },
                "LOCK": {
                    "begin_bit": 63,
                    "end_bit":   63,
                    "function":  "scale",
                    "units":     "none",
                    "scalar":    1.0,
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
                    "writeable": false
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
                    "writeable": false
                },
                "MIN_POWER": {
                    "begin_bit": 16,
                    "end_bit":   30,
                    "function":  "scale",
                    "units":     "watts",
                    "scalar":    1.25e-1,
                    "writeable": false
                },
                "MAX_POWER": {
                    "begin_bit": 32,
                    "end_bit":   46,
                    "function":  "scale",
                    "units":     "watts",
                    "scalar":    1.25e-1,
                    "writeable": false
                },
                "MAX_TIME_WINDOW": {
                    "begin_bit": 48,
                    "end_bit":   54,
                    "function":  "7_bit_float",
                    "units":     "seconds",
                    "scalar":    9.765625e-04,
                    "writeable": false
                }
            }
        },
        "DRAM_POWER_LIMIT": {
            "offset": "0x618",
            "domain": "board_memory",
            "fields": {
                "POWER_LIMIT": {
                    "begin_bit": 0,
                    "end_bit":   14,
                    "function":  "scale",
                    "units":     "watts",
                    "scalar":    1.25e-1,
                    "writeable": true
                },
                "ENABLE": {
                    "begin_bit": 15,
                    "end_bit":   15,
                    "function":  "scale",
                    "units":     "none",
                    "scalar":    1.0,
                    "writeable": true
                },
                "TIME_WINDOW": {
                    "begin_bit": 17,
                    "end_bit":   23,
                    "function":  "7_bit_float",
                    "units":     "seconds",
                    "scalar":    9.765625e-04,
                    "writeable": true
                },
                "LOCK": {
                    "begin_bit": 31,
                    "end_bit":   31,
                    "function":  "scale",
                    "units":     "none",
                    "scalar":    1,
                    "writeable": false
                }
            }
        },
        "DRAM_ENERGY_STATUS": {
            "offset": "0x619",
            "domain": "board_memory",
            "fields": {
                "ENERGY": {
                    "begin_bit": 0,
                    "end_bit":   31,
                    "function":  "overflow",
                    "units":     "joules",
                    "scalar":    1.5258789063e-05,
                    "writeable": false
                }
            }
        },
        "DRAM_PERF_STATUS": {
            "offset": "0x61B",
            "domain": "board_memory",
            "fields": {
                "THROTTLE_TIME": {
                    "begin_bit": 0,
                    "end_bit":   31,
                    "function":  "scale",
                    "units":     "seconds",
                    "scalar":    9.765625e-04,
                    "writeable": false
                }
            }
        },
        "DRAM_POWER_INFO": {
            "offset": "0x61C",
            "domain": "board_memory",
            "fields": {
                "THERMAL_SPEC_POWER": {
                    "begin_bit": 0,
                    "end_bit":   14,
                    "function":  "scale",
                    "units":     "watts",
                    "scalar":    1.25e-1,
                    "writeable": false
                },
                "MIN_POWER": {
                    "begin_bit": 16,
                    "end_bit":   30,
                    "function":  "scale",
                    "units":     "watts",
                    "scalar":    1.25e-1,
                    "writeable": false
                },
                "MAX_POWER": {
                    "begin_bit": 32,
                    "end_bit":   46,
                    "function":  "scale",
                    "units":     "watts",
                    "scalar":    1.25e-1,
                    "writeable": false
                },
                "MAX_TIME_WINDOW": {
                    "begin_bit": 48,
                    "end_bit":   54,
                    "function":  "7_bit_float",
                    "units":     "seconds",
                    "scalar":    9.765625e-04,
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
                    "writeable": true
                },
                "MAX_RATIO": {
                    "begin_bit": 0,
                    "end_bit":   6,
                    "function":  "scale",
                    "units":     "hertz",
                    "scalar":    1e8,
                    "writeable": true
                }
            }
        }
    }
}
)";
        return result;
    }
}
