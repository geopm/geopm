.. role:: raw-html-m2r(raw)
   :format: html


geopm::DCGMIOGroup(7) -- IOGroup providing signals and controls for NVIDIA GPUs 
=================================================================================================

DESCRIPTION
-----------

The DCGMIOGroup implements the `geopm::IOGroup(3) <GEOPM_CXX_MAN_IOGroup.3.html>`_ 
interface to provide hardware signals for NVIDIA GPUs from the NVIDIA Datacenter GPU Manager.  This IO Group is intended for use with the `NVMLIOGroup <NVMLIOGroup.7.html>`

ENABLING DCGM
~~~~~~~~~~~~~~~
To enable the DCGMIOGroup geopm must be configured with --enable-dcgm --enable-nvml 

SIGNALS
-------

* ``DCGM::SM_ACTIVE``: Streaming Multiprocessor (SM) activity expressed as a ratio of cycles.

  *  ``Aggregation``: Average.

  *  ``Format``: Double.

  *  ``Unit``: None
* ``DCGM::SM_OCCUPANCY``: Warp residency expressed as a ratio of maximum warps.
  *  ``Aggregation``: Average

  *  ``Format``: Double

  *  ``Unit``: None
* ``DCGM::DRAM_ACTIVE``: DRAM send and receive metrics expressed as a ration of cycles
  *  ``Aggregation``: Average

  *  ``Format``: Double

  *  ``Unit``: None

SIGNAL ALIASES
~~~~~~~~~~~~~~~~
Several high level aliases are provided.  Their descirption and mapping to
underlying IO Group signals is provided below.

* ``GPU_CORE_ACTIVITY``: GPU Compute core activity expressed as a ratio of cycles.

  * ``Aliased Signal``: DCGM::SM_ACTIVE
* ``GPU_UNCORE_ACTIVITY``: GPU memory access activity (i.e. send & receive) expressed as a ratio of cycles.

  * ``Aliased Signal``: DCGM::DRAM_ACTIVE

CONTROLS
--------

* ``DCGM::FIELD_UPDATE_RATE``: Rate at which field data is polled
  *  ``Aggregation``: Expect_same

  *  ``Format``: Double

  *  ``Unit``: Seconds
* ``DCGM::MAX_STORAGE_TIME``: The maximum time field data will be stored
  *  ``Aggregation``: Expect_same

  *  ``Format``: Double

  *  ``Unit``: Seconds
* ``DCGM::MAX_SAMPLES``: The maximum number of samples to be stored.  0 implies no limit
  *  ``Aggregation``: Expect_same

  *  ``Format``: Integer

  *  ``Unit``: Seconds

CONTROL ALIASES
~~~~~~~~~~~~~~~~
No control aliases are provided.

SEE ALSO
--------

`geopm(7) <geopm.7.html>`_\ ,
`geopm::IOGroup(3) <GEOPM_CXX_MAN_IOGroup.3.html>`_\ ,
`geopmwrite(1) <geopmwrite.1.html>`_\ ,
`geopmread(1) <geopmread.1.html>`_
