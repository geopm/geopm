
geopm_pio_msr(7) -- Signals and controls for Model Specific Registers (MSRs) 
============================================================================

Description
-----------

The MSR IOGroup implements the
`geopm::IOGroup(3) <GEOPM_CXX_MAN_IOGroup.3.html>`_ interface to provide
hardware signals and controls for Model Specific Registers (MSRs).

Configuration
~~~~~~~~~~~~~

The set of signals and controls supported by the MSR IOGroup is configurable,
as the MSRs that are available depend on the architecture and the particular
CPU. The MSR IOGroup will declare a set of common signals and controls,
including MSRs for CPU performance, temperature and power.

Additional MSRs can be specified via configuration files. If the
``GEOPM_PLUGIN_PATH`` environment variable is set, the paths specified there
will be checked for any JSON files prefixed with "msr_". The default plugin
path will also be searched. The files must follow this schema:

.. literalinclude:: ../../json_schemas/msrs.schema.json
    :language: json

This manual includes a list of signals and controls that are more commonly
available. Use geopmread and geopmwrite to query the full set of signals and
controls that are available on a particular system.

Concurrent Access
~~~~~~~~~~~~~~~~~

Usage and access to most MSRs is restricted to ring 0. However, there is
no hardware mechanism available to reserve access to MSRs, so access is on a
first-come-first-served basis. As a result, it's possible for a process to
overwrite previously programmed settings by another process. So care must be
taken when using GEOPM with other tools that might also access and configure
MSRs (e.g. a Virtual Machine Monitor or a performance monitoring tool) to
avoid concurrent access and unexpected results.

Please consult the following for further information and guidelines for
sharing access to MSRs: Performance Monitoring Unit Sharing Guide (Intel white
paper).

Signals
-------

* ``TIMESTAMP_COUNTER``:
  An always running, monotonically increasing counter that is incremented at
  a constant rate. For use as a wall clock timer. This is an alias for:
  "MSR::TIME_STAMP_COUNTER:TIMESTAMP_COUNT".

* ``CPU_FREQUENCY_STATUS`` (hertz):
  The current operating frequency of the CPU. This is an alias for:
  "MSR::PERF_STATUS:FREQ".

* ``CPU_FREQUENCY_CONTROL`` (hertz):
  Target operating frequency of the CPU based on the control register. This is
  an alias for: "MSR::PERF_CTL:FREQ".

* ``CPU_UNCORE_FREQUENCY_STATUS``:
  Target operating frequency of the uncore. This is an alias for:
  "MSR::UNCORE_PERF_STATUS:FREQ"

* ``CPU_FREQUENCY_MAX`` (hertz):
  Maximum processor frequency. This is an alias for:
  "MSR::TURBO_RATIO_LIMIT:MAX_RATIO_LIMIT_0".

* ``ENERGY_PACKAGE`` (joules):
  An increasing meter of energy consumed by the package over time. It will
  reset periodically due to roll-over. This is an alias for:
  "MSR::PKG_ENERGY_STATUS:ENERGY".

* ``ENERGY_DRAM`` (joules):
  An increasing meter of energy consumed by the DRAM over time. It will reset
  periodically due to roll-over. This is an alias for:
  "MSR::DRAM_ENERGY_STATUS:ENERGY".

* ``INSTRUCTIONS_RETIRED``:
  The count of the number of instructions executed. This is an alias for:
  "MSR::FIXED_CTR0:INST_RETIRED_ANY".

* ``CYCLES_THREAD``:
  The count of the number of cycles while the logical processor is not in a
  halt state.  The count rate may change based on core frequency. This is an
  alias for: "MSR::FIXED_CTR1:CPU_CLK_UNHALTED_THREAD".

* ``CYCLES_REFERENCE``:
  The count of the number of cycles while the logical processor is not in a
  halt state and not in a stop-clock state. The count rate is fixed at the
  TIMESTAMP_COUNT rate. This is an alias for:
  "MSR::FIXED_CTR2:CPU_CLK_UNHALTED_REF_TSC".

* ``POWER_PACKAGE_MIN`` (watts):
  The minimum power limit based on the electrical specification. This is an
  alias for: "MSR::PKG_POWER_INFO:MIN_POWER".

* ``POWER_PACKAGE_MAX`` (watts):
  The maximum power limit based on the electrical specification. This is an
  alias for: "MSR::PKG_POWER_INFO:MAX_POWER".

* ``POWER_PACKAGE_TDP`` (watts):
  Maximum power to stay within the thermal limits based on the design (TDP).
  This is an alias for: "MSR::PKG_POWER_INFO:THERMAL_SPEC_POWER".

* ``POWER_PACKAGE_LIMIT`` (watts):
  The average power usage limit over the time window specified in
  PL1_TIME_WINDOW. This is an alias for:
  "MSR::PKG_POWER_LIMIT:PL1_POWER_LIMIT".

* ``POWER_PACKAGE_TIME_WINDOW`` (seconds):
  The time window associated with power limit 1. This is an alias for:
  "MSR::PKG_POWER_LIMIT:PL1_TIME_WINDOW".

Controls
--------

* ``CPU_FREQUENCY_CONTROL`` (hertz):
  Set the target operating frequency of the CPU based on the control register.

* ``POWER_PACKAGE_LIMIT`` (watts):
  Set the average power usage limit over the time window specified in
  PL1_TIME_WINDOW.

* ``POWER_PACKAGE_TIME_WINDOW`` (seconds):
  Set the time window associated with power limit 1.

Example
-------

The following example uses geopmread and geopmwrite command-line tools.
These steps can also be followed within an agent.

Setting Frequency
~~~~~~~~~~~~~~~~~

* Set target operating frequency:

``geopmwrite CPU_FREQUENCY_CONTROL core 0 1700000000``

* Read setting and current operating frequency:

``geopmread CPU_FREQUENCY_CONTROL core 0``
``geopmread CPU_FREQUENCY_STATUS core 0``

Setting A Power Limit
~~~~~~~~~~~~~~~~~~~~~

* Set power limit

``geopmwrite POWER_PACKAGE_LIMIT package 0 20``

* Read setting and current power

``geopmread POWER_PACKAGE_LIMIT package 0``
``geopmread POWER_PACKAGE package 0``


See Also
--------

`geopm(7) <geopm.7.html>`_\ ,
`geopm::IOGroup(3) <GEOPM_CXX_MAN_IOGroup.3.html>`_\ ,
`geopmwrite(1) <geopmwrite.1.html>`_\ ,
`geopmread(1) <geopmread.1.html>`_
