geopm_pio_levelzero(7) -- IOGroup providing signals and controls for Intel GPUs
===============================================================================

Description
-----------

The LevelZeroIOGroup implements the :doc:`geopm::IOGroup(3) <GEOPM_CXX_MAN_IOGroup.3>`
interface to provide hardware signals and controls for Intel GPUs.

Requirements
^^^^^^^^^^^^

To use the GEOPM LevelZero signals and controls GEOPM must be compiled against the oneAPI LevelZero libraries and must be run on a system with discrete GPUs supported by LevelZero.  To compile against the oneAPI LevelZero libraries geopm must be configured using the --enable-levelzero flag.  The optional --with-levelzero flag may be used to indicate the path of the required libraries.  In addition the user must export ZES_ENABLE_SYSMAN=1 as specified by the Intel oneAPI Level Zero Sysman documentation.  See the Sysman specification for more info on related environment variables and their usage.

Signals
-------

``LEVELZERO::GPU_CORE_FREQUENCY_STATUS``
    The current frequency of the GPU Compute Hardware.

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

``LEVELZERO::GPU_ENERGY``
    GPU energy in joules.

    *  **Aggregation**: sum
    *  **Domain**: gpu
    *  **Format**: double
    *  **Unit**: joules

``LEVELZERO::GPU_ENERGY_TIMESTAMP``
    Timestamp for the GPU energy read in seconds.

    *  **Aggregation**: sum
    *  **Domain**: gpu
    *  **Format**: double
    *  **Unit**: seconds

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
    average GPU power over 40ms (via geopmread) or 8 control loop iterations.  Derivative signal based on ``LEVELZERO::GPU_ENERGY``.

    *  **Aggregation**: average
    *  **Domain**: gpu
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

Aliases
-------

This IOGroup provides the following high-level aliases:

Signal Aliases
^^^^^^^^^^^^^^

``GPU_ENERGY``
    Maps to ``LEVELZERO::GPU_ENERGY``.

``GPU_POWER``
    Maps to ``LEVELZERO::GPU_POWER``.

``GPU_CORE_FREQUENCY_STATUS``
    Maps to ``LEVELZERO::GPU_CORE_FREQUENCY_STATUS``.

``GPU_CORE_ACTIVITY``
    Maps to ``LEVELZERO::GPU_CORE_UTILIZATION``.

``GPU_UNCORE_ACTIVITY``
    Maps to ``LEVELZERO::GPU_UNCORE_UTILIZATION``.


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
:doc:`geopm::IOGroup(3) <GEOPM_CXX_MAN_IOGroup.3>`\ ,
:doc:`geopmwrite(1) <geopmwrite.1>`\ ,
:doc:`geopmread(1) <geopmread.1>`
