
geopm_pio: GEOPM PlatformIO interface
=====================================

DESCRIPTION
-----------

The ``PlatformIO`` class provides a high-level interface for signals
(system monitors) and controls (system settings).  There are a large
number of built-in signals and controls.  These built-in signals and
controls include hardware metrics, hardware settings, and signals
derived from application behavior.  Application behavior is tracked by
GEOPM's integration with MPI and OpenMP and also by application use of
the `geopm_prof_c(3) <geopm_prof_c.3.html>`_ mark-up interface. In
addition to the built-in features, ``PlatformIO`` can be extended
through the `geopm::IOGroup(3) <GEOPM_CXX_MAN_IOGroup.3.html>`_ plugin
interface to provide arbitrary signals and controls.

A domain is a discrete component within a compute node where a signal
or control is applicable.  For more information about the
``geopm_domain_e`` enum and the hierarchical platform description see
`geopm::PlatformTopo(3) <GEOPM_CXX_MAN_PlatformTopo.3.html>`_.  A
signal represents any measurement in SI units that can be sampled or
any unit-free integer that can be read.  A control represents a
request for a hardware domain to operate such that a related signal
measured from the hardware domain will track the request.  For
example, the user can set a ``POWER_PACKAGE_LIMIT`` in units of
*watts* and the related signal, ``POWER_PACKAGE``\ , will remain below
the limit.  Similarly the user can set a ``CPU_FREQUENCY_CONTROL`` in
*hertz* and the related signal, ``CPU_FREQUENCY_STATUS`` will show the
CPU operating at the value set.

ALIASING SIGNALS AND CONTROLS
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
``PACKAGE_ENERGY`` signal which can be used on any platform to measure
the current package energy value.  Agents are encouraged to request
high level signals and controls to make the implementation more
portable.  The high level signals and controls supported by built-in
``IOGroup`` classes are listed below.  See `geopm::PluginFactory(3) <GEOPM_CXX_MAN_PluginFactory.3.html>`_
section on `SEARCH AND LOAD ORDER <GEOPM_CXX_MAN_PluginFactory.3.html#plugin-search-path-and-load-order>`__ for information about how the
``GEOPM_PLUGIN_PATH`` environment variable is used to select which
``IOGroup`` implementation is used in the case where more than one
provides the same high level signal or control.

Signal names that end in ``#`` (for example, raw MSR values) are 64-bit
integers encoded to be stored as doubles.  When accessing these
integer signals, the return value of ``read_signal()`` or ``sample()``
should not be used directly as a double precision number.  To
decode the 64-bit integer from the double use
``geopm_signal_to_field()`` described in `geopm_hash(3) <geopm_hash.3.html>`_.  The
`geopm::MSRIOGroup(3) <GEOPM_CXX_MAN_MSRIOGroup.3.html>`_ also provides raw MSR field signals that are
encoded in this way.


DESCRIPTIONS OF HIGH LEVEL ALIASES
----------------------------------

*
  ``TIME``:

      Time elapsed since the beginning of execution.

*
  ``EPOCH_COUNT``:

      Number of completed executions of an epoch.  Prior to the first call
      by the application to ``geopm_prof_epoch()`` the signal returns as ``-1``.
      With each call to ``geopm_prof_epoch()`` the count increases by one.

*
  ``REGION_HASH``:

      The hash of the region of code (see `geopm_prof_c(3) <geopm_prof_c.3.html>`_\ ) currently being
      run by all ranks, otherwise ``GEOPM_REGION_HASH_UNMARKED``.

*
  ``REGION_HINT``:

      The region hint (see `geopm_prof_c(3) <geopm_prof_c.3.html>`_\ ) associated with the currently
      running region.  For any interval when all ranks are within an MPI
      function inside of a user defined region, the hint will change from the
      hint associated with the user defined region to ``GEOPM_REGION_HINT_NETWORK``.
      If the user defined region was defined with ``GEOPM_REGION_HINT_NETWORK`` and
      there is an interval within the region when all ranks are within an MPI
      function, GEOPM will not attribute the time spent within the MPI function as
      MPI time in the report files.  It will be instead attributed to the time
      spent in the region as a whole.

*
  ``REGION_PROGRESS``:

      Minimum per-rank reported progress through the current region.

*
  ``REGION_RUNTIME``:

      Maximum per-rank of the last recorded runtime for the current
      region.

*
  ``ENERGY_PACKAGE``:

      Total energy aggregated over the processor package.

*
  ``POWER_PACKAGE``:

      Total power aggregated over the processor package.

*
  ``CPU_FREQUENCY_STATUS``:

      Average CPU frequency over the specified domain.

*
  ``ENERGY_DRAM``:

      Total energy aggregated over the DRAM DIMMs associated with a NUMA node.

*
  ``POWER_DRAM``:

      Total power aggregated over the DRAM DIMMs associated with a NUMA node.

*
  ``POWER_PACKAGE_MIN``:

      Minimum setting for package power over the given domain.

*
  ``POWER_PACKAGE_MAX``:

      Maximum setting for package power over the given domain.

*
  ``POWER_PACKAGE_TDP``:

      Maximum sustainable setting for package power over the given domain.

*
  ``CYCLES_THREAD``:

      Average over the domain of clock cycles executed by cores since
      the beginning of execution.

*
  ``CYCLES_REFERENCE``:

      Average over the domain of clock reference cycles since the
      beginning of execution.

*
  ``GPU_ENERGY``:

      Total energy aggregated over the GPU package.

*
  ``GPU_POWER``:

      Total power aggregated over the GPU package.

*
  ``GPU_CORE_FREQUENCY_STATUS``:

      Average achieved GPU core frequency over the specified domain.

*
  ``GPU_CORE_FREQUENCY_CONTROL``:

      Average requested GPU core frequency over the specified domain.

*
  ``GPU_CORE_FREQENCY_MIN_AVAIL``:

      Minimum supported GPU core frequency over the specified domain.

*
  ``GPU_CORE_FREQENCY_MAX_AVAIL``:

      Maximum supported GPU core frequency over the specified domain.

*
  ``GPU_UTILIZATION``:

      Average GPU utilization expressed as a ratio of cycles.

*
  ``GPU_TEMPERATURE``:

      Average GPU temperature in degrees Celsius.

*
  ``GPU_POWER_LIMIT_CONTROL``:

      Average GPU power usage limit.

*
  ``GPU_CORE_ACTIVITY``:

      GPU compute core activity expressed as a ratio of cycles.

*
  ``GPU_UNCORE_ACTIVITY``:

      GPU memory access activity expressed as a ratio of cycles.


SEE ALSO
--------

`geopm(7) <geopm.7.html>`_\ ,
`geopm_pio_c(3) <geopm_pio_c.3.html>`_\ ,
`geopm_prof_c(3) <geopm_prof_c.3.html>`_\ ,
`geopm_topo_c(3) <geopm_topo_c.3.html>`_\ ,
