
geopm_agent_cpu_activity(7) -- agent for selecting CPU frequency based on CPU compute activity
=================================================================================================






Description
-----------

The goal of this Agent is to save CPU energy by scaling CPU frequency based upon
the compute activity of each CPU as provided by the CPU_COMPUTE_ACTIVITY
signal and modified by the CPU_UTILIZATION signal.

The **CPUActivityAgent** scales the core frequency in the range of ``Fce`` to ``Fcmax``,
where ``Fcmax`` is provided via the policy or the **MSRIOGroup** and ``Fce`` is provided
via the policy or the **PlatformCharacterizationIOGroup**.

If the policy is used ``Fcmax`` is provided as ``CPU_FREQ_MAX`` and ``Fce`` is provided
as ``CPU_FREQ_EFFICIENT``.  These will override (but not overwrite) and node
local characterization information that would have been provided by the
**PlatformCharacterizationIOGroup** and the **MSRIOGroup**.

If the policy values for ``CPU_FREQ_MAX`` and ``CPU_FREQ_EFFICIENT`` are NAN, the agent
will attempt to read these values from the **MSRIOGroup** and
**PlatformCharacterizationIOGroup**.  In this case ``Fcmax`` is provided by the
``CPU_FREQUENCY_MAX_AVAIL`` high level signal alias and ``Fce`` is provided by the
``NODE_CHARACTERIZATION::CPU_CORE_FREQUENCY_EFFICIENT`` signal.

Low compute activity regions (compute activity of 0.0) run at the ``Fce`` frequency,
high activity regions (compute activity of 1.0) run at the ``Fcmax`` frequency,
and regions in between the extremes run at a frequency (``Fcreq``) selected using the equation:

``Fcreq = Fce + (Fcmax - Fce) * CPU_COMPUTE_ACTIVITY``

The ``CPU_COMPUTE_ACTIVITY`` is defined as a derivative signal based on the ``MSR::PPERF::PCNT``
scalability metric.

The **CPUActivityAgent** also scales the uncore frequency in the range of
``Fue`` to ``Fumax``,  where ``Fumax`` is provided via the policy or the **MSRIOGroup**
and ``Fue`` is provided via the policy or the **PlatformCharacterizationIOGroup**.

If the policy is used ``Fumax`` is provided as ``CPU_UNCORE_FREQ_MAX`` and ``Fce`` is provided
as ``CPU_UNCORE_FREQ_EFFICIENT``.  These will override (but not overwrite) and node
local characterization information that would have been provided by the
**PlatformCharacterizationIOGroup** and the **MSRIOGroup**.

If the policy values for ``CPU_UNCORE_FREQ_MAX`` and ``CPU_UNCORE_FREQ_EFFICIENT`` are NAN,
the agent will attempt to read these values from the **MSRIOGroup** and
**PlatformCharacterizationIOGroup**.  In this case ``Fumax`` is provided by the
``CPU_UNCORE_FREQUENCY_MAX_CONTROL`` high level signal alias and ``Fce`` is provided by
the ``NODE_CHARACTERIZATION::CPU_UNCORE_FREQUENCY_EFFICIENT`` signal.

Low uncore activity regions (uncore activity of 0.0) run at the ``Fue`` frequency,
high activity regions (uncore activity of 1.0) run at the ``Fumax`` frequency,
and regions in between the extremes run at a frequency (``Fureq``) selected using
the equation:

``Fureq = Fue + (Fumax - Fue) * CPU_UNCORE_ACTIVITY``

The ``CPU_UNCORE_ACTIVITY`` is defined as a simple ratio of the current memory bandwdith
divided by the maximum possible bandwidth at the current ``CPU_UNCORE_FREQUENCY_STATUS`` value.

The ``Fe`` value for all domains isintended to be an energy efficient frequency
that is selected via system characterization.  The recommended approach to selecting
``Fe`` for a given domain is to perform a frequency sweep of that domain (CPU or UNCORE)
using a workload that scales strongly with the domain frequency.
Using this approach ``Fe`` will be the frequency that provides the lowest
energy consumption for the workload.

``Fmax`` is intended to be the maximum allowable frequency for the domain,
and may be set as the domain's default  maximum frequency, or limited based
upon user/admin preference.

The **CPUActivityAgent** provides an optional input of ``phi`` that allows for biasing the
frequency range for both domains used by the agent.  The default ``phi`` value of 0.5 provides frequency
selection in the full range from ``Fe`` to ``Fmax``.  A ``phi`` value less than 0.5 biases the
agent towards higher frequencies by increasing the ``Fe`` value provided by the policy.
In the extreme case (``phi`` of 0) ``Fe`` will be raised to ``Fmax``.  A ``phi`` value greater than
0.5 biases the agent towards lower frequencies by reducing the ``Fmax`` value provided
by the policy.  In the extreme case (``phi`` of 1.0) ``Fmax`` will be lowered to ``Fe``.

Agent Name
----------

The agent described in this manual is selected in many geopm
interfaces with the ``"cpu_activity"`` agent name.  This name can be
passed to :doc:`geopmlaunch(1) <geopmlaunch.1>` as the argument to the ``--geopm-agent``
option, or the ``GEOPM_AGENT`` environment variable can be set to this
name (see :doc:`geopm(7) <geopm.7>`\ ).  This name can also be passed to the
:doc:`geopmagent(1) <geopmagent.1>` as the argument to the ``'-a'`` option.

Policy Parameters
-----------------
      The ``Fe`` & ``Fmax`` for each domain, as well as ``phi`` and the
      agent sample period are policy values.
      Setting ``Fe`` & ``Fmax`` for a domain to the same value can
      be used to force the entire application to run at a fixed frequency.

  ``CPU_FREQ_MAX``\ :
      The maximum cpu frequency in hertz that the algorithm is
      allowed to choose.  If NAN is passed, it will use the
      maximum available frequency by default.

  ``CPU_FREQ_EFFICIENT``\ :
      The minimum cpu frequency in hertz that the algorithm is
      allowed to choose.  If NAN is passed, the
      ``NODE_CHARACTERIZATION::CPU_CORE_FREQUENCY_EFFICIENT`` signal
      will be used.  If the signal is not available or uninitialized the
      system minimum frequency will be used by default.

  ``CPU_UNCORE_FREQ_MAX``\ :
      The maximum cpu uncore frequency in hertz that the algorithm is
      allowed to choose.  If NAN is passed, the maximum available
      frequency will be used.

  ``CPU_UNCORE_FREQ_EFFICIENT``\ :
      The minimum cpu uncore frequency in hertz that the algorithm is
      allowed to choose.  If NAN is passed, the
      ``NODE_CHARACTERIZATION::CPU_UNCORE_FREQUENCY_EFFICIENT`` signal
      will be used.  If the signal is not available or uninitialized the
      system minimum frequency will be used by default.

  ``CPU_PHI``\ :
      The performance bias knob.  The value must be between
      0.0 and 1.0. If NAN is passed, it will use 0.5 by default.

  ``SAMPLE_PERIOD``\ :
      The rate at which the agent control loop operates.  10ms by
      default.  Less than 10ms is not recommended.

  ``CPU_UNCORE_FREQ_#``\ :
      The uncore frequency associated with the same numbered
      maximum memory bandwidth.
      Used to build a mapping of uncore frequencies to maximum
      memory bandwidths for frequency steering.
      If this policy value is NAN the corresponding
      ``NODE_CHARACTERIZATION::CPU_UNCORE_FREQUENCY_#`` signal
      will be used.  If all these policy values are NAN and the signals unavailable
      (or uninitialized) then dynamic uncore frequency decisions will be disabled.

  ``MAX_MEMORY_BANDWIDTH_#``\ :
      The maximum possible memory bandwidth associated with the
      same numbered uncore frequency.
      Used to build a mapping of uncore frequencies to maximum
      memory bandwidths for frequency steering.
      If this policy value is NAN the corresponding
      ``NODE_CHARACTERIZATION::CPU_UNCORE_MAXIMUM_MEMORY_BANDWIDTH_#`` signal
      will be used.  If all these policy values are NAN and the signals unavailable
      (or uninitialized) then dynamic uncore frequency decisions will be disabled.

Report Extensions
-----------------

  ``Core Frequency Requests``
      The number of core frequency requests made by the agent

  ``Uncore Frequency Requests``
      The number of uncore frequency requests made by the agent

  ``Initial (Pre-PHI) Maximum Core Frequency``
     ``Fcmax`` before ``phi`` has been taken into account

  ``Initial (Pre-PHI) Efficient Core Frequency``
     ``Fce`` before ``phi`` has been taken into account

  ``Initial (Pre-PHI) Core Frequency Range``
     The core frequency selection range of the agent before ``phi`` has
     been taken into account

  ``Initial (Pre-PHI) Maximum Uncore Frequency``
     ``Fumax`` before ``phi`` has been taken into account

  ``Initial (Pre-PHI) Efficient Uncore Frequency``
     ``Fue`` before ``phi`` has been taken into account

  ``Initial (Pre-PHI) Uncore Frequency Range``
     The uncore frequency selection range of the agent before ``phi`` has
     been taken into account

  ``Actual (Post-PHI) Maximum Core Frequency``
     ``Fcmax`` after ``phi`` has been taken into account

  ``Actual (Post-PHI) Efficient Core Frequency``
     ``Fce`` after ``phi`` has been taken into account

  ``Actual (Post-PHI) Core Frequency Range``
     The core frequency selection range of the agent after ``phi`` has
     been taken into account

  ``Actual (Post-PHI) Maximum Uncore Frequency``
     ``Fumax`` after ``phi`` has been taken into account

  ``Actual (Post-PHI) Efficient Uncore Frequency``
     ``Fue`` after ``phi`` has been taken into account

  ``Actual (Post-PHI) Uncore Frequency Range``
     The uncore frequency selection range of the agent after ``phi`` has
     been taken into account

  ``Uncore Frequency # Maximum Memory Bandwidth``
     The maximum memory bandwidth associated with the frequency ``#``. This
     is the value used to determine ``CPU_UNCORE_ACTIVITY`` when running
     at the specified frequency.

Control Loop Rate
-----------------

      The agent gates the Controller's control loop to a cadence of 10ms.

SEE ALSO
--------

:doc:`geopm(7) <geopm.7>`\ ,
:doc:`geopm_agent_monitor(7) <geopm_agent_monitor.7>`\ ,
:doc:`geopm_agent_energy_efficient(7) <geopm_agent_energy_efficient.7>`\ ,
:doc:`geopm::Agent(3) <GEOPM_CXX_MAN_Agent.3>`\ ,
:doc:`geopm_agent_c(3) <geopm_agent_c.3>`\ ,
:doc:`geopm_prof_c(3) <geopm_prof_c.3>`\ ,
:doc:`geopmagent(1) <geopmagent.1>`\ ,
:doc:`geopmlaunch(1) <geopmlaunch.1>`
