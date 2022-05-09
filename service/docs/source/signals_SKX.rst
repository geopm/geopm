SKX Platform Signals
====================

CPUINFO::FREQ_MAX
    - description: Maximum processor frequency
    - units: hertz
    - aggregation: expect_same
    - domain: board
    - iogroup: CpuinfoIOGroup
CPUINFO::FREQ_MIN
    - description: Minimum processor frequency
    - units: hertz
    - aggregation: expect_same
    - domain: board
    - iogroup: CpuinfoIOGroup
CPUINFO::FREQ_STEP
    - description: Step size between process frequency settings
    - units: hertz
    - aggregation: expect_same
    - domain: board
    - iogroup: CpuinfoIOGroup
CPUINFO::FREQ_STICKER
    - description: Processor base frequency
    - units: hertz
    - aggregation: expect_same
    - domain: board
    - iogroup: CpuinfoIOGroup
CPU_FREQUENCY_CONTROL
    - description: Target operating frequency of the CPU based on the control register.
    - alias_for: MSR::PERF_CTL:FREQ
    - units: hertz
    - aggregation: select_first
    - domain: core
    - iogroup: MSRIOGroup
CPU_FREQUENCY_MAX
    - description: Maximum processor frequency.
    - alias_for: MSR::TURBO_RATIO_LIMIT:MAX_RATIO_LIMIT_0
    - units: hertz
    - aggregation: select_first
    - domain: package
    - iogroup: MSRIOGroup
CPU_FREQUENCY_MIN
    - description: Minimum processor frequency
    - alias_for: CPUINFO::FREQ_MIN
    - units: hertz
    - aggregation: expect_same
    - domain: board
    - iogroup: CpuinfoIOGroup
CPU_FREQUENCY_STATUS
    - description: The current operating frequency of the CPU.
    - alias_for: MSR::PERF_STATUS:FREQ
    - units: hertz
    - aggregation: average
    - domain: cpu
    - iogroup: MSRIOGroup
CPU_FREQUENCY_STEP
    - description: Step size between process frequency settings
    - alias_for: CPUINFO::FREQ_STEP
    - units: hertz
    - aggregation: expect_same
    - domain: board
    - iogroup: CpuinfoIOGroup
CPU_FREQUENCY_STICKER
    - description: Processor base frequency
    - alias_for: CPUINFO::FREQ_STICKER
    - units: hertz
    - aggregation: expect_same
    - domain: board
    - iogroup: CpuinfoIOGroup
CYCLES_REFERENCE
    - description: The count of the number of cycles while the logical processor is not in a halt state and not in a stop-clock state.  The count rate is fixed at the TIMESTAMP_COUNT rate.
    - alias_for: MSR::FIXED_CTR2:CPU_CLK_UNHALTED_REF_TSC
    - units: none
    - aggregation: sum
    - domain: cpu
    - iogroup: MSRIOGroup
CYCLES_THREAD
    - description: The count of the number of cycles while the logical processor is not in a halt state.  The count rate may change based on core frequency.
    - alias_for: MSR::FIXED_CTR1:CPU_CLK_UNHALTED_THREAD
    - units: none
    - aggregation: sum
    - domain: cpu
    - iogroup: MSRIOGroup
ENERGY_DRAM
    - description: An increasing meter of energy consumed by the DRAM over time.  It will reset periodically due to roll-over.
    - alias_for: MSR::DRAM_ENERGY_STATUS:ENERGY
    - units: joules
    - aggregation: sum
    - domain: memory
    - iogroup: MSRIOGroup
ENERGY_PACKAGE
    - description: An increasing meter of energy consumed by the package over time.  It will reset periodically due to roll-over.
    - alias_for: MSR::PKG_ENERGY_STATUS:ENERGY
    - units: joules
    - aggregation: sum
    - domain: package
    - iogroup: MSRIOGroup
INSTRUCTIONS_RETIRED
    - description: The count of the number of instructions executed.
    - alias_for: MSR::FIXED_CTR0:INST_RETIRED_ANY
    - units: none
    - aggregation: sum
    - domain: cpu
    - iogroup: MSRIOGroup
MSR::APERF#
    - description: Refer to the Intel(R) 64 and IA-32 Architectures Software Developer's Manual for information about this MSR
    - units: none
    - aggregation: select_first
    - domain: cpu
    - iogroup: MSRIOGroup
MSR::APERF:ACNT
    - description: Refer to the Intel(R) 64 and IA-32 Architectures Software Developer's Manual for information about this MSR
    - units: none
    - aggregation: sum
    - domain: cpu
    - iogroup: MSRIOGroup
MSR::DRAM_ENERGY_STATUS#
    - description: Refer to the Intel(R) 64 and IA-32 Architectures Software Developer's Manual for information about this MSR
    - units: none
    - aggregation: select_first
    - domain: memory
    - iogroup: MSRIOGroup
MSR::DRAM_ENERGY_STATUS:ENERGY
    - description: An increasing meter of energy consumed by the DRAM over time.  It will reset periodically due to roll-over.
    - units: joules
    - aggregation: sum
    - domain: memory
    - iogroup: MSRIOGroup
MSR::DRAM_PERF_STATUS#
    - description: Refer to the Intel(R) 64 and IA-32 Architectures Software Developer's Manual for information about this MSR
    - units: none
    - aggregation: select_first
    - domain: memory
    - iogroup: MSRIOGroup
MSR::DRAM_PERF_STATUS:THROTTLE_TIME
    - description: Refer to the Intel(R) 64 and IA-32 Architectures Software Developer's Manual for information about this MSR
    - units: seconds
    - aggregation: select_first
    - domain: memory
    - iogroup: MSRIOGroup
MSR::DRAM_POWER_INFO#
    - description: Refer to the Intel(R) 64 and IA-32 Architectures Software Developer's Manual for information about this MSR
    - units: none
    - aggregation: select_first
    - domain: memory
    - iogroup: MSRIOGroup
MSR::DRAM_POWER_INFO:MAX_POWER
    - description: Refer to the Intel(R) 64 and IA-32 Architectures Software Developer's Manual for information about this MSR
    - units: watts
    - aggregation: select_first
    - domain: memory
    - iogroup: MSRIOGroup
MSR::DRAM_POWER_INFO:MAX_TIME_WINDOW
    - description: Refer to the Intel(R) 64 and IA-32 Architectures Software Developer's Manual for information about this MSR
    - units: seconds
    - aggregation: select_first
    - domain: memory
    - iogroup: MSRIOGroup
MSR::DRAM_POWER_INFO:MIN_POWER
    - description: Refer to the Intel(R) 64 and IA-32 Architectures Software Developer's Manual for information about this MSR
    - units: watts
    - aggregation: select_first
    - domain: memory
    - iogroup: MSRIOGroup
MSR::DRAM_POWER_INFO:THERMAL_SPEC_POWER
    - description: Refer to the Intel(R) 64 and IA-32 Architectures Software Developer's Manual for information about this MSR
    - units: watts
    - aggregation: select_first
    - domain: memory
    - iogroup: MSRIOGroup
MSR::DRAM_POWER_LIMIT#
    - description: Refer to the Intel(R) 64 and IA-32 Architectures Software Developer's Manual for information about this MSR
    - units: none
    - aggregation: select_first
    - domain: memory
    - iogroup: MSRIOGroup
MSR::DRAM_POWER_LIMIT:ENABLE
    - description: Refer to the Intel(R) 64 and IA-32 Architectures Software Developer's Manual for information about this MSR
    - units: none
    - aggregation: select_first
    - domain: memory
    - iogroup: MSRIOGroup
MSR::DRAM_POWER_LIMIT:LOCK
    - description: Refer to the Intel(R) 64 and IA-32 Architectures Software Developer's Manual for information about this MSR
    - units: none
    - aggregation: select_first
    - domain: memory
    - iogroup: MSRIOGroup
MSR::DRAM_POWER_LIMIT:POWER_LIMIT
    - description: Refer to the Intel(R) 64 and IA-32 Architectures Software Developer's Manual for information about this MSR
    - units: watts
    - aggregation: select_first
    - domain: memory
    - iogroup: MSRIOGroup
MSR::DRAM_POWER_LIMIT:TIME_WINDOW
    - description: Refer to the Intel(R) 64 and IA-32 Architectures Software Developer's Manual for information about this MSR
    - units: seconds
    - aggregation: select_first
    - domain: memory
    - iogroup: MSRIOGroup
MSR::FIXED_CTR0#
    - description: Refer to the Intel(R) 64 and IA-32 Architectures Software Developer's Manual for information about this MSR
    - units: none
    - aggregation: select_first
    - domain: cpu
    - iogroup: MSRIOGroup
MSR::FIXED_CTR0:INST_RETIRED_ANY
    - description: The count of the number of instructions executed.
    - units: none
    - aggregation: sum
    - domain: cpu
    - iogroup: MSRIOGroup
MSR::FIXED_CTR1#
    - description: Refer to the Intel(R) 64 and IA-32 Architectures Software Developer's Manual for information about this MSR
    - units: none
    - aggregation: select_first
    - domain: cpu
    - iogroup: MSRIOGroup
MSR::FIXED_CTR1:CPU_CLK_UNHALTED_THREAD
    - description: The count of the number of cycles while the logical processor is not in a halt state.  The count rate may change based on core frequency.
    - units: none
    - aggregation: sum
    - domain: cpu
    - iogroup: MSRIOGroup
MSR::FIXED_CTR2#
    - description: Refer to the Intel(R) 64 and IA-32 Architectures Software Developer's Manual for information about this MSR
    - units: none
    - aggregation: select_first
    - domain: cpu
    - iogroup: MSRIOGroup
MSR::FIXED_CTR2:CPU_CLK_UNHALTED_REF_TSC
    - description: The count of the number of cycles while the logical processor is not in a halt state and not in a stop-clock state.  The count rate is fixed at the TIMESTAMP_COUNT rate.
    - units: none
    - aggregation: sum
    - domain: cpu
    - iogroup: MSRIOGroup
MSR::FIXED_CTR_CTRL#
    - description: Refer to the Intel(R) 64 and IA-32 Architectures Software Developer's Manual for information about this MSR
    - units: none
    - aggregation: select_first
    - domain: cpu
    - iogroup: MSRIOGroup
MSR::FIXED_CTR_CTRL:EN0_OS
    - description: Refer to the Intel(R) 64 and IA-32 Architectures Software Developer's Manual for information about this MSR
    - units: none
    - aggregation: select_first
    - domain: cpu
    - iogroup: MSRIOGroup
MSR::FIXED_CTR_CTRL:EN0_PMI
    - description: Refer to the Intel(R) 64 and IA-32 Architectures Software Developer's Manual for information about this MSR
    - units: none
    - aggregation: select_first
    - domain: cpu
    - iogroup: MSRIOGroup
MSR::FIXED_CTR_CTRL:EN0_USR
    - description: Refer to the Intel(R) 64 and IA-32 Architectures Software Developer's Manual for information about this MSR
    - units: none
    - aggregation: select_first
    - domain: cpu
    - iogroup: MSRIOGroup
MSR::FIXED_CTR_CTRL:EN1_OS
    - description: Refer to the Intel(R) 64 and IA-32 Architectures Software Developer's Manual for information about this MSR
    - units: none
    - aggregation: select_first
    - domain: cpu
    - iogroup: MSRIOGroup
MSR::FIXED_CTR_CTRL:EN1_PMI
    - description: Refer to the Intel(R) 64 and IA-32 Architectures Software Developer's Manual for information about this MSR
    - units: none
    - aggregation: select_first
    - domain: cpu
    - iogroup: MSRIOGroup
MSR::FIXED_CTR_CTRL:EN1_USR
    - description: Refer to the Intel(R) 64 and IA-32 Architectures Software Developer's Manual for information about this MSR
    - units: none
    - aggregation: select_first
    - domain: cpu
    - iogroup: MSRIOGroup
MSR::FIXED_CTR_CTRL:EN2_OS
    - description: Refer to the Intel(R) 64 and IA-32 Architectures Software Developer's Manual for information about this MSR
    - units: none
    - aggregation: select_first
    - domain: cpu
    - iogroup: MSRIOGroup
MSR::FIXED_CTR_CTRL:EN2_PMI
    - description: Refer to the Intel(R) 64 and IA-32 Architectures Software Developer's Manual for information about this MSR
    - units: none
    - aggregation: select_first
    - domain: cpu
    - iogroup: MSRIOGroup
MSR::FIXED_CTR_CTRL:EN2_USR
    - description: Refer to the Intel(R) 64 and IA-32 Architectures Software Developer's Manual for information about this MSR
    - units: none
    - aggregation: select_first
    - domain: cpu
    - iogroup: MSRIOGroup
MSR::IA32_PERFEVTSEL0#
    - description: Refer to the Intel(R) 64 and IA-32 Architectures Software Developer's Manual for information about this MSR
    - units: none
    - aggregation: select_first
    - domain: cpu
    - iogroup: MSRIOGroup
MSR::IA32_PERFEVTSEL0:ANYTHREAD
    - description: Refer to the Intel(R) 64 and IA-32 Architectures Software Developer's Manual for information about this MSR
    - units: none
    - aggregation: select_first
    - domain: cpu
    - iogroup: MSRIOGroup
MSR::IA32_PERFEVTSEL0:CMASK
    - description: Refer to the Intel(R) 64 and IA-32 Architectures Software Developer's Manual for information about this MSR
    - units: none
    - aggregation: select_first
    - domain: cpu
    - iogroup: MSRIOGroup
MSR::IA32_PERFEVTSEL0:EDGE
    - description: Refer to the Intel(R) 64 and IA-32 Architectures Software Developer's Manual for information about this MSR
    - units: none
    - aggregation: select_first
    - domain: cpu
    - iogroup: MSRIOGroup
MSR::IA32_PERFEVTSEL0:EN
    - description: Refer to the Intel(R) 64 and IA-32 Architectures Software Developer's Manual for information about this MSR
    - units: none
    - aggregation: select_first
    - domain: cpu
    - iogroup: MSRIOGroup
MSR::IA32_PERFEVTSEL0:EVENT_SELECT
    - description: Refer to the Intel(R) 64 and IA-32 Architectures Software Developer's Manual for information about this MSR
    - units: none
    - aggregation: select_first
    - domain: cpu
    - iogroup: MSRIOGroup
MSR::IA32_PERFEVTSEL0:INT
    - description: Refer to the Intel(R) 64 and IA-32 Architectures Software Developer's Manual for information about this MSR
    - units: none
    - aggregation: select_first
    - domain: cpu
    - iogroup: MSRIOGroup
MSR::IA32_PERFEVTSEL0:INV
    - description: Refer to the Intel(R) 64 and IA-32 Architectures Software Developer's Manual for information about this MSR
    - units: none
    - aggregation: select_first
    - domain: cpu
    - iogroup: MSRIOGroup
MSR::IA32_PERFEVTSEL0:OS
    - description: Refer to the Intel(R) 64 and IA-32 Architectures Software Developer's Manual for information about this MSR
    - units: none
    - aggregation: select_first
    - domain: cpu
    - iogroup: MSRIOGroup
MSR::IA32_PERFEVTSEL0:PC
    - description: Refer to the Intel(R) 64 and IA-32 Architectures Software Developer's Manual for information about this MSR
    - units: none
    - aggregation: select_first
    - domain: cpu
    - iogroup: MSRIOGroup
MSR::IA32_PERFEVTSEL0:UMASK
    - description: Refer to the Intel(R) 64 and IA-32 Architectures Software Developer's Manual for information about this MSR
    - units: none
    - aggregation: select_first
    - domain: cpu
    - iogroup: MSRIOGroup
MSR::IA32_PERFEVTSEL0:USR
    - description: Refer to the Intel(R) 64 and IA-32 Architectures Software Developer's Manual for information about this MSR
    - units: none
    - aggregation: select_first
    - domain: cpu
    - iogroup: MSRIOGroup
MSR::IA32_PERFEVTSEL1#
    - description: Refer to the Intel(R) 64 and IA-32 Architectures Software Developer's Manual for information about this MSR
    - units: none
    - aggregation: select_first
    - domain: cpu
    - iogroup: MSRIOGroup
MSR::IA32_PERFEVTSEL1:ANYTHREAD
    - description: Refer to the Intel(R) 64 and IA-32 Architectures Software Developer's Manual for information about this MSR
    - units: none
    - aggregation: select_first
    - domain: cpu
    - iogroup: MSRIOGroup
MSR::IA32_PERFEVTSEL1:CMASK
    - description: Refer to the Intel(R) 64 and IA-32 Architectures Software Developer's Manual for information about this MSR
    - units: none
    - aggregation: select_first
    - domain: cpu
    - iogroup: MSRIOGroup
MSR::IA32_PERFEVTSEL1:EDGE
    - description: Refer to the Intel(R) 64 and IA-32 Architectures Software Developer's Manual for information about this MSR
    - units: none
    - aggregation: select_first
    - domain: cpu
    - iogroup: MSRIOGroup
MSR::IA32_PERFEVTSEL1:EN
    - description: Refer to the Intel(R) 64 and IA-32 Architectures Software Developer's Manual for information about this MSR
    - units: none
    - aggregation: select_first
    - domain: cpu
    - iogroup: MSRIOGroup
MSR::IA32_PERFEVTSEL1:EVENT_SELECT
    - description: Refer to the Intel(R) 64 and IA-32 Architectures Software Developer's Manual for information about this MSR
    - units: none
    - aggregation: select_first
    - domain: cpu
    - iogroup: MSRIOGroup
MSR::IA32_PERFEVTSEL1:INT
    - description: Refer to the Intel(R) 64 and IA-32 Architectures Software Developer's Manual for information about this MSR
    - units: none
    - aggregation: select_first
    - domain: cpu
    - iogroup: MSRIOGroup
MSR::IA32_PERFEVTSEL1:INV
    - description: Refer to the Intel(R) 64 and IA-32 Architectures Software Developer's Manual for information about this MSR
    - units: none
    - aggregation: select_first
    - domain: cpu
    - iogroup: MSRIOGroup
MSR::IA32_PERFEVTSEL1:OS
    - description: Refer to the Intel(R) 64 and IA-32 Architectures Software Developer's Manual for information about this MSR
    - units: none
    - aggregation: select_first
    - domain: cpu
    - iogroup: MSRIOGroup
MSR::IA32_PERFEVTSEL1:PC
    - description: Refer to the Intel(R) 64 and IA-32 Architectures Software Developer's Manual for information about this MSR
    - units: none
    - aggregation: select_first
    - domain: cpu
    - iogroup: MSRIOGroup
MSR::IA32_PERFEVTSEL1:UMASK
    - description: Refer to the Intel(R) 64 and IA-32 Architectures Software Developer's Manual for information about this MSR
    - units: none
    - aggregation: select_first
    - domain: cpu
    - iogroup: MSRIOGroup
MSR::IA32_PERFEVTSEL1:USR
    - description: Refer to the Intel(R) 64 and IA-32 Architectures Software Developer's Manual for information about this MSR
    - units: none
    - aggregation: select_first
    - domain: cpu
    - iogroup: MSRIOGroup
MSR::IA32_PERFEVTSEL2#
    - description: Refer to the Intel(R) 64 and IA-32 Architectures Software Developer's Manual for information about this MSR
    - units: none
    - aggregation: select_first
    - domain: cpu
    - iogroup: MSRIOGroup
MSR::IA32_PERFEVTSEL2:ANYTHREAD
    - description: Refer to the Intel(R) 64 and IA-32 Architectures Software Developer's Manual for information about this MSR
    - units: none
    - aggregation: select_first
    - domain: cpu
    - iogroup: MSRIOGroup
MSR::IA32_PERFEVTSEL2:CMASK
    - description: Refer to the Intel(R) 64 and IA-32 Architectures Software Developer's Manual for information about this MSR
    - units: none
    - aggregation: select_first
    - domain: cpu
    - iogroup: MSRIOGroup
MSR::IA32_PERFEVTSEL2:EDGE
    - description: Refer to the Intel(R) 64 and IA-32 Architectures Software Developer's Manual for information about this MSR
    - units: none
    - aggregation: select_first
    - domain: cpu
    - iogroup: MSRIOGroup
MSR::IA32_PERFEVTSEL2:EN
    - description: Refer to the Intel(R) 64 and IA-32 Architectures Software Developer's Manual for information about this MSR
    - units: none
    - aggregation: select_first
    - domain: cpu
    - iogroup: MSRIOGroup
MSR::IA32_PERFEVTSEL2:EVENT_SELECT
    - description: Refer to the Intel(R) 64 and IA-32 Architectures Software Developer's Manual for information about this MSR
    - units: none
    - aggregation: select_first
    - domain: cpu
    - iogroup: MSRIOGroup
MSR::IA32_PERFEVTSEL2:INT
    - description: Refer to the Intel(R) 64 and IA-32 Architectures Software Developer's Manual for information about this MSR
    - units: none
    - aggregation: select_first
    - domain: cpu
    - iogroup: MSRIOGroup
MSR::IA32_PERFEVTSEL2:INV
    - description: Refer to the Intel(R) 64 and IA-32 Architectures Software Developer's Manual for information about this MSR
    - units: none
    - aggregation: select_first
    - domain: cpu
    - iogroup: MSRIOGroup
MSR::IA32_PERFEVTSEL2:OS
    - description: Refer to the Intel(R) 64 and IA-32 Architectures Software Developer's Manual for information about this MSR
    - units: none
    - aggregation: select_first
    - domain: cpu
    - iogroup: MSRIOGroup
MSR::IA32_PERFEVTSEL2:PC
    - description: Refer to the Intel(R) 64 and IA-32 Architectures Software Developer's Manual for information about this MSR
    - units: none
    - aggregation: select_first
    - domain: cpu
    - iogroup: MSRIOGroup
MSR::IA32_PERFEVTSEL2:UMASK
    - description: Refer to the Intel(R) 64 and IA-32 Architectures Software Developer's Manual for information about this MSR
    - units: none
    - aggregation: select_first
    - domain: cpu
    - iogroup: MSRIOGroup
MSR::IA32_PERFEVTSEL2:USR
    - description: Refer to the Intel(R) 64 and IA-32 Architectures Software Developer's Manual for information about this MSR
    - units: none
    - aggregation: select_first
    - domain: cpu
    - iogroup: MSRIOGroup
MSR::IA32_PERFEVTSEL3#
    - description: Refer to the Intel(R) 64 and IA-32 Architectures Software Developer's Manual for information about this MSR
    - units: none
    - aggregation: select_first
    - domain: cpu
    - iogroup: MSRIOGroup
MSR::IA32_PERFEVTSEL3:ANYTHREAD
    - description: Refer to the Intel(R) 64 and IA-32 Architectures Software Developer's Manual for information about this MSR
    - units: none
    - aggregation: select_first
    - domain: cpu
    - iogroup: MSRIOGroup
MSR::IA32_PERFEVTSEL3:CMASK
    - description: Refer to the Intel(R) 64 and IA-32 Architectures Software Developer's Manual for information about this MSR
    - units: none
    - aggregation: select_first
    - domain: cpu
    - iogroup: MSRIOGroup
MSR::IA32_PERFEVTSEL3:EDGE
    - description: Refer to the Intel(R) 64 and IA-32 Architectures Software Developer's Manual for information about this MSR
    - units: none
    - aggregation: select_first
    - domain: cpu
    - iogroup: MSRIOGroup
MSR::IA32_PERFEVTSEL3:EN
    - description: Refer to the Intel(R) 64 and IA-32 Architectures Software Developer's Manual for information about this MSR
    - units: none
    - aggregation: select_first
    - domain: cpu
    - iogroup: MSRIOGroup
MSR::IA32_PERFEVTSEL3:EVENT_SELECT
    - description: Refer to the Intel(R) 64 and IA-32 Architectures Software Developer's Manual for information about this MSR
    - units: none
    - aggregation: select_first
    - domain: cpu
    - iogroup: MSRIOGroup
MSR::IA32_PERFEVTSEL3:INT
    - description: Refer to the Intel(R) 64 and IA-32 Architectures Software Developer's Manual for information about this MSR
    - units: none
    - aggregation: select_first
    - domain: cpu
    - iogroup: MSRIOGroup
MSR::IA32_PERFEVTSEL3:INV
    - description: Refer to the Intel(R) 64 and IA-32 Architectures Software Developer's Manual for information about this MSR
    - units: none
    - aggregation: select_first
    - domain: cpu
    - iogroup: MSRIOGroup
MSR::IA32_PERFEVTSEL3:OS
    - description: Refer to the Intel(R) 64 and IA-32 Architectures Software Developer's Manual for information about this MSR
    - units: none
    - aggregation: select_first
    - domain: cpu
    - iogroup: MSRIOGroup
MSR::IA32_PERFEVTSEL3:PC
    - description: Refer to the Intel(R) 64 and IA-32 Architectures Software Developer's Manual for information about this MSR
    - units: none
    - aggregation: select_first
    - domain: cpu
    - iogroup: MSRIOGroup
MSR::IA32_PERFEVTSEL3:UMASK
    - description: Refer to the Intel(R) 64 and IA-32 Architectures Software Developer's Manual for information about this MSR
    - units: none
    - aggregation: select_first
    - domain: cpu
    - iogroup: MSRIOGroup
MSR::IA32_PERFEVTSEL3:USR
    - description: Refer to the Intel(R) 64 and IA-32 Architectures Software Developer's Manual for information about this MSR
    - units: none
    - aggregation: select_first
    - domain: cpu
    - iogroup: MSRIOGroup
MSR::IA32_PMC0#
    - description: Refer to the Intel(R) 64 and IA-32 Architectures Software Developer's Manual for information about this MSR
    - units: none
    - aggregation: select_first
    - domain: cpu
    - iogroup: MSRIOGroup
MSR::IA32_PMC0:PERFCTR
    - description: Refer to the Intel(R) 64 and IA-32 Architectures Software Developer's Manual for information about this MSR
    - units: none
    - aggregation: sum
    - domain: cpu
    - iogroup: MSRIOGroup
MSR::IA32_PMC1#
    - description: Refer to the Intel(R) 64 and IA-32 Architectures Software Developer's Manual for information about this MSR
    - units: none
    - aggregation: select_first
    - domain: cpu
    - iogroup: MSRIOGroup
MSR::IA32_PMC1:PERFCTR
    - description: Refer to the Intel(R) 64 and IA-32 Architectures Software Developer's Manual for information about this MSR
    - units: none
    - aggregation: sum
    - domain: cpu
    - iogroup: MSRIOGroup
MSR::IA32_PMC2#
    - description: Refer to the Intel(R) 64 and IA-32 Architectures Software Developer's Manual for information about this MSR
    - units: none
    - aggregation: select_first
    - domain: cpu
    - iogroup: MSRIOGroup
MSR::IA32_PMC2:PERFCTR
    - description: Refer to the Intel(R) 64 and IA-32 Architectures Software Developer's Manual for information about this MSR
    - units: none
    - aggregation: sum
    - domain: cpu
    - iogroup: MSRIOGroup
MSR::IA32_PMC3#
    - description: Refer to the Intel(R) 64 and IA-32 Architectures Software Developer's Manual for information about this MSR
    - units: none
    - aggregation: select_first
    - domain: cpu
    - iogroup: MSRIOGroup
MSR::IA32_PMC3:PERFCTR
    - description: Refer to the Intel(R) 64 and IA-32 Architectures Software Developer's Manual for information about this MSR
    - units: none
    - aggregation: sum
    - domain: cpu
    - iogroup: MSRIOGroup
MSR::MISC_ENABLE#
    - description: Refer to the Intel(R) 64 and IA-32 Architectures Software Developer's Manual for information about this MSR
    - units: none
    - aggregation: select_first
    - domain: package
    - iogroup: MSRIOGroup
MSR::MISC_ENABLE:ENHANCED_SPEEDSTEP_TECH_ENABLE
    - description: Refer to the Intel(R) 64 and IA-32 Architectures Software Developer's Manual for information about this MSR
    - units: none
    - aggregation: select_first
    - domain: package
    - iogroup: MSRIOGroup
MSR::MISC_ENABLE:LIMIT_CPUID_MAXVAL
    - description: Refer to the Intel(R) 64 and IA-32 Architectures Software Developer's Manual for information about this MSR
    - units: none
    - aggregation: select_first
    - domain: package
    - iogroup: MSRIOGroup
MSR::MISC_ENABLE:TURBO_MODE_DISABLE
    - description: Refer to the Intel(R) 64 and IA-32 Architectures Software Developer's Manual for information about this MSR
    - units: none
    - aggregation: select_first
    - domain: package
    - iogroup: MSRIOGroup
MSR::MPERF#
    - description: Refer to the Intel(R) 64 and IA-32 Architectures Software Developer's Manual for information about this MSR
    - units: none
    - aggregation: select_first
    - domain: cpu
    - iogroup: MSRIOGroup
MSR::MPERF:MCNT
    - description: Refer to the Intel(R) 64 and IA-32 Architectures Software Developer's Manual for information about this MSR
    - units: none
    - aggregation: sum
    - domain: cpu
    - iogroup: MSRIOGroup
MSR::PACKAGE_THERM_STATUS#
    - description: Refer to the Intel(R) 64 and IA-32 Architectures Software Developer's Manual for information about this MSR
    - units: none
    - aggregation: select_first
    - domain: package
    - iogroup: MSRIOGroup
MSR::PACKAGE_THERM_STATUS:CRITICAL_TEMP_LOG
    - description: Refer to the Intel(R) 64 and IA-32 Architectures Software Developer's Manual for information about this MSR
    - units: none
    - aggregation: select_first
    - domain: package
    - iogroup: MSRIOGroup
MSR::PACKAGE_THERM_STATUS:CRITICAL_TEMP_STATUS
    - description: Refer to the Intel(R) 64 and IA-32 Architectures Software Developer's Manual for information about this MSR
    - units: none
    - aggregation: select_first
    - domain: package
    - iogroup: MSRIOGroup
MSR::PACKAGE_THERM_STATUS:DIGITAL_READOUT
    - description: Refer to the Intel(R) 64 and IA-32 Architectures Software Developer's Manual for information about this MSR
    - units: celsius
    - aggregation: average
    - domain: package
    - iogroup: MSRIOGroup
MSR::PACKAGE_THERM_STATUS:POWER_LIMIT_STATUS
    - description: Refer to the Intel(R) 64 and IA-32 Architectures Software Developer's Manual for information about this MSR
    - units: none
    - aggregation: select_first
    - domain: package
    - iogroup: MSRIOGroup
MSR::PACKAGE_THERM_STATUS:POWER_NOTIFICATION_LOG
    - description: Refer to the Intel(R) 64 and IA-32 Architectures Software Developer's Manual for information about this MSR
    - units: none
    - aggregation: select_first
    - domain: package
    - iogroup: MSRIOGroup
MSR::PACKAGE_THERM_STATUS:PROCHOT_EVENT
    - description: Refer to the Intel(R) 64 and IA-32 Architectures Software Developer's Manual for information about this MSR
    - units: none
    - aggregation: select_first
    - domain: package
    - iogroup: MSRIOGroup
MSR::PACKAGE_THERM_STATUS:PROCHOT_LOG
    - description: Refer to the Intel(R) 64 and IA-32 Architectures Software Developer's Manual for information about this MSR
    - units: none
    - aggregation: select_first
    - domain: package
    - iogroup: MSRIOGroup
MSR::PACKAGE_THERM_STATUS:THERMAL_STATUS_FLAG
    - description: Refer to the Intel(R) 64 and IA-32 Architectures Software Developer's Manual for information about this MSR
    - units: none
    - aggregation: select_first
    - domain: package
    - iogroup: MSRIOGroup
MSR::PACKAGE_THERM_STATUS:THERMAL_STATUS_LOG
    - description: Refer to the Intel(R) 64 and IA-32 Architectures Software Developer's Manual for information about this MSR
    - units: none
    - aggregation: select_first
    - domain: package
    - iogroup: MSRIOGroup
MSR::PACKAGE_THERM_STATUS:THERMAL_THRESH_1_LOG
    - description: Refer to the Intel(R) 64 and IA-32 Architectures Software Developer's Manual for information about this MSR
    - units: none
    - aggregation: select_first
    - domain: package
    - iogroup: MSRIOGroup
MSR::PACKAGE_THERM_STATUS:THERMAL_THRESH_1_STATUS
    - description: Refer to the Intel(R) 64 and IA-32 Architectures Software Developer's Manual for information about this MSR
    - units: none
    - aggregation: select_first
    - domain: package
    - iogroup: MSRIOGroup
MSR::PACKAGE_THERM_STATUS:THERMAL_THRESH_2_LOG
    - description: Refer to the Intel(R) 64 and IA-32 Architectures Software Developer's Manual for information about this MSR
    - units: none
    - aggregation: select_first
    - domain: package
    - iogroup: MSRIOGroup
MSR::PACKAGE_THERM_STATUS:THERMAL_THRESH_2_STATUS
    - description: Refer to the Intel(R) 64 and IA-32 Architectures Software Developer's Manual for information about this MSR
    - units: none
    - aggregation: select_first
    - domain: package
    - iogroup: MSRIOGroup
MSR::PERF_CTL#
    - description: Refer to the Intel(R) 64 and IA-32 Architectures Software Developer's Manual for information about this MSR
    - units: none
    - aggregation: select_first
    - domain: core
    - iogroup: MSRIOGroup
MSR::PERF_CTL:FREQ
    - description: Target operating frequency of the CPU based on the control register.
    - units: hertz
    - aggregation: select_first
    - domain: core
    - iogroup: MSRIOGroup
MSR::PERF_GLOBAL_CTRL#
    - description: Refer to the Intel(R) 64 and IA-32 Architectures Software Developer's Manual for information about this MSR
    - units: none
    - aggregation: select_first
    - domain: cpu
    - iogroup: MSRIOGroup
MSR::PERF_GLOBAL_CTRL:EN_FIXED_CTR0
    - description: Refer to the Intel(R) 64 and IA-32 Architectures Software Developer's Manual for information about this MSR
    - units: none
    - aggregation: select_first
    - domain: cpu
    - iogroup: MSRIOGroup
MSR::PERF_GLOBAL_CTRL:EN_FIXED_CTR1
    - description: Refer to the Intel(R) 64 and IA-32 Architectures Software Developer's Manual for information about this MSR
    - units: none
    - aggregation: select_first
    - domain: cpu
    - iogroup: MSRIOGroup
MSR::PERF_GLOBAL_CTRL:EN_FIXED_CTR2
    - description: Refer to the Intel(R) 64 and IA-32 Architectures Software Developer's Manual for information about this MSR
    - units: none
    - aggregation: select_first
    - domain: cpu
    - iogroup: MSRIOGroup
MSR::PERF_GLOBAL_CTRL:EN_PMC0
    - description: Refer to the Intel(R) 64 and IA-32 Architectures Software Developer's Manual for information about this MSR
    - units: none
    - aggregation: select_first
    - domain: cpu
    - iogroup: MSRIOGroup
MSR::PERF_GLOBAL_CTRL:EN_PMC1
    - description: Refer to the Intel(R) 64 and IA-32 Architectures Software Developer's Manual for information about this MSR
    - units: none
    - aggregation: select_first
    - domain: cpu
    - iogroup: MSRIOGroup
MSR::PERF_GLOBAL_CTRL:EN_PMC2
    - description: Refer to the Intel(R) 64 and IA-32 Architectures Software Developer's Manual for information about this MSR
    - units: none
    - aggregation: select_first
    - domain: cpu
    - iogroup: MSRIOGroup
MSR::PERF_GLOBAL_CTRL:EN_PMC3
    - description: Refer to the Intel(R) 64 and IA-32 Architectures Software Developer's Manual for information about this MSR
    - units: none
    - aggregation: select_first
    - domain: cpu
    - iogroup: MSRIOGroup
MSR::PERF_GLOBAL_OVF_CTRL#
    - description: Refer to the Intel(R) 64 and IA-32 Architectures Software Developer's Manual for information about this MSR
    - units: none
    - aggregation: select_first
    - domain: cpu
    - iogroup: MSRIOGroup
MSR::PERF_GLOBAL_OVF_CTRL:CLEAR_OVF_FIXED_CTR0
    - description: Refer to the Intel(R) 64 and IA-32 Architectures Software Developer's Manual for information about this MSR
    - units: none
    - aggregation: select_first
    - domain: cpu
    - iogroup: MSRIOGroup
MSR::PERF_GLOBAL_OVF_CTRL:CLEAR_OVF_FIXED_CTR1
    - description: Refer to the Intel(R) 64 and IA-32 Architectures Software Developer's Manual for information about this MSR
    - units: none
    - aggregation: select_first
    - domain: cpu
    - iogroup: MSRIOGroup
MSR::PERF_GLOBAL_OVF_CTRL:CLEAR_OVF_FIXED_CTR2
    - description: Refer to the Intel(R) 64 and IA-32 Architectures Software Developer's Manual for information about this MSR
    - units: none
    - aggregation: select_first
    - domain: cpu
    - iogroup: MSRIOGroup
MSR::PERF_GLOBAL_OVF_CTRL:CLEAR_OVF_PMC0
    - description: Refer to the Intel(R) 64 and IA-32 Architectures Software Developer's Manual for information about this MSR
    - units: none
    - aggregation: select_first
    - domain: cpu
    - iogroup: MSRIOGroup
MSR::PERF_GLOBAL_OVF_CTRL:CLEAR_OVF_PMC1
    - description: Refer to the Intel(R) 64 and IA-32 Architectures Software Developer's Manual for information about this MSR
    - units: none
    - aggregation: select_first
    - domain: cpu
    - iogroup: MSRIOGroup
MSR::PERF_GLOBAL_OVF_CTRL:CLEAR_OVF_PMC2
    - description: Refer to the Intel(R) 64 and IA-32 Architectures Software Developer's Manual for information about this MSR
    - units: none
    - aggregation: select_first
    - domain: cpu
    - iogroup: MSRIOGroup
MSR::PERF_GLOBAL_OVF_CTRL:CLEAR_OVF_PMC3
    - description: Refer to the Intel(R) 64 and IA-32 Architectures Software Developer's Manual for information about this MSR
    - units: none
    - aggregation: select_first
    - domain: cpu
    - iogroup: MSRIOGroup
MSR::PERF_STATUS#
    - description: Refer to the Intel(R) 64 and IA-32 Architectures Software Developer's Manual for information about this MSR
    - units: none
    - aggregation: select_first
    - domain: cpu
    - iogroup: MSRIOGroup
MSR::PERF_STATUS:FREQ
    - description: The current operating frequency of the CPU.
    - units: hertz
    - aggregation: average
    - domain: cpu
    - iogroup: MSRIOGroup
MSR::PKG_ENERGY_STATUS#
    - description: Refer to the Intel(R) 64 and IA-32 Architectures Software Developer's Manual for information about this MSR
    - units: none
    - aggregation: select_first
    - domain: package
    - iogroup: MSRIOGroup
MSR::PKG_ENERGY_STATUS:ENERGY
    - description: An increasing meter of energy consumed by the package over time.  It will reset periodically due to roll-over.
    - units: joules
    - aggregation: sum
    - domain: package
    - iogroup: MSRIOGroup
MSR::PKG_POWER_INFO#
    - description: Refer to the Intel(R) 64 and IA-32 Architectures Software Developer's Manual for information about this MSR
    - units: none
    - aggregation: select_first
    - domain: package
    - iogroup: MSRIOGroup
MSR::PKG_POWER_INFO:MAX_POWER
    - description: The maximum power limit based on the electrical specification.
    - units: watts
    - aggregation: sum
    - domain: package
    - iogroup: MSRIOGroup
MSR::PKG_POWER_INFO:MAX_TIME_WINDOW
    - description: Refer to the Intel(R) 64 and IA-32 Architectures Software Developer's Manual for information about this MSR
    - units: seconds
    - aggregation: select_first
    - domain: package
    - iogroup: MSRIOGroup
MSR::PKG_POWER_INFO:MIN_POWER
    - description: The minimum power limit based on the electrical specification.
    - units: watts
    - aggregation: sum
    - domain: package
    - iogroup: MSRIOGroup
MSR::PKG_POWER_INFO:THERMAL_SPEC_POWER
    - description: Maximum power to stay within the thermal limits based on the design (TDP).
    - units: watts
    - aggregation: sum
    - domain: package
    - iogroup: MSRIOGroup
MSR::PKG_POWER_LIMIT#
    - description: Refer to the Intel(R) 64 and IA-32 Architectures Software Developer's Manual for information about this MSR
    - units: none
    - aggregation: select_first
    - domain: package
    - iogroup: MSRIOGroup
MSR::PKG_POWER_LIMIT:LOCK
    - description: Refer to the Intel(R) 64 and IA-32 Architectures Software Developer's Manual for information about this MSR
    - units: none
    - aggregation: select_first
    - domain: package
    - iogroup: MSRIOGroup
MSR::PKG_POWER_LIMIT:PL1_CLAMP_ENABLE
    - description: Refer to the Intel(R) 64 and IA-32 Architectures Software Developer's Manual for information about this MSR
    - units: none
    - aggregation: select_first
    - domain: package
    - iogroup: MSRIOGroup
MSR::PKG_POWER_LIMIT:PL1_LIMIT_ENABLE
    - description: Refer to the Intel(R) 64 and IA-32 Architectures Software Developer's Manual for information about this MSR
    - units: none
    - aggregation: select_first
    - domain: package
    - iogroup: MSRIOGroup
MSR::PKG_POWER_LIMIT:PL1_POWER_LIMIT
    - description: The average power usage limit over the time window specified in PL1_TIME_WINDOW.
    - units: watts
    - aggregation: select_first
    - domain: package
    - iogroup: MSRIOGroup
MSR::PKG_POWER_LIMIT:PL1_TIME_WINDOW
    - description: The time window associated with power limit 1.
    - units: seconds
    - aggregation: select_first
    - domain: package
    - iogroup: MSRIOGroup
MSR::PKG_POWER_LIMIT:PL2_CLAMP_ENABLE
    - description: Refer to the Intel(R) 64 and IA-32 Architectures Software Developer's Manual for information about this MSR
    - units: none
    - aggregation: select_first
    - domain: package
    - iogroup: MSRIOGroup
MSR::PKG_POWER_LIMIT:PL2_LIMIT_ENABLE
    - description: Refer to the Intel(R) 64 and IA-32 Architectures Software Developer's Manual for information about this MSR
    - units: none
    - aggregation: select_first
    - domain: package
    - iogroup: MSRIOGroup
MSR::PKG_POWER_LIMIT:PL2_POWER_LIMIT
    - description: Refer to the Intel(R) 64 and IA-32 Architectures Software Developer's Manual for information about this MSR
    - units: watts
    - aggregation: select_first
    - domain: package
    - iogroup: MSRIOGroup
MSR::PKG_POWER_LIMIT:PL2_TIME_WINDOW
    - description: Refer to the Intel(R) 64 and IA-32 Architectures Software Developer's Manual for information about this MSR
    - units: seconds
    - aggregation: select_first
    - domain: package
    - iogroup: MSRIOGroup
MSR::PLATFORM_INFO#
    - description: Refer to the Intel(R) 64 and IA-32 Architectures Software Developer's Manual for information about this MSR
    - units: none
    - aggregation: select_first
    - domain: package
    - iogroup: MSRIOGroup
MSR::PLATFORM_INFO:MAX_EFFICIENCY_RATIO
    - description: Refer to the Intel(R) 64 and IA-32 Architectures Software Developer's Manual for information about this MSR
    - units: hertz
    - aggregation: select_first
    - domain: package
    - iogroup: MSRIOGroup
MSR::PLATFORM_INFO:MAX_NON_TURBO_RATIO
    - description: Refer to the Intel(R) 64 and IA-32 Architectures Software Developer's Manual for information about this MSR
    - units: hertz
    - aggregation: select_first
    - domain: package
    - iogroup: MSRIOGroup
MSR::PLATFORM_INFO:PROGRAMMABLE_RATIO_LIMITS_TURBO_MODE
    - description: Indicates whether the MSR::TURBO_RATIO_LIMIT:* signals are also available as controls.
    - units: none
    - aggregation: select_first
    - domain: package
    - iogroup: MSRIOGroup
MSR::PLATFORM_INFO:PROGRAMMABLE_TCC_ACTIVATION_OFFSET
    - description: Refer to the Intel(R) 64 and IA-32 Architectures Software Developer's Manual for information about this MSR
    - units: none
    - aggregation: select_first
    - domain: package
    - iogroup: MSRIOGroup
MSR::PLATFORM_INFO:PROGRAMMABLE_TDP_LIMITS_TURBO_MODE
    - description: Refer to the Intel(R) 64 and IA-32 Architectures Software Developer's Manual for information about this MSR
    - units: none
    - aggregation: select_first
    - domain: package
    - iogroup: MSRIOGroup
MSR::PPERF#
    - description: Refer to the Intel(R) 64 and IA-32 Architectures Software Developer's Manual for information about this MSR
    - units: none
    - aggregation: select_first
    - domain: cpu
    - iogroup: MSRIOGroup
MSR::PPERF:PCNT
    - description: Refer to the Intel(R) 64 and IA-32 Architectures Software Developer's Manual for information about this MSR
    - units: none
    - aggregation: sum
    - domain: cpu
    - iogroup: MSRIOGroup
MSR::PQR_ASSOC#
    - description: Refer to the Intel(R) 64 and IA-32 Architectures Software Developer's Manual for information about this MSR
    - units: none
    - aggregation: select_first
    - domain: cpu
    - iogroup: MSRIOGroup
MSR::PQR_ASSOC:RMID
    - description: Refer to the Intel(R) 64 and IA-32 Architectures Software Developer's Manual for information about this MSR
    - units: none
    - aggregation: select_first
    - domain: cpu
    - iogroup: MSRIOGroup
MSR::QM_CTR#
    - description: Refer to the Intel(R) 64 and IA-32 Architectures Software Developer's Manual for information about this MSR
    - units: none
    - aggregation: select_first
    - domain: package
    - iogroup: MSRIOGroup
MSR::QM_CTR:ERROR
    - description: Refer to the Intel(R) 64 and IA-32 Architectures Software Developer's Manual for information about this MSR
    - units: none
    - aggregation: select_first
    - domain: package
    - iogroup: MSRIOGroup
MSR::QM_CTR:RM_DATA
    - description: Refer to the Intel(R) 64 and IA-32 Architectures Software Developer's Manual for information about this MSR
    - units: none
    - aggregation: sum
    - domain: package
    - iogroup: MSRIOGroup
MSR::QM_CTR:UNAVAILABLE
    - description: Refer to the Intel(R) 64 and IA-32 Architectures Software Developer's Manual for information about this MSR
    - units: none
    - aggregation: select_first
    - domain: package
    - iogroup: MSRIOGroup
MSR::QM_EVTSEL#
    - description: Refer to the Intel(R) 64 and IA-32 Architectures Software Developer's Manual for information about this MSR
    - units: none
    - aggregation: select_first
    - domain: package
    - iogroup: MSRIOGroup
MSR::QM_EVTSEL:EVENT_ID
    - description: Refer to the Intel(R) 64 and IA-32 Architectures Software Developer's Manual for information about this MSR
    - units: none
    - aggregation: select_first
    - domain: package
    - iogroup: MSRIOGroup
MSR::QM_EVTSEL:RMID
    - description: Refer to the Intel(R) 64 and IA-32 Architectures Software Developer's Manual for information about this MSR
    - units: none
    - aggregation: select_first
    - domain: package
    - iogroup: MSRIOGroup
MSR::RAPL_POWER_UNIT#
    - description: Refer to the Intel(R) 64 and IA-32 Architectures Software Developer's Manual for information about this MSR
    - units: none
    - aggregation: select_first
    - domain: package
    - iogroup: MSRIOGroup
MSR::RAPL_POWER_UNIT:ENERGY
    - description: Refer to the Intel(R) 64 and IA-32 Architectures Software Developer's Manual for information about this MSR
    - units: joules
    - aggregation: select_first
    - domain: package
    - iogroup: MSRIOGroup
MSR::RAPL_POWER_UNIT:POWER
    - description: Refer to the Intel(R) 64 and IA-32 Architectures Software Developer's Manual for information about this MSR
    - units: watts
    - aggregation: select_first
    - domain: package
    - iogroup: MSRIOGroup
MSR::RAPL_POWER_UNIT:TIME
    - description: Refer to the Intel(R) 64 and IA-32 Architectures Software Developer's Manual for information about this MSR
    - units: seconds
    - aggregation: select_first
    - domain: package
    - iogroup: MSRIOGroup
MSR::TEMPERATURE_TARGET#
    - description: Refer to the Intel(R) 64 and IA-32 Architectures Software Developer's Manual for information about this MSR
    - units: none
    - aggregation: select_first
    - domain: core
    - iogroup: MSRIOGroup
MSR::TEMPERATURE_TARGET:PROCHOT_MIN
    - description: Refer to the Intel(R) 64 and IA-32 Architectures Software Developer's Manual for information about this MSR
    - units: celsius
    - aggregation: expect_same
    - domain: core
    - iogroup: MSRIOGroup
MSR::TEMPERATURE_TARGET:TCC_ACTIVE_OFFSET
    - description: Refer to the Intel(R) 64 and IA-32 Architectures Software Developer's Manual for information about this MSR
    - units: celsius
    - aggregation: select_first
    - domain: core
    - iogroup: MSRIOGroup
MSR::THERM_STATUS#
    - description: Refer to the Intel(R) 64 and IA-32 Architectures Software Developer's Manual for information about this MSR
    - units: none
    - aggregation: select_first
    - domain: core
    - iogroup: MSRIOGroup
MSR::THERM_STATUS:CRITICAL_TEMP_LOG
    - description: Refer to the Intel(R) 64 and IA-32 Architectures Software Developer's Manual for information about this MSR
    - units: none
    - aggregation: select_first
    - domain: core
    - iogroup: MSRIOGroup
MSR::THERM_STATUS:CRITICAL_TEMP_STATUS
    - description: Refer to the Intel(R) 64 and IA-32 Architectures Software Developer's Manual for information about this MSR
    - units: none
    - aggregation: select_first
    - domain: core
    - iogroup: MSRIOGroup
MSR::THERM_STATUS:DIGITAL_READOUT
    - description: Refer to the Intel(R) 64 and IA-32 Architectures Software Developer's Manual for information about this MSR
    - units: celsius
    - aggregation: average
    - domain: core
    - iogroup: MSRIOGroup
MSR::THERM_STATUS:POWER_LIMIT_STATUS
    - description: Refer to the Intel(R) 64 and IA-32 Architectures Software Developer's Manual for information about this MSR
    - units: none
    - aggregation: select_first
    - domain: core
    - iogroup: MSRIOGroup
MSR::THERM_STATUS:POWER_NOTIFICATION_LOG
    - description: Refer to the Intel(R) 64 and IA-32 Architectures Software Developer's Manual for information about this MSR
    - units: none
    - aggregation: select_first
    - domain: core
    - iogroup: MSRIOGroup
MSR::THERM_STATUS:PROCHOT_EVENT
    - description: Refer to the Intel(R) 64 and IA-32 Architectures Software Developer's Manual for information about this MSR
    - units: none
    - aggregation: select_first
    - domain: core
    - iogroup: MSRIOGroup
MSR::THERM_STATUS:PROCHOT_LOG
    - description: Refer to the Intel(R) 64 and IA-32 Architectures Software Developer's Manual for information about this MSR
    - units: none
    - aggregation: select_first
    - domain: core
    - iogroup: MSRIOGroup
MSR::THERM_STATUS:READING_VALID
    - description: Refer to the Intel(R) 64 and IA-32 Architectures Software Developer's Manual for information about this MSR
    - units: none
    - aggregation: select_first
    - domain: core
    - iogroup: MSRIOGroup
MSR::THERM_STATUS:RESOLUTION
    - description: Refer to the Intel(R) 64 and IA-32 Architectures Software Developer's Manual for information about this MSR
    - units: celsius
    - aggregation: select_first
    - domain: core
    - iogroup: MSRIOGroup
MSR::THERM_STATUS:THERMAL_STATUS_FLAG
    - description: Refer to the Intel(R) 64 and IA-32 Architectures Software Developer's Manual for information about this MSR
    - units: none
    - aggregation: select_first
    - domain: core
    - iogroup: MSRIOGroup
MSR::THERM_STATUS:THERMAL_STATUS_LOG
    - description: Refer to the Intel(R) 64 and IA-32 Architectures Software Developer's Manual for information about this MSR
    - units: none
    - aggregation: select_first
    - domain: core
    - iogroup: MSRIOGroup
MSR::THERM_STATUS:THERMAL_THRESH_1_LOG
    - description: Refer to the Intel(R) 64 and IA-32 Architectures Software Developer's Manual for information about this MSR
    - units: none
    - aggregation: select_first
    - domain: core
    - iogroup: MSRIOGroup
MSR::THERM_STATUS:THERMAL_THRESH_1_STATUS
    - description: Refer to the Intel(R) 64 and IA-32 Architectures Software Developer's Manual for information about this MSR
    - units: none
    - aggregation: select_first
    - domain: core
    - iogroup: MSRIOGroup
MSR::THERM_STATUS:THERMAL_THRESH_2_LOG
    - description: Refer to the Intel(R) 64 and IA-32 Architectures Software Developer's Manual for information about this MSR
    - units: none
    - aggregation: select_first
    - domain: core
    - iogroup: MSRIOGroup
MSR::THERM_STATUS:THERMAL_THRESH_2_STATUS
    - description: Refer to the Intel(R) 64 and IA-32 Architectures Software Developer's Manual for information about this MSR
    - units: none
    - aggregation: select_first
    - domain: core
    - iogroup: MSRIOGroup
MSR::TIME
    - description: Time in seconds
    - units: seconds
    - aggregation: select_first
    - domain: board
    - iogroup: MSRIOGroup
MSR::TIME_STAMP_COUNTER#
    - description: Refer to the Intel(R) 64 and IA-32 Architectures Software Developer's Manual for information about this MSR
    - units: none
    - aggregation: select_first
    - domain: cpu
    - iogroup: MSRIOGroup
MSR::TIME_STAMP_COUNTER:TIMESTAMP_COUNT
    - description: An always running, monotonically increasing counter that is incremented at a constant rate.  For use as a wall clock timer.
    - units: none
    - aggregation: select_first
    - domain: cpu
    - iogroup: MSRIOGroup
MSR::TURBO_RATIO_LIMIT#
    - description: Refer to the Intel(R) 64 and IA-32 Architectures Software Developer's Manual for information about this MSR
    - units: none
    - aggregation: select_first
    - domain: package
    - iogroup: MSRIOGroup
MSR::TURBO_RATIO_LIMIT:MAX_RATIO_LIMIT_0
    - description: Maximum turbo frequency with up to MSR::TURBO_RATIO_LIMIT_CORES:NUMCORE_0 active cores.
    - units: hertz
    - aggregation: select_first
    - domain: package
    - iogroup: MSRIOGroup
MSR::TURBO_RATIO_LIMIT:MAX_RATIO_LIMIT_1
    - description: Maximum turbo frequency with more than MSR::TURBO_RATIO_LIMIT_CORES:NUMCORE_0 and up to MSR::TURBO_RATIO_LIMIT_CORES:NUMCORE_1 active cores.
    - units: hertz
    - aggregation: select_first
    - domain: package
    - iogroup: MSRIOGroup
MSR::TURBO_RATIO_LIMIT:MAX_RATIO_LIMIT_2
    - description: Maximum turbo frequency with more than MSR::TURBO_RATIO_LIMIT_CORES:NUMCORE_1 and up to MSR::TURBO_RATIO_LIMIT_CORES:NUMCORE_2 active cores.
    - units: hertz
    - aggregation: select_first
    - domain: package
    - iogroup: MSRIOGroup
MSR::TURBO_RATIO_LIMIT:MAX_RATIO_LIMIT_3
    - description: Maximum turbo frequency with more than MSR::TURBO_RATIO_LIMIT_CORES:NUMCORE_2 and up to MSR::TURBO_RATIO_LIMIT_CORES:NUMCORE_3 active cores.
    - units: hertz
    - aggregation: select_first
    - domain: package
    - iogroup: MSRIOGroup
MSR::TURBO_RATIO_LIMIT:MAX_RATIO_LIMIT_4
    - description: Maximum turbo frequency with more than MSR::TURBO_RATIO_LIMIT_CORES:NUMCORE_3 and up to MSR::TURBO_RATIO_LIMIT_CORES:NUMCORE_4 active cores.
    - units: hertz
    - aggregation: select_first
    - domain: package
    - iogroup: MSRIOGroup
MSR::TURBO_RATIO_LIMIT:MAX_RATIO_LIMIT_5
    - description: Maximum turbo frequency with more than MSR::TURBO_RATIO_LIMIT_CORES:NUMCORE_4 and up to MSR::TURBO_RATIO_LIMIT_CORES:NUMCORE_5 active cores.
    - units: hertz
    - aggregation: select_first
    - domain: package
    - iogroup: MSRIOGroup
MSR::TURBO_RATIO_LIMIT:MAX_RATIO_LIMIT_6
    - description: Maximum turbo frequency with more than MSR::TURBO_RATIO_LIMIT_CORES:NUMCORE_5 and up to MSR::TURBO_RATIO_LIMIT_CORES:NUMCORE_6 active cores.
    - units: hertz
    - aggregation: select_first
    - domain: package
    - iogroup: MSRIOGroup
MSR::TURBO_RATIO_LIMIT:MAX_RATIO_LIMIT_7
    - description: Maximum turbo frequency with more than MSR::TURBO_RATIO_LIMIT_CORES:NUMCORE_6 and up to MSR::TURBO_RATIO_LIMIT_CORES:NUMCORE_7 active cores.
    - units: hertz
    - aggregation: select_first
    - domain: package
    - iogroup: MSRIOGroup
MSR::TURBO_RATIO_LIMIT_CORES#
    - description: Refer to the Intel(R) 64 and IA-32 Architectures Software Developer's Manual for information about this MSR
    - units: none
    - aggregation: select_first
    - domain: package
    - iogroup: MSRIOGroup
MSR::TURBO_RATIO_LIMIT_CORES:NUMCORE_0
    - description: Maximum number of active cores for a maximum turbo frequency of MSR::TURBO_RATIO_LIMIT:MAX_RATIO_LIMIT_0.
    - units: none
    - aggregation: select_first
    - domain: package
    - iogroup: MSRIOGroup
MSR::TURBO_RATIO_LIMIT_CORES:NUMCORE_1
    - description: Maximum number of active cores for a maximum turbo frequency of MSR::TURBO_RATIO_LIMIT:MAX_RATIO_LIMIT_1.
    - units: none
    - aggregation: select_first
    - domain: package
    - iogroup: MSRIOGroup
MSR::TURBO_RATIO_LIMIT_CORES:NUMCORE_2
    - description: Maximum number of active cores for a maximum turbo frequency of MSR::TURBO_RATIO_LIMIT:MAX_RATIO_LIMIT_2.
    - units: none
    - aggregation: select_first
    - domain: package
    - iogroup: MSRIOGroup
MSR::TURBO_RATIO_LIMIT_CORES:NUMCORE_3
    - description: Maximum number of active cores for a maximum turbo frequency of MSR::TURBO_RATIO_LIMIT:MAX_RATIO_LIMIT_3.
    - units: none
    - aggregation: select_first
    - domain: package
    - iogroup: MSRIOGroup
MSR::TURBO_RATIO_LIMIT_CORES:NUMCORE_4
    - description: Maximum number of active cores for a maximum turbo frequency of MSR::TURBO_RATIO_LIMIT:MAX_RATIO_LIMIT_4.
    - units: none
    - aggregation: select_first
    - domain: package
    - iogroup: MSRIOGroup
MSR::TURBO_RATIO_LIMIT_CORES:NUMCORE_5
    - description: Maximum number of active cores for a maximum turbo frequency of MSR::TURBO_RATIO_LIMIT:MAX_RATIO_LIMIT_5.
    - units: none
    - aggregation: select_first
    - domain: package
    - iogroup: MSRIOGroup
MSR::TURBO_RATIO_LIMIT_CORES:NUMCORE_6
    - description: Maximum number of active cores for a maximum turbo frequency of MSR::TURBO_RATIO_LIMIT:MAX_RATIO_LIMIT_6.
    - units: none
    - aggregation: select_first
    - domain: package
    - iogroup: MSRIOGroup
MSR::TURBO_RATIO_LIMIT_CORES:NUMCORE_7
    - description: Maximum number of active cores for a maximum turbo frequency of MSR::TURBO_RATIO_LIMIT:MAX_RATIO_LIMIT_7.
    - units: none
    - aggregation: select_first
    - domain: package
    - iogroup: MSRIOGroup
MSR::UNCORE_PERF_STATUS#
    - description: Refer to the Intel(R) 64 and IA-32 Architectures Software Developer's Manual for information about this MSR
    - units: none
    - aggregation: select_first
    - domain: package
    - iogroup: MSRIOGroup
MSR::UNCORE_PERF_STATUS:FREQ
    - description: Refer to the Intel(R) 64 and IA-32 Architectures Software Developer's Manual for information about this MSR
    - units: hertz
    - aggregation: select_first
    - domain: package
    - iogroup: MSRIOGroup
MSR::UNCORE_RATIO_LIMIT#
    - description: Refer to the Intel(R) 64 and IA-32 Architectures Software Developer's Manual for information about this MSR
    - units: none
    - aggregation: select_first
    - domain: package
    - iogroup: MSRIOGroup
MSR::UNCORE_RATIO_LIMIT:MAX_RATIO
    - description: Refer to the Intel(R) 64 and IA-32 Architectures Software Developer's Manual for information about this MSR
    - units: hertz
    - aggregation: select_first
    - domain: package
    - iogroup: MSRIOGroup
MSR::UNCORE_RATIO_LIMIT:MIN_RATIO
    - description: Refer to the Intel(R) 64 and IA-32 Architectures Software Developer's Manual for information about this MSR
    - units: hertz
    - aggregation: select_first
    - domain: package
    - iogroup: MSRIOGroup
POWER_DRAM
    - description: Average DRAM power over 40 ms or 8 control loop iterations
    - alias_for: ENERGY_DRAM rate of change
    - units: watts
    - aggregation: sum
    - domain: memory
    - iogroup: MSRIOGroup
POWER_PACKAGE
    - description: Average package power over 40 ms or 8 control loop iterations
    - alias_for: ENERGY_PACKAGE rate of change
    - units: watts
    - aggregation: sum
    - domain: package
    - iogroup: MSRIOGroup
POWER_PACKAGE_MAX
    - description: The maximum power limit based on the electrical specification.
    - alias_for: MSR::PKG_POWER_INFO:MAX_POWER
    - units: watts
    - aggregation: sum
    - domain: package
    - iogroup: MSRIOGroup
POWER_PACKAGE_MIN
    - description: The minimum power limit based on the electrical specification.
    - alias_for: MSR::PKG_POWER_INFO:MIN_POWER
    - units: watts
    - aggregation: sum
    - domain: package
    - iogroup: MSRIOGroup
POWER_PACKAGE_TDP
    - description: Maximum power to stay within the thermal limits based on the design (TDP).
    - alias_for: MSR::PKG_POWER_INFO:THERMAL_SPEC_POWER
    - units: watts
    - aggregation: sum
    - domain: package
    - iogroup: MSRIOGroup
QM_CTR_SCALED
    - description: Resource Monitor Data converted to bytes
    - alias_for: MSR::QM_CTR:RM_DATA multiplied by 90112 (provided by cpuid)
    - units: none
    - aggregation: sum
    - domain: package
    - iogroup: MSRIOGroup
QM_CTR_SCALED_RATE
    - description: Resource Monitor Data converted to bytes/second
    - alias_for: QM_CTR_SCALED rate of change
    - units: none
    - aggregation: sum
    - domain: package
    - iogroup: MSRIOGroup
TEMPERATURE_CORE
    - description: Core temperature
    - alias_for: Temperature derived from PROCHOT and MSR::THERM_STATUS:DIGITAL_READOUT
    - units: celsius
    - aggregation: average
    - domain: core
    - iogroup: MSRIOGroup
TEMPERATURE_PACKAGE
    - description: Package temperature
    - alias_for: Temperature derived from PROCHOT and MSR::PACKAGE_THERM_STATUS:DIGITAL_READOUT
    - units: celsius
    - aggregation: average
    - domain: package
    - iogroup: MSRIOGroup
TIME
    - description: Time since the start of application profiling.
    - units: seconds
    - aggregation: select_first
    - domain: cpu
    - iogroup: TimeIOGroup
TIME::ELAPSED
    - description: Time since the start of application profiling.
    - units: seconds
    - aggregation: select_first
    - domain: cpu
    - iogroup: TimeIOGroup
TIMESTAMP_COUNTER
    - description: An always running, monotonically increasing counter that is incremented at a constant rate.  For use as a wall clock timer.
    - alias_for: MSR::TIME_STAMP_COUNTER:TIMESTAMP_COUNT
    - units: none
    - aggregation: select_first
    - domain: cpu
    - iogroup: MSRIOGroup
