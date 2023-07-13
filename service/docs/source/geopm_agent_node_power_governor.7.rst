
geopm_agent_power_governor(7) -- agent enforces a power cap
===========================================================






Description
-----------

The :doc:`geopm::NodePowerGovernorAgent(3) <GEOPM_CXX_MAN_NodePowerGovernorAgent.3>` enforces a per-compute-node power cap of the
total platform power systems that support the Platform Power Limit feature.  The platform power is determined by the platform vendor
implementation.  If the feature is not supported on the system of interest the
:doc:`geopm::PowerGovernorAgent(3) <GEOPM_CXX_MAN_PowerGovernorAgent.3>` can be used for CPU only power limiting.

Agent Behavior Highlights
-------------------------

Policies and samples are vectors of double precision values where each
value has a meaning defined by the :doc:`geopm::Agent(3) <GEOPM_CXX_MAN_Agent.3>` implementation.
The Agent interface also provides methods for the Agent to extend
reports and traces with additional Agent-specific information.


*
  **Agent Name**\ :

      Set the ``--geopm-agent`` launch option or ``GEOPM_AGENT`` environment
      variable to ``"node_power_governor"`` and the Controller will select the
      ``NodePowerGovernorAgent`` for its control handler.  See :doc:`geopmlaunch(1) <geopmlaunch.1>`
      and :doc:`geopm(7) <geopm.7>` for more information about launch options and
      environment variables.

*
  **Agent Policy Definitions**\ :

  ``NODE_POWER_LIMIT``\ :
      Sets the average power cap per compute
      node in units of watts.  A power limit
      is required, ``NAN`` will result in an
      error.

*
  **Agent Sample Definitions**\ :

  ``POWER``\ :
      Median total platform power for the node in watts
      measured over the last epoch.


  ``IS_CONVERGED``\ :
      Will be ``1.0`` if the power policy has been
      enforced and the power consumption by all nodes is
      within the assigned limits, otherwise it will be
      ``0.0``.


  ``POWER_AVERAGE_ENFORCED``\ :
      Enforced power limit averaged over all
      compute nodes.  This value corresponds to
      the ``NODE_POWER_LIMIT`` policy
      field and is expected to match unless the
      policy is unachievable.

*
  **Trace Column Extensions**\ :

  ``power_budget``\ :
      Power budget assigned to the compute node associated
      with the trace file at time of sampling.


*
  **Report Extensions**\ :
  N/A

*
  **Control Loop Gate**\ :

      The agent gates the Controller's control loop to a cadence of *5ms*
      (milliseconds).

See Also
--------

:doc:`geopm(7) <geopm.7>`\ ,
:doc:`geopmagent(1) <geopmagent.1>`\ ,
:doc:`geopm_agent(3) <geopm_agent.3>`\ ,
:doc:`geopm::Agent(3) <GEOPM_CXX_MAN_Agent.3>`\ ,
:doc:`geopm::NodePowerGovernorAgent(3) <GEOPM_CXX_MAN_NodePowerGovernorAgent.3>`\ ,
:doc:`geopm::PowerGovernorAgent(3) <GEOPM_CXX_MAN_PowerGovernorAgent.3>`\ ,
:doc:`geopmlaunch(1) <geopmlaunch.1>`\ ,
:doc:`geopm_agent_monitor(7) <geopm_agent_monitor.7>`\ ,
:doc:`geopm_agent_power_balancer(7) <geopm_agent_power_balancer.7>`\ ,
:doc:`geopm_prof(3) <geopm_prof.3>`
