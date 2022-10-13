geopm_agent_cpu_activity(7) -- agent for selecting CPU frequency based on CPU compute activity
=================================================================================================

Description
-----------

.. note::
    This is currently an experimental agent and is only available when
    building GEOPM with the ``--enable-beta`` flag. Some areas or aspects that
    are subject to change include its interface (e.g. the policy) and
    algorithm. It is also possible that this agent may be refactored and
    combined with other agents.

The goal of **CPUActivityAgent** is to save CPU energy by scaling CPU frequency
based upon the compute activity of each CPU as provided by the
CPU_COMPUTE_ACTIVITY signal and modified by the CPU_UTILIZATION signal.

The agent scales the core frequency in the range of ``Fce`` to ``Fcmax``, where
``Fcmax`` is provided via the policy as ``CPU_FREQ_MAX`` and ``Fce`` is provided via
the policy as ``CPU_FREQ_EFFICIENT``.

Low compute activity regions (compute activity of 0.0) run at the ``Fce`` frequency,
high activity regions (compute activity of 1.0) run at the ``Fcmax`` frequency,
and regions in between the extremes run at a frequency (``Fcreq``) selected using the equation:

``Fcreq = Fce + (Fcmax - Fce) * CPU_COMPUTE_ACTIVITY``

The ``CPU_COMPUTE_ACTIVITY`` is defined as a derivative signal based on the MSR::PPERF::PCNT
scalability metric.

The agent also scales the uncore frequency in the range of ``Fue`` to
``Fumax``, where ``Fumax`` is provided via the policy as ``CPU_UNCORE_FREQ_MAX``
and ``Fue`` is provided via the policy as ``CPU_UNCORE_FREQ_EFFICIENT``.

Low uncore activity regions (uncore activity of 0.0) run at the ``Fue`` frequency,
high activity regions (uncore activity of 1.0) run at the ``Fumax`` frequency,
and regions in between the extremes run at a frequency (``Fureq``) selected using
the equation:

``Fureq = Fue + (Fumax - Fue) * CPU_UNCORE_ACTIVITY``

The ``CPU_UNCORE_ACTIVITY`` is defined as a simple ratio of the current memory bandwdith
divided by the maximum possible bandwidth at the current ``CPU_UNCORE_FREQUENCY_STATUS`` value.

The ``Fe`` value for all domains is intended to be an energy efficient frequency
that is selected via system characterization.  The recommended approach to selecting
``Fe`` for a given domain is to perform a frequency sweep of that domain (CPU or UNCORE)
using a workload that scales strongly with the domain frequency.
Using this approach ``Fe`` will be the frequency that provides the lowest
energy consumption for the workload.

``Fmax`` is intended to be the maximum allowable frequency for the domain,
and may be set as the domain's default maximum frequency, or limited based
upon user/admin preference.

The agent provides an optional input of ``phi`` that allows for biasing the
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

The ``Fe`` & ``Fmax`` for each domain and the ``phi`` input
are policy values.
Setting ``Fe`` & ``Fmax`` for a domain to the same value can
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

Policy Generation
-----------------

Generating a policy for the CPU Compute Activity agent requires system
characterization using the GEOPM experiment infrastructure located in
``$GEOPM_SOURCE/integration/experiment/`` and the aritmetic intensity
benchmark located in ``$GEOPM_SOURCE/integration/apps/arithmetic_intensity``.
Prior to starting, the arithmetic intensity benchmark needs to be built (use
the ``build.sh`` script provided in the benchmark's folder).


.. note::
    Before performing the system characterization, please ensure the
    system is quiesced (i.e. not running other heavy processes/workloads).

The first step is to generate the execution script by running::

    gen_slurm.sh 1 arithmetic_intensity uncore_frequency_sweep

The generated ``test.sbatch`` should be modified to enable Memory Bandwidth
Monitoring by adding the following above the experiment script invocation::

    srun -N ${SLURM_NNODES} geopmwrite MSR::PQR_ASSOC:RMID board 0 0
    srun -N ${SLURM_NNODES} geopmwrite MSR::QM_EVTSEL:RMID board 0 0
    srun -N ${SLURM_NNODES} geopmwrite MSR::QM_EVTSEL:EVENT_ID board 0 2

Without this, the uncore bandwidth characteriztaion analysis scripts will not
be able to accurately determine the maximum memory bandwidth at each uncore
frequency.

Additionally the ``test.sbatch`` should be modified to include the following
experiment options, where the text within angle brackets (``<>``) needs to be
replaced with relevant system (or administrator chosen) values::

    --geopm-report-signals="MSR::QM_CTR_SCALED_RATE@package,CPU_UNCORE_FREQUENCY_STATUS@package,MSR::CPU_SCALABILITY_RATIO@package,CPU_FREQUENCY_MAX_CONTROL@package,CPU_UNCORE_FREQUENCY_MIN_CONTROL@package,CPU_UNCORE_FREQUENCY_MAX_CONTROL@package" \
    --min-frequency=<min. core frequency> \
    --max-frequency=<max. core frequency> \
    --step-frequency=100000000 \
    --min-uncore-frequency=<min uncore frequency> \
    --max-uncore-frequency=<max uncore frequency> \
    --step-uncore-frequency=100000000 \
    --trial-count=5 \

``geopmread`` can be used to derive the frequencies required in the experiment
options. For example::

    geopmread CPU_FREQUENCY_MAX_AVAIL board 0
    geopmread CPU_FREQUENCY_MIN_AVAIL board 0
    geopmread CPU_UNCORE_FREQUENCY_MAX_CONTROL board 0
    geopmread CPU_UNCORE_FREQUENCY_MAX_CONTROL board 0

The ``test.sbatch`` script should also be modified to increase the run time to
a sufficiently large value. This will depend on the system, but a full core and
uncore frequency sweep could take about 10 hours, for example.

Then the ``test.sbatch`` script should be run on the node of interest using::

    sbatch -w <node of interest> test.sbatch

This will run multiple kernels of varying intensity that stress the core and
uncore to help with system characterization.

After sourcing the ``$GEOPM_SOURCE/integration/config/run_env.sh`` file, the
CPU compute activity agent policy can then be generated by running::

    integration/experiment/uncore_frequency_sweep/gen_cpu_activity_policy_recommendation.py --path <UNCORE_SWEEP_DIR> --region-list "intensity_1","intensity_16"

This version of the agent allows a single system wide configuration to be
passed in via the policy.

Example Policy
--------------

An example policy generated using a pair of workloads, one core bound
and one uncore bound, is provided below.  Repeated NAN entries are
skipped for space::

    {"CPU_FREQ_MAX": 3700000000,
     "CPU_FREQ_EFFICIENT": "NAN",
     "CPU_UNCORE_FREQ_MAX": 2400000000,
     "CPU_UNCORE_FREQ_EFFICIENT": "NAN",
     "CPU_PHI": 0.5,
     "SAMPLE_PERIOD": 0.01,
     "CPU_UNCORE_FREQ_0": 1200000000,
     "MAX_MEMORY_BANDWIDTH_0": 45414967307.69231,
     "CPU_UNCORE_FREQ_1": 1300000000,
     "MAX_MEMORY_BANDWIDTH_1": 64326515384.61539,
     "CPU_UNCORE_FREQ_2": 1400000000,
     "MAX_MEMORY_BANDWIDTH_2": 72956528846.15384,
     "CPU_UNCORE_FREQ_3": 1500000000,
     "MAX_MEMORY_BANDWIDTH_3": 77349315384.61539,
     "CPU_UNCORE_FREQ_4": 1600000000,
     "MAX_MEMORY_BANDWIDTH_4": 82345998076.92308,
     "CPU_UNCORE_FREQ_5": 1700000000,
     "MAX_MEMORY_BANDWIDTH_5": 87738286538.46153,
     "CPU_UNCORE_FREQ_6": 1800000000,
     "MAX_MEMORY_BANDWIDTH_6": 91966364814.81482,
     "CPU_UNCORE_FREQ_7": 1900000000,
     "MAX_MEMORY_BANDWIDTH_7": 96728174074.07408,
     "CPU_UNCORE_FREQ_8": 2000000000,
     "MAX_MEMORY_BANDWIDTH_8": 100648379629.6296,
     "CPU_UNCORE_FREQ_9": 2100000000,
     "MAX_MEMORY_BANDWIDTH_9": 102409246296.2963,
     "CPU_UNCORE_FREQ_10": 2200000000,
     "MAX_MEMORY_BANDWIDTH_10": 103624103703.7037,
     "CPU_UNCORE_FREQ_11": 2300000000,
     "MAX_MEMORY_BANDWIDTH_11": 104268944444.4444,
     "CPU_UNCORE_FREQ_12": 2400000000,
     "MAX_MEMORY_BANDWIDTH_12": 104748888888.8889,
     "CPU_UNCORE_FREQ_13": "NAN",
     "MAX_MEMORY_BANDWIDTH_13": "NAN",
     ...
     "CPU_UNCORE_FREQ_28": "NAN",
     "MAX_MEMORY_BANDWIDTH_28": "NAN"}

Report Extensions
-----------------

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

Control Loop Rate
-----------------

      The agent gates the Controller's control loop to a cadence of 10ms.

SEE ALSO
--------

:doc:`geopm(7) <geopm.7>`\ ,
:doc:`geopm_agent_monitor(7) <geopm_agent_monitor.7>`\ ,
:doc:`geopm::Agent(3) <GEOPM_CXX_MAN_Agent.3>`\ ,
:doc:`geopm_agent(3) <geopm_agent.3>`\ ,
:doc:`geopm_prof(3) <geopm_prof.3>`\ ,
:doc:`geopmagent(1) <geopmagent.1>`\ ,
:doc:`geopmlaunch(1) <geopmlaunch.1>`
