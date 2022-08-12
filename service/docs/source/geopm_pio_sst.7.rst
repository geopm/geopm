geopm_pio_sst(7) -- Signals and controls for Intel Speed Select Technology
==========================================================================

Description
-----------

The SSTIOGroup implements the :doc:`geopm::IOGroup(3)
<GEOPM_CXX_MAN_IOGroup.3>` interface to provide hardware signals
and controls for key Intel Speed Select Technology features on
supported Intel platforms with Linux Kernel versions >=5.3

SST-CP: Core Power
^^^^^^^^^^^^^^^^^^^

SST Core Power feature enables the user to specify priority of cores
for power distribution in a power-constrained scenario. Cores are
assigned to different Class Level of Service (CLOS). Each CLOS has a
defined min/max frequency and weight, which determines how the
frequency is allocated.

SST-TF: Turbo Frequency
^^^^^^^^^^^^^^^^^^^^^^^

SST Turbo Frequency feature enables the user to dynamically set
different turbo ratio limits for different cores. Cores are specified
as high-priority or low-priority via the SST-CP feature. Cores in CLOS
0 or 1 are high priority and can reach higher than the all-core turbo
ratio limit when there is sufficient power headroom. Cores in CLOS 2
or 3 are constrained below the all-core turbo ratio limit. This
feature only works when SST-CP is enabled.

Signals
-------

System Info
^^^^^^^^^^^

``SST::CONFIG_LEVEL``
    Returns the system's configuration level (SST-PP feature)

    * **Aggregation**: expect_same
    * **Domain**: package
    * **Format**: double
    * **Unit**: n/a

``SST::COREPRIORITY_SUPPORT``
    Returns 1 if SST-CP feature is supported, 0 if
    unsupported.

    * **Aggregation**: sum
    * **Domain**: package
    * **Format**: double
    * **Unit**: n/a

``SST::HIGHPRIORITY_NCORES:n``
    Returns the count of high priority turbo frequency
    cores in bucket n. Buckets 0 - 7 are supported. Buckets are defined
    by the number of high priority cores (cores in CLOS 0 or 1), and
    they determine the frequencies that are obtainable by those
    cores. Generally, if there are fewer high priority cores, the
    increase in turbo frequency limit is greater.

    * **Aggregation**: expect_same
    * **Domain**: package
    * **Format**: double
    * **Unit**: n/a

``SST::HIGHPRIORITY_FREQUENCY_SSE:n``
    Returns the high priority turbo frequency for bucket
    *n* at the SSE license level.

    * **Aggregation**: expect_same
    * **Domain**: package
    * **Format**: double
    * **Unit**: n/a

``SST::HIGHPRIORITY_FREQUENCY_AVX2:n``
    Returns the high priority turbo frequency for bucket
    *n* at the AVX2 license level.

    * **Aggregation**: expect_same
    * **Domain**: package
    * **Format**: double
    * **Unit**: n/a

``SST::HIGHPRIORITY_FREQUENCY_AVX512:n``
    Returns the high priority turbo frequency for bucket
    n at the AVX512 license level.

    * **Aggregation**: expect_same
    * **Domain**: package
    * **Format**: double
    * **Unit**: n/a

``SST::LOWPRIORITY_FREQUENCY:[SSE|AVX2|AVX512]``
    Returns the low-priority turbo frequency of the
    specified license level. Note these frequencies do not change based
    on the number of high priority cores.

    * **Aggregation**: expect_same
    * **Domain**: package
    * **Format**: double
    * **Unit**: n/a

``SST::TURBOFREQ_SUPPORT``
    Returns 1 if SST-TF feature is supported, 0 if
    unsupported.

    * **Aggregation**: sum
    * **Domain**: package
    * **Format**: double
    * **Unit**: n/a

Configuration
"""""""""""""

``SST::TURBO_ENABLE``
    Returns 1 if SST-TF feature is enabled, 0 if
    disabled.

    * **Aggregation**: sum
    * **Domain**: package
    * **Format**: double
    * **Unit**: n/a

``SST::COREPRIORITY_ENABLE``
    Returns 1 if SST-CP feature is enabled, 0 if
    disabled.

    * **Aggregation**: expect_same
    * **Domain**: package
    * **Format**: double
    * **Unit**: n/a

``SST::COREPRIORITY:ASSOCIATION``
    Returns the CPU's assigned CLOS.

    * **Aggregation**: expect_same
    * **Domain**: core
    * **Format**: double
    * **Unit**: n/a

``SST::COREPRIORITY:n:PRIORITY``
    Returns proportional priority for CLOS *n*. A lower
    value indicates a higher importance. Priority ranges from 0-1 and is
    used to distribute power amongst cores.

    * **Aggregation**: expect_same
    * **Domain**: package
    * **Format**: double
    * **Unit**: n/a

``SST::COREPRIORITY:n:FREQUENCY_MIN``
    Returns the minimum frequency of CLOS *n*. Given
    sufficient power headroom, all cores will receive this minimum
    frequency before any remaining power is distributed.

    * **Aggregation**: expect_same
    * **Domain**: package
    * **Format**: double
    * **Unit**: n/a


``SST::COREPRIORITY:n:FREQUENCY_MAX``
    Returns the maximum frequency of CLOS *n*. Power will
    not be distributed to cores beyond this maximum frequency.

    * **Aggregation**: expect_same
    * **Domain**: package
    * **Format**: double
    * **Unit**: n/a

Controls
--------

``SST::TURBO_ENABLE``
    Enable SST-TF feature. Enabling SST-TF also causes SST-CP to be enabled.

    * **Aggregation**: sum
    * **Domain**: package
    * **Format**: double
    * **Unit**: n/a

``SST::COREPRIORITY_ENABLE``
    Enable SST-CP feature. Disabling SST-CP also causes SST-TF to be disabled.

    * **Aggregation**: sum
    * **Domain**: package
    * **Format**: double
    * **Unit**: n/a

``SST::COREPRIORITY:ASSOCIATION``
    Assign a core to a CLOS.

    * **Aggregation**: expect_same
    * **Domain**: core
    * **Format**: double
    * **Unit**: n/a

``SST::COREPRIORITY:n:PRIORITY``
    Set proportional priority for CLOS *n*. A lower value
    indicates a higher importance. Weight ranges from 0-1 and is used to
    distribute power amongst cores.

    * **Aggregation**: expect_same
    * **Domain**: package
    * **Format**: double
    * **Unit**: n/a

``SST::COREPRIORITY:n:FREQUENCY_MIN``
    Set the minimum frequency of CLOS *n*. Given
    sufficient power headroom, all cores will receive this minimum
    frequency before any remaining power is distributed.

    * **Aggregation**: expect_same
    * **Domain**: package
    * **Format**: double
    * **Unit**: n/a

``SST::COREPRIORITY:n:FREQUENCY_MAX``
    Set the maximum frequency of CLOS *n*. Power will not
    be distributed to cores beyond this maximum frequency.

    * **Aggregation**: expect_same
    * **Domain**: package
    * **Format**: double
    * **Unit**: n/a

Example
-------

The following example uses geopmread and geopmwrite command-line
tools.  These steps can also be followed within an agent.

Enabling SST-TF
^^^^^^^^^^^^^^^

* Enable SST-CP:

  ``geopmwrite SST::COREPRIORITY_ENABLE:ENABLE board 0 1``

* Enable SST-TF:

  ``geopmwrite SST::TURBO_ENABLE:ENABLE board 0 1``

* Ensure that the turbo ratio limit MSR has been overwritten to allow
  higher all-core turbo frequencies.

  ``geopmwrite MSR::TURBO_RATIO_LIMIT:MAX_RATIO_LIMIT_0 board 0 255e8``

  ``geopmwrite MSR::TURBO_RATIO_LIMIT:MAX_RATIO_LIMIT_1 board 0 255e8``

    ...

  ``geopmwrite MSR::TURBO_RATIO_LIMIT:MAX_RATIO_LIMIT_7 board 0 255e8``

Configuring CLOS
^^^^^^^^^^^^^^^^

* Set the weight 0-1. Lower weight indicates higher priority. CLOS
  priority decreases as the CLOS number increases and weights should
  indicate that to achieve decent behavior.

  ``geopmwrite SST::COREPRIORITY:0:PRIORITY board 0 0``

  ``geopmwrite SST::COREPRIORITY:1:PRIORITY board 0 0.34``

  ``geopmwrite SST::COREPRIORITY:2:PRIORITY board 0 0.67``

  ``geopmwrite SST::COREPRIORITY:3:PRIORITY board 0 1``

* Set the min and max frequencies per CLOS.

  ``geopmwrite SST::COREPRIORITY:0:MIN_FREQUENCY board 0 1.5e9``

  ``geopmwrite SST::COREPRIORITY:0:MAX_FREQUENCY board 0 3.6e9``

Setting Core Priorities
^^^^^^^^^^^^^^^^^^^^^^^

To assign core 3 to CLOS 1:

  ``geopmwrite SST::COREPRIORITY:ASSOCIATION core 3 1``

See Also
--------

:doc:`geopm(7) <geopm.7>`,
:doc:`geopm::IOGroup(3) <GEOPM_CXX_MAN_IOGroup.3>`,
:doc:`geopmwrite(1) <geopmwrite.1>`,
:doc:`geopmread(1) <geopmread.1>`,
:doc:`geopm::Agg(3) <GEOPM_CXX_MAN_Agg.3>`
