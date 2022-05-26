geopm_pio_cnl(7) -- Signals and controls for Compute Node Linux Board-Level Metrics
===================================================================================

DESCRIPTION
-----------

The CNLIOGroup implements the `geopm::IOGroup(3)
<GEOPM_CXX_MAN_IOGroup.3.html>`_ interface to provide signals and controls for
board-level energy and power metrics on Compute Node Linux (CNL).

.. _cnl-requirements:

REQUIREMENTS
------------
This IOGroup's signals are only exposed when GEOPM is compiled with the
``--enable-cnl-iogroup`` build configuration option, and GEOPM can read
files in ``/sys/cray/pm_counters``.

SIGNALS
-------

All signals are available with board-level scope, on systems that expose the
``pm_counters`` interface.

``CNL::POWER_BOARD``
    Returns the current board power consumption, in watts. This signal maps to
    ``/sys/cray/pm_counters/power``.

    **Aggregation**: Average

    **Domain**: Board

    **Format**: Integer

    **Unit**: Watts

``CNL::ENERGY_BOARD``
    Returns the accumulated energy consumed by the board, in joules. This
    signal maps to ``/sys/cray/pm_counters/energy``.

    **Aggregation**: Sum

    **Domain**: Board

    **Format**: Integer

    **Unit**: Joules

``CNL::POWER_MEMORY``
    Returns the current power consumption of memory components on the board, in
    watts. This signal maps to ``/sys/cray/pm_counters/memory_power``.

    **Aggregation**: Average

    **Domain**: Board

    **Format**: Integer

    **Unit**: Watts

``CNL::ENERGY_MEMORY``
    Returns the accumulated energy consumed by memory components on the board, in
    joules. This signal maps to ``/sys/cray/pm_counters/memory_energy``.

    **Aggregation**: Sum

    **Domain**: Board

    **Format**: Integer

    **Unit**: Joules

``CNL::POWER_BOARD_CPU``
    Returns the current power consumption of CPU components on the board, in
    watts. This signal maps to ``/sys/cray/pm_counters/cpu_power``.

    **Aggregation**: Average

    **Domain**: Board

    **Format**: Integer

    **Unit**: Watts

``CNL::ENERGY_BOARD_CPU``
    Returns the accumulated energy consumed by CPU components on the board, in
    joules. This signal maps to ``/sys/cray/pm_counters/cpu_energy``.

    **Aggregation**: Sum

    **Domain**: Board

    **Format**: Integer

    **Unit**: Joules

``CNL::SAMPLE_RATE``
    Returns the sample rate of the underlying metrics, in hertz. This signal maps
    to ``/sys/cray/pm_counters/raw_scan_hz``.

    **Aggregation**: Expect same

    **Domain**: Board

    **Format**: Integer

    **Unit**: Hertz

``CNL::SAMPLE_ELAPSED_TIME``
    Returns the time that the current samples were updated, in seconds since this
    IOGroup was initialized. This signal is derived from
    ``/sys/cray/pm_counters/raw_scan_hz`` and ``/sys/cray/pm_counters/freshness``.

    **Aggregation**: Max

    **Domain**: Board

    **Format**: Double

    **Unit**: Seconds

CONTROLS
--------

This IOGroup does not expose any controls.

SIGNAL ALIASES
--------------

This IOGroup exposes the following high-level aliases:

* ``BOARD_POWER`` (**TODO**: currently ``POWER_BOARD``) maps to ``CNL::POWER_BOARD``
* ``BOARD_ENERGY`` (**TODO**: currently ``ENERGY_BOARD``) maps to ``CNL::ENERGY_BOARD``
* ``BOARD_MEMORY_POWER`` (**TODO**: currently ``POWER_MEMORY``) maps to ``CNL::POWER_MEMORY``
* ``BOARD_MEMORY_ENERGY`` (**TODO**: currently ``ENERGY_MEMORY``) maps to ``CNL::ENERGY_MEMORY``
* ``BOARD_CPU_POWER`` (**TODO**: currently ``POWER_BOARD_CPU``) maps to ``CNL::POWER_BOARD_CPU``
* ``BOARD_CPU_ENERGY`` (**TODO**: currently ``ENERGY_BOARD_CPU``) maps to ``CNL::ENERGY_BOARD_CPU``

SEE ALSO
--------

`geopm(7) <geopm.7.html>`_,
`geopm_pio(7) <geopm_pio.7.html>`_,
`geopm::IOGroup(3) <GEOPM_CXX_MAN_IOGroup.3.html>`_,
`geopm::CNLIOGroup(3) <GEOPM_CXX_MAN_CNLIOGroup.3.html>`_,
`geopmwrite(1) <geopmwrite.1.html>`_,
`geopmread(1) <geopmread.1.html>`_
