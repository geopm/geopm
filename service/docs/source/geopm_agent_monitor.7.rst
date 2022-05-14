.. role:: raw-html-m2r(raw)
   :format: html


geopm_agent_monitor(7) -- agent implementation for aggregating statistics
=========================================================================






DESCRIPTION
-----------

An implementation of the Agent interface that enforces no policies and
is intended for application tracing and profiling only.  It collects
statistics from each node and aggregates them over the whole
application run.

AGENT BEHAVIOR HIGHLIGHTS
-------------------------

Policies and samples are vectors of double precision values where each
value has a meaning defined by the `geopm::Agent(3) <GEOPM_CXX_MAN_Agent.3.html>`_ implementation.
The Agent interface also provides methods for the Agent to extend
reports and traces with additional Agent-specific information.


* 
  **Agent Name**\ :

      Set the ``--geopm-agent`` launch option or ``GEOPM_AGENT`` environment
      variable to "monitor" and the Controller will select the
      ``MonitorAgent`` for its control handler.  See `geopmlaunch(1) <geopmlaunch.1.html>`_ and
      `geopm(7) <geopm.7.html>`_ for more information about launch options and
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

      The Monitor agent gates the Controller's control loop to a cadence of *5ms*.

SEE ALSO
--------

`geopm(7) <geopm.7.html>`_\ ,
`geopm_agent_power_balancer(7) <geopm_agent_power_balancer.7.html>`_\ ,
`geopm_agent_power_governor(7) <geopm_agent_power_governor.7.html>`_\ ,
`geopm::Agent(3) <GEOPM_CXX_MAN_Agent.3.html>`_\ ,
`geopm_agent_c(3) <geopm_agent_c.3.html>`_\ ,
`geopmagent(1) <geopmagent.1.html>`_\ ,
`geopmlaunch(1) <geopmlaunch.1.html>`_
