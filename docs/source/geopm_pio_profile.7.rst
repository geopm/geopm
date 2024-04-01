geopm_pio_profile(7) -- Signals and controls for the ProfileIOGroup
===================================================================

Description
-----------
The ProfileIOGroup implements the :doc:`geopm::IOGroup(3)
<geopm::IOGroup.3>` interface to provide signals and controls from
the application based on calls to GEOPM's prof API :doc:`geopm_prof(3)
<geopm_prof.3>`.

The signals defined herein are useful primarily for end-of-run reporting
purposes and for implementing new agents via the :doc:`geopm::Agent(3)
<geopm::Agent.3>` interface.

Requirements
------------
This IOGroup's signals are only exposed while the GEOPM HPC Runtime is running.
The signals are only available to code running within the GEOPM HPC Runtime.
The signals **cannot** be queried via ``geopmread``.

Signals
-------
``PROFILE::REGION_HASH``
    The hash (or ID) of the region of code (see :doc:`geopm_prof(3)
    <geopm_prof.3>`) currently being run by all ranks, otherwise
    ``GEOPM_REGION_HASH_UNMARKED``.  See the enum ``geopm_region_hash_e``
    defined in `geopm_hash.h
    <https://github.com/geopm/geopm/blob/dev/service/src/geopm_hash.h>`_ for
    more information.

    * **Aggregation**: region_hash
    * **Domain**: cpu
    * **Format**: hex
    * **Unit**: hash of region name

``PROFILE::REGION_HINT``
    The region hint (see :doc:`geopm_prof(3) <geopm_prof.3>`) associated
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

``PROFILE::REGION_PROGRESS``
    Sum of the fractional progress reported by all CPUs over the aggregated
    domain.  When utilizing OpenMP, the per-CPU returned value will be on the
    interval [0, 1 / *num_thread*] where *num_thread* is the number of
    requested OpenMP threads per rank.  When running single-threaded (i.e. not
    leveraging OpenMP), the per-CPU returned value will either be zero (no
    active threads), or on the interval [0, 1].

    When aggregating this signal to the board domain, the aggregated value will
    always be on the interval [0, *num_rank*] where *num_rank* is equal to the
    number of ranks (processes) in the run.  0 indicates no progress while 1 (if
    examining the per-CPU signal) or *num_rank* (if examining the board
    aggregated signal) indicates the region is complete.

    This signal is not valid outside of a region.  Specifically, this signal
    should be ignored any time the ``REGION_HASH`` signal aggregated to the
    same domain resolves to ``GEOPM_REGION_HASH_UNMARKED``.

    * **Aggregation**: sum
    * **Domain**: cpu
    * **Format**: float
    * **Unit**: progress percentage

``PROFILE::TIME_HINT_UNKNOWN``
    The total amount of time that a CPU was measured to be running with this
    hint value on the Linux logical CPU specified.

    * **Aggregation**: average
    * **Domain**: cpu
    * **Format**: double
    * **Unit**: seconds

``PROFILE::TIME_HINT_UNSET``
    The total amount of time that a CPU was measured to be running with this
    hint value on the Linux logical CPU specified.

    * **Aggregation**: average
    * **Domain**: cpu
    * **Format**: double
    * **Unit**: seconds

``PROFILE::TIME_HINT_COMPUTE``
    The total amount of time that a CPU was measured to be running with this
    hint value on the Linux logical CPU specified.

    * **Aggregation**: average
    * **Domain**: cpu
    * **Format**: double
    * **Unit**: seconds

``PROFILE::TIME_HINT_MEMORY``
    The total amount of time that a CPU was measured to be running with this
    hint value on the Linux logical CPU specified.

    * **Aggregation**: average
    * **Domain**: cpu
    * **Format**: double
    * **Unit**: seconds

``PROFILE::TIME_HINT_NETWORK``
    The total amount of time that a CPU was measured to be running with this
    hint value on the Linux logical CPU specified.

    * **Aggregation**: average
    * **Domain**: cpu
    * **Format**: double
    * **Unit**: seconds

``PROFILE::TIME_HINT_IO``
    The total amount of time that a CPU was measured to be running with this
    hint value on the Linux logical CPU specified.

    * **Aggregation**: average
    * **Domain**: cpu
    * **Format**: double
    * **Unit**: seconds

``PROFILE::TIME_HINT_SERIAL``
    The total amount of time that a CPU was measured to be running with this
    hint value on the Linux logical CPU specified.

    * **Aggregation**: average
    * **Domain**: cpu
    * **Format**: double
    * **Unit**: seconds

``PROFILE::TIME_HINT_PARALLEL``
    The total amount of time that a CPU was measured to be running with this
    hint value on the Linux logical CPU specified.

    * **Aggregation**: average
    * **Domain**: cpu
    * **Format**: double
    * **Unit**: seconds

``PROFILE::TIME_HINT_IGNORE``
    The total amount of time that a CPU was measured to be running with this
    hint value on the Linux logical CPU specified.

    * **Aggregation**: average
    * **Domain**: cpu
    * **Format**: double
    * **Unit**: seconds

Controls
--------
This IOGroup does not provide any controls.

Aliases
-------
This IOGroup provides the following high-level aliases:

Signal Aliases
^^^^^^^^^^^^^^
``REGION_HASH``
    Maps to ``PROFILE::REGION_HASH``

``REGION_HINT``
    Maps to ``PROFILE::REGION_HINT``

``REGION_PROGRESS``
    Maps to ``PROFILE::REGION_PROGRESS``

``TIME_HINT_UNKNOWN``
    Maps to ``PROFILE::TIME_HINT_UNKNOWN``

``TIME_HINT_UNSET``
    Maps to ``PROFILE::TIME_HINT_UNSET``

``TIME_HINT_COMPUTE``
    Maps to ``PROFILE::TIME_HINT_COMPUTE``

``TIME_HINT_MEMORY``
    Maps to ``PROFILE::TIME_HINT_MEMORY``

``TIME_HINT_NETWORK``
    Maps to ``PROFILE::TIME_HINT_NETWORK``

``TIME_HINT_IO``
    Maps to ``PROFILE::TIME_HINT_IO``

``TIME_HINT_SERIAL``
    Maps to ``PROFILE::TIME_HINT_SERIAL``

``TIME_HINT_PARALLEL``
    Maps to ``PROFILE::TIME_HINT_PARALLEL``

``TIME_HINT_IGNORE``
    Maps to ``PROFILE::TIME_HINT_IGNORE``

See Also
--------
:doc:`geopm(7) <geopm.7>`,
:doc:`geopm_pio(7) <geopm_pio.7>`,
:doc:`geopm::IOGroup(3) <geopm::IOGroup.3>`,
:doc:`geopm::Agg(3) <geopm::Agg.3>`,
:doc:`geopm_prof(3) <geopm_prof.3>`,
:doc:`geopm::Agent(3) <geopm::Agent.3>`
