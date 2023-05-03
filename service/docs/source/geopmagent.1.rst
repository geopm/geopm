geopmagent(1) -- query agent information and create static policies
===================================================================

Synopsis
--------

.. code-block:: bash

       geopmadmin [--config-default | --config-override | --msr-allowlist] [--cpuid]

       geopmagent [-a AGENT] [-p POLICY0 [, POLICY1, ...] ]

Description
-----------

Used to get information about GEOPM agents on a system and create
static policies for those agents.  When no options are provided,
``geopmagent`` will display the names of all available agents.  When the
agent is specified with option ``-a`` but ``-p`` is not given,
``geopmagent`` will print names of the agent's policies and samples.  If
both ``-a`` and ``-p`` are provided then ``geopmagent`` will print to
standard output a JSON-formatted policy string corresponding to the
policy values specified by the ``-p`` option.  The argument to ``-p`` is a
comma-separated list of policy parameters.  To determine the parameter
names and ordering appropriate for the ``-p`` option, run ``geopmagent``
with only the ``-a`` option and inspect the output line beginning with
the string ``"Policy:"``.  The JSON-formatted string can be piped to a
file to be used as the ``GEOPM_POLICY`` as described in :doc:`geopm(7) <geopm.7>` to
provide a static input policy for the GEOPM controller.  If ``-p`` is
given but ``-a`` is not, an error is reported.

The JSON output generated when using the ``-a`` and ``-p`` options follows
this schema:

.. literalinclude:: ../../../json_schemas/geopmagent_policy.schema.json
    :language: json

Options
-------
--help      Print brief summary of the command line usage information, then
            exit.
--version   Print version of :doc:`geopm(7) <geopm.7>` to standard output,
            then exit.
-a AGENT    Specify the name of the agent.
-p POLICY   The values to be set for each policy in a comma-separated list.
            Values other than the first policy are optional and will be set to
            ``NAN`` if not provided, indicating that the Agent should use a default
            value.  If the agent does not require any policy values this option
            must be specified as ``"None"`` or ``"none"``.

Examples
--------

List all available agents on the system:

.. code-block:: console

   $ geopmagent
   frequency_map
   monitor
   power_balancer
   power_governor


Get ``power_balancer`` agent policy and sample names:

.. code-block:: console

   $ geopmagent -a power_balancer
   Policy: POWER_CAP,STEP_COUNT,MAX_EPOCH_RUNTIME,POWER_SLACK
   Sample: STEP_COUNT,MAX_EPOCH_RUNTIME,SUM_POWER_SLACK,MIN_POWER_HEADROOM


Create policy for ``power_governor`` agent with 250 watts per node power
budget:

.. code-block:: console

   $ geopmagent -a power_governor -p 250
   {"CPU_POWER_LIMIT" : 250}


Create policy for ``power_balancer`` agent with 250 watts per node power
budget and other policies set to default:

.. code-block:: console

   $ geopmagent -a power_balancer -p 250
   {"POWER_CAP" : 250}


Create policy for monitor agent which does not require any policies.
Note that GEOPM uses the monitor agent by default, in which case
specifying ``--geopm-agent`` and ``--geopm-policy`` are optional.

.. code-block:: console

   $ geopmagent -a monitor -p None
   {}



See Also
--------

:doc:`geopm(7) <geopm.7>`,
:doc:`geopm_agent_frequency_map(7) <geopm_agent_frequency_map.7>`,
:doc:`geopm_agent_monitor(7) <geopm_agent_monitor.7>`,
:doc:`geopm_agent_power_balancer(7) <geopm_agent_power_balancer.7>`,
:doc:`geopm_agent_power_governor(7) <geopm_agent_power_governor.7>`,
:doc:`geopm_agent_ffnet(7) <geopm_agent_ffnet.7>`,
:doc:`geopm_agent(3) <geopm_agent.3>`
