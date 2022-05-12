
geopm_pio_sst(7) -- Signals and controls for Intel Speed Select Technology
==========================================================================

DESCRIPTION
-----------

The SSTIOGroup implements the `geopm::IOGroup(3)
<GEOPM_CXX_MAN_IOGroup.3.html>`_ interface to provide hardware signals
and controls for key Intel Speed Select Technology features on
supported Intel platforms with Linux Kernel versions >=5.3


SST-CP: Core Power
~~~~~~~~~~~~~~~~~~~

SST Core Power feature enables the user to specify priority of cores
for power distribution in a power-constrained scenario. Cores are
assigned to different Class Level of Service (CLOS). Each CLOS has a
defined min/max frequency and weight, which determines how the
frequency is allocated.


SST-TF: Turbo Frequency
~~~~~~~~~~~~~~~~~~~~~~~

SST Turbo Frequency feature enables the user to dynamically set
different turbo ratio limits for different cores. Cores are specified
as high-priority or low-priority via the SST-CP feature. Cores in CLOS
0 or 1 are high priority and can reach higher than the all-core turbo
ratio limit when there is sufficient power headroom. Cores in CLOS 2
or 3 are constrained below the all-core turbo ratio limit. This
feature only works when SST-CP is enabled.

SIGNALS
-------

SYSTEM INFO
~~~~~~~~~~~

* ``SST::CONFIG_LEVEL``:
  (Package scope) Returns the system's configuration level (SST-PP
  feature)

* ``SST::COREPRIORITY_SUPPORT``:
  (Package scope) Returns 1 if SST-CP feature is supported, 0 if
  unsupported.

* ``SST::TURBOFREQ_SUPPORT``:
  (Package scope) Returns 1 if SST-TF feature is supported, 0 if
  unsupported.

* ``SST::HIGHPRIORITY_NCORES:n``:
  (Package scope) Returns the count of high priority turbo frequency
  cores in bucket n. Buckets 0 - 7 are supported. Buckets are defined
  by the number of high priority cores (cores in CLOS 0 or 1), and
  they determine the frequencies that are obtainable by those
  cores. Generally, if there are fewer high priority cores, the
  increase in turbo frequency limit is greater.

* ``SST::HIGHPRIORITY_FREQUENCY_SSE:n``:
  (Package scope) Returns the high priority turbo frequency for bucket
  n at the SSE license level.

* ``SST::HIGHPRIORITY_FREQUENCY_AVX2:n``:
  (Package scope) Returns the high priority turbo frequency for bucket
  n at the AVX2 license level.

* ``SST::HIGHPRIORITY_FREQUENCY_AVX512:n``:
  (Package scope) Returns the high priority turbo frequency for bucket
  n at the AVX512 license level.

* ``SST::LOWPRIORITY_FREQUENCY:[SSE|AVX2|AVX512]``:
  (Package scope) RReturns the low-priority turbo frequency of the
  specified licence level. Note these frequencies do not change based
  on the number of high priority cores.


CONFIGURATION
~~~~~~~~~~~~~

* ``SST::TURBO_ENABLE``:
  (Package scope) Returns 1 if SST-TF feature is enabled, 0 if
  disabled.

* ``SST::COREPRIORITY_ENABLE``:
  (Package scope) Returns 1 if SST-CP feature is enabled, 0 if
  disabled.

* ``SST::COREPRIORITY:ASSOCIATION``:
  (Core scope) Returns the core's assigned CLOS.

* ``SST::COREPRIORITY:n:WEIGHT``:
  (Package scope) Returns proportional priority for CLOS n. A lower
  weight indicates a higher priority. Weight ranges from 0-15 and is
  used to distribute power amongst cores.

* ``SST::COREPRIORITY:n:FREQUENCY_MIN``:
  (Package scope) Returns the minimum frequency of CLOS n. Given
  sufficient power headroom, all cores will receive this minimum
  frequency before any remaining power is distributed.

* ``SST::COREPRIORITY:n:FREQUENCY_MAX``:
  (Package scope) Returns the maximum frequency of CLOS n. Power will
  not be distributed to cores beyond this maximum frequency.

CONTROLS
--------

* ``SST::TURBO_ENABLE``:
  (Package scope) Enable SST-TF feature.

* ``SST::COREPRIORITY_ENABLE``:
  (Package scope) Enable SST-CP feature.

* ``SST::COREPRIORITY:ASSOCIATION``:
  (Core scope) Assign a core to a CLOS.

* ``SST::COREPRIORITY:n:WEIGHT``:
  (Package scope) Set proportional priority for CLOS n. A lower weight
  indicates a higher priority. Weight ranges from 0-15 and is used to
  distribute power amongst cores.

* ``SST::COREPRIORITY:n:FREQUENCY_MIN``:
  (Package scope) Set the minimum frequency of CLOS n. Given
  sufficient power headroom, all cores will receive this minimum
  frequency before any remaining power is distributed.

* ``SST::COREPRIORITY:n:FREQUENCY_MAX``:
  (Package scope) Set the maximum frequency of CLOS n. Power will not
  be distributed to cores beyond this maximum frequency.

EXAMPLE
-------

The following example uses geopmread and geopmwrite command-line
tools.  These steps can also be followed within an agent. Enabling
steps are also in the ``SSTFrequencyGovernor``.

ENABLING SST-TF
~~~~~~~~~~~~~~~

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

CONFIGURING CLOS
~~~~~~~~~~~~~~~~

* Set the weight 0-15. Lower weight indicates higher priority. CLOS
  priority decreases as the CLOS number increases and weights should
  indicate that to achieve decent behavior.

  ``geopmwrite SST::COREPRIORITY:0:WEIGHT board 0 0``

  ``geopmwrite SST::COREPRIORITY:1:WEIGHT board 0 5``

  ``geopmwrite SST::COREPRIORITY:2:WEIGHT board 0 10``

  ``geopmwrite SST::COREPRIORITY:3:WEIGHT board 0 15``

* Set the min and max frequencies per CLOS.

  ``geopmwrite SST::COREPRIORITY:0:MIN_FREQUENCY board 0 1.5e9``

  ``geopmwrite SST::COREPRIORITY:0:MAX_FREQUENCY board 0 3.6e9``


SETTING CORE PRIORITIES
~~~~~~~~~~~~~~~~~~~~~~~

To assign core 3 to CLOS 1:

  ``geopmwrite SST::COREPRIORITY:ASSOCIATION core 3 1``

SEE ALSO
--------

`geopm(7) <geopm.7.html>`_\ ,
`geopm::IOGroup(3) <GEOPM_CXX_MAN_IOGroup.3.html>`_\ ,
`geopmwrite(1) <geopmwrite.1.html>`_\ ,
`geopmread(1) <geopmread.1.html>`_
