
geopm_agent_monitor(7) -- agent implementation for aggregating statistics
=========================================================================






Description
-----------

An implementation of the Agent interface that enforces no policies and
is intended for application tracing and profiling only.  It collects
statistics from each node and aggregates them over the whole
application run.

Agent Behavior Highlights
-------------------------

Policies and samples are vectors of double precision values where each
value has a meaning defined by the :doc:`geopm::Agent(3) <geopm::Agent.3>` implementation.
The Agent interface also provides methods for the Agent to extend
reports and traces with additional Agent-specific information.


*
  **Agent Name**\ :

      Set the ``--geopm-agent`` launch option or ``GEOPM_AGENT`` environment
      variable to "monitor" and the Controller will select the
      ``MonitorAgent`` for its control handler.  See :doc:`geopmlaunch(1) <geopmlaunch.1>` and
      :doc:`geopm(7) <geopm.7>` for more information about launch options and
      environment variables.

*
  **Agent Policy Definitions**\ : N/A

*
  **Agent Sample Definitions**\ : N/A

*
  **Trace Column Extensions**\ : N/A

*
  **Report Extensions**\ : N/A

*
  **Control Loop Gate**\ :

      The Monitor agent gates the Controller's control loop to a
      cadence of *200 ms*.

See Also
--------

:doc:`geopm(7) <geopm.7>`\ ,
:doc:`geopm_agent_power_balancer(7) <geopm_agent_power_balancer.7>`\ ,
:doc:`geopm_agent_power_governor(7) <geopm_agent_power_governor.7>`\ ,
:doc:`geopm::Agent(3) <geopm::Agent.3>`\ ,
:doc:`geopm_agent(3) <geopm_agent.3>`\ ,
:doc:`geopmagent(1) <geopmagent.1>`\ ,
:doc:`geopmlaunch(1) <geopmlaunch.1>`
