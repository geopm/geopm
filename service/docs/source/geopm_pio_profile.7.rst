geopm_pio_profile(7) -- Signals and controls for the ProfileIOGroup
===================================================================================

Description
-----------

The ProfileIOGroup implements the `geopm::IOGroup(3)
<GEOPM_CXX_MAN_IOGroup.3.html>`_ interface to provide signals and controls from
the application based on calls to GEOPM's prof API `geopm_prof_c(3)
<geopm_prof_c.3.html>`_.

The signals defined herein are useful primarily for end-of-run reporting
purposes and for implementing new agents via the `geopm::Agent(3)
<GEOPM_CXX_MAN_Agent.3.html>`_ interface.

Requirements
------------
This IOGroup's signals are only exposed while the GEOPM HPC Runtime is running.
The signals are only available to code running within the GEOPM HPC Runtime.
The signals **cannot** be queried via ``geopmread``.

Signals
-------

``PROFILE::CPU_REGION_HASH`` TODO - Currently ``REGION_HASH``; change in implementation.
    The hash (or ID) of the region of code (see `geopm_prof_c(3)
    <geopm_prof_c.3.html>`_ ) currently being run by all ranks, otherwise
    ``GEOPM_REGION_HASH_UNMARKED``.  See the enum ``geopm_region_hash_e``
    defined in `geopm_hash.h
    <https://github.com/geopm/geopm/blob/dev/service/src/geopm_hash.h>`_ for
    more information.

      * **Aggregation**: region_hash
      * **Domain**: cpu
      * **Format**: hex
      * **Unit**: crc32 hash of region name

``PROFILE::CPU_REGION_HINT`` TODO - Currently ``REGION_HINT``; change in implementation.
    The region hint (see `geopm_prof_c(3) <geopm_prof_c.3.html>`_) associated
    with the currently running region.  For any interval when all ranks are
    within an MPI function inside of a user defined region, the hint will
    change from the hint associated with the user defined region to
    ``GEOPM_REGION_HINT_NETWORK``.  If the user defined region was defined with
    ``GEOPM_REGION_HINT_NETWORK`` and there is an interval within the region
    when all ranks are within an MPI function, GEOPM will not attribute the
    time spent within the MPI function as MPI time in the report files.  It
    will be instead attributed to the time spent in the region as a whole.  See
    the enum ``geopm_region_hint_e`` defined in `geopm_hint.h
    <https://github.com/geopm/geopm/blob/dev/service/src/geopm_hint.h>`_ for
    more information.


      * **Aggregation**: region_hint
      * **Domain**: cpu
      * **Format**: hex
      * **Unit**: geopm_region_hint_e

``PROFILE::CPU_REGION_PROGRESS`` TODO - Currently ``REGION_PROGRESS``; change in implementation.
    Minimum per-rank reported progress through the current region.  The
    returned value will be on the interval [0, 1].  0 indicates no progress
    while 1 indicates the region is complete.

      * **Aggregation**: min
      * **Domain**: cpu
      * **Format**: float
      * **Unit**: progress percentage

``PROFILE::CPU_TIME_HINT_UNKNOWN`` TODO - Currently ``TIME_HINT_UNKNOWN``; change in implementation.
    The total amount of time that a CPU was measured to be running with this
    hint value on the Linux logical CPU specified.

      * **Aggregation**: average
      * **Domain**: cpu
      * **Format**: float
      * **Unit**: seconds

``PROFILE::CPU_TIME_HINT_UNSET`` TODO - Currently ``TIME_HINT_UNSET``; change in implementation.
    The total amount of time that a CPU was measured to be running with this
    hint value on the Linux logical CPU specified.

      * **Aggregation**: average
      * **Domain**: cpu
      * **Format**: float
      * **Unit**: seconds

``PROFILE::CPU_TIME_HINT_COMPUTE`` TODO - Currently ``TIME_HINT_COMPUTE``; change in implementation.
    The total amount of time that a CPU was measured to be running with this
    hint value on the Linux logical CPU specified.

      * **Aggregation**: average
      * **Domain**: cpu
      * **Format**: float
      * **Unit**: seconds

``PROFILE::CPU_TIME_HINT_MEMORY`` TODO - Currently ``TIME_HINT_MEMORY``; change in implementation.
    The total amount of time that a CPU was measured to be running with this
    hint value on the Linux logical CPU specified.

      * **Aggregation**: average
      * **Domain**: cpu
      * **Format**: float
      * **Unit**: seconds

``PROFILE::CPU_TIME_HINT_NETWORK`` TODO - Currently ``TIME_HINT_NETWORK``; change in implementation.
    The total amount of time that a CPU was measured to be running with this
    hint value on the Linux logical CPU specified.

      * **Aggregation**: average
      * **Domain**: cpu
      * **Format**: float
      * **Unit**: seconds

``PROFILE::CPU_TIME_HINT_IO`` TODO - Currently ``TIME_HINT_IO``; change in implementation.
    The total amount of time that a CPU was measured to be running with this
    hint value on the Linux logical CPU specified.

      * **Aggregation**: average
      * **Domain**: cpu
      * **Format**: float
      * **Unit**: seconds

``PROFILE::CPU_TIME_HINT_SERIAL`` TODO - Currently ``TIME_HINT_SERIAL``; change in implementation.
    The total amount of time that a CPU was measured to be running with this
    hint value on the Linux logical CPU specified.

      * **Aggregation**: average
      * **Domain**: cpu
      * **Format**: float
      * **Unit**: seconds

``PROFILE::CPU_TIME_HINT_PARALLEL`` TODO - Currently ``TIME_HINT_PARALLEL``; change in implementation.
    The total amount of time that a CPU was measured to be running with this
    hint value on the Linux logical CPU specified.

      * **Aggregation**: average
      * **Domain**: cpu
      * **Format**: float
      * **Unit**: seconds

``PROFILE::CPU_TIME_HINT_IGNORE`` TODO - Currently ``TIME_HINT_IGNORE``; change in implementation.
    The total amount of time that a CPU was measured to be running with this
    hint value on the Linux logical CPU specified.

      * **Aggregation**: average
      * **Domain**: cpu
      * **Format**: float
      * **Unit**: seconds

Controls
--------

This IOGroup does not expose any controls.

Signal Aliases
--------------

This IOGroup exposes the following high-level aliases:

``CPU_REGION_HASH``
    Aliases to ``PROFILE::CPU_REGION_HASH``

``CPU_REGION_HINT``
    Aliases to ``PROFILE::CPU_REGION_HINT``

``CPU_REGION_PROGRESS``
    Aliases to ``PROFILE::CPU_REGION_PROGRESS``

``CPU_TIME_HINT_UNKNOWN``
    Aliases to ``PROFILE::CPU_TIME_HINT_UNKNOWN``

``CPU_TIME_HINT_UNSET``
    Aliases to ``PROFILE::CPU_TIME_HINT_UNSET``

``CPU_TIME_HINT_COMPUTE``
    Aliases to ``PROFILE::CPU_TIME_HINT_COMPUTE``

``CPU_TIME_HINT_MEMORY``
    Aliases to ``PROFILE::CPU_TIME_HINT_MEMORY``

``CPU_TIME_HINT_NETWORK``
    Aliases to ``PROFILE::CPU_TIME_HINT_NETWORK``

``CPU_TIME_HINT_IO``
    Aliases to ``PROFILE::CPU_TIME_HINT_IO``

``CPU_TIME_HINT_SERIAL``
    Aliases to ``PROFILE::CPU_TIME_HINT_SERIAL``

``CPU_TIME_HINT_PARALLEL``
    Aliases to ``PROFILE::CPU_TIME_HINT_PARALLEL``

``CPU_TIME_HINT_IGNORE``
    Aliases to ``PROFILE::CPU_TIME_HINT_IGNORE``

``CPU_TIME_HINT_UNKNOWN``
    Aliases to ``PROFILE::CPU_TIME_HINT_UNKNOWN``

``CPU_TIME_HINT_UNKNOWN``
    Aliases to ``PROFILE::CPU_TIME_HINT_UNKNOWN``

See Also
--------

`geopm(7) <geopm.7.html>`_,
`geopm_pio(7) <geopm_pio.7.html>`_,
`geopm::IOGroup(3) <GEOPM_CXX_MAN_IOGroup.3.html>`_,
`geopm::Agg(3) <GEOPM_CXX_MAN_Agg.3.html>`_,
`geopm_prof_c(3) <geopm_prof_c.3.html>`_,
`geopm::Agent(3) <GEOPM_CXX_MAN_Agent.3.html>`_
