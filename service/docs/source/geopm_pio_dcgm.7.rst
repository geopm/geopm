

geopm::DCGMIOGroup(7) -- IOGroup providing signals and controls for NVIDIA GPUs
=================================================================================================

DESCRIPTION
-----------

The DCGMIOGroup implements the `geopm::IOGroup(3) <GEOPM_CXX_MAN_IOGroup.3.html>`_
interface to provide hardware signals for NVIDIA GPUs from the NVIDIA Datacenter GPU Manager.  This IO Group is intended for use with the `NVMLIOGroup <NVMLIOGroup.7.html>`

REQUIREMENTS
~~~~~~~~~~~~
To use the GEOPM DCGM signals and controls GEOPM must be compiled against the DCGM libraries and must be run on a system with hardware supported by DCGM.

SIGNALS
-------

* ``DCGM::SM_ACTIVE``:

    Streaming Multiprocessor (SM) activity expressed as a ratio of cycles.

  *  ``Aggregation``: Average.

  *  ``Domain``: GPU

  *  ``Format``: Double.

  *  ``Unit``: None
* ``DCGM::SM_OCCUPANCY``:

    Warp residency expressed as a ratio of maximum warps.

  *  ``Aggregation``: Average

  *  ``Domain``: GPU

  *  ``Format``: Double

  *  ``Unit``: None
* ``DCGM::DRAM_ACTIVE``:

    DRAM send and receive metrics expressed as a ration of cycles

  *  ``Aggregation``: Average

  *  ``Domain``: GPU

  *  ``Format``: Double

  *  ``Unit``: None

SIGNAL ALIASES
~~~~~~~~~~~~~~~~
Several high level aliases are provided.  Their mapping to
underlying IO Group signals is provided below.

* ``GPU_CORE_ACTIVITY``:

    Aliases to DCGM::SM_ACTIVE

* ``GPU_UNCORE_ACTIVITY``:

    Aliases to DCGM::DRAM_ACTIVE

CONTROLS
--------
Every control is exposed as a signal with the same name.  The relevant signal aggregation information is provided below.

* ``DCGM::FIELD_UPDATE_RATE``:

    Rate at which field data is polled

  *  ``Aggregation``: Expect_same

  *  ``Domain``: Board

  *  ``Format``: Double

  *  ``Unit``: Seconds
* ``DCGM::MAX_STORAGE_TIME``:

    The maximum time field data will be stored

  *  ``Aggregation``: Expect_same

  *  ``Domain``: Board

  *  ``Format``: Double

  *  ``Unit``: Seconds
* ``DCGM::MAX_SAMPLES``:

    The maximum number of samples to be stored.  0 implies no limit

  *  ``Aggregation``: Expect_same

  *  ``Domain``: Board

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
