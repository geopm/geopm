geopm_pio_msr(7) -- Signals and controls for Model Specific Registers (MSRs)
============================================================================

Description
-----------

The MSR IOGroup implements the
:doc:`geopm::IOGroup(3) <geopm::IOGroup.3>` interface to provide
hardware signals and controls for Model Specific Registers (MSRs).

Configuration
^^^^^^^^^^^^^

The set of signals and controls supported by the MSR IOGroup is configurable,
as the MSRs that are available depend on the architecture and the particular
CPU. The MSR IOGroup will declare a set of common signals and controls,
including MSRs for CPU performance, temperature and power.

Additional MSRs can be specified via configuration files. If the
``GEOPM_MSR_CONFIG_PATH`` environment variable is set, the paths specified there
will be checked for any JSON files prefixed with ``msr_``. The ``/etc/geopm``
directory will also be searched. The files must follow this schema:

.. literalinclude:: ../json_schemas/msrs.schema.json
    :language: json

For an example of an MSR configuration file, please see:
`<msr_reasons.json> <https://github.com/geopm/geopm/blob/dev/examples/custom_msr/msr_reasons.json>`_

.. note::

   Before GEOPM 3.0, MSR configuration files were stored near the GEOPM
   library objects (e.g., in ``/usr/lib64/geopm``), and discovered in the
   search paths from the ``GEOPM_PLUGIN_PATH`` environment variable. Since
   version 3.0, GEOPM additionally searches ``/etc/geopm`` and locations from
   the ``GEOPM_MSR_CONFIG_PATH`` environment variable. Future releases may
   stop searching the library and plugin paths for MSR configuration data.
   If GEOPM uses a configuration from the old location, then a deprecation
   warning is emitted.

This guide includes a list of signals and controls that are more commonly
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

Signals
-------
Some MSR signals are available on specific miroarchitectures.
:ref:`Architectural signals <geopm_pio_msr.7:architectural signals>` are
available across Intel x86 microarchitectures.

.. contents:: Categories of MSR signals:
   :local:

Architectural Signals
^^^^^^^^^^^^^^^^^^^^^
.. geopm-msr-json:: ../json_data/msr_data_arch.json
   :no-controls:

Knights Landing (KNL) Signals
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
.. geopm-msr-json:: ../json_data/msr_data_knl.json
   :no-controls:

Sandy Bridge (SNB) Signals
^^^^^^^^^^^^^^^^^^^^^^^^^^
.. geopm-msr-json:: ../json_data/msr_data_snb.json
   :no-controls:

Haswell (HSX) Signals
^^^^^^^^^^^^^^^^^^^^^
.. geopm-msr-json:: ../json_data/msr_data_hsx.json
   :no-controls:

Skylake (SKX) Signals
^^^^^^^^^^^^^^^^^^^^^
.. geopm-msr-json:: ../json_data/msr_data_skx.json
   :no-controls:

``MSR::CPU_SCALABILITY_RATIO``
    Measure of CPU Scalability as determined by the derivative
    of PCNT divided by the derivative of ACNT over 8 samples.

    *  **Aggregation**: average
    *  **Domain**: cpu
    *  **Format**: double
    *  **Unit**: none

Controls
--------
Some MSR controls are available on specific miroarchitectures.
:ref:`Architectural controls <geopm_pio_msr.7:architectural controls>` are
available across Intel x86 microarchitectures.

.. contents:: Categories of MSR controls:
   :local:

Architectural Controls
^^^^^^^^^^^^^^^^^^^^^^
.. geopm-msr-json:: ../json_data/msr_data_arch.json
   :no-signals:

Knights Landing (KNL) Controls
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
.. geopm-msr-json:: ../json_data/msr_data_knl.json
   :no-signals:

Sandy Bridge (SNB) Controls
^^^^^^^^^^^^^^^^^^^^^^^^^^^
.. geopm-msr-json:: ../json_data/msr_data_snb.json
   :no-signals:

Haswell (HSX) Controls
^^^^^^^^^^^^^^^^^^^^^^
.. geopm-msr-json:: ../json_data/msr_data_hsx.json
   :no-signals:

Skylake (SKX) Controls
^^^^^^^^^^^^^^^^^^^^^^
.. geopm-msr-json:: ../json_data/msr_data_skx.json
   :no-signals:


Aliases
-------

This IOGroup provides the following high-level aliases:

Signal Aliases
^^^^^^^^^^^^^^

``BOARD_ENERGY``
    Maps to ``MSR::PLATFORM_ENERGY_STATUS:ENERGY``

``BOARD_POWER``
    Maps to ``MSR::BOARD_POWER``

``BOARD_POWER_LIMIT_CONTROL``
    Maps to ``MSR::PLATFORM_POWER_LIMIT:PL1_POWER_LIMIT``

``BOARD_POWER_TIME_WINDOW_CONTROL``
    Maps to ``MSR::PLATFORM_POWER_LIMIT:PL1_TIME_WINDOW``

``CPU_CORE_TEMPERATURE``
    Maps to ``MSR::THERM_STATUS:DIGITAL_READOUT``

``CPU_CYCLES_REFERENCE``
    Maps to ``MSR::MPERF:MCNT``

``CPU_CYCLES_THREAD``
    Maps to ``MSR::APERF:ACNT``

``CPU_ENERGY``
    Maps to ``MSR::PKG_ENERGY_STATUS:ENERGY``

``CPU_FREQUENCY_MAX_CONTROL``
    Maps to ``MSR::HWP_REQUEST:MAXIMUM_PERFORMANCE`` or to ``MSR::PERF_CTL:FREQ`` if HWP is disabled.

``CPU_FREQUENCY_MAX_AVAIL``
    Maps to ``MSR::TURBO_RATIO_LIMIT:MAX_RATIO_LIMIT_0``

``CPU_FREQUENCY_MIN_CONTROL``
    Maps to ``MSR::HWP_REQUEST:MINIMUM_PERFORMANCE``.  Not exposed if HWP is disabled.

``CPU_FREQUENCY_STATUS``
    Maps to ``MSR::PERF_STATUS:FREQ``

``CPU_INSTRUCTIONS_RETIRED``
    Maps to ``MSR::FIXED_CTR0:INST_RETIRED_ANY``.  Requires the fixed counters to be enabled (``geopmwrite -e``).

``CPU_PACKAGE_TEMPERATURE``
    Maps to ``MSR::PACKAGE_THERM_STATUS:DIGITAL_READOUT``

``CPU_POWER``
    Average package power over 40 ms or 8 control loop iterations.

``CPU_POWER_LIMIT_CONTROL``
    Maps to ``MSR::PKG_POWER_LIMIT:PL1_POWER_LIMIT``

``CPU_POWER_MAX_AVAIL``
    Maps to ``MSR::PKG_POWER_INFO:MAX_POWER``

``CPU_POWER_MIN_AVAIL``
    Maps to ``MSR::PKG_POWER_INFO:MIN_POWER``

``CPU_POWER_LIMIT_DEFAULT``
    Maps to ``MSR::PKG_POWER_INFO:THERMAL_SPEC_POWER``

``CPU_POWER_TIME_WINDOW_CONTROL``
    Maps to ``MSR::PKG_POWER_LIMIT:PL1_TIME_WINDOW``

``CPU_TIMESTAMP_COUNTER``
    Maps to ``MSR::TIME_STAMP_COUNTER:TIMESTAMP_COUNT``

``CPU_UNCORE_FREQUENCY_MAX_CONTROL``
    Maps to ``MSR::UNCORE_RATIO_LIMIT:MAX_RATIO``

``CPU_UNCORE_FREQUENCY_MIN_CONTROL``
    Maps to ``MSR::UNCORE_RATIO_LIMIT:MIN_RATIO``

``CPU_UNCORE_FREQUENCY_STATUS``
    Maps to ``MSR::UNCORE_PERF_STATUS:FREQ``

``DRAM_ENERGY``
    Maps to ``MSR::DRAM_ENERGY_STATUS:ENERGY``

``DRAM_POWER``
    Average DRAM power over 40 ms or 8 control loop iterations.

``MSR::BOARD_ENERGY``
    Maps to ``MSR::PLATFORM_ENERGY_STATUS:ENERGY``

``MSR::BOARD_POWER``
    Average board power over 40 ms or 8 control loop iterations.  Derived from ``MSR::BOARD_ENERGY``

``MSR::QM_CTR_SCALED``
    Maps to ``MSR::QM_CTR:RM_DATA``, scaled by the processor's counter resolution for bandwidth accounting in bytes.

``MSR::QM_CTR_SCALED_RATE``
    Maps to the rate of change in ``MSR::QM_CTR_SCALED``.

Control Aliases
^^^^^^^^^^^^^^^

``CPU_FREQUENCY_DESIRED_CONTROL``
    Maps to ``MSR::HWP_REQUEST:DESIRED_PERFORMANCE``

``CPU_FREQUENCY_MAX_CONTROL``
    Maps to ``MSR::HWP_REQUEST:MAXIMUM_PERFORMANCE`` or to ``MSR::PERF_CTL:FREQ`` if HWP is disabled

``CPU_FREQUENCY_MIN_CONTROL``
    Maps to ``MSR::HWP_REQUEST:MINIMUM_PERFORMANCE``.  Not exposed if HWP is disabled.

``CPU_POWER_LIMIT_CONTROL``
    Maps to ``MSR::PKG_POWER_LIMIT:PL1_POWER_LIMIT``

``CPU_POWER_TIME_WINDOW_CONTROL``
    Maps to ``MSR::PKG_POWER_LIMIT:PL1_TIME_WINDOW``

``CPU_UNCORE_FREQUENCY_MAX_CONTROL``
    Maps to ``MSR::UNCORE_RATIO_LIMIT:MAX_RATIO``

``CPU_UNCORE_FREQUENCY_MIN_CONTROL``
    Maps to ``MSR::UNCORE_RATIO_LIMIT:MIN_RATIO``

Example
-------

The following example uses geopmread and geopmwrite command-line tools.
These steps can also be followed within an agent.

Setting Frequency
^^^^^^^^^^^^^^^^^

* Set target operating frequency:

``geopmwrite CPU_FREQUENCY_MAX_CONTROL core 0 1700000000``

* Read setting and current operating frequency:

``geopmread CPU_FREQUENCY_MAX_CONTROL core 0``
``geopmread CPU_FREQUENCY_STATUS core 0``

Setting A Power Limit
^^^^^^^^^^^^^^^^^^^^^

* Set power limit

``geopmwrite CPU_POWER_LIMIT_CONTROL package 0 20``

* Read setting and current power

``geopmread CPU_POWER_LIMIT_CONTROL package 0``
``geopmread CPU_POWER package 0``


See Also
--------

:doc:`geopm_pio(7) <geopm_pio.7>`\ ,
:doc:`geopm(7) <geopm.7>`\ ,
:doc:`geopm::IOGroup(3) <geopm::IOGroup.3>`\ ,
:doc:`geopmwrite(1) <geopmwrite.1>`\ ,
:doc:`geopmread(1) <geopmread.1>`
