.. role:: raw-html-m2r(raw)
   :format: html


geopm_agent_power_balancer(7) -- agent optimizes performance under a power cap
==============================================================================






DESCRIPTION
-----------

The PowerBalancerAgent is designed to enforce an application wide
average per-compute-node power cap while varying the power cap of
individual compute nodes to optimize overall application performance.
This is achieved by providing more than average power to the compute
nodes reporting lower performance and less than average power to the
nodes with higher performance.  The algorithm is designed to mitigate
load imbalance in the application through the redistribution of power.
First the average power cap is sent down to all nodes.  Each node
measures application performance under this power cap, then sends up
its performance.  The root agent then sends down the worst performance
of all the nodes.  Finally, each node attempts to reduce its power
consumption until its performance matches that of the worse node, and
sends up the extra unused power.  On the next loop of the algorithm,
this extra slack power will be distributed evenly across all nodes to
try to improve the performance of the slowest nodes.

The relationship between power cap and application performance is
dependent on many factors including the instruction mix of the
application, manufacturing variation between the processors, problem
size, and data locality.  Because these factors and their
relationships are not easily measured or predicted, determining the
correct power budget to achieve balanced performance is determined
empirically through trials.

The application performance is measured by the duration of application
epoch.  The epoch run time is reported by each MPI rank once every
trip around the outer loop of an iterative application, if the
application has been annotated with a call to ``geopm_prof_epoch()``.  See
the `geopm_prof_c(3) <geopm_prof_c.3.html>`_ man page for more information about geopm
profiling methods.  Note that the epoch runtime used by the
PowerBalancerAgent excludes any time spent in MPI communication
routines or regions marked with the ``GEOPM_REGION_HINT_IGNORE`` hint.  A
windowed median filter is applied to the sequence of epoch times
recorded by each rank, and then the maximum of these median filtered
values across all MPI ranks running on each compute node is used as
the measure of the inverse of compute node performance.

AGENT BEHAVIOR HIGHLIGHTS
-------------------------

Policies and samples are vectors of double precision values where each
value has a meaning defined by the `geopm::Agent(3) <GEOPM_CXX_MAN_Agent.3.html>`_ implementation.
The Agent interface also provides methods for the Agent to extend
reports and traces with additional Agent-specific information.


* 
  **Agent Name**\ :

      Set the ``--geopm-agent`` launch option or ``GEOPM_AGENT`` environment
      variable to "power_balancer" and the Controller will select the
      PowerBalancerAgent for its control handler.  See `geopmlaunch(1) <geopmlaunch.1.html>`_
      and `geopm(7) <geopm.7.html>`_ for more information about launch options and
      environment variables.

* 
  **Agent Policy Definitions**\ :

  ``POWER_PACKAGE_LIMIT_TOTAL``\ :
      Sets the average power cap per compute
      node in units of watts.  The power cap applied to any
      one compute node may be higher or lower than this
      parameter, but the average power cap in aggregate
      across all compute nodes controlled by the policy will
      be equal to this value.  If NAN is passed for the power
      limit, the value will default to the thermal design power
      (TDP).


  ``STEP_COUNT``\ :
      Used as an inter-agent message passed from parent to
      child agents in the balancer's tree-hierarchical
      implementation.  This parameter is not used if the
      ``POWER_PACKAGE_LIMIT_TOTAL`` policy is non-zero.  When
      creating a static policy file, the
      ``POWER_PACKAGE_LIMIT_TOTAL`` should be non-zero, and
      this value can be set to zero.  If set to NAN, it will
      default to 0.


  ``MAX_EPOCH_RUNTIME``\ :
      Used as an inter-agent message passed from
      parent to child agents in the balancer's
      tree-hierarchical implementation.  This
      parameter is not used if the
      ``POWER_PACKAGE_LIMIT_TOTAL`` policy is non-zero.
      When creating a static policy file, the
      ``POWER_PACKAGE_LIMIT_TOTAL`` should be non-zero,
      and this value can be set to zero.  If set to
      NAN, it will default to 0.


  ``POWER_SLACK``\ :
      Used as an inter-agent message passed from parent to
      child agents in the balancer's tree-hierarchical
      implementation.  This parameter is not used if the
      ``POWER_PACKAGE_LIMIT_TOTAL`` policy is non-zero.  When
      creating a static policy file, the
      ``POWER_PACKAGE_LIMIT_TOTAL`` should be non-zero, and
      this value can be set to zero.  If set to NAN, it
      will default to 0.

* 
  **Agent Sample Definitions**\ :

   ``STEP_COUNT``\ :
      Number of iterations of the optimization algorithm
      since the start of the application or the last update
      to the average power cap received at the root.  Note
      that the algorithm is comprised of three types of
      steps which are repeated, and the type of step can be
      inferred by the ``STEP_COUNT`` modulo three: 0 implies
      sending down a power cap or slack power, 1 implies
      measuring the runtime under the latest distribution
      of power, and 2 implies that the power limit is being
      reduced until the slowest runtime is met and slack
      power is sent up the tree.


   ``MAX_EPOCH_RUNTIME``\ :
      Maximum runtime measured after applying
      uniform power cap, or after the last
      redistribution of slack power.


   ``SUM_POWER_SLACK``\ :
      Sum of all slack power available after reducing
      the power limits to achieve the maximum runtime
      reported by any node under the current
      distribution of power limits over compute nodes.

* 
  **Trace Column Extensions**\ :

   ``policy_power_cap``\ :
       The latest power cap received through the
       policy.  This will be 0 unless receiving a new
       power cap from the root.


   ``policy_step_count``\ :
       The current value of the algorithm step counter.
       The current state is the step count modulo 3.


   ``policy_max_epoch_runtime``\ :
       The maximum runtime across all nodes as
       received from the parent.


   ``policy_power_slack``\ :
       The latest power slack value received from the parent.


   ``epoch_runtime``\ :
       Time interval in seconds between the last two epoch
       calls by the application averaged over all ranks on
       the node and excluding time spent in MPI calls.


   ``power_limit``\ :
       Power limit assigned to the compute node associated
       with the trace file at time of sampling.


   ``enforced_power_limit``\ :
       The actual power limit that was set on the
       node.  It may be different from the requested
       limit due to hardware constraints.

* 
  **Report Extensions**\ :
  N/A

* 
  **Control Loop Gate**\ :

      The agent gates the Controller's control loop to a cadence of 5
      milliseconds.

SEE ALSO
--------

`geopm(7) <geopm.7.html>`_\ ,
`geopm_agent_energy_efficient(7) <geopm_agent_energy_efficient.7.html>`_\ ,
`geopm_agent_monitor(7) <geopm_agent_monitor.7.html>`_\ ,
`geopm_agent_power_governor(7) <geopm_agent_power_governor.7.html>`_\ ,
`geopm::Agent(3) <GEOPM_CXX_MAN_Agent.3.html>`_\ ,
`geopm_agent_c(3) <geopm_agent_c.3.html>`_\ ,
`geopm_prof_c(3) <geopm_prof_c.3.html>`_\ ,
`geopmagent(1) <geopmagent.1.html>`_\ ,
`geopmlaunch(1) <geopmlaunch.1.html>`_
