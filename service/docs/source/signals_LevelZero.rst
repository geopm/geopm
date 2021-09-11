LevelZero Platform Signals
==========================

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
CPU_FREQUENCY_MIN
    - description: Minimum processor frequency
    - alias_for: CPUINFO::FREQ_MIN
    - units: hertz
    - aggregation: expect_same
    - domain: board
    - iogroup: CpuinfoIOGroup
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
FREQUENCY_ACCELERATOR
Accelerator compute/GPU domain frequency in hertz
    - alias_for: LEVELZERO::FREQUENCY_GPU
FREQUENCY_MIN
    - description: Minimum processor frequency
    - alias_for: CPUINFO::FREQ_MIN
    - units: hertz
    - aggregation: expect_same
    - domain: board
    - iogroup: CpuinfoIOGroup
FREQUENCY_STEP
    - description: Step size between process frequency settings
    - alias_for: CPUINFO::FREQ_STEP
    - units: hertz
    - aggregation: expect_same
    - domain: board
    - iogroup: CpuinfoIOGroup
FREQUENCY_STICKER
    - description: Processor base frequency
    - alias_for: CPUINFO::FREQ_STICKER
    - units: hertz
    - aggregation: expect_same
    - domain: board
    - iogroup: CpuinfoIOGroup
LEVELZERO::ACTIVE_TIME
    - GPU active time
LEVELZERO::ACTIVE_TIME_COMPUTE
    - GPU Compute engine active time
LEVELZERO::ACTIVE_TIME_COMPUTE_TIMESTAMP
    - GPU Compute engine active time reading timestamp
Value cached on LEVELZERO::ACTIVE_TIME_COMPUTE read
    - LEVELZERO::ACTIVE_TIME_COPY
GPU Copy engine active time
    - LEVELZERO::ACTIVE_TIME_COPY_TIMESTAMP
GPU Copy engine active time timestamp
    - Value cached on LEVELZERO::ACTIVE_TIME_COPY read
LEVELZERO::ACTIVE_TIME_TIMESTAMP
    - GPU active time reading timestamp
      Value cached on LEVELZERO::ACTIVE_TIME read
LEVELZERO::ENERGY
    - Accelerator energy in Joules
LEVELZERO::ENERGY_TIMESTAMP
    - Accelerator energy timestamp in seconds
      Value cached on LEVELZERO::ENERGY read
LEVELZERO::FREQUENCY_GPU
    - Accelerator compute/GPU domain frequency in hertz
LEVELZERO::FREQUENCY_MAX_GPU
    - Accelerator compute/GPU domain maximum frequency in hertz
LEVELZERO::FREQUENCY_MAX_MEMORY
    - Accelerator memory domain maximum frequency in hertz
LEVELZERO::FREQUENCY_MEMORY
    - Accelerator memory domain frequency in hertz
LEVELZERO::FREQUENCY_MIN_GPU
    - Accelerator compute/GPU domain minimum frequency in hertz
LEVELZERO::FREQUENCY_MIN_MEMORY
    - Accelerator memory domain minimum frequency in hertz
LEVELZERO::POWER
    - Average accelerator power over 40 ms or 8 control loop
      iterations
    - alias_for: LEVELZERO::ENERGY rate of change
LEVELZERO::POWER_LIMIT_DEFAULT
    - Default power limit in Watts
LEVELZERO::POWER_LIMIT_MAX
    - Maximum power limit in Watts
LEVELZERO::POWER_LIMIT_MIN
    - Minimum power limit in Watts
LEVELZERO::UTILIZATION
    - GPU utilizationn Level Zero logical engines may map to the same
      hardware resulting in a reduced signal range (i.e. not 0 to 1)
    - alias_for: LEVELZERO::ACTIVE_TIME rate of change
LEVELZERO::UTILIZATION_COMPUTE
    - Compute engine utilizationn Level Zero logical engines may map
      to the same hardware resulting in a reduced signal range
      (i.e. not 0 to 1)
    - alias_for: LEVELZERO::ACTIVE_TIME_COMPUTE rate of change
LEVELZERO::UTILIZATION_COPY
    - Copy engine utilizationn Level Zero logical engines may map to
      the same hardware resulting in a reduced signal range (i.e. not
      0 to 1)
    - alias_for: LEVELZERO::ACTIVE_TIME_COPY rate of change
POWER_ACCELERATOR
    - Average accelerator power over 40 ms or 8 control loop
      iterations
    - alias_for: LEVELZERO::ENERGY rate of change
    - alias_for: LEVELZERO::POWER
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
