
geopm_agent_cpu_activity(7) -- agent for selecting CPU frequency based on CPU compute activity
=================================================================================================






DESCRIPTION
-----------

The goal of this Agent is to save CPU energy by scaling CPU frequency based upon
the compute activity of each CPU as provided by the CPU_COMPUTE_ACTIVITY
signal and modified by the CPU_UTILIZATION signal.

The **CPUActivityAgent** scales the core frequency in the range of ``Fce`` to ``Fcmax``,
where ``Fcmax`` is provided via the policy as ``CPU_FREQ_MAX`` and ``Fce`` is provided via
the policy as ``CPU_FREQ_EFFICIENT``.

Low compute activity regions (compute activity of 0.0) run at the ``Fce`` frequency,
high activity regions (compute activity of 1.0) run at the ``Fcmax`` frequency,
and regions in between the extremes run at a frequency (``Fcreq``) selected using the equation:

``Fcreq = Fce + (Fcmax - Fce) * CPU_COMPUTE_ACTIVITY``

The ``CPU_COMPUTE_ACTIVITY`` is defined as a derivative signal based on the MSR::PPERF::PCNT
scalability metric.

The **CPUActivityAgent** also scales the uncore frequency in the range of
``Fue`` to ``Fumax``, where ``Fumax`` is provided via the policy as ``CPU_UNCORE_FREQ_MAX``
and ``Fue`` is provided via the policy as ``CPU_UNCORE_FREQ_EFFICIENT``.

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

AGENT BEHAVIOR HIGHLIGHTS
-------------------------

Policies and samples are vectors of double precision values where each
value has a meaning defined by the `geopm::Agent(3) <GEOPM_CXX_MAN_Agent.3.html>`_ implementation.
The Agent interface also provides methods for the Agent to extend
reports and traces with additional Agent-specific information.

*
  **Agent Name**:

      Set the ``--geopm-agent`` launch option or ``GEOPM_AGENT`` environment
      variable to ``"cpu_activity"`` and the Controller will select the
      EnergyEfficientAgent for its control handler.  See
      `geopm_launch(1) <geopm_launch.1.html>`_ and `geopm(7) <geopm.7.html>`_ for more information about
      launch options and environment variables.

*
  **Agent Policy Definitions**:

      The ``Fe`` & ``Fmax`` for each domain, as well as ``phi`` and the
      agent sample period are policy values.
      Setting ``Fe`` & ``Fmax`` for each domain to the same value can
      be used to force the entire application to run at a fixed frequency.

  ``CPU_FREQ_MAX``\ :
      The maximum cpu frequency in hertz that the algorithm is
      allowed to choose.  If NAN is passed, it will use the
      maximum available frequency by default.

  ``CPU_FREQ_EFFICIENT``\ :
      The minimum cpu frequency in hertz that the algorithm is
      allowed to choose.  If NAN is passed, it will use the system
      minimum frequency by default.

  ``CPU_UNCORE_FREQ_MAX``\ :
      The maximum cpu uncore frequency in hertz that the algorithm is
      allowed to choose.  If NAN is passed, it will use the
      maximum available frequency by default.

  ``CPU_UNCORE_FREQ_EFFICIENT``\ :
      The minimum cpu uncore frequency in hertz that the algorithm is
      allowed to choose.  If NAN is passed, it will use the system
      minimum frequency by default.

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

  ``MAX_MEMORY_BANDWIDTH_#``\ :
      The maximum possible memory bandwidth associated with the
      same numbered uncore frequency.
      Used to build a mapping of uncore frequencies to maximum
      memory bandwidths for frequency steering.

*
  **Agent Sample Definitions**\ :
  N/A

*
  **Trace Column Extensions**\ :
  N/A

*
  **Report Extensions**\ :

  ``Core Frequency Requests``
      The number of core frequency requests made by the agent

  ``Uncore Frequency Requests``
      The number of uncore frequency requests made by the agent

  ``Resolved Maximum Core Frequency``\ :
     ``Fcmax`` after ``phi`` has been taken into account

  ``Resolved Efficient Core Frequency``\ :
     ``Fce`` after ``phi`` has been taken into account

  ``Resolved Core Frequency Range``\ :
     The core frequency selection range of the agent after ``phi`` has
     been taken into account

  ``Resolved Maximum Uncore Frequency``\ :
     ``Fumax`` after ``phi`` has been taken into account

  ``Resolved Efficient Uncore Frequency``\ :
     ``Fue`` after ``phi`` has been taken into account

  ``Resolved Uncore Frequency Range``\ :
     The uncore frequency selection range of the agent after ``phi`` has
     been taken into account

*

*
  **Control Loop Gate**\ :

      The agent gates the Controller's control loop to a cadence of 10ms.

SEE ALSO
--------

`geopm(7) <geopm.7.html>`_\ ,
`geopm_agent_monitor(7) <geopm_agent_monitor.7.html>`_\ ,
`geopm_agent_energy_efficient(7) <geopm_agent_energy_efficient.7.html>`_\ ,
`geopm::Agent(3) <GEOPM_CXX_MAN_Agent.3.html>`_\ ,
`geopm_agent_c(3) <geopm_agent_c.3.html>`_\ ,
`geopm_prof_c(3) <geopm_prof_c.3.html>`_\ ,
`geopmagent(1) <geopmagent.1.html>`_\ ,
`geopmlaunch(1) <geopmlaunch.1.html>`_
