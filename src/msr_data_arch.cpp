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
    const std::string arch_msr_json(void)
    {
        static const std::string result = R"(
{
    "msrs": {
        "TIME_STAMP_COUNTER": {
            "offset": "0x10",
            "domain": "cpu",
            "fields": {
                "TIMESTAMP_COUNT": {
                    "begin_bit": 0,
                    "end_bit":   47,
                    "function":  "overflow",
                    "units":     "none",
                    "scalar":    1.0,
                    "writeable": false
                }
            }
        },
        "MPERF": {
            "offset": "0xE7",
            "domain": "cpu",
            "fields": {
                "MCNT": {
                    "begin_bit": 0,
                    "end_bit":   47,
                    "function":  "overflow",
                    "units":     "none",
                    "scalar":    1.0,
                    "writeable": false
                }
            }
        },
        "APERF": {
            "offset": "0xE8",
            "domain": "cpu",
            "fields": {
                "ACNT": {
                    "begin_bit": 0,
                    "end_bit":   47,
                    "function":  "overflow",
                    "units":     "none",
                    "scalar":    1.0,
                    "writeable": false
                }
            }
        },
        "THERM_STATUS": {
            "offset": "0x19C",
            "domain": "core",
            "fields": {
                "THERMAL_STATUS_FLAG": {
                    "begin_bit": 0,
                    "end_bit":   0,
                    "function":  "scale",
                    "units":     "none",
                    "scalar":    1.0,
                    "writeable": false
                },
                "THERMAL_STATUS_LOG": {
                    "begin_bit": 1,
                    "end_bit":   1,
                    "function":  "scale",
                    "units":     "none",
                    "scalar":    1.0,
                    "writeable": true
                },
                "PROCHOT_EVENT": {
                    "begin_bit": 2,
                    "end_bit":   2,
                    "function":  "scale",
                    "units":     "none",
                    "scalar":    1.0,
                    "writeable": false
                },
                "PROCHOT_LOG": {
                    "begin_bit": 3,
                    "end_bit":   3,
                    "function":  "scale",
                    "units":     "none",
                    "scalar":    1.0,
                    "writeable": true
                },
                "CRITICAL_TEMP_STATUS": {
                    "begin_bit": 4,
                    "end_bit":   4,
                    "function":  "scale",
                    "units":     "none",
                    "scalar":    1.0,
                    "writeable": false
                },
                "CRITICAL_TEMP_LOG": {
                    "begin_bit": 5,
                    "end_bit":   5,
                    "function":  "scale",
                    "units":     "none",
                    "scalar":    1.0,
                    "writeable": true
                },
                "THERMAL_THRESH_1_STATUS": {
                    "begin_bit": 6,
                    "end_bit":   6,
                    "function":  "scale",
                    "units":     "none",
                    "scalar":    1.0,
                    "writeable": false
                },
                "THERMAL_THRESH_1_LOG": {
                    "begin_bit": 7,
                    "end_bit":   7,
                    "function":  "scale",
                    "units":     "none",
                    "scalar":    1.0,
                    "writeable": true
                },
                "THERMAL_THRESH_2_STATUS": {
                    "begin_bit": 8,
                    "end_bit":   8,
                    "function":  "scale",
                    "units":     "none",
                    "scalar":    1.0,
                    "writeable": false
                },
                "THERMAL_THRESH_2_LOG": {
                    "begin_bit": 9,
                    "end_bit":   9,
                    "function":  "scale",
                    "units":     "none",
                    "scalar":    1.0,
                    "writeable": true
                },
                "POWER_LIMIT_STATUS": {
                    "begin_bit": 10,
                    "end_bit":   10,
                    "function":  "scale",
                    "units":     "none",
                    "scalar":    1.0,
                    "writeable": false
                },
                "POWER_NOTIFICATION_LOG": {
                    "begin_bit": 11,
                    "end_bit":   11,
                    "function":  "scale",
                    "units":     "none",
                    "scalar":    1.0,
                    "writeable": true
                },
                "DIGITAL_READOUT": {
                    "begin_bit": 16,
                    "end_bit":   22,
                    "function":  "scale",
                    "units":     "celsius",
                    "scalar":    1.0,
                    "writeable": false
                },
                "RESOLUTION": {
                    "begin_bit": 27,
                    "end_bit":   30,
                    "function":  "scale",
                    "units":     "celsius",
                    "scalar":    1.0,
                    "writeable": false
                },
                "READING_VALID": {
                    "begin_bit": 31,
                    "end_bit":   31,
                    "function":  "scale",
                    "units":     "none",
                    "scalar":    1.0,
                    "writeable": false
                }
            }
        },
        "MISC_ENABLE": {
            "offset": "0x1A0",
            "domain": "package",
            "fields": {
                "ENHANCED_SPEEDSTEP_TECH_ENABLE": {
                    "begin_bit": 16,
                    "end_bit":   16,
                    "function":  "scale",
                    "units":     "none",
                    "scalar":    1.0,
                    "writeable": false
                },
                "TURBO_MODE_DISABLE": {
                    "begin_bit": 38,
                    "end_bit":   38,
                    "function":  "scale",
                    "units":     "none",
                    "scalar":    1.0,
                    "writeable": false
                }
            }
        },
        "PACKAGE_THERM_STATUS": {
            "offset": "0x1B1",
            "domain": "package",
            "fields": {
                "THERMAL_STATUS_FLAG": {
                    "begin_bit": 0,
                    "end_bit":   0,
                    "function":  "scale",
                    "units":     "none",
                    "scalar":    1.0,
                    "writeable": false
                },
                "THERMAL_STATUS_LOG": {
                    "begin_bit": 1,
                    "end_bit":   1,
                    "function":  "scale",
                    "units":     "none",
                    "scalar":    1.0,
                    "writeable": true
                },
                "PROCHOT_EVENT": {
                    "begin_bit": 2,
                    "end_bit":   2,
                    "function":  "scale",
                    "units":     "none",
                    "scalar":    1.0,
                    "writeable": false
                },
                "PROCHOT_LOG": {
                    "begin_bit": 3,
                    "end_bit":   3,
                    "function":  "scale",
                    "units":     "none",
                    "scalar":    1.0,
                    "writeable": true
                },
                "CRITICAL_TEMP_STATUS": {
                    "begin_bit": 4,
                    "end_bit":   4,
                    "function":  "scale",
                    "units":     "none",
                    "scalar":    1.0,
                    "writeable": false
                },
                "CRITICAL_TEMP_LOG": {
                    "begin_bit": 5,
                    "end_bit":   5,
                    "function":  "scale",
                    "units":     "none",
                    "scalar":    1.0,
                    "writeable": true
                },
                "THERMAL_THRESH_1_STATUS": {
                    "begin_bit": 6,
                    "end_bit":   6,
                    "function":  "scale",
                    "units":     "none",
                    "scalar":    1.0,
                    "writeable": false
                },
                "THERMAL_THRESH_1_LOG": {
                    "begin_bit": 7,
                    "end_bit":   7,
                    "function":  "scale",
                    "units":     "none",
                    "scalar":    1.0,
                    "writeable": true
                },
                "THERMAL_THRESH_2_STATUS": {
                    "begin_bit": 8,
                    "end_bit":   8,
                    "function":  "scale",
                    "units":     "none",
                    "scalar":    1.0,
                    "writeable": false
                },
                "THERMAL_THRESH_2_LOG": {
                    "begin_bit": 9,
                    "end_bit":   9,
                    "function":  "scale",
                    "units":     "none",
                    "scalar":    1.0,
                    "writeable": true
                },
                "POWER_LIMIT_STATUS": {
                    "begin_bit": 10,
                    "end_bit":   10,
                    "function":  "scale",
                    "units":     "none",
                    "scalar":    1.0,
                    "writeable": false
                },
                "POWER_NOTIFICATION_LOG": {
                    "begin_bit": 11,
                    "end_bit":   11,
                    "function":  "scale",
                    "units":     "none",
                    "scalar":    1.0,
                    "writeable": true
                },
                "DIGITAL_READOUT": {
                    "begin_bit": 16,
                    "end_bit":   22,
                    "function":  "scale",
                    "units":     "celsius",
                    "scalar":    1.0,
                    "writeable": false
                }
            }
        },
        "FIXED_CTR0": {
            "offset": "0x309",
            "domain": "cpu",
            "fields": {
                "INST_RETIRED_ANY": {
                    "begin_bit": 0,
                    "end_bit":   39,
                    "function":  "overflow",
                    "units":     "none",
                    "scalar":    1.0,
                    "writeable": false
                }
            }
        },
        "FIXED_CTR1": {
            "offset": "0x30A",
            "domain": "cpu",
            "fields": {
                "CPU_CLK_UNHALTED_THREAD": {
                    "begin_bit": 0,
                    "end_bit":   39,
                    "function":  "overflow",
                    "units":     "none",
                    "scalar":    1.0,
                    "writeable": false
                }
            }
        },
        "FIXED_CTR2": {
            "offset": "0x30B",
            "domain": "cpu",
            "fields": {
                "CPU_CLK_UNHALTED_REF_TSC": {
                    "begin_bit": 0,
                    "end_bit":   39,
                    "function":  "overflow",
                    "units":     "none",
                    "scalar":    1.0,
                    "writeable": false
                }
            }
        },
        "FIXED_CTR_CTRL": {
            "offset": "0x38D",
            "domain": "cpu",
            "fields": {
                "EN0_OS": {
                    "begin_bit": 0,
                    "end_bit":   0,
                    "function":  "scale",
                    "units":     "none",
                    "scalar":    1.0,
                    "writeable": true
                },
                "EN0_USR": {
                    "begin_bit": 1,
                    "end_bit":   1,
                    "function":  "scale",
                    "units":     "none",
                    "scalar":    1.0,
                    "writeable": true
                },
                "EN0_PMI": {
                    "begin_bit": 3,
                    "end_bit":   3,
                    "function":  "scale",
                    "units":     "none",
                    "scalar":    1.0,
                    "writeable": true
                },
                "EN1_OS": {
                    "begin_bit": 4,
                    "end_bit":   4,
                    "function":  "scale",
                    "units":     "none",
                    "scalar":    1.0,
                    "writeable": true
                },
                "EN1_USR": {
                    "begin_bit": 5,
                    "end_bit":   5,
                    "function":  "scale",
                    "units":     "none",
                    "scalar":    1.0,
                    "writeable": true
                },
                "EN1_PMI": {
                    "begin_bit": 7,
                    "end_bit":   7,
                    "function":  "scale",
                    "units":     "none",
                    "scalar":    1.0,
                    "writeable": true
                },
                "EN2_OS": {
                    "begin_bit": 8,
                    "end_bit":   8,
                    "function":  "scale",
                    "units":     "none",
                    "scalar":    1.0,
                    "writeable": true
                },
                "EN2_USR": {
                    "begin_bit": 9,
                    "end_bit":   9,
                    "function":  "scale",
                    "units":     "none",
                    "scalar":    1.0,
                    "writeable": true
                },
                "EN2_PMI": {
                    "begin_bit": 11,
                    "end_bit":   11,
                    "function":  "scale",
                    "units":     "none",
                    "scalar":    1.0,
                    "writeable": true
                }
            }
        },
        "PERF_GLOBAL_CTRL": {
            "offset": "0x38F",
            "domain": "cpu",
            "fields": {
                "EN_PMC0": {
                    "begin_bit": 0,
                    "end_bit":   0,
                    "function":  "scale",
                    "units":     "none",
                    "scalar":    1.0,
                    "writeable": true
                },
                "EN_PMC1": {
                    "begin_bit": 1,
                    "end_bit":   1,
                    "function":  "scale",
                    "units":     "none",
                    "scalar":    1.0,
                    "writeable": true
                },
                "EN_FIXED_CTR0": {
                    "begin_bit": 32,
                    "end_bit":   32,
                    "function":  "scale",
                    "units":     "none",
                    "scalar":    1.0,
                    "writeable": true
                },
                "EN_FIXED_CTR1": {
                    "begin_bit": 33,
                    "end_bit":   33,
                    "function":  "scale",
                    "units":     "none",
                    "scalar":    1.0,
                    "writeable": true
                },
                "EN_FIXED_CTR2": {
                    "begin_bit": 34,
                    "end_bit":   34,
                    "function":  "scale",
                    "units":     "none",
                    "scalar":    1.0,
                    "writeable": true
                }
            }
        },
        "PERF_GLOBAL_OVF_CTRL": {
            "offset": "0x390",
            "domain": "cpu",
            "fields": {
                "CLEAR_OVF_PMC0": {
                    "begin_bit": 0,
                    "end_bit":   0,
                    "function":  "scale",
                    "units":     "none",
                    "scalar":    1.0,
                    "writeable": true
                },
                "CLEAR_OVF_PMC1": {
                    "begin_bit": 1,
                    "end_bit":   1,
                    "function":  "scale",
                    "units":     "none",
                    "scalar":    1.0,
                    "writeable": true
                },
                "CLEAR_OVF_FIXED_CTR0": {
                    "begin_bit": 32,
                    "end_bit":   32,
                    "function":  "scale",
                    "units":     "none",
                    "scalar":    1.0,
                    "writeable": true
                },
                "CLEAR_OVF_FIXED_CTR1": {
                    "begin_bit": 33,
                    "end_bit":   33,
                    "function":  "scale",
                    "units":     "none",
                    "scalar":    1.0,
                    "writeable": true
                },
                "CLEAR_OVF_FIXED_CTR2": {
                    "begin_bit": 34,
                    "end_bit":   34,
                    "function":  "scale",
                    "units":     "none",
                    "scalar":    1.0,
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
