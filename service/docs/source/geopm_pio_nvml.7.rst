geopm_pio_nvml(7) -- IOGroup providing signals and controls for NVIDIA GPUs
===========================================================================

Description
-----------

The NVMLIOGroup implements the :doc:`geopm::IOGroup(3) <GEOPM_CXX_MAN_IOGroup.3>`
interface to provide hardware signals and controls for NVIDIA GPUs.

Requirements
^^^^^^^^^^^^

To use the GEOPM NVML signals and controls GEOPM must be compiled against the NVML libraries and must be run on a system with hardware supported by NVML.  To compile against the NVML libraries geopm must be configured using the --enable-nvml flag.  The optional --with-nvml flag may be used to indicate the path of the required libraries.

Signals
-------

``NVML::GPU_CORE_FREQUENCY_STATUS``
    NVIDIA Streaming Multiprocessor (SM) frequency in hertz.

    *  **Aggregation**: average
    *  **Domain**: gpu
    *  **Format**: double
    *  **Unit**: hertz

``NVML::GPU_UTILIZATION``:
    Fraction of time the GPU operated on a kernel in the last set of driver samples.

    *  **Aggregation**: average
    *  **Domain**: gpu
    *  **Format**: double
    *  **Unit**: n/a

``NVML::GPU_POWER``:
    GPU power usage in watts.

    *  **Aggregation**: sum
    *  **Domain**: gpu
    *  **Format**: double
    *  **Unit**: watts

``NVML::GPU_UNCORE_FREQUENCY_STATUS``
    GPU memory frequency in hertz.

    *  **Aggregation**: average
    *  **Domain**: gpu
    *  **Format**: double
    *  **Unit**: hertz

``NVML::GPU_CORE_THROTTLE_REASONS``
    GPU clock throttling reasons.  Refer to NVIDIA NVML documentation for encoding information.

    *  **Aggregation**: average
    *  **Domain**: gpu
    *  **Format**: double
    *  **Unit**: n/a

``NVML::GPU_TEMPERATURE``
    GPU Temperature in degrees Celsius.

    *  **Aggregation**: average
    *  **Domain**: gpu
    *  **Format**: double
    *  **Unit**: celsius

``NVML::GPU_ENERGY_CONSUMPTION_TOTAL``
    GPU energy consumption in joules since the driver was loaded.

    *  **Aggregation**: sum
    *  **Domain**: gpu
    *  **Format**: double
    *  **Unit**: joules

``NVML::GPU_PERFORMANCE_STATE``
    GPU performance state, defined by the NVML API as a value from 0 to 15.

    *  **Aggregation**: expect_same
    *  **Domain**: gpu
    *  **Format**: double
    *  **Unit**: n/a

``NVML::GPU_PCIE_RX_THROUGHPUT``
    GPU PCIE receive throughput in bytes per second over a 20 millisecond period.

    *  **Aggregation**: sum
    *  **Domain**: gpu
    *  **Format**: double
    *  **Unit**: bytes/second

``NVML::GPU_PCIE_TX_THROUGHPUT``
    GPU PCIE transmit throughput in bytes per second over a 20 millisecond period.

    *  **Aggregation**: sum
    *  **Domain**: gpu
    *  **Format**: double
    *  **Unit**: bytes/second

``NVML::GPU_CPU_ACTIVE_AFFINITIZATION``
    GPU associated with the specified cpu as determined by querying active processes on the GPU.  If no GPUs map to the CPU -1 is returned.  If multiple GPUs map to the CPU, NAN is returned.

    *  **Aggregation**: expect_same
    *  **Domain**: cpu
    *  **Format**: double
    *  **Unit**: n/a

``NVML::GPU_UNCORE_UTILIZATION``
    Fraction of time the GPU memory was accessed in the last set of driver samples.

    *  **Aggregation**: average
    *  **Domain**: gpu
    *  **Format**: double
    *  **Unit**: n/a

``NVML::GPU_CORE_FREQUENCY_MAX_AVAIL``
    Streaming Multiprocessor maximum frequency in hertz.

    *  **Aggregation**: expect_same
    *  **Domain**: gpu
    *  **Format**: double
    *  **Unit**: hertz

``NVML::GPU_CORE_FREQUENCY_MIN_AVAIL``
    Streaming Multiprocessor minimum frequency in hertz.

    *  **Aggregation**: expect_same
    *  **Domain**: gpu
    *  **Format**: double
    *  **Unit**: hertz

``NVML::GPU_CORE_FREQUENCY_STEP``
    The Streaming Multiprocessor frequency step size in hertz.  The average step size is provided in the case where the step size is variable.

    *  **Aggregation**: expect_same
    *  **Domain**: gpu
    *  **Format**: double
    *  **Unit**: hertz

Controls
--------

Every control is exposed as a signal with the same name.  The relevant signal aggregation information is provided below.

``NVML::GPU_CORE_FREQUENCY_MIN_CONTROL``
    Sets the minimum frequency request for the Streaming Multiprocessor.

    *  **Aggregation**: expect_same
    *  **Domain**: gpu_chip
    *  **Format**: double
    *  **Unit**: hertz

``NVML::GPU_CORE_FREQUENCY_MAX_CONTROL``
    Sets the minimum frequency request for the Streaming Multiprocessor.

    *  **Aggregation**: expect_same
    *  **Domain**: gpu_chip
    *  **Format**: double
    *  **Unit**: hertz

``NVML::GPU_CORE_FREQUENCY_RESET_CONTROL``
    Resets Streaming Multiprocessor frequency min and max limits to default values.  Parameter provided is unused.

    *  **Aggregation**: expect_same
    *  **Domain**: gpu
    *  **Format**: double
    *  **Unit**: n/a

``NVML::GPU_POWER_LIMIT_CONTROL``
    Sets GPU power limit in watts.

    *  **Aggregation**: double
    *  **Domain**: gpu
    *  **Format**: sum
    *  **Unit**: watts

Aliases
-------

This IOGroup provides the following high-level aliases:

Signal Aliases
^^^^^^^^^^^^^^

``GPU_POWER``
    Maps to ``NVML::GPU_POWER``.

``GPU_CORE_FREQUENCY_STATUS``
    Maps to ``NVML::GPU_CORE_FREQUENCY_STATUS``.

``GPU_CORE_FREQUENCY_MIN_AVAIL``
    Maps to ``NVML::GPU_CORE_FREQUENCY_MIN_AVAIL``.

``GPU_CORE_FREQUENCY_MAX_AVAIL``
    Maps to ``NVML::GPU_CORE_FREQUENCY_MAX_AVAIL``.

``GPU_CORE_FREQUENCY_STEP``
    Maps to ``NVML::GPU_CORE_FREQUENCY_STEP``.

``GPU_ENERGY``
    Maps to ``NVML::GPU_ENERGY_CONSUMPTION_TOTAL``.

``GPU_TEMPERATURE``
    Maps to ``NVML::GPU_TEMPERATURE``.

``GPU_UTILIZATION``
    Maps to ``NVML::GPU_UTILIZATION``.

Control Aliases
^^^^^^^^^^^^^^^

``GPU_CORE_FREQUENCY_MAX_CONTROL``
    Maps to ``NVML::GPU_CORE_FREQUENCY_MAX_CONTROL``

``GPU_CORE_FREQUENCY_MIN_CONTROL``
    Maps to ``NVML::GPU_CORE_FREQUENCY_MIN_CONTROL``

``GPU_POWER_LIMIT_CONTROL``
    Maps to ``NVML::GPU_POWER_LIMIT_CONTROL``

See Also
--------


`NVML API <https://docs.nvidia.com/deploy/nvml-api/nvml-api-reference.html#nvml-api-reference>`_\ ,
:doc:`geopm(7) <geopm.7>`\ ,
:doc:`geopm::IOGroup(3) <GEOPM_CXX_MAN_IOGroup.3>`\ ,
:doc:`geopmwrite(1) <geopmwrite.1>`\ ,
:doc:`geopmread(1) <geopmread.1>`,
:doc:`geopm::Agg(3) <GEOPM_CXX_MAN_Agg.3>`
