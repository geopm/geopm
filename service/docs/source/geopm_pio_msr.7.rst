geopm_pio_msr(7) -- Signals and controls for Model Specific Registers (MSRs)
============================================================================

Description
-----------

The MSR IOGroup implements the
:doc:`geopm::IOGroup(3) <GEOPM_CXX_MAN_IOGroup.3>` interface to provide
hardware signals and controls for Model Specific Registers (MSRs).

Configuration
^^^^^^^^^^^^^

The set of signals and controls supported by the MSR IOGroup is configurable,
as the MSRs that are available depend on the architecture and the particular
CPU. The MSR IOGroup will declare a set of common signals and controls,
including MSRs for CPU performance, temperature and power.

Additional MSRs can be specified via configuration files. If the
``GEOPM_PLUGIN_PATH`` environment variable is set, the paths specified there
will be checked for any JSON files prefixed with ``msr_``. The default plugin
path will also be searched. The files must follow this schema:

.. literalinclude:: ../../json_schemas/msrs.schema.json
    :language: json

This manual includes a list of signals and controls that are more commonly
available. Use geopmread and geopmwrite to query the full set of signals and
controls that are available on a particular system.

Concurrent Access
^^^^^^^^^^^^^^^^^

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

Aliases
-------

This IOGroup provides the following high-level aliases:

Signal Aliases
^^^^^^^^^^^^^^

``CPU_FREQUENCY_MAX_CONTROL``
    Aliases to MSR::PERF_CTL:FREQ

``CPU_FREQUENCY_STATUS``
    Aliases to MSR::PERF_STATUS:FREQ

``CPU_UNCORE_FREQUENCY_STATUS``
    Aliases to MSR::UNCORE_PERF_STATUS:FREQ

``CPU_FREQUENCY_MAX``
    Aliases to MSR::TURBO_RATIO_LIMIT:MAX_RATIO_LIMIT_0

``CPU_CYCLES_REFERENCE``
    Aliases to MSR::FIXED_CTR2:CPU_CLK_UNHALTED_REF_TSC

``CPU_CYCLES_THREAD``
    Aliases to MSR::FIXED_CTR1:CPU_CLK_UNHALTED_THREAD

``DRAM_ENERGY``
    Aliases to MSR::DRAM_ENERGY_STATUS:ENERGY

``CPU_ENERGY``
    Aliases to MSR::PKG_ENERGY_STATUS:ENERGY

``CPU_INSTRUCTIONS_RETIRED``
    Aliases to MSR::FIXED_CTR0:INST_RETIRED_ANY

``CPU_POWER_LIMIT``
    Aliases to MSR::PKG_POWER_LIMIT:PL1_POWER_LIMIT

``CPU_POWER_MAX``
    Aliases to MSR::PKG_POWER_INFO:MAX_POWER

``CPU_POWER_MIN``
    Aliases to MSR::PKG_POWER_INFO:MIN_POWER

``CPU_POWER_TDP``
    Aliases to MSR::PKG_POWER_INFO:THERMAL_SPEC_POWER

``CPU_POWER_TIME_WINDOW``
    Aliases to MSR::PKG_POWER_LIMIT:PL1_TIME_WINDOW

``CPU_TIMESTAMP_COUNTER``
    Aliases to MSR::TIME_STAMP_COUNTER:TIMESTAMP_COUNT

Control Aliases
^^^^^^^^^^^^^^^

``CPU_FREQUENCY_CONTROL``
    Aliases to MSR::PERF_CTL:FREQ

``POWER_PACKAGE_LIMIT``
    Aliases to MSR::PKG_POWER_LIMIT:PL1_POWER_LIMIT

``POWER_PACKAGE_TIME_WINDOW``
    Aliases to MSR::PKG_POWER_LIMIT:PL1_TIME_WINDOW

Example
-------

The following example uses geopmread and geopmwrite command-line tools.
These steps can also be followed within an agent.

Setting Frequency
^^^^^^^^^^^^^^^^^

* Set target operating frequency:

``geopmwrite CPU_FREQUENCY_CONTROL core 0 1700000000``

* Read setting and current operating frequency:

``geopmread CPU_FREQUENCY_CONTROL core 0``
``geopmread CPU_FREQUENCY_STATUS core 0``

Setting A Power Limit
^^^^^^^^^^^^^^^^^^^^^

* Set power limit

``geopmwrite POWER_PACKAGE_LIMIT package 0 20``

* Read setting and current power

``geopmread POWER_PACKAGE_LIMIT package 0``
``geopmread POWER_PACKAGE package 0``


See Also
--------

:doc:`geopm_pio(7) <geopm_pio.7>`\ ,
:doc:`geopm(7) <geopm.7>`\ ,
:doc:`geopm::IOGroup(3) <GEOPM_CXX_MAN_IOGroup.3>`\ ,
:doc:`geopmwrite(1) <geopmwrite.1>`\ ,
:doc:`geopmread(1) <geopmread.1>`
