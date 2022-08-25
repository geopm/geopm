
geopm_pio(7) -- GEOPM PlatformIO interface
==========================================

Description
-----------

The ``PlatformIO`` class provides a high-level interface for signals
(system monitors) and controls (system settings).  There are a large
number of built-in signals and controls.  These built-in signals and
controls include hardware metrics, hardware settings, and signals
derived from application behavior.  Application behavior is tracked by
GEOPM's integration with MPI and OpenMP and also by application use of
the :doc:`geopm_prof(3) <geopm_prof.3>` mark-up interface. In
addition to the built-in features, ``PlatformIO`` can be extended
through the :doc:`geopm::IOGroup(3) <GEOPM_CXX_MAN_IOGroup.3>` plugin
interface to provide arbitrary signals and controls.

A domain is a discrete component within a compute node where a signal
or control is applicable.  For more information about the
``geopm_domain_e`` enum and the hierarchical platform description see
:doc:`geopm_topo(3) <geopm_topo.3>`.  A
signal represents any measurement in SI units that can be sampled or
any unit-free integer that can be read.  A control represents a
request for a hardware domain to operate such that a related signal
measured from the hardware domain will track the request.  For
example, the user can set a ``CPU_POWER_LIMIT_CONTROL`` in units of
*watts* and the related signal, ``CPU_POWER``\ , will remain below
the limit.  Similarly the user can set a ``CPU_FREQUENCY_MAX_CONTROL`` in
*hertz* and the related signal, ``CPU_FREQUENCY_STATUS`` will show the
CPU operating at the value set.

See the :doc:`geopmread(1) <geopmread.1>` and :doc:`geopmwrite(1)
<geopmwrite.1>` tools for command-line interaction with the ``PlatformIO``
interface.

Aliasing Signals And Controls
-----------------------------

There are two classes of signals and control names: "low level" and
"high level".  All ``IOGroup``\ 's are expected to provide low level
signals and controls with names that are prefixed with the ``IOGroup``
name and two colons, e.g. the ``MSRIOGroup`` provides the
``MSR::PKG_ENERGY_STATUS:ENERGY`` signal.  If the signal or control may
be supported on more than one platform, the implementation should be
aliased to a high level name.  This high level name enables the signal
or control to be supported by more than one ``IOGroup``\ , and different
platforms will support the loading different sets of ``IOGroups``.  The
``MSRIOGroup`` aliases the above signal to the high level
``CPU_ENERGY`` signal which can be used on any platform to measure
the current CPU energy value.  Agents are encouraged to request
high level signals and controls to make the implementation more
portable.  The high level signals and controls supported by built-in
``IOGroup`` classes are listed below.  See :doc:`geopm::PluginFactory(3)
<GEOPM_CXX_MAN_PluginFactory.3>` section on :ref:`SEARCH AND LOAD ORDER
<GEOPM_CXX_MAN_PluginFactory.3:Plugin Search Path And Load Order>` for
information about how the ``GEOPM_PLUGIN_PATH`` environment variable is used to
select which ``IOGroup`` implementation is used in the case where more than one
provides the same high level signal or control.

Signal names that end in ``#`` (for example, raw MSR values) are 64-bit
integers encoded to be stored as doubles.  When accessing these
integer signals, the return value of ``read_signal()`` or ``sample()``
should not be used directly as a double precision number.  To
decode the 64-bit integer from the double use
``geopm_signal_to_field()`` described in :doc:`geopm_hash(3) <geopm_hash.3>`.  The
:doc:`geopm::MSRIOGroup(3) <GEOPM_CXX_MAN_MSRIOGroup.3>` also provides raw MSR field signals that are
encoded in this way.


Descriptions Of High Level Aliases
----------------------------------

``BOARD_ENERGY``
    Total energy measured on the server's board. See :ref:`geopm_pio_cnl(7)
    <geopm_pio_cnl.7:Requirements>` for signal availability requirements.

``BOARD_POWER``
    Power measured on the server's board. See :ref:`geopm_pio_cnl(7)
    <geopm_pio_cnl.7:Requirements>` for signal availability requirements.

``CPU_CORE_TEMPERATURE``
    CPU core temperature, in degrees Celsius.

``CPU_CYCLES_REFERENCE``
    The count of the number of cycles while the logical processor is not in a
    halt state and not in a stop-clock state. The count rate is fixed at the
    TIMESTAMP_COUNT rate.

``CPU_CYCLES_THREAD``
    The count of the number of cycles while the logical processor is not in a
    halt state.  The count rate may change based on core frequency.

``CPU_ENERGY``
    An increasing meter of energy consumed by the package over time. It will
    reset periodically due to roll-over.

``CPU_FREQUENCY_MAX_CONTROL``
    Target maximum operating frequency of the CPU based on the control
    register.

``CPU_FREQUENCY_MIN_AVAIL``
    Minimum achievable processor frequency on the system.

``CPU_FREQUENCY_MAX_AVAIL``
    Maximum achievable processor frequency on the system.

``CPU_FREQUENCY_MIN_CONTROL``
    Target minimum operating frequency of the CPU based on the control
    register.

``CPU_FREQUENCY_STATUS``
    The current operating frequency of the CPU.

``CPU_FREQUENCY_STEP``
    Step size between processor frequency settings.

``CPU_FREQUENCY_STICKER``
    Processor base frequency.

``CPU_INSTRUCTIONS_RETIRED``
    The count of the number of instructions executed.

``CPU_PACKAGE_TEMPERATURE``
    CPU package temperature, in degrees Celsius.

``CPU_POWER_LIMIT_CONTROL``
    The average power usage limit over the time window specified in
    PL1_TIME_WINDOW.

``CPU_POWER_TIME_WINDOW``
    The time window associated with power limit 1.

``CPU_POWER_MAX_AVAIL``
    The maximum power limit based on the electrical specification.

``CPU_POWER_MIN_AVAIL``
    The minimum power limit based on the electrical specification.

``CPU_POWER_LIMIT_DEFAULT``
    Maximum power to stay within the thermal limits based on the design (TDP).

``CPU_POWER``
    Total power aggregated over the processor package.

``CPU_TIMESTAMP_COUNTER``
    An always running, monotonically increasing counter that is incremented at
    a constant rate. For use as a wall clock timer.

``CPU_UNCORE_FREQUENCY_STATUS``
    Target operating frequency of the uncore.

``CPU_UNCORE_FREQUENCY_MAX_CONTROL``
    Control that limits the maximum frequency of the uncore.

``CPU_UNCORE_FREQUENCY_MIN_CONTROL``
    Control that limits the minimum frequency of the uncore.

``DRAM_ENERGY``
    An increasing meter of energy consumed by the DRAM over time. It will reset
    periodically due to roll-over.

``DRAM_POWER``
    Total power aggregated over the DRAM DIMMs associated with a NUMA node.

``EPOCH_COUNT``
    Number of completed executions of an epoch.  Prior to the first call
    by the application to ``geopm_prof_epoch()`` the signal returns as ``-1``.
    With each call to ``geopm_prof_epoch()`` the count increases by one.

``GPU_CORE_ACTIVITY``
    GPU compute core activity expressed as a ratio of cycles.

``GPU_CORE_FREQENCY_MAX_AVAIL``
    Maximum supported GPU core frequency over the specified domain.

``GPU_CORE_FREQENCY_MIN_AVAIL``
    Minimum supported GPU core frequency over the specified domain.

``GPU_CORE_FREQUENCY_MAX_CONTROL``
    Control that limits the maximum GPU core frequency.

``GPU_CORE_FREQUENCY_MIN_CONTROL``
    Control that limits the minimum GPU core frequency.

``GPU_CORE_FREQUENCY_STATUS``
    Average achieved GPU core frequency over the specified domain.

``GPU_ENERGY``
    Total energy aggregated over the GPU package.

``GPU_POWER_LIMIT_CONTROL``
    Average GPU power usage limit.

``GPU_POWER``
    Total power aggregated over the GPU package.

``GPU_TEMPERATURE``
    Average GPU temperature in degrees Celsius.

``GPU_UNCORE_ACTIVITY``
    GPU memory access activity expressed as a ratio of cycles.

``GPU_UTILIZATION``
    Average GPU utilization expressed as a ratio of cycles.

``REGION_HASH``
    The hash of the region of code (see :doc:`geopm_prof(3) <geopm_prof.3>`\ ) currently being
    run by all ranks, otherwise ``GEOPM_REGION_HASH_UNMARKED``.

``REGION_HINT``
    The region hint (see :doc:`geopm_prof(3) <geopm_prof.3>`\ ) associated with the currently
    running region.  For any interval when all ranks are within an MPI
    function inside of a user defined region, the hint will change from the
    hint associated with the user defined region to ``GEOPM_REGION_HINT_NETWORK``.
    If the user defined region was defined with ``GEOPM_REGION_HINT_NETWORK`` and
    there is an interval within the region when all ranks are within an MPI
    function, GEOPM will not attribute the time spent within the MPI function as
    MPI time in the report files.  It will be instead attributed to the time
    spent in the region as a whole.

``REGION_PROGRESS``
    Minimum per-rank reported progress through the current region.

``REGION_RUNTIME``
    Maximum per-rank of the last recorded runtime for the current
    region.

``TIME``
    Time elapsed since the beginning of execution.


Low Level Signals and Controls
------------------------------

The high level alias signals and controls defined in this man page may be
supported by one or more IOGroups.  These IOGroups also provide signals and
controls which extend the capabilities described in this page.  These signals
and controls are described as "low level signals and controls", and these have
names that are prefixed by the IOGroup name that provides it.  For example, the
``MSRIOGroup`` provides the ``MSR::PERF_CTL:FREQ`` low level control.  This is
the underlying implementation for the high level alias
``CPU_FREQUENCY_MAX_CONTROL`` on x86 platforms when HWP is disabled.  Some low
level signals and controls do not have high level aliases associated with them.
To learn about these low level signals and controls please consult the chapter
7 man page for each IOGroup as linked below.

- :doc:`geopm_pio_cnl(7) <geopm_pio_cnl.7>`
- :doc:`geopm_pio_cpuinfo(7) <geopm_pio_cpuinfo.7>`
- :doc:`geopm_pio_dcgm(7) <geopm_pio_dcgm.7>`
- :doc:`geopm_pio_levelzero(7) <geopm_pio_levelzero.7>`
- :doc:`geopm_pio_msr(7) <geopm_pio_msr.7>`
- :doc:`geopm_pio_nvml(7) <geopm_pio_nvml.7>`
- :doc:`geopm_pio_profile(7) <geopm_pio_profile.7>`
- :doc:`geopm_pio_service(7) <geopm_pio_service.7>`
- :doc:`geopm_pio_sst(7) <geopm_pio_sst.7>`
- :doc:`geopm_pio_time(7) <geopm_pio_time.7>`


See Also
--------

:doc:`geopm(7) <geopm.7>`,
:doc:`geopm_prof(3) <geopm_prof.3>`,
:doc:`geopm_topo(3) <geopm_topo.3>`,
