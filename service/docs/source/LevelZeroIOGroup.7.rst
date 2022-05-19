.. role:: raw-html-m2r(raw)
   :format: html


geopm::LevelZeroIOGroup(7) -- IOGroup providing signals and controls for Intel GPUs 
=================================================================================================

DESCRIPTION
-----------

The LevelZeroIOGroup implements the `geopm::IOGroup(3) <GEOPM_CXX_MAN_IOGroup.3.html>`_ 
interface to provide hardware signals and controls for Intel GPUs.

ENABLING LevelZero
~~~~~~~~~~~~~~~
To enable the LevelZeroIOGroup geopm must be configured with --enable-levelzero. 

USING LevelZero
~~~~~~~~~~~~~~~
The LevelZerorIOGroup uses Sysman to access GPU signals and controls.  Due to this the user must export ZES_ENABLE_SYSMAN=1 as specified by the Intel oneAPI Level Zero Sysman documentation.  See the Sysman specification for more info on related environment variables and their usage.

SIGNALS
-------

* ``LEVELZERO::GPU_CORE_FREQUENCY_STATUS``: The current frequency of the GPU Compute Hardware
  
  *  ``Aggregation``: Average
  
  *  ``Format``: Double
  
  *  ``Unit``: Hertz
* ``LEVELZERO::GPU_CORE_FREQUENCY_MAX_AVAIL``: The maximum supported frequency of the GPU Compute Hardware 
  
  *  ``Aggregation``: Average
  
  *  ``Format``: Double
  
  *  ``Unit``: Hertz
* ``LEVELZERO::GPU_CORE_FREQUENCY_MIN_AVAIL``: The minimum supported frequency of the GPU Compute Hardware 
  
  *  ``Aggregation``: Average
  
  *  ``Format``: Double
  
  *  ``Unit``: Hertz
* ``LEVELZERO::GPU_CORE_FREQUENCY_MAX_CONTROL``: The maximum supported frequency of the GPU Compute Hardware 
  
  *  ``Aggregation``: Average
  
  *  ``Format``: Double
  
  *  ``Unit``: Hertz
* ``LEVELZERO::GPU_CORE_FREQUENCY_MIN_CONTROL``: The minimum supported frequency of the GPU Compute Hardware 
  
  *  ``Aggregation``: Average
  
  *  ``Format``: Double
  
  *  ``Unit``: Hertz
* ``LEVELZERO::GPU_ENERGY``: GPU energy in Joules
  
  *  ``Aggregation``: Average (TODO: ensure NVML and LEVELZERO use the same Aggregation)
  
  *  ``Format``: Double
  
  *  ``Unit``: Joules
* ``LEVELZERO::GPU_ENERGY_TIMESTAMP``: Timestamp for the GPU energy read in seconds
  
  *  ``Aggregation``: Average 
  
  *  ``Format``: Double
  
  *  ``Unit``: Seconds
* ``LEVELZERO::GPU_UNCORE_FREQUENCY_STATUS``: The current frequency of the GPU Memory hardware 
  
  *  ``Aggregation``: Average
  
  *  ``Format``: Double
  
  *  ``Unit``: Hertz
* ``LEVELZERO::GPU_UNCORE_FREQUENCY_MAX_AVAIL``: The maximum supported frequency of the GPU Memory Hardware 
  
  *  ``Aggregation``: Average
  
  *  ``Format``: Double
  
  *  ``Unit``: Hertz
* ``LEVELZERO::GPU_UNCORE_FREQUENCY_MIN_AVAIL``: The minimum supported frequency of the GPU Memory Hardware 
  
  *  ``Aggregation``: Average
  
  *  ``Format``: Double
  
  *  ``Unit``: Hertz
* ``LEVELZERO::GPU_POWER_LIMIT_DEFAULT``: Default power limit of the GPU in Watts
  
  *  ``Aggregation``: Average (TODO: confirm consistency with NVML Aggregation)
  
  *  ``Format``: Double
  
  *  ``Unit``: Watts
* ``LEVELZERO::GPU_POWER_LIMIT_MIN_AVAIL``: The minimum supported power limit in Watts
  
  *  ``Aggregation``: Average
  
  *  ``Format``: Double
  
  *  ``Unit``: Watts
* ``LEVELZERO::GPU_POWER_LIMIT_MAX_AVAIL``:  The maximum supported power limit in Watts
  
  *  ``Aggregation``: Average
  
  *  ``Format``: Double
  
  *  ``Unit``: Watts
* ``LEVELZERO::GPU_ACTIVE_TIME``: Time in seconds that this resource is actively running a workload.  See the Intel oneAPI Level Zero Sysman documentation for more info.
  
  *  ``Aggregation``: Average
  
  *  ``Format``: Double
  
  *  ``Unit``: Seconds
* ``LEVELZERO::GPU_ACTIVE_TIME_TIMESTAMP``: The timestamp for the ``LEVELZERO::GPU_ACTIVE_TIME`` read in seconds.  See the Intel oneAPI Level Zero Sysman documentation for more info.
  
  *  ``Aggregation``: Average
  
  *  ``Format``: Double
  
  *  ``Unit``: Seconds
* ``LEVELZERO::GPU_CORE_ACTIVE_TIME``: Time in seconds that the GPU compute engines (EUs) are actively running a workload.  See the Intel oneAPI Level Zero Sysman documentation for more info.
  
  *  ``Aggregation``: Average
  
  *  ``Format``: Double
  
  *  ``Unit``: Seconds
* ``LEVELZERO::GPU_CORE_ACTIVE_TIME_TIMESTAMP``: The timestamp for the ``LEVELZERO::GPU_CORE_ACTIVE_TIME`` signal read in seconds.  See the Intel oneAPI Level Zero Sysman documentation for more info.
  
  *  ``Aggregation``: Average
  
  *  ``Format``: Double
  
  *  ``Unit``: Seconds
* ``LEVELZERO::GPU_UNCORE_ACTIVE_TIME``: Time in seconds that the GPU copy engines are actively running a workload.  See the Intel oneAPI Level Zero Sysman documentation for more info.
  
  *  ``Aggregation``: Average
  
  *  ``Format``: Double
  
  *  ``Unit``: Seconds
* ``LEVELZERO::GPU_UNCORE_ACTIVE_TIME_TIMESTAMP``: The timestamp for the ``LEVELZERO::GPU_UNCORE_ACTIVE_TIME`` signal read in seconds.  See the Intel oneAPI Level Zero Sysman documentation for more info.
  
  *  ``Aggregation``: Average
  
  *  ``Format``: Double
  
  *  ``Unit``: Seconds
* ``LEVELZERO::GPU_CORE_FREQUENCY_CONTROL``: The last frequency request for the GPU Compute Hardware
  
  *  ``Aggregation``: Average
  
  *  ``Format``: Double 
  
  *  ``Unit``: Hertz
* ``LEVELZERO::GPU_CORE_FREQUENCY_MIN_CONTROL``: The last minimum frequency request set for the GPU Compute Hardware 
  
  *  ``Aggregation``: Average
  
  *  ``Format``: Double
  
  *  ``Unit``: Hertz
* ``LEVELZERO::GPU_CORE_FREQUENCY_MAX_CONTROL``: The last maximum frequency request set for the GPU Compute Hardware 
  
  *  ``Aggregation``: Average
  
  *  ``Format``: Double
  
  *  ``Unit``: Hertz
* ``LEVELZERO::GPU_POWER``: Average GPU power over 40ms (via geopmread) or 8 control loop iterations.  Derivative signal based on LEVELZERO::GPU_ENERGY
  
  *  ``Aggregation``: Average 
  
  *  ``Format``: Double
  
  *  ``Unit``: Watts
* ``LEVELZERO::GPU_UTILIZATION``: Utilization of all GPU engines.  Level Zero logical engines may map to the same hardware, resulting in a reduced signal range (i.e. less than 0 to 1) in some cases.  See the LevelZero Sysman Engine documentation for more info. 
  
  *  ``Aggregation``: Average
  
  *  ``Format``: Double
  
  *  ``Unit``: None
* ``LEVELZERO::GPU_CORE_UTILIZATION``: Utilization of the GPU Compute Engines (EUs).  Level Zero logical engines may map to the same hardware, resulting in a reduced signal range (i.e. less than 0 to 1) in some cases.  See the LevelZero Sysman Engine documentation for more info. 
  
  *  ``Aggregation``: Average
  
  *  ``Format``: Double
  
  *  ``Unit``: None
* ``LEVELZERO::GPU_UNCORE_UTILIZATION``: Utilization of the GPU Copy Engines.  Level Zero logical engines may map to the same hardware, resulting in a reduced signal range (i.e. less than 0 to 1) in some cases.  See the LevelZero Sysman Engine documentation for more info. 
  
  *  ``Aggregation``: Average
  
  *  ``Format``: Double
  
  *  ``Unit``: None

SIGNAL ALIASES
~~~~~~~~~~~~~~~~
Several high level aliases are provided.  Their mapping  to
underlying IO Group signals is provided below.

* ``GPU_ENERGY``: LEVELZERO::GPU_ENERGY
* ``GPU_POWER``: LEVELZERO::GPU_POWER
* ``GPU_CORE_FREQUENCY_CONTROL``: LEVELZERO::GPU_CORE_FREQUENCY_CONTROL

CONTROLS
--------

* ``LEVELZERO::GPU_CORE_FREQUENCY_MIN_CONTROL``: Sets the minimum frequency request for the GPU Compute Hardware 
  
  *  ``Aggregation``: Average
  
  *  ``Format``: Double
  
  *  ``Unit``: Hertz
* ``LEVELZERO::GPU_CORE_FREQUENCY_MAX_CONTROL``: Sets the minimum frequency request for the GPU Compute Hardware 

  *  ``Aggregation``: Average
  
  *  ``Format``: Double
  
  *  ``Unit``: Hertz
* ``LEVELZERO::GPU_CORE_FREQUENCY_CONTROL``: Sets both the minimum and maximum frequency request for the GPU Compute Hardware to a single user provided value (min=max)

  *  ``Aggregation``: Average
  
  *  ``Format``: Double
  
  *  ``Unit``: Hertz

CONTROL ALIASES
~~~~~~~~~~~~~~~~
Several high level aliases are provided.  Their mapping  to
underlying IO Group signals is provided below.
* ``GPU_CORE_FREQUENCY_CONTROL``: LEVELZERO::GPU_CORE_FREQUENCY_CONTROL


SEE ALSO
--------

`geopm(7) <geopm.7.html>`_\ ,
`geopm::IOGroup(3) <GEOPM_CXX_MAN_IOGroup.3.html>`_\ ,
`geopmwrite(1) <geopmwrite.1.html>`_\ ,
`geopmread(1) <geopmread.1.html>`_
