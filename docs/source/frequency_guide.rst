User Guide for Frequency Control
================================

Description
-----------
GEOPM supports many interfaces to manipulate frequency. This guide provides
specifics on each of these interfaces including suggested use, implementation
nuances, and common pitfalls. Supported interfaces include:

* Direct frequency requests on CPU core/uncore and GPU
* Hardware P-States (HWP) on CPU
* Speed Select Technology - Core Priority (SST-CP) and Turbo Frequency (SST-TF)
  on Intel CPUs ICX+

GEOPM frequency decisions interact with the OS frequency drivers. This
interaction is detailed below. At a high level, frequency drivers may include
settings to drive frequency with the objective of achieving greater performance
or to lower power consumption, while other frequency driver settings may adhere
more strictly to user frequency requests. The interaction between driver and
GEOPM-driven frequency setting will vary depending upon the interface used.
Generally speaking, if a ``userspace`` governor is available on a given driver,
it is more likely to play nicely with GEOPM.

The ideal mechanism of frequency control depends of course on the use case and
priorities of the user. If performance repeatability is critical or the user
knows of ideal frequency settings, direct frequency requests are likely ideal.
If the user wishes to leverage as much of the power headroom as possible, SST
or HWP interfaces are useful. If the user also wishes the system to steer power
intelligently between cores and uncore based on internal telemetry, HWP will work
best. If computation on a given node is heterogeneous (i.e. some CPUs are given
more work or more critical work than others), SST features are likely to work
best.

Linux Frequency Drivers
-----------------------

Linux CPU driver information can be found using ``cpupower`` as shown in the
example below. If the driver has a ``userspace`` CPU governor available, that will
generally not interfere with GEOPM frequency decisions.

.. code-block:: bash

    $ cpupower frequency-info
    analyzing CPU 0:
      driver: acpi-cpufreq
      CPUs which run at the same hardware frequency: 0
      CPUs which need to have their frequency coordinated by software: 0
      maximum transition latency: 10.0 us
      hardware limits: 1000 MHz - 2.10 GHz
      available frequency steps:  2.10 GHz, 2.10 GHz, 2.00 GHz, 1.90 GHz, 1.80 GHz, 1.70 GHz, 1.60 GHz, 1.50 GHz, 1.40 GHz, 1.30 GHz, 1.20 GHz, 1.10 GHz, 1000 MHz
      available cpufreq governors: ondemand performance schedutil
      current policy: frequency should be within 1000 MHz and 2.10 GHz.
                      The governor "performance" may decide which speed to use
                      within this range.
      current CPU frequency: Unable to call hardware
      current CPU frequency: 1.02 GHz (asserted by call to kernel)
      boost state support:
        Supported: yes
        Active: yes

When using the ``userspace`` or ``performance`` governor, the ``acpi-cpufreq``
driver will not interfere with frequency decisions made by GEOPM. Other
governors may interfere with GEOPM frequency settings.

The ``intel_pstate`` driver allows users access to ``Hardware P-States`` on
systems that support that feature. The ``performance`` governor is more likely
to adhere to GEOPM frequency decisions. The ``powersave`` governor tends to
lower CPU frequency when cores are running idle.

Direct Frequency Requests
-------------------------

This is the most straightforward mechanism for selecting a specific frequency.

.. code-block:: bash

    $ geopmread CPU_FREQUENCY_STATUS core 0
    3400000000

    $ geopmwrite MSR::PERF_CTL:FREQ core 0 1.2e9
    $ geopmread CPU_FREQUENCY_STATUS core 0
    1200000000

To set CPU uncore frequency, both min and max can be specified. To fix
uncore frequency to a specific value, set ``CPU_UNCORE_FREQUENCY_MAX_CONTROL``
and ``CPU_UNCORE_FREQUENCY_MIN_CONTROL`` to the same value. Note that there is
no guarantee that the specified frequencies will be achieved, as the system may
be limited by hardware constraints.

GPU Frequency
~~~~~~~~~~~~~

Similar to CPU uncore frequency, GPU frequencies are specified via a min and max.
Some hardware has underlying logic that require
``GPU_CORE_FREQUENCY_MIN_CONTROL`` to always be less than
``GPU_CORE_FREQUENCY_MAX_CONTROL``. If you try to set the min control greater
than the max or the max control less than the min, it will result in a failure.
To avoid this issue, use the batch interface on geopmwrite.

Example: Set GPU 0 frequency to 1.0 GHz

.. code-block:: bash

    $ echo -e "GPU_CORE_FREQUENCY_MAX_CONTROL gpu 0 1e9\
               \nGPU_CORE_FREQUENCY_MIN_CONTROL gpu 0 1e9"\
               | geopmwrite -f -"

Example: Let GPU 1 frequency float between 800 MHz and 1.2 GHz

.. code-block:: bash

    $ echo -e "GPU_CORE_FREQUENCY_MAX_CONTROL gpu 1 8e8\
               \nGPU_CORE_FREQUENCY_MIN_CONTROL gpu 1 1.2e9"\
               | geopmwrite -f -"

Example: Set all GPU frequencies to use the full frequency range.

.. code-block:: bash

     $ echo -e "GPU_CORE_FREQUENCY_MIN_CONTROL board 0\
                `geopmread GPU_CORE_FREQUENCY_MIN_AVAIL board 0`\
                \nGPU_CORE_FREQUENCY_MAX_CONTROL board 0\
                `geopmread GPU_CORE_FREQUENCY_MAX_AVAIL board 0`"\
                | geopmwrite -f -

Intel Hardware P-States (HWP)
-----------------------------

Users of Intel Hardware P-States (HWP) can set per-CPU or per-socket hints,
like desired energy-perf bias and suggested min/max frequency range. These
hints are ingested along with hardware telemetry and used to allow the CPU
to steer its own frequency setting dynamically. This allows the system to 
respond more rapidly than the OS could to changes in workload behavior while
maintaining power requirements and desired user priority.

The main control mechanism with HWP is the per-core
``ENERGY_PERFORMANCE_PREFERENCE`` or EPP. This is a value 0-15, where 0 is
higher performance and 15 is higher energy efficiency. The user can also
specify a frequency range per-core. Typically, a core with an EPP of 0 will
try to reach the higher end of the frequency range while a core with an EPP of
15 will try to remain at the lower end of its frequency range. If no frequency
range is specified, the core will have access to the full frequency range. Note
that setting min and max frequencies does not guarantee performance. Cores are still bound by
power/thermal/current limits and can be restricted beyond the minimum frequency
if required to do so by hardware limitations.

``HWP_CAPABILITIES`` includes frequencies of interest. The full frequency range
is [``LOWEST_PERFORMANCE``, ``HIGHEST_PERFORMANCE``], and
``GUARANTEED_PERFORMANCE`` is the frequency that all cores should be able to
reach simultaneously when running a typical power-hungry workload.

Using GEOPM with HWP
~~~~~~~~~~~~~~~~~~~~

Enabling HWP on a system cannot be done using GEOPM. This is because a system
reset is required to disable it once it has been enabled. Thus, it interferes
with GEOPM's save/restore capabilities.

The instructions below assume that the ``intel_pstate`` driver is active.

To check the status of HWP, use ``HWP_ENABLE``. A return value of 0 indicates
HWP is disabled.

.. code-block:: bash

    $ geopmread MSR::PM_ENABLE:HWP_ENABLE package 0
    1

Set a core's HWP parameters (and check frequency to see the change reflected).
Here we will try to make core 0 be more power hungry and core 4 be more energy
efficient.

.. code-block:: bash

    $ # Check core 0 and 4 frequency (running idle)
    $ geopmread CPU_FREQUENCY_STATUS cpu 0
    1000000000
    $ geopmread CPU_FREQUENCY_STATUS cpu 4
    1000000000
    $ geopmwrite MSR::HWP_REQUEST:ENERGY_PERFORMANCE_PREFERENCE cpu 4 15
    $ geopmwrite MSR::HWP_REQUEST:MAXIMUM_PERFORMANCE cpu 4 1.4e9
    $ geopmwrite MSR::HWP_REQUEST:ENERGY_PERFORMANCE_PREFERENCE cpu 0 0
    $ geopmwrite MSR::HWP_REQUEST:MINIMUM_PERFORMANCE cpu 0 2.2e9
    $ geopmread CPU_FREQUENCY_STATUS cpu 0
    2200000000
    $ geopmread CPU_FREQUENCY_STATUS cpu 4
    1000000000


Intel Speed Select Technology (SST)
-----------------------------------

Intel Speed Select Technology (SST) includes a multitude of features for
heterogeneous power steering on CPUs. GEOPM supports SST-CP (Core Priority) and
SST-TF (Turbo Frequency). SST-CP is used to assign cores to different priority
buckets numbered 0-3, where 0 is highest priority and 3 is lowest priority for power
distribution. This feature uses similar controls as HWP. If the power budget is
exceeded, the frequency of lower priority cores is decreased more than the frequency
of higher priority cores.

SST-TF allows cores in high priority buckets 0/1 to achieve higher turbo
frequencies than is typically allowed by reducing the maximum turbo frequencies
of the lower priority cores. That way, even in power constrained scenarios
where all cores are active, cores with high priority work can be granted a
higher proportion of the power budget. This is achieved without violating any
hardware constraints, unlike overclocking.

These features are described in detail in the
:doc:`geopm_pio_sst(7)<geopm_pio_sst.7>` man page.
