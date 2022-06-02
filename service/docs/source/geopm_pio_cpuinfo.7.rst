geopm_pio_cpu(7) -- Signals and controls for CPU Info IO Group
==============================================================

Description
-----------

The CPUInfoIOGroup implements the :doc:`geopm::IOGroup(3)
<GEOPM_CXX_MAN_IOGroup.3>` interface to provide signals for CPU-related
information on Intel Architecture.


Signals
-------
``CPUINFO::FREQ_MIN``
    Returns the CPUs minimum achievable frequency.

    * **Aggregation**: N/A (all CPUs on a board should return the same value)

    * **Domain**: CPU

    * **Format**: Integer

    * **Unit**: Hertz

``CPUINFO::FREQ_MAX``
    Returns the CPUs maximum achievable frequency. This is the frequency that
    a single CPU can achieve when all other CPUs are in C6, turbo is enabled,
    and the system is not power constrained.

   * **Aggregation**: N/A (all CPUs on a board should return the same value)

   * **Domain**: CPU

   * **Format**: Integer

   * **Unit**: Hertz

``CPUINFO::FREQ_STICKER``
    Returns the processor base frequency. This is the maximum guaranteed
    achievable frequency.

   * **Aggregation**: N/A (all CPUs on a board should return the same value)

   * **Domain**: CPU

   * **Format**: Integer

   * **Unit**: Hertz

``CPUINFO::FREQ_STEP``
    Returns the set size between process frequency settings. 

   * **Aggregation**: N/A (all CPUs on a board should return the same value)

   * **Domain**: CPU

   * **Format**: Integer

   * **Unit**: Hertz

Signal Aliases
~~~~~~~~~~~~~~
Several high level aliases are provided.  Their mapping  to
underlying IO Group signals is provided below.

``CPU_FREQUENCY_MIN``
    Aliases to ``CPUINFO::FREQ_MIN``

``CPU_FREQUENCY_STICKER``
    Aliases to ``CPUINFO::FREQ_STICKER``

``CPU_FREQUENCY_STEP``
    Aliases to ``CPUINFO::FREQ_STEP``

See Also
--------

:doc:`geopm(7) <geopm.7>`\ ,
:doc:`geopm::IOGroup(3) <GEOPM_CXX_MAN_IOGroup.3>`\ ,
:doc:`geopmwrite(1) <geopmwrite.1>`\ ,
:doc:`geopmread(1) <geopmread.1>`
