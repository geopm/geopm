.. role:: raw-html-m2r(raw)
   :format: html


geopm_agent_energy_efficient(7) -- agent for saving energy, also finds optimal region frequencies
=================================================================================================






DESCRIPTION
-----------

The goal of this Agent is to save energy without degrading performance
beyond an acceptable limit.  It achieves this by setting frequencies
per region so that memory- and I/O-bound regions run at lower
frequencies without increasing runtime but CPU-bound regions are still
run at high frequencies.

The **EnergyEfficientAgent** finds the optimal frequency for each region
dynamically by measuring the performance of each region and reducing
the frequency as long as the performance is still within acceptable
limits.  The performance metric used is the maximum of the runtimes
reported by each rank for the last execution of the region in question
(lower is better).  Up to 10% performance loss is allowed, unless a
different tolerance is specified with the ``PERF_MARGIN`` policy.  To
avoid increasing energy due to performance loss, when not in a marked
region, it will always run at the maximum frequency from the policy.

AGENT BEHAVIOR HIGHLIGHTS
-------------------------

Policies and samples are vectors of double precision values where each
value has a meaning defined by the `geopm::Agent(3) <GEOPM_CXX_MAN_Agent.3.html>`_ implementation.
The Agent interface also provides methods for the Agent to extend
reports and traces with additional Agent-specific information.


* 
  **Agent Name**:

      Set the ``--geopm-agent`` launch option or ``GEOPM_AGENT`` environment
      variable to ``"energy_efficient"`` and the Controller will select the
      EnergyEfficientAgent for its control handler.  See
      `geopm_launch(1) <geopm_launch.1.html>`_ and `geopm(7) <geopm.7.html>`_ for more information about
      launch options and environment variables.

* 
  **Agent Policy Definitions**:

      The minimum and maximum frequency are passed down as policies.
      Setting both to the same value can be used to force the entire
      application to run at one frequency.

  ``FREQ_MIN``\ :
      The minimum frequency in hertz that the algorithm is
      allowed to choose for any region.  If NAN is passed, it
      will use the minimum available frequency by default.


  ``FREQ_MAX``\ :
      The maximum frequency in hertz that the algorithm is
      allowed to choose for any region.  If NAN is passed, it
      will use the maximum available frequency by default.


  ``PERF_MARGIN``\ :
      The maximum performance degradation allowed when
      trying to lower the frequency for a region.  The
      value must be a fraction between ``0.0`` (*0%*) and
      ``1.0`` (*100%*) of performance at ``FREQ_MAX``. If NAN is
      passed, it will use ``0.10`` (*10%*) by default.


  ``FREQ_FIXED``\ :
      The maximum frequency in hertz used for jobs without a
      GEOPM controller.  This value is not used when the
      Agent is running inside a Controller. If NAN is
      passed, it will use the maximum available frequency by
      default.

* 
  **Agent Sample Definitions**\ :
  N/A

* 
  **Trace Column Extensions**\ :
  N/A

* 
  **Report Extensions**\ :

      The per-node learned best-fit frequency for each region is added to
      the report.

* 
  **Control Loop Gate**\ :

      The agent gates the Controller's control loop to a cadence of 5ms.

SEE ALSO
--------

`geopm(7) <geopm.7.html>`_\ ,
`geopm_agent_monitor(7) <geopm_agent_monitor.7.html>`_\ ,
`geopm_agent_power_balancer(7) <geopm_agent_power_balancer.7.html>`_\ ,
`geopm_agent_power_governor(7) <geopm_agent_power_governor.7.html>`_\ ,
`geopm::Agent(3) <GEOPM_CXX_MAN_Agent.3.html>`_\ ,
`geopm_agent_c(3) <geopm_agent_c.3.html>`_\ ,
`geopm_prof_c(3) <geopm_prof_c.3.html>`_\ ,
`geopmagent(1) <geopmagent.1.html>`_\ ,
`geopmlaunch(1) <geopmlaunch.1.html>`_
