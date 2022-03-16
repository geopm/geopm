SKX Platform Controls
=====================

CPU_FREQUENCY_CONTROL
    - description: Target operating frequency of the CPU based on the control register.
    - alias_for: MSR::PERF_CTL:FREQ
    - units: hertz
    - domain: core
    - iogroup: MSRIOGroup
MSR::DRAM_POWER_LIMIT:ENABLE
    - description: Refer to the Intel(R) 64 and IA-32 Architectures Software Developer's Manual for information about this MSR
    - units: none
    - domain: board_memory
    - iogroup: MSRIOGroup
MSR::DRAM_POWER_LIMIT:POWER_LIMIT
    - description: Refer to the Intel(R) 64 and IA-32 Architectures Software Developer's Manual for information about this MSR
    - units: watts
    - domain: board_memory
    - iogroup: MSRIOGroup
MSR::DRAM_POWER_LIMIT:TIME_WINDOW
    - description: Refer to the Intel(R) 64 and IA-32 Architectures Software Developer's Manual for information about this MSR
    - units: seconds
    - domain: board_memory
    - iogroup: MSRIOGroup
MSR::FIXED_CTR_CTRL:EN0_OS
    - description: Refer to the Intel(R) 64 and IA-32 Architectures Software Developer's Manual for information about this MSR
    - units: none
    - domain: cpu
    - iogroup: MSRIOGroup
MSR::FIXED_CTR_CTRL:EN0_PMI
    - description: Refer to the Intel(R) 64 and IA-32 Architectures Software Developer's Manual for information about this MSR
    - units: none
    - domain: cpu
    - iogroup: MSRIOGroup
MSR::FIXED_CTR_CTRL:EN0_USR
    - description: Refer to the Intel(R) 64 and IA-32 Architectures Software Developer's Manual for information about this MSR
    - units: none
    - domain: cpu
    - iogroup: MSRIOGroup
MSR::FIXED_CTR_CTRL:EN1_OS
    - description: Refer to the Intel(R) 64 and IA-32 Architectures Software Developer's Manual for information about this MSR
    - units: none
    - domain: cpu
    - iogroup: MSRIOGroup
MSR::FIXED_CTR_CTRL:EN1_PMI
    - description: Refer to the Intel(R) 64 and IA-32 Architectures Software Developer's Manual for information about this MSR
    - units: none
    - domain: cpu
    - iogroup: MSRIOGroup
MSR::FIXED_CTR_CTRL:EN1_USR
    - description: Refer to the Intel(R) 64 and IA-32 Architectures Software Developer's Manual for information about this MSR
    - units: none
    - domain: cpu
    - iogroup: MSRIOGroup
MSR::FIXED_CTR_CTRL:EN2_OS
    - description: Refer to the Intel(R) 64 and IA-32 Architectures Software Developer's Manual for information about this MSR
    - units: none
    - domain: cpu
    - iogroup: MSRIOGroup
MSR::FIXED_CTR_CTRL:EN2_PMI
    - description: Refer to the Intel(R) 64 and IA-32 Architectures Software Developer's Manual for information about this MSR
    - units: none
    - domain: cpu
    - iogroup: MSRIOGroup
MSR::FIXED_CTR_CTRL:EN2_USR
    - description: Refer to the Intel(R) 64 and IA-32 Architectures Software Developer's Manual for information about this MSR
    - units: none
    - domain: cpu
    - iogroup: MSRIOGroup
MSR::IA32_PERFEVTSEL0:ANYTHREAD
    - description: Refer to the Intel(R) 64 and IA-32 Architectures Software Developer's Manual for information about this MSR
    - units: none
    - domain: cpu
    - iogroup: MSRIOGroup
MSR::IA32_PERFEVTSEL0:CMASK
    - description: Refer to the Intel(R) 64 and IA-32 Architectures Software Developer's Manual for information about this MSR
    - units: none
    - domain: cpu
    - iogroup: MSRIOGroup
MSR::IA32_PERFEVTSEL0:EDGE
    - description: Refer to the Intel(R) 64 and IA-32 Architectures Software Developer's Manual for information about this MSR
    - units: none
    - domain: cpu
    - iogroup: MSRIOGroup
MSR::IA32_PERFEVTSEL0:EN
    - description: Refer to the Intel(R) 64 and IA-32 Architectures Software Developer's Manual for information about this MSR
    - units: none
    - domain: cpu
    - iogroup: MSRIOGroup
MSR::IA32_PERFEVTSEL0:EVENT_SELECT
    - description: Refer to the Intel(R) 64 and IA-32 Architectures Software Developer's Manual for information about this MSR
    - units: none
    - domain: cpu
    - iogroup: MSRIOGroup
MSR::IA32_PERFEVTSEL0:INT
    - description: Refer to the Intel(R) 64 and IA-32 Architectures Software Developer's Manual for information about this MSR
    - units: none
    - domain: cpu
    - iogroup: MSRIOGroup
MSR::IA32_PERFEVTSEL0:INV
    - description: Refer to the Intel(R) 64 and IA-32 Architectures Software Developer's Manual for information about this MSR
    - units: none
    - domain: cpu
    - iogroup: MSRIOGroup
MSR::IA32_PERFEVTSEL0:OS
    - description: Refer to the Intel(R) 64 and IA-32 Architectures Software Developer's Manual for information about this MSR
    - units: none
    - domain: cpu
    - iogroup: MSRIOGroup
MSR::IA32_PERFEVTSEL0:PC
    - description: Refer to the Intel(R) 64 and IA-32 Architectures Software Developer's Manual for information about this MSR
    - units: none
    - domain: cpu
    - iogroup: MSRIOGroup
MSR::IA32_PERFEVTSEL0:UMASK
    - description: Refer to the Intel(R) 64 and IA-32 Architectures Software Developer's Manual for information about this MSR
    - units: none
    - domain: cpu
    - iogroup: MSRIOGroup
MSR::IA32_PERFEVTSEL0:USR
    - description: Refer to the Intel(R) 64 and IA-32 Architectures Software Developer's Manual for information about this MSR
    - units: none
    - domain: cpu
    - iogroup: MSRIOGroup
MSR::IA32_PERFEVTSEL1:ANYTHREAD
    - description: Refer to the Intel(R) 64 and IA-32 Architectures Software Developer's Manual for information about this MSR
    - units: none
    - domain: cpu
    - iogroup: MSRIOGroup
MSR::IA32_PERFEVTSEL1:CMASK
    - description: Refer to the Intel(R) 64 and IA-32 Architectures Software Developer's Manual for information about this MSR
    - units: none
    - domain: cpu
    - iogroup: MSRIOGroup
MSR::IA32_PERFEVTSEL1:EDGE
    - description: Refer to the Intel(R) 64 and IA-32 Architectures Software Developer's Manual for information about this MSR
    - units: none
    - domain: cpu
    - iogroup: MSRIOGroup
MSR::IA32_PERFEVTSEL1:EN
    - description: Refer to the Intel(R) 64 and IA-32 Architectures Software Developer's Manual for information about this MSR
    - units: none
    - domain: cpu
    - iogroup: MSRIOGroup
MSR::IA32_PERFEVTSEL1:EVENT_SELECT
    - description: Refer to the Intel(R) 64 and IA-32 Architectures Software Developer's Manual for information about this MSR
    - units: none
    - domain: cpu
    - iogroup: MSRIOGroup
MSR::IA32_PERFEVTSEL1:INT
    - description: Refer to the Intel(R) 64 and IA-32 Architectures Software Developer's Manual for information about this MSR
    - units: none
    - domain: cpu
    - iogroup: MSRIOGroup
MSR::IA32_PERFEVTSEL1:INV
    - description: Refer to the Intel(R) 64 and IA-32 Architectures Software Developer's Manual for information about this MSR
    - units: none
    - domain: cpu
    - iogroup: MSRIOGroup
MSR::IA32_PERFEVTSEL1:OS
    - description: Refer to the Intel(R) 64 and IA-32 Architectures Software Developer's Manual for information about this MSR
    - units: none
    - domain: cpu
    - iogroup: MSRIOGroup
MSR::IA32_PERFEVTSEL1:PC
    - description: Refer to the Intel(R) 64 and IA-32 Architectures Software Developer's Manual for information about this MSR
    - units: none
    - domain: cpu
    - iogroup: MSRIOGroup
MSR::IA32_PERFEVTSEL1:UMASK
    - description: Refer to the Intel(R) 64 and IA-32 Architectures Software Developer's Manual for information about this MSR
    - units: none
    - domain: cpu
    - iogroup: MSRIOGroup
MSR::IA32_PERFEVTSEL1:USR
    - description: Refer to the Intel(R) 64 and IA-32 Architectures Software Developer's Manual for information about this MSR
    - units: none
    - domain: cpu
    - iogroup: MSRIOGroup
MSR::IA32_PERFEVTSEL2:ANYTHREAD
    - description: Refer to the Intel(R) 64 and IA-32 Architectures Software Developer's Manual for information about this MSR
    - units: none
    - domain: cpu
    - iogroup: MSRIOGroup
MSR::IA32_PERFEVTSEL2:CMASK
    - description: Refer to the Intel(R) 64 and IA-32 Architectures Software Developer's Manual for information about this MSR
    - units: none
    - domain: cpu
    - iogroup: MSRIOGroup
MSR::IA32_PERFEVTSEL2:EDGE
    - description: Refer to the Intel(R) 64 and IA-32 Architectures Software Developer's Manual for information about this MSR
    - units: none
    - domain: cpu
    - iogroup: MSRIOGroup
MSR::IA32_PERFEVTSEL2:EN
    - description: Refer to the Intel(R) 64 and IA-32 Architectures Software Developer's Manual for information about this MSR
    - units: none
    - domain: cpu
    - iogroup: MSRIOGroup
MSR::IA32_PERFEVTSEL2:EVENT_SELECT
    - description: Refer to the Intel(R) 64 and IA-32 Architectures Software Developer's Manual for information about this MSR
    - units: none
    - domain: cpu
    - iogroup: MSRIOGroup
MSR::IA32_PERFEVTSEL2:INT
    - description: Refer to the Intel(R) 64 and IA-32 Architectures Software Developer's Manual for information about this MSR
    - units: none
    - domain: cpu
    - iogroup: MSRIOGroup
MSR::IA32_PERFEVTSEL2:INV
    - description: Refer to the Intel(R) 64 and IA-32 Architectures Software Developer's Manual for information about this MSR
    - units: none
    - domain: cpu
    - iogroup: MSRIOGroup
MSR::IA32_PERFEVTSEL2:OS
    - description: Refer to the Intel(R) 64 and IA-32 Architectures Software Developer's Manual for information about this MSR
    - units: none
    - domain: cpu
    - iogroup: MSRIOGroup
MSR::IA32_PERFEVTSEL2:PC
    - description: Refer to the Intel(R) 64 and IA-32 Architectures Software Developer's Manual for information about this MSR
    - units: none
    - domain: cpu
    - iogroup: MSRIOGroup
MSR::IA32_PERFEVTSEL2:UMASK
    - description: Refer to the Intel(R) 64 and IA-32 Architectures Software Developer's Manual for information about this MSR
    - units: none
    - domain: cpu
    - iogroup: MSRIOGroup
MSR::IA32_PERFEVTSEL2:USR
    - description: Refer to the Intel(R) 64 and IA-32 Architectures Software Developer's Manual for information about this MSR
    - units: none
    - domain: cpu
    - iogroup: MSRIOGroup
MSR::IA32_PERFEVTSEL3:ANYTHREAD
    - description: Refer to the Intel(R) 64 and IA-32 Architectures Software Developer's Manual for information about this MSR
    - units: none
    - domain: cpu
    - iogroup: MSRIOGroup
MSR::IA32_PERFEVTSEL3:CMASK
    - description: Refer to the Intel(R) 64 and IA-32 Architectures Software Developer's Manual for information about this MSR
    - units: none
    - domain: cpu
    - iogroup: MSRIOGroup
MSR::IA32_PERFEVTSEL3:EDGE
    - description: Refer to the Intel(R) 64 and IA-32 Architectures Software Developer's Manual for information about this MSR
    - units: none
    - domain: cpu
    - iogroup: MSRIOGroup
MSR::IA32_PERFEVTSEL3:EN
    - description: Refer to the Intel(R) 64 and IA-32 Architectures Software Developer's Manual for information about this MSR
    - units: none
    - domain: cpu
    - iogroup: MSRIOGroup
MSR::IA32_PERFEVTSEL3:EVENT_SELECT
    - description: Refer to the Intel(R) 64 and IA-32 Architectures Software Developer's Manual for information about this MSR
    - units: none
    - domain: cpu
    - iogroup: MSRIOGroup
MSR::IA32_PERFEVTSEL3:INT
    - description: Refer to the Intel(R) 64 and IA-32 Architectures Software Developer's Manual for information about this MSR
    - units: none
    - domain: cpu
    - iogroup: MSRIOGroup
MSR::IA32_PERFEVTSEL3:INV
    - description: Refer to the Intel(R) 64 and IA-32 Architectures Software Developer's Manual for information about this MSR
    - units: none
    - domain: cpu
    - iogroup: MSRIOGroup
MSR::IA32_PERFEVTSEL3:OS
    - description: Refer to the Intel(R) 64 and IA-32 Architectures Software Developer's Manual for information about this MSR
    - units: none
    - domain: cpu
    - iogroup: MSRIOGroup
MSR::IA32_PERFEVTSEL3:PC
    - description: Refer to the Intel(R) 64 and IA-32 Architectures Software Developer's Manual for information about this MSR
    - units: none
    - domain: cpu
    - iogroup: MSRIOGroup
MSR::IA32_PERFEVTSEL3:UMASK
    - description: Refer to the Intel(R) 64 and IA-32 Architectures Software Developer's Manual for information about this MSR
    - units: none
    - domain: cpu
    - iogroup: MSRIOGroup
MSR::IA32_PERFEVTSEL3:USR
    - description: Refer to the Intel(R) 64 and IA-32 Architectures Software Developer's Manual for information about this MSR
    - units: none
    - domain: cpu
    - iogroup: MSRIOGroup
MSR::PACKAGE_THERM_STATUS:CRITICAL_TEMP_LOG
    - description: Refer to the Intel(R) 64 and IA-32 Architectures Software Developer's Manual for information about this MSR
    - units: none
    - domain: package
    - iogroup: MSRIOGroup
MSR::PACKAGE_THERM_STATUS:POWER_NOTIFICATION_LOG
    - description: Refer to the Intel(R) 64 and IA-32 Architectures Software Developer's Manual for information about this MSR
    - units: none
    - domain: package
    - iogroup: MSRIOGroup
MSR::PACKAGE_THERM_STATUS:PROCHOT_LOG
    - description: Refer to the Intel(R) 64 and IA-32 Architectures Software Developer's Manual for information about this MSR
    - units: none
    - domain: package
    - iogroup: MSRIOGroup
MSR::PACKAGE_THERM_STATUS:THERMAL_STATUS_LOG
    - description: Refer to the Intel(R) 64 and IA-32 Architectures Software Developer's Manual for information about this MSR
    - units: none
    - domain: package
    - iogroup: MSRIOGroup
MSR::PACKAGE_THERM_STATUS:THERMAL_THRESH_1_LOG
    - description: Refer to the Intel(R) 64 and IA-32 Architectures Software Developer's Manual for information about this MSR
    - units: none
    - domain: package
    - iogroup: MSRIOGroup
MSR::PACKAGE_THERM_STATUS:THERMAL_THRESH_2_LOG
    - description: Refer to the Intel(R) 64 and IA-32 Architectures Software Developer's Manual for information about this MSR
    - units: none
    - domain: package
    - iogroup: MSRIOGroup
MSR::PERF_CTL:FREQ
    - description: Target operating frequency of the CPU based on the control register.
    - units: hertz
    - domain: core
    - iogroup: MSRIOGroup
MSR::PERF_GLOBAL_CTRL:EN_FIXED_CTR0
    - description: Refer to the Intel(R) 64 and IA-32 Architectures Software Developer's Manual for information about this MSR
    - units: none
    - domain: cpu
    - iogroup: MSRIOGroup
MSR::PERF_GLOBAL_CTRL:EN_FIXED_CTR1
    - description: Refer to the Intel(R) 64 and IA-32 Architectures Software Developer's Manual for information about this MSR
    - units: none
    - domain: cpu
    - iogroup: MSRIOGroup
MSR::PERF_GLOBAL_CTRL:EN_FIXED_CTR2
    - description: Refer to the Intel(R) 64 and IA-32 Architectures Software Developer's Manual for information about this MSR
    - units: none
    - domain: cpu
    - iogroup: MSRIOGroup
MSR::PERF_GLOBAL_CTRL:EN_PMC0
    - description: Refer to the Intel(R) 64 and IA-32 Architectures Software Developer's Manual for information about this MSR
    - units: none
    - domain: cpu
    - iogroup: MSRIOGroup
MSR::PERF_GLOBAL_CTRL:EN_PMC1
    - description: Refer to the Intel(R) 64 and IA-32 Architectures Software Developer's Manual for information about this MSR
    - units: none
    - domain: cpu
    - iogroup: MSRIOGroup
MSR::PERF_GLOBAL_CTRL:EN_PMC2
    - description: Refer to the Intel(R) 64 and IA-32 Architectures Software Developer's Manual for information about this MSR
    - units: none
    - domain: cpu
    - iogroup: MSRIOGroup
MSR::PERF_GLOBAL_CTRL:EN_PMC3
    - description: Refer to the Intel(R) 64 and IA-32 Architectures Software Developer's Manual for information about this MSR
    - units: none
    - domain: cpu
    - iogroup: MSRIOGroup
MSR::PERF_GLOBAL_OVF_CTRL:CLEAR_OVF_FIXED_CTR0
    - description: Refer to the Intel(R) 64 and IA-32 Architectures Software Developer's Manual for information about this MSR
    - units: none
    - domain: cpu
    - iogroup: MSRIOGroup
MSR::PERF_GLOBAL_OVF_CTRL:CLEAR_OVF_FIXED_CTR1
    - description: Refer to the Intel(R) 64 and IA-32 Architectures Software Developer's Manual for information about this MSR
    - units: none
    - domain: cpu
    - iogroup: MSRIOGroup
MSR::PERF_GLOBAL_OVF_CTRL:CLEAR_OVF_FIXED_CTR2
    - description: Refer to the Intel(R) 64 and IA-32 Architectures Software Developer's Manual for information about this MSR
    - units: none
    - domain: cpu
    - iogroup: MSRIOGroup
MSR::PERF_GLOBAL_OVF_CTRL:CLEAR_OVF_PMC0
    - description: Refer to the Intel(R) 64 and IA-32 Architectures Software Developer's Manual for information about this MSR
    - units: none
    - domain: cpu
    - iogroup: MSRIOGroup
MSR::PERF_GLOBAL_OVF_CTRL:CLEAR_OVF_PMC1
    - description: Refer to the Intel(R) 64 and IA-32 Architectures Software Developer's Manual for information about this MSR
    - units: none
    - domain: cpu
    - iogroup: MSRIOGroup
MSR::PERF_GLOBAL_OVF_CTRL:CLEAR_OVF_PMC2
    - description: Refer to the Intel(R) 64 and IA-32 Architectures Software Developer's Manual for information about this MSR
    - units: none
    - domain: cpu
    - iogroup: MSRIOGroup
MSR::PERF_GLOBAL_OVF_CTRL:CLEAR_OVF_PMC3
    - description: Refer to the Intel(R) 64 and IA-32 Architectures Software Developer's Manual for information about this MSR
    - units: none
    - domain: cpu
    - iogroup: MSRIOGroup
MSR::PKG_POWER_LIMIT:PL1_CLAMP_ENABLE
    - description: Refer to the Intel(R) 64 and IA-32 Architectures Software Developer's Manual for information about this MSR
    - units: none
    - domain: package
    - iogroup: MSRIOGroup
MSR::PKG_POWER_LIMIT:PL1_LIMIT_ENABLE
    - description: Refer to the Intel(R) 64 and IA-32 Architectures Software Developer's Manual for information about this MSR
    - units: none
    - domain: package
    - iogroup: MSRIOGroup
MSR::PKG_POWER_LIMIT:PL1_POWER_LIMIT
    - description: The average power usage limit over the time window specified in PL1_TIME_WINDOW.
    - units: watts
    - domain: package
    - iogroup: MSRIOGroup
MSR::PKG_POWER_LIMIT:PL1_TIME_WINDOW
    - description: The time window associated with power limit 1.
    - units: seconds
    - domain: package
    - iogroup: MSRIOGroup
MSR::PKG_POWER_LIMIT:PL2_CLAMP_ENABLE
    - description: Refer to the Intel(R) 64 and IA-32 Architectures Software Developer's Manual for information about this MSR
    - units: none
    - domain: package
    - iogroup: MSRIOGroup
MSR::PKG_POWER_LIMIT:PL2_LIMIT_ENABLE
    - description: Refer to the Intel(R) 64 and IA-32 Architectures Software Developer's Manual for information about this MSR
    - units: none
    - domain: package
    - iogroup: MSRIOGroup
MSR::PKG_POWER_LIMIT:PL2_POWER_LIMIT
    - description: Refer to the Intel(R) 64 and IA-32 Architectures Software Developer's Manual for information about this MSR
    - units: watts
    - domain: package
    - iogroup: MSRIOGroup
MSR::PKG_POWER_LIMIT:PL2_TIME_WINDOW
    - description: Refer to the Intel(R) 64 and IA-32 Architectures Software Developer's Manual for information about this MSR
    - units: seconds
    - domain: package
    - iogroup: MSRIOGroup
MSR::PQR_ASSOC:RMID
    - description: Refer to the Intel(R) 64 and IA-32 Architectures Software Developer's Manual for information about this MSR
    - units: none
    - domain: cpu
    - iogroup: MSRIOGroup
MSR::QM_EVTSEL:EVENT_ID
    - description: Refer to the Intel(R) 64 and IA-32 Architectures Software Developer's Manual for information about this MSR
    - units: none
    - domain: package
    - iogroup: MSRIOGroup
MSR::QM_EVTSEL:RMID
    - description: Refer to the Intel(R) 64 and IA-32 Architectures Software Developer's Manual for information about this MSR
    - units: none
    - domain: package
    - iogroup: MSRIOGroup
MSR::THERM_STATUS:CRITICAL_TEMP_LOG
    - description: Refer to the Intel(R) 64 and IA-32 Architectures Software Developer's Manual for information about this MSR
    - units: none
    - domain: core
    - iogroup: MSRIOGroup
MSR::THERM_STATUS:POWER_NOTIFICATION_LOG
    - description: Refer to the Intel(R) 64 and IA-32 Architectures Software Developer's Manual for information about this MSR
    - units: none
    - domain: core
    - iogroup: MSRIOGroup
MSR::THERM_STATUS:PROCHOT_LOG
    - description: Refer to the Intel(R) 64 and IA-32 Architectures Software Developer's Manual for information about this MSR
    - units: none
    - domain: core
    - iogroup: MSRIOGroup
MSR::THERM_STATUS:THERMAL_STATUS_LOG
    - description: Refer to the Intel(R) 64 and IA-32 Architectures Software Developer's Manual for information about this MSR
    - units: none
    - domain: core
    - iogroup: MSRIOGroup
MSR::THERM_STATUS:THERMAL_THRESH_1_LOG
    - description: Refer to the Intel(R) 64 and IA-32 Architectures Software Developer's Manual for information about this MSR
    - units: none
    - domain: core
    - iogroup: MSRIOGroup
MSR::THERM_STATUS:THERMAL_THRESH_2_LOG
    - description: Refer to the Intel(R) 64 and IA-32 Architectures Software Developer's Manual for information about this MSR
    - units: none
    - domain: core
    - iogroup: MSRIOGroup
MSR::UNCORE_RATIO_LIMIT:MAX_RATIO
    - description: Refer to the Intel(R) 64 and IA-32 Architectures Software Developer's Manual for information about this MSR
    - units: hertz
    - domain: package
    - iogroup: MSRIOGroup
MSR::UNCORE_RATIO_LIMIT:MIN_RATIO
    - description: Refer to the Intel(R) 64 and IA-32 Architectures Software Developer's Manual for information about this MSR
    - units: hertz
    - domain: package
    - iogroup: MSRIOGroup
POWER_PACKAGE_LIMIT
    - description: The average power usage limit over the time window specified in PL1_TIME_WINDOW.
    - alias_for: MSR::PKG_POWER_LIMIT:PL1_POWER_LIMIT
    - units: watts
    - domain: package
    - iogroup: MSRIOGroup
POWER_PACKAGE_TIME_WINDOW
    - description: The time window associated with power limit 1.
    - alias_for: MSR::PKG_POWER_LIMIT:PL1_TIME_WINDOW
    - units: seconds
    - domain: package
    - iogroup: MSRIOGroup
