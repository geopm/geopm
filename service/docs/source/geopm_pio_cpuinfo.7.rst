geopm_pio_cpuinfo(7) -- Signals and controls for the CPUInfoIOGroup
===================================================================

Description
-----------

The CPUInfoIOGroup implements the :doc:`geopm::IOGroup(3)
<GEOPM_CXX_MAN_IOGroup.3>` interface to provide signals for CPU-related
information on Intel Architecture.

Signals
-------
``CPUINFO::FREQ_MIN``
    Returns the CPUs minimum achievable frequency.

    * **Aggregation**: expect_same
    * **Domain**: cpu
    * **Format**: double
    * **Unit**: hertz

``CPUINFO::FREQ_MAX``
    In the case where the acpi_cpufreq driver is enabled this returns
    a value one MHz above the ``CPUINFO::FREQ_STICKER`` value.  In the
    case where intel_pstate driver is enabled this returns the CPUs
    maximum achievable frequency. This is the frequency that a single
    CPU can achieve when all other CPUs are in C6, turbo is enabled,
    and the system is not power constrained.

    * **Aggregation**: expect_same
    * **Domain**: cpu
    * **Format**: double
    * **Unit**: hertz

``CPUINFO::FREQ_STICKER``
    Returns the processor base frequency. This is the maximum guaranteed
    achievable frequency.

    * **Aggregation**: expect_same
    * **Domain**: cpu
    * **Format**: double
    * **Unit**: hertz

``CPUINFO::FREQ_STEP``
    Returns the step size between process frequency settings.

    * **Aggregation**: expect_same
    * **Domain**: cpu
    * **Format**: double
    * **Unit**: hertz

Aliases
-------

This IOGroup provides the following high-level aliases:

Signal Aliases
^^^^^^^^^^^^^^

``CPU_FREQUENCY_MIN``
    Maps to ``CPUINFO::FREQ_MIN``

``CPU_FREQUENCY_STICKER``
    Maps to ``CPUINFO::FREQ_STICKER``

``CPU_FREQUENCY_STEP``
    Maps to ``CPUINFO::FREQ_STEP``

See Also
--------

:doc:`geopm(7) <geopm.7>`,
:doc:`geopm::IOGroup(3) <GEOPM_CXX_MAN_IOGroup.3>`,
:doc:`geopmwrite(1) <geopmwrite.1>`,
:doc:`geopmread(1) <geopmread.1>`,
:doc:`geopm::Agg(3) <GEOPM_CXX_MAN_Agg.3>`
