geopm_agent_cpu_activity(7) -- agent for selecting CPU frequency based on CPU compute activity
=================================================================================================

Description
-----------

The goal of **CPUActivityAgent** is to save CPU energy by scaling CPU frequency
based upon the compute activity of each CPU as provided by the
CPU_COMPUTE_ACTIVITY signal and modified by the CPU_UTILIZATION signal.

The agent scales the core frequency in the range of ``Fce`` to ``Fcmax``, where
``Fcmax`` and ``Fce`` are provided via the MSRIOGroup and ConstConfigIOGroup respectively.

Low compute activity regions (compute activity of 0.0) run at the ``Fce`` frequency,
high activity regions (compute activity of 1.0) run at the ``Fcmax`` frequency,
and regions in between the extremes run at a frequency (``Fcreq``) selected using the equation:

``Fcreq = Fce + (Fcmax - Fce) * CPU_COMPUTE_ACTIVITY``

The ``CPU_COMPUTE_ACTIVITY`` is defined as a derivative signal based on the MSR::PPERF::PCNT
scalability metric.

The agent also scales the uncore frequency in the range of ``Fue`` to
``Fumax``, where ``Fumax`` and ``Fue`` are povided by the
MSRIOGroup and ConstConfigIOGroup respectively.

Low uncore activity regions (uncore activity of 0.0) run at the ``Fue`` frequency,
high activity regions (uncore activity of 1.0) run at the ``Fumax`` frequency,
and regions in between the extremes run at a frequency (``Fureq``) selected using
the equation:

``Fureq = Fue + (Fumax - Fue) * CPU_UNCORE_ACTIVITY``

The ``CPU_UNCORE_ACTIVITY`` is defined as a simple ratio of the current memory bandwidth
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
agent towards higher frequencies by increasing the ``Fe`` value.
In the extreme case (``phi`` of 0) ``Fe`` will be raised to ``Fmax``.  A ``phi`` value greater than
0.5 biases the agent towards lower frequencies by reducing the ``Fmax`` value.
In the extreme case (``phi`` of 1.0) ``Fmax`` will be lowered to ``Fe``.

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

The ``Phi`` input is the only policy value.

  ``CPU_PHI``\ :
      The performance bias knob.  The value must be between
      0.0 and 1.0. If NAN is passed, it will use 0.5 by default.

ConstConfigIOGroup Configuration File Generation
------------------------------------------------

Generating a ConstConfigIOGroup configuration file for the CPU Compute Activity agent requires
system characterization using the GEOPM experiment infrastructure located in
``$GEOPM_SOURCE/integration/experiment/`` and the arithmetic intensity
benchmark located in ``$GEOPM_SOURCE/integration/apps/arithmetic_intensity``.
Prior to starting, the arithmetic intensity benchmark needs to be built (use
the ``build.sh`` script provided in the benchmark's folder).


.. note::
    Before performing the system characterization, please ensure the
    system is quiesced (i.e. not running other heavy processes/workloads).

The automated method of characterization requires running::

    PYTHONPATH=${GEOPM_SOURCE}:\
    ${GEOPM_SOURCE}/integration/experiment:${PYTHONPATH} \
    python3 ${GEOPM_SOURCE}/integration/test/test_cpu_characterization.py

This will generate a file named ``const_config_io-characterization.json``
in the current working directory containing configuration information for
the node.  If successful, this is all that is required.

The manual method of characterization consists of several steps.
The first step is to generate the execution script by running
the following for SLURM based systems::

    gen_slurm.sh 1 arithmetic_intensity uncore_frequency_sweep

Or if you're running a PBS based system run the following::

    gen_pbs.sh 1 arithmetic_intensity uncore_frequency_sweep

The generated ``test.sbatch``  or ``test.pbs`` should be modified to enable
Memory Bandwidth Monitoring by adding the following above the experiment
script invocation::

    srun -N ${SLURM_NNODES} geopmwrite MSR::PQR_ASSOC:RMID board 0 0
    srun -N ${SLURM_NNODES} geopmwrite MSR::QM_EVTSEL:RMID board 0 0
    srun -N ${SLURM_NNODES} geopmwrite MSR::QM_EVTSEL:EVENT_ID board 0 2

Without this, the uncore bandwidth characterization analysis scripts will not
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
CPU compute activity agent ConstConfigIOGroup configuration file can then be generated by running::

    integration/experiment/uncore_frequency_sweep/gen_cpu_activity_constconfig_recommendation.py --path <UNCORE_SWEEP_DIR> --region-list "intensity_1","intensity_16"

Depending on the number of runs, system noise, and other factors there may be more than one reasonable
value for ``Fe`` for a given domain.  In these cases a warning similar to the following will be provided::

    'Warning: Found N possible alternate Fe value(s) within 5% energy consumption of Fe for <Control>.
     Consider using the energy-margin options.\n'

If this occurs the user may choose to use the provided configuration file OR rerun the recommendation script with
any the energy-margin options ``--core-energy-margin`` & ``--uncore-energy-margin`` along with a value such
as 0.05 (5%). These options will attempt to identify a lower ``Fe`` for the respective domain that costs less than
the energy consumed at ``Fe`` plus the energy-margin percentage provided.

An example ConstConfigIOGroup configuration file is provided below::

    {
        "CPU_FREQUENCY_EFFICIENT_HIGH_INTENSITY": {
            "domain": "board",
            "description": "Defines the efficient core frequency to use for CPUs.  Based on a workload that scales strongly with the frequency domain",
            "units": "hertz",
            "aggregation": "average",
            "values": [2000000000.0]
        },
        "CPU_UNCORE_FREQUENCY_EFFICIENT_HIGH_INTENSITY": {
            "domain": "board",
            "description": "Defines the efficient uncore frequency to use for CPUs.  Based on a workload that scales strongly with the frequency domain",
            "units": "hertz",
            "aggregation": "average",
            "values": [2000000000.0]
        },
        "CPU_UNCORE_FREQUENCY_0": {
            "domain": "board",
            "description": "CPU Uncore Frequency associated with CPU_UNCORE_MAX_MEMORY_BANDWIDTH_0",
            "units": "hertz",
            "aggregation": "average",
            "values": [1200000000.0]
        },
        "CPU_UNCORE_MAX_MEMORY_BANDWIDTH_0": {
            "domain": "board",
            "description": "Maximum memory bandwidth in bytes perf second associated with CPU_UNCORE_FREQUENCY_0",
            "units": "none",
            "aggregation": "average",
            "values": [45639800000.0]
        },
        "CPU_UNCORE_FREQUENCY_1": {
            "domain": "board",
            "description": "CPU Uncore Frequency associated with CPU_UNCORE_MAX_MEMORY_BANDWIDTH_1",
            "units": "hertz",
            "aggregation": "average",
            "values": [1400000000.0]
        },
        "CPU_UNCORE_MAX_MEMORY_BANDWIDTH_1": {
            "domain": "board",
            "description": "Maximum memory bandwidth in bytes perf second associated with CPU_UNCORE_FREQUENCY_1",
            "units": "none",
            "aggregation": "average",
            "values": [73881616666.66667]
        },
        "CPU_UNCORE_FREQUENCY_2": {
            "domain": "board",
            "description": "CPU Uncore Frequency associated with CPU_UNCORE_MAX_MEMORY_BANDWIDTH_2",
            "units": "hertz",
            "aggregation": "average",
            "values": [1600000000.0]
        },
        "CPU_UNCORE_MAX_MEMORY_BANDWIDTH_2": {
            "domain": "board",
            "description": "Maximum memory bandwidth in bytes perf second associated with CPU_UNCORE_FREQUENCY_2",
            "units": "none",
            "aggregation": "average",
            "values": [85787733333.33333]
        },
        "CPU_UNCORE_FREQUENCY_3": {
            "domain": "board",
            "description": "CPU Uncore Frequency associated with CPU_UNCORE_MAX_MEMORY_BANDWIDTH_3",
            "units": "hertz",
            "aggregation": "average",
            "values": [1800000000.0]
        },
        "CPU_UNCORE_MAX_MEMORY_BANDWIDTH_3": {
            "domain": "board",
            "description": "Maximum memory bandwidth in bytes perf second associated with CPU_UNCORE_FREQUENCY_3",
            "units": "none",
            "aggregation": "average",
            "values": [97272166666.66667]
        },
        "CPU_UNCORE_FREQUENCY_4": {
            "domain": "board",
            "description": "CPU Uncore Frequency associated with CPU_UNCORE_MAX_MEMORY_BANDWIDTH_4",
            "units": "hertz",
            "aggregation": "average",
            "values": [2000000000.0]
        },
        "CPU_UNCORE_MAX_MEMORY_BANDWIDTH_4": {
            "domain": "board",
            "description": "Maximum memory bandwidth in bytes perf second associated with CPU_UNCORE_FREQUENCY_4",
            "units": "none",
            "aggregation": "average",
            "values": [106515333333.33333]
        }
    }

Example Policy
--------------

An example policy is provided below::

    {"CPU_PHI": 0.5}

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
:doc:`geopm::Agent(3) <geopm::Agent.3>`\ ,
:doc:`geopm_agent(3) <geopm_agent.3>`\ ,
:doc:`geopm_prof(3) <geopm_prof.3>`\ ,
:doc:`geopmagent(1) <geopmagent.1>`\ ,
:doc:`geopmlaunch(1) <geopmlaunch.1>`
