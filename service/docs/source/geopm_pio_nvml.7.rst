

geopm::NVMLIOGroup(7) -- IOGroup providing signals and controls for NVIDIA GPUs
=================================================================================================

DESCRIPTION
-----------

The NVMLIOGroup implements the `geopm::IOGroup(3) <GEOPM_CXX_MAN_IOGroup.3.html>`_
interface to provide hardware signals and controls for NVIDIA GPUs.

REQUIREMENTS
~~~~~~~~~~~~
To use the GEOPM NVML signals and controls GEOPM must be compiled against the NVML libraries and must be run on a system with hardware supported by NVML.

SIGNALS
-------

* ``NVML::GPU_FREQUENCY_STATUS``: NVIDIA Streaming Multiprocessor (SM) frequency in hertz.

  *  ``Aggregation``: Average

  *  ``Domain``: GPU

  *  ``Format``: Double

  *  ``Unit``: Hertz
* ``NVML::GPU_UTILIZATION``: Fraction of time the GPU operated on a kernel in the last set of driver samples

  *  ``Aggregation``: Average

  *  ``Domain``: GPU

  *  ``Format``: Double

  *  ``Unit``: None
* ``NVML::GPU_POWER``: GPU Power usage in watts

  *  ``Aggregation``: Sum

  *  ``Domain``: GPU

  *  ``Format``: Double

  *  ``Unit``: Watts
* ``NVML::GPU_POWER_LIMIT_CONTROL``: GPU Power limit in watts

  *  ``Aggregation``: Sum

  *  ``Domain``: GPU

  *  ``Format``: Double

  *  ``Unit``: Watts
* ``NVML::GPU_MEMORY_FREQUENCY_STATUS``: GPU memory frequency in Hertz

  *  ``Aggregation``: Average

  *  ``Domain``: GPU

  *  ``Format``: Double

  *  ``Unit``: Hertz
* ``NVML::GPU_THROTTLE_REASONS``: GPU clock throttling reasons.  Refer to NVIDIA NVML documentation for encoding information.

  *  ``Aggregation``: Expect_same (note: this is incorrect.  File bug and fix)

  *  ``Domain``: GPU

  *  ``Format``: Double

  *  ``Unit``: None
* ``NVML::GPU_TEMPERATURE``: GPU Temperature in degrees Celsius

  *  ``Aggregation``: Average

  *  ``Domain``: GPU

  *  ``Format``: Double

  *  ``Unit``: Celsius
* ``NVML::GPU_ENERGY_CONSUMPTION_TOTAL``: GPU energy consumptoin in joules since the driver was loaded

  *  ``Aggregation``: Sum

  *  ``Domain``: GPU

  *  ``Format``: Double

  *  ``Unit``: Joules
* ``NVML::GPU_PERFORMANCE_STATE``: GPU performance state, defined by the NVML API as a value from 0 to 15

  *  ``Aggregation``: expect same

  *  ``Domain``: GPU

  *  ``Format``: Double

  *  ``Unit``: None
* ``NVML::GPU_PCIE_RX_THROUGHPUT``: GPU PCIE receive throughput in Bytes per Second over a 20 millisecond period

  *  ``Aggregation``: Sum

  *  ``Domain``: GPU

  *  ``Format``: Double

  *  ``Unit``: Bytes/Second
* ``NVML::GPU_PCIE_TX_THROUGHPUT``: GPU PCIE transmit throughput in Bytes per Second over a 20 millisecond period

  *  ``Aggregation``: Sum

  *  ``Domain``: GPU

  *  ``Format``: Double

  *  ``Unit``: Bytes/Second
* ``NVML::GPU_CPU_ACTIVE_AFFINITIZATION``: GPU associated with the specified cpu as determined by querying active processes on the GPU.  If no GPUs map to the CPU -1 is returned.  If multiple GPU map to the CPU NAN is returned.

  *  ``Aggregation``: Expect_same

  *  ``Domain``: CPU

  *  ``Format``: Double

  *  ``Unit``: None
* ``NVML::GPU_MEMORY_UTILIZATION``: Fraction of time the GPU memory was accessed in the last set of driver samples

  *  ``Aggregation``:

  *  ``Domain``: GPU

  *  ``Format``: Double

  *  ``Unit``: None
* ``NVML::GPU_FREQUENCY_MAX_AVAIL``: Streaming Multiprocessor maximum frequency in hertz

  *  ``Aggregation``: Expect_same

  *  ``Domain``: GPU

  *  ``Format``: Double

  *  ``Unit``: Hertz
* ``NVML::GPU_FREQUENCY_MIN_AVAIL``: Streaming Multiprocessor minimum frequency in hertz

  *  ``Aggregation``: Expect_same

  *  ``Domain``: GPU

  *  ``Format``: Double

  *  ``Unit``: Hertz
* ``NVML::GPU_FREQUENCY_CONTROL``: Latest frequency control request in hertz

  *  ``Aggregation``: Average

  *  ``Domain``: GPU

  *  ``Format``: Double

  *  ``Unit``: Hertz
* ``NVML::GPU_FREQUENCY_RESET_CONTROL``: Resets streaming Multiprocessor frequency min and max limits to default values

  *  ``Aggregation``: Average

  *  ``Domain``: GPU

  *  ``Format``: Double

  *  ``Unit``: None

SIGNAL ALIASES
~~~~~~~~~~~~~~~~
Several high level aliases are provided.  Their mapping to
underlying IO Group signals is provided below.

* ``GPU_POWER``: NVML::GPU_POWER

* ``GPU_CORE_FREQUENCY_STATUS``: NVML::GPU_FREQUENCY_STATUS

* ``GPU_CORE_FREQUENCY_MIN_AVAIL``: NVML::GPU_FREQUENCY_MIN_AVAIL

* ``GPU_CORE_FREQUENCY_MAX_AVAIL``: NVML::GPU_FREQUENCY_MAX_AVAIL

* ``GPU_ENERGY``: NVML::GPU_ENERGY_CONSUMPTION_TOTAL

* ``GPU_TEMPERATURE``: NVML::GPU_TEMPERATURE

* ``GPU_UTILIZATION``: NVML::GPU_UTILIZATION

* ``GPU_POWER_LIMIT_CONTROL``: NVML::GPU_POWER_LIMIT_CONTROL

* ``GPU_CORE_FREQUENCY_CONTROL``: NVML::GPU_FREQUENCY_CONTROL

CONTROLS
--------

* ``NVML::GPU_FREQUENCY_CONTROL``: Sets Streaming Multiprocessor frequency min and max to the same limit (in hertz)

  *  ``Aggregation``: Average

  *  ``Domain``: GPU

  *  ``Format``: Double

  *  ``Unit``: Hertz
* ``NVML::GPU_FREQUENCY_RESET_CONTROL``: Resets Streaming Multiprocessor frequency min and max limits to default values.  Parameter provided is unused.


  *  ``Aggregation``: Average

  *  ``Domain``: GPU

  *  ``Format``: Double

  *  ``Unit``: None
* ``NVML::GPU_POWER_LIMIT_CONTROL``: Sets GPU power limit in watts

  *  ``Aggregation``: Double

  *  ``Domain``: GPU

  *  ``Format``: Sum

  *  ``Unit``: Watts

CONTROL ALIASES
~~~~~~~~~~~~~~~~
Several high level aliases are provided.  Their mapping to
underlying IO Group signals is provided below.

* ``GPU_POWER_LIMIT_CONTROL``:  NVML::GPU_POWER_LIMIT_CONTROL

* ``GPU_CORE_FREQUENCY_CONTROL``: NVML::GPU_FREQUENCY_CONTROL

SEE ALSO
--------

`geopm(7) <geopm.7.html>`_\ ,
`geopm::IOGroup(3) <GEOPM_CXX_MAN_IOGroup.3.html>`_\ ,
`geopmwrite(1) <geopmwrite.1.html>`_\ ,
`geopmread(1) <geopmread.1.html>`_
