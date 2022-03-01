.. role:: raw-html-m2r(raw)
   :format: html


geopm_agent_power_governor(7) -- agent enforces a power cap
===========================================================






DESCRIPTION
-----------

The PowerGovernorAgent enforces a per-compute-node power cap of the
total power of all packages (sockets).

AGENT BEHAVIOR HIGHLIGHTS
-------------------------

Policies and samples are vectors of double precision values where each
value has a meaning defined by the `geopm::Agent(3) <GEOPM_CXX_MAN_Agent.3.html>`_ implementation.
The Agent interface also provides methods for the Agent to extend
reports and traces with additional Agent-specific information.


* 
  **Agent Name**\ :

      Set the ``--geopm-agent`` launch option or ``GEOPM_AGENT`` environment
      variable to ``"power_governor"`` and the Controller will select the
      PowerGovernorAgent for its control handler.  See `geopmlaunch(1) <geopmlaunch.1.html>`_
      and `geopm(7) <geopm.7.html>`_ for more information about launch options and
      environment variables.

* 
  **Agent Policy Definitions**\ :

  ``POWER_PACKAGE_LIMIT_TOTAL``\ :
      Sets the average power cap per compute
      node in units of watts.  If NAN is
      passed for the power cap, the value
      will default to the thermal design
      power (TDP).  Each package on the node
      will be given an equal fraction of the
      total power.

* 
  **Agent Sample Definitions**\ :

  ``POWER``\ :
      Median total package and DRAM power for the node in watts
      measured over the last epoch.


  ``IS_CONVERGED``\ :
      Will be ``1.0`` if the power policy has been
      enforced and the power consumption by all nodes is
      within the assigned limits, otherwise it will be
      ``0.0``.


  ``POWER_AVERAGE_ENFORCED``\ :
      Enforced power limit averaged over all
      compute nodes.  This value corresponds to
      the ``POWER_PACKAGE_LIMIT_TOTAL`` policy
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

      The agent gates the Controller's control loop to a cadence of *5ms*.

SEE ALSO
--------

`geopm(7) <geopm.7.html>`_\ ,
`geopm_agent_energy_efficient(7) <geopm_agent_energy_efficient.7.html>`_\ ,
`geopm_agent_monitor(7) <geopm_agent_monitor.7.html>`_\ ,
`geopm_agent_power_balancer(7) <geopm_agent_power_balancer.7.html>`_\ ,
`geopm::Agent(3) <GEOPM_CXX_MAN_Agent.3.html>`_\ ,
`geopm_agent_c(3) <geopm_agent_c.3.html>`_\ ,
`geopm_prof_c(3) <geopm_prof_c.3.html>`_\ ,
`geopmagent(1) <geopmagent.1.html>`_\ ,
`geopmlaunch(1) <geopmlaunch.1.html>`_
