geopm_pio_levelzero(7) -- IOGroup providing signals and controls for Intel GPUs
===============================================================================

Description
-----------

The LevelZeroIOGroup implements the :doc:`geopm::IOGroup(3) <geopm::IOGroup.3>`
interface to provide hardware signals and controls for Intel GPUs.

Requirements
^^^^^^^^^^^^

To use the GEOPM LevelZero signals and controls GEOPM must be compiled against the oneAPI LevelZero libraries and must be run on a system with discrete GPUs supported by LevelZero.  To compile against the oneAPI LevelZero libraries geopm must be configured using the --enable-levelzero flag.  The optional --with-levelzero flag may be used to indicate the path of the required libraries.  In addition the user must export ZES_ENABLE_SYSMAN=1 as specified by the Intel oneAPI Level Zero Sysman documentation.  See the Sysman specification for more info on related environment variables and their usage.

Since signals and controls are exposed via the Sysman API they will be impacted by Sysman environment variables.  Please review `oneAPI LevelZero Sysman Environment Variables <https://spec.oneapi.io/level-zero/latest/sysman/PROG.html#environment-variables>`_ and `oneAPI LevelZero Core Programming Guide Environment Variables <https://spec.oneapi.io/level-zero/latest/core/PROG.html#environment-variables>`_.

Note on RAS Signals
^^^^^^^^^^^^^^^^^^^

The Monitoring of RAS counters have a high overhead (0.5 seconds each to read). And so, reporting of any errors while monitoring these signals (for e.g., due to unsupported firmware) will be delayed until the user attempts to actually read any of these signals.


Signals
-------



``LEVELZERO::GPU_CORE_FREQUENCY_STATUS``
    The current frequency of the GPU Compute Hardware.

    *  **Aggregation**: average
    *  **Domain**: gpu_chip
    *  **Format**: double
    *  **Unit**: hertz

``LEVELZERO::GPU_CORE_FREQUENCY_EFFICIENT``
    The efficient minimum frequency of the GPU Compute Hardware.

    *  **Aggregation**: average
    *  **Domain**: gpu_chip
    *  **Format**: double
    *  **Unit**: hertz

``LEVELZERO::GPU_CORE_FREQUENCY_MAX_AVAIL``
    The maximum supported frequency of the GPU Compute Hardware.

    *  **Aggregation**: expect_same
    *  **Domain**: gpu_chip
    *  **Format**: double
    *  **Unit**: hertz

``LEVELZERO::GPU_CORE_FREQUENCY_MIN_AVAIL``
    The minimum supported frequency of the GPU Compute Hardware.

    *  **Aggregation**: expect_same
    *  **Domain**: gpu_chip
    *  **Format**: double
    *  **Unit**: hertz

``LEVELZERO::GPU_CORE_TEMPERATURE_MAXIMUM``
    The maximum measured temperature across all sensors in the GPU accelerator."

    *  **Aggregation**: max
    *  **Domain**: gpu_chip
    *  **Format**: double
    *  **Unit**: celsius

``LEVELZERO::GPU_MEMORY_TEMPERATURE_MAXIMUM``
    The maximum measured temperature across all sensors in the GPU memory."

    *  **Aggregation**: max
    *  **Domain**: gpu_chip
    *  **Format**: double
    *  **Unit**: celsius

``LEVELZERO::GPU_CORE_FREQUENCY_STEP``
    The GPU Compute Hardware frequency step size in hertz.  The average step size is provided in the case where the step size is variable.

    *  **Aggregation**: expect_same
    *  **Domain**: gpu
    *  **Format**: double
    *  **Unit**: hertz

``LEVELZERO::GPU_ENERGY``
    GPU energy in joules.

    *  **Aggregation**: sum
    *  **Domain**: gpu
    *  **Format**: double
    *  **Unit**: joules

``LEVELZERO::GPU_CORE_ENERGY``
    GPU Compute Hardware chip energy in joules.

    *  **Aggregation**: sum
    *  **Domain**: gpu_chip for multi-chip systems or gpu for single chip per gpu systems
    *  **Format**: double
    *  **Unit**: joules

``LEVELZERO::GPU_CORE_ENERGY_TIMESTAMP``
    GPU compute hardware domain energy timestamp in seconds.  Value cached on LEVELZERO::GPU_CORE_ENERGY read.

    *  **Aggregation**: sum
    *  **Domain**: gpu_chip for multi-chip systems or gpu for single chip per gpu systems
    *  **Format**: double
    *  **Unit**: seconds

``LEVELZERO::GPU_ENERGY_TIMESTAMP``
    Timestamp for the GPU energy read in seconds.

    *  **Aggregation**: sum
    *  **Domain**: gpu
    *  **Format**: double
    *  **Unit**: seconds

``LEVELZERO::GPU_CORE_PERFORMANCE_FACTOR``
    Performance Factor of the GPU Compute Hardware Domain. Expresses a trade-off between energy provided to the GPU compute hardware and the supporting units.  A value of 1 indicates a compute focused energy trade-off, a value of 0 indicates a memory focused energy trade-off.  Default value is 0.5

    *  **Aggregation**: averge
    *  **Domain**: gpu_chip for multi-chip systems or gpu for single chip per gpu systems
    *  **Format**: double
    *  **Unit**: none

``LEVELZERO::GPU_UNCORE_FREQUENCY_STATUS``
    The current frequency of the GPU Memory hardware.

    *  **Aggregation**: average
    *  **Domain**: gpu_chip
    *  **Format**: double
    *  **Unit**: hertz

``LEVELZERO::GPU_UNCORE_FREQUENCY_MAX_AVAIL``
    The maximum supported frequency of the GPU Memory Hardware.

    *  **Aggregation**: expect_same
    *  **Domain**: gpu_chip
    *  **Format**: double
    *  **Unit**: hertz

``LEVELZERO::GPU_UNCORE_FREQUENCY_MIN_AVAIL``
    The minimum supported frequency of the GPU Memory Hardware.

    *  **Aggregation**: expect_same
    *  **Domain**: gpu_chip
    *  **Format**: double
    *  **Unit**: hertz

``LEVELZERO::GPU_POWER_LIMIT_DEFAULT``
    Default power limit of the GPU in watts.

    *  **Aggregation**: sum
    *  **Domain**: gpu
    *  **Format**: double
    *  **Unit**: watts

``LEVELZERO::GPU_POWER_LIMIT_MIN_AVAIL``
    The minimum supported power limit in watts.

    *  **Aggregation**: sum
    *  **Domain**: gpu
    *  **Format**: double
    *  **Unit**: watts

``LEVELZERO::GPU_POWER_LIMIT_MAX_AVAIL``
    The maximum supported power limit in watts.

    *  **Aggregation**: sum
    *  **Domain**: gpu
    *  **Format**: double
    *  **Unit**: watts

``LEVELZERO::GPU_RAS_RESET_COUNT_CORRECTABLE``
    The number of correctable accelerator engine resets by the driver.

    *  **Aggregation**: sum
    *  **Domain**: gpu_chip
    *  **Format**: double
    *  **Unit**: none

``LEVELZERO::GPU_RAS_PROGRAMMING_ERRCOUNT_CORRECTABLE``
    The number of correctable hardware exceptions generated by the way workloads have programmed the hardware.

    *  **Aggregation**: sum
    *  **Domain**: gpu_chip
    *  **Format**: double
    *  **Unit**: none

``LEVELZERO::GPU_RAS_DRIVER_ERRCOUNT_CORRECTABLE``
    The number of correctable low level driver communication errors.

    *  **Aggregation**: sum
    *  **Domain**: gpu_chip
    *  **Format**: double
    *  **Unit**: none

``LEVELZERO::GPU_RAS_COMPUTE_ERRCOUNT_CORRECTABLE``
    The number of correctable errors in the compute accelerator hardware.

    *  **Aggregation**: sum
    *  **Domain**: gpu_chip
    *  **Format**: double
    *  **Unit**: none

``LEVELZERO::GPU_RAS_NONCOMPUTE_ERRCOUNT_CORRECTABLE``
    The number of correctable errors in the fixed-function accelerator hardware.

    *  **Aggregation**: sum
    *  **Domain**: gpu_chip
    *  **Format**: double
    *  **Unit**: none

``LEVELZERO::GPU_RAS_CACHE_ERRCOUNT_CORRECTABLE``
    The number of correctable errors in caches (L1/L3/register file/shared local memory/sampler).

    *  **Aggregation**: sum
    *  **Domain**: gpu_chip
    *  **Format**: double
    *  **Unit**: none

``LEVELZERO::GPU_RAS_DISPLAY_ERRCOUNT_CORRECTABLE``
    The number of correctable errors in the display.

    *  **Aggregation**: sum
    *  **Domain**: gpu_chip
    *  **Format**: double
    *  **Unit**: none

``LEVELZERO::GPU_RAS_RESET_COUNT_UNCORRECTABLE``
    The number of uncorrectable accelerator engine resets by the driver.

    *  **Aggregation**: sum
    *  **Domain**: gpu_chip
    *  **Format**: double
    *  **Unit**: none

``LEVELZERO::GPU_RAS_PROGRAMMING_ERRCOUNT_UNCORRECTABLE``
    The number of uncorrectable hardware exceptions generated by the way workloads have programmed the hardware.

    *  **Aggregation**: sum
    *  **Domain**: gpu_chip
    *  **Format**: double
    *  **Unit**: none

``LEVELZERO::GPU_RAS_DRIVER_ERRCOUNT_UNCORRECTABLE``
    The number of uncorrectable low level driver communication errors.

    *  **Aggregation**: sum
    *  **Domain**: gpu_chip
    *  **Format**: double
    *  **Unit**: none

``LEVELZERO::GPU_RAS_COMPUTE_ERRCOUNT_UNCORRECTABLE``
    The number of uncorrectable errors in the compute accelerator hardware.

    *  **Aggregation**: sum
    *  **Domain**: gpu_chip
    *  **Format**: double
    *  **Unit**: none

``LEVELZERO::GPU_RAS_NONCOMPUTE_ERRCOUNT_UNCORRECTABLE``
    The number of uncorrectable errors in the fixed-function accelerator hardware.

    *  **Aggregation**: sum
    *  **Domain**: gpu_chip
    *  **Format**: double
    *  **Unit**: none

``LEVELZERO::GPU_RAS_CACHE_ERRCOUNT_UNCORRECTABLE``
    The number of uncorrectable errors in caches (L1/L3/register file/shared local memory/sampler).

    *  **Aggregation**: sum
    *  **Domain**: gpu_chip
    *  **Format**: double
    *  **Unit**: none

``LEVELZERO::GPU_RAS_DISPLAY_ERRCOUNT_UNCORRECTABLE``
    The number of uncorrectable errors in the display.

    *  **Aggregation**: sum
    *  **Domain**: gpu_chip
    *  **Format**: double
    *  **Unit**: none

``LEVELZERO::GPU_ACTIVE_TIME``
    Time in seconds that this resource is actively running a workload.  See the Intel oneAPI Level Zero Sysman documentation for more info.

    *  **Aggregation**: sum
    *  **Domain**: gpu_chip
    *  **Format**: double
    *  **Unit**: seconds

``LEVELZERO::GPU_ACTIVE_TIME_TIMESTAMP``
    The timestamp for the ``LEVELZERO::GPU_ACTIVE_TIME`` read in seconds.  See the Intel oneAPI Level Zero Sysman documentation for more info.

    *  **Aggregation**: sum
    *  **Domain**: gpu_chip
    *  **Format**: double
    *  **Unit**: seconds

``LEVELZERO::GPU_CORE_ACTIVE_TIME``
    Time in seconds that the GPU compute engines (EUs) are actively running a workload.  See the Intel oneAPI Level Zero Sysman documentation for more info.

    *  **Aggregation**: sum
    *  **Domain**: gpu_chip
    *  **Format**: double
    *  **Unit**: seconds

``LEVELZERO::GPU_CORE_ACTIVE_TIME_TIMESTAMP``
    The timestamp for the ``LEVELZERO::GPU_CORE_ACTIVE_TIME`` signal read in seconds.  See the Intel oneAPI Level Zero Sysman documentation for more info.

    *  **Aggregation**: sum
    *  **Domain**: gpu_chip
    *  **Format**: double
    *  **Unit**: seconds

``LEVELZERO::GPU_UNCORE_ACTIVE_TIME``
    Time in seconds that the GPU copy engines are actively running a workload.  See the Intel oneAPI Level Zero Sysman documentation for more info.

    *  **Aggregation**: sum
    *  **Domain**: gpu_chip
    *  **Format**: double
    *  **Unit**: seconds

``LEVELZERO::GPU_UNCORE_ACTIVE_TIME_TIMESTAMP``
    The timestamp for the ``LEVELZERO::GPU_UNCORE_ACTIVE_TIME`` signal read in seconds.  See the Intel oneAPI Level Zero Sysman documentation for more info.

    *  **Aggregation**: sum
    *  **Domain**: gpu_chip
    *  **Format**: double
    *  **Unit**: seconds

``LEVELZERO::GPU_POWER``
    Average GPU power over 40ms (via geopmread) or 8 control loop iterations.  Derivative signal based on ``LEVELZERO::GPU_ENERGY``.

    *  **Aggregation**: average
    *  **Domain**: gpu
    *  **Format**: double
    *  **Unit**: watts

``LEVELZERO::GPU_CORE_POWER``
    Average GPU Compute Hardware power over 40ms (via geopmread) or 8 control loop iterations.  Derivative signal based on ``LEVELZERO::GPU_CORE_ENERGY``.

    *  **Aggregation**: average
    *  **Domain**: gpu_chip
    *  **Format**: double
    *  **Unit**: watts

``LEVELZERO::GPU_UTILIZATION``
    Utilization of all GPU engines.  Level Zero logical engines may map to the same hardware, resulting in a reduced signal range (i.e. less than 0 to 1) in some cases.  See the LevelZero Sysman Engine documentation for more info.

    *  **Aggregation**: average
    *  **Domain**: gpu
    *  **Format**: double
    *  **Unit**: none

``LEVELZERO::GPU_CORE_UTILIZATION``
    Utilization of the GPU Compute Engines (EUs).  Level Zero logical engines may map to the same hardware, resulting in a reduced signal range (i.e. less than 0 to 1) in some cases.  See the LevelZero Sysman Engine documentation for more info.

    *  **Aggregation**: average
    *  **Domain**: gpu_chip
    *  **Format**: double
    *  **Unit**: none

``LEVELZERO::GPU_UNCORE_UTILIZATION``
    Utilization of the GPU Copy Engines.  Level Zero logical engines may map to the same hardware, resulting in a reduced signal range (i.e. less than 0 to 1) in some cases.  See the LevelZero Sysman Engine documentation for more info.

    *  **Aggregation**: average
    *  **Domain**: gpu_chip
    *  **Format**: double
    *  **Unit**: none

``LEVELZERO::GPU_CORE_THROTTLE_REASONS``
    GPU Compute Hardware throttle reasons.  See oneAPI Level Zero Sysman Spec for decoding.

    *  **Aggregation**: integer_bitwise_or
    *  **Domain**: gpu_chip
    *  **Format**: integer
    *  **Unit**: none

Controls
--------
Every control is exposed as a signal with the same name.  The relevant signal aggregation information is provided below.

``LEVELZERO::GPU_CORE_FREQUENCY_MIN_CONTROL``
    Sets the minimum frequency request for the GPU Compute Hardware.

    *  **Aggregation**: expect_same
    *  **Domain**: gpu_chip
    *  **Format**: double
    *  **Unit**: hertz

``LEVELZERO::GPU_CORE_FREQUENCY_MAX_CONTROL``
    Sets the minimum frequency request for the GPU Compute Hardware.

    *  **Aggregation**: expect_same
    *  **Domain**: gpu_chip
    *  **Format**: double
    *  **Unit**: hertz

``LEVELZERO::GPU_CORE_PERFORMANCE_FACTOR_CONTROL``
    Performance Factor of the GPU Compute Hardware Domain. Expresses a trade-off between energy provided to the GPU compute hardware and the supporting units.  A value of 1 indicates a compute focused energy trade-off, a value of 0 indicates a memory focused energy trade-off.  Default value is 0.5

    *  **Aggregation**: averge
    *  **Domain**: gpu_chip
    *  **Format**: double
    *  **Unit**: none


Aliases
-------

This IOGroup provides the following high-level aliases:

Signal Aliases
^^^^^^^^^^^^^^

``GPU_ENERGY``
    Maps to ``LEVELZERO::GPU_ENERGY``.

``GPU_POWER``
    Maps to ``LEVELZERO::GPU_POWER``.

``GPU_CORE_ENERGY``
    Maps to ``LEVELZERO::GPU_CORE_ENERGY``.

``GPU_CORE_POWER``
    Maps to ``LEVELZERO::GPU_CORE_POWER``.

``GPU_UTILIZATION``
    Maps to ``LEVELZERO::GPU_UTILIZATION``.

``GPU_CORE_ACTIVITY``
    Maps to ``LEVELZERO::GPU_CORE_UTILIZATION``.

``GPU_UNCORE_ACTIVITY``
    Maps to ``LEVELZERO::GPU_UNCORE_UTILIZATION``.

``GPU_CORE_FREQUENCY_STATUS``
    Maps to ``LEVELZERO::GPU_CORE_FREQUENCY_STATUS``.

``GPU_CORE_FREQUENCY_MIN_AVAIL``
    Maps to ``LEVELZERO::GPU_CORE_FREQUENCY_MIN_AVAIL``.

``GPU_CORE_FREQUENCY_MAX_AVAIL``
    Maps to ``LEVELZERO::GPU_CORE_FREQUENCY_MAX_AVAIL``.

``GPU_CORE_FREQUENCY_MIN_CONTROL``
    Maps to ``LEVELZERO::GPU_CORE_FREQUENCY_MIN_CONTROL``.

``GPU_CORE_FREQUENCY_MAX_CONTROL``
    Maps to ``LEVELZERO::GPU_CORE_FREQUENCY_MAX_CONTROL``.

``GPU_CORE_FREQUENCY_STEP``
    Maps to ``LEVELZERO::GPU_CORE_FREQUENCY_STEP``.

``LEVELZERO::GPU_CORE_PERFORMANCE_FACTOR_CONTROL``
    Maps to ``LEVELZERO::GPU_CORE_PERFORMANCE_FACTOR``
    Writes to performance factor may not be granted.  To confirm the actual
    control setting the signal must be read.

Control Aliases
^^^^^^^^^^^^^^^

``GPU_CORE_FREQUENCY_MAX_CONTROL``
    Maps to ``LEVELZERO::GPU_CORE_FREQUENCY_MAX_CONTROL``

``GPU_CORE_FREQUENCY_MIN_CONTROL``
    Maps to ``LEVELZERO::GPU_CORE_FREQUENCY_MIN_CONTROL``


See Also
--------


`oneAPI LevelZero Sysman <https://spec.oneapi.io/level-zero/latest/sysman/PROG.html>`_\ ,
:doc:`geopm(7) <geopm.7>`\ ,
:doc:`geopm::IOGroup(3) <geopm::IOGroup.3>`\ ,
:doc:`geopmwrite(1) <geopmwrite.1>`\ ,
:doc:`geopmread(1) <geopmread.1>`
