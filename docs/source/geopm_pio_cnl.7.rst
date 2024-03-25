geopm_pio_cnl(7) -- Signals and controls for Compute Node Linux Board-Level Metrics
===================================================================================

Description
-----------
The CNLIOGroup implements the :doc:`geopm::IOGroup(3)
<GEOPM_CXX_MAN_IOGroup.3>` interface to provide signals and controls for
board-level energy and power metrics on Compute Node Linux (CNL).

Requirements
------------
This IOGroup's signals are only exposed when GEOPM is compiled with the
``--enable-cnl-iogroup`` build configuration option, and GEOPM can read
files in ``/sys/cray/pm_counters``.

Signals
-------
All signals are available with board-level scope, on systems that expose the
``pm_counters`` interface.

``CNL::BOARD_POWER``
    Returns the current board power consumption, in watts. This signal maps to
    ``/sys/cray/pm_counters/power``.

    * **Aggregation**: sum
    * **Domain**: board
    * **Format**: integer
    * **Unit**: watts

``CNL::BOARD_ENERGY``
    Returns the accumulated energy consumed by the board, in joules. This
    signal maps to ``/sys/cray/pm_counters/energy``.

    * **Aggregation**: sum
    * **Domain**: board
    * **Format**: integer
    * **Unit**: joules

``CNL::POWER_MEMORY``
    Returns the current power consumption of memory components on the board, in
    watts. This signal maps to ``/sys/cray/pm_counters/memory_power``.

    * **Aggregation**: sum
    * **Domain**: board
    * **Format**: integer
    * **Unit**: watts

``CNL::ENERGY_MEMORY``
    Returns the accumulated energy consumed by memory components on the board, in
    joules. This signal maps to ``/sys/cray/pm_counters/memory_energy``.

    * **Aggregation**: sum
    * **Domain**: board
    * **Format**: integer
    * **Unit**: joules

``CNL::BOARD_POWER_CPU``
    Returns the current power consumption of CPU components on the board, in
    watts. This signal maps to ``/sys/cray/pm_counters/cpu_power``.

    * **Aggregation**: sum
    * **Domain**: board
    * **Format**: integer
    * **Unit**: watts

``CNL::BOARD_ENERGY_CPU``
    Returns the accumulated energy consumed by CPU components on the board, in
    joules. This signal maps to ``/sys/cray/pm_counters/cpu_energy``.

    * **Aggregation**: sum
    * **Domain**: board
    * **Format**: integer
    * **Unit**: joules

``CNL::SAMPLE_RATE``
    Returns the sample rate of the underlying metrics, in hertz. This signal maps
    to ``/sys/cray/pm_counters/raw_scan_hz``.

    * **Aggregation**: expect same
    * **Domain**: board
    * **Format**: integer
    * **Unit**: hertz

``CNL::SAMPLE_ELAPSED_TIME``
    Returns the time that the current samples were updated, in seconds since this
    IOGroup was initialized. This signal is derived from
    ``/sys/cray/pm_counters/raw_scan_hz`` and ``/sys/cray/pm_counters/freshness``.

    * **Aggregation**: max
    * **Domain**: board
    * **Format**: double
    * **Unit**: seconds

Controls
--------
This IOGroup does not expose any controls.

Aliases
-------

Signal Aliases
^^^^^^^^^^^^^^
This IOGroup exposes the following high-level aliases:

* ``BOARD_POWER`` aliases to ``CNL::BOARD_POWER``
* ``BOARD_ENERGY`` aliases to ``CNL::BOARD_ENERGY``


See Also
--------
:doc:`geopm(7) <geopm.7>`,
:doc:`geopm_pio(7) <geopm_pio.7>`,
:doc:`geopm::IOGroup(3) <GEOPM_CXX_MAN_IOGroup.3>`,
:doc:`geopm::CNLIOGroup(3) <GEOPM_CXX_MAN_CNLIOGroup.3>`,
:doc:`geopmwrite(1) <geopmwrite.1>`,
:doc:`geopmread(1) <geopmread.1>`,
:doc:`geopm::Agg(3) <GEOPM_CXX_MAN_Agg.3>`
