.. role:: raw-html-m2r(raw)
   :format: html


geopmendpoint(1) -- command line tool for dynamic policy control
================================================================






SYNOPSIS
--------

``geopmendpoint`` [\ ``-c`` | ``-d`` | ``-a`` | ``-p`` *POLICY0*\ ,\ *POLICY1*\ ,...] *ENDPOINT*

DESCRIPTION
-----------

Command line interface to create, query, read, write, and destroy a
GEOPM runtime endpoint.  A GEOPM endpoint is a shared memory attach
point to connect a managing process (e.g. a resource manager or job
scheduler) to a GEOPM controller.  The endpoint supports writing of
agent-specific policies and the reading of agent-specific signals.
The agent signals and controls are described on the `geopmagent(1) <geopmagent.1.html>`_
man page.  The *ENDPOINT* positional argument specifies the endpoint
name and is typically the name associated with the job or the job
identifier.  This string is arbitrary, but cannot contain the '/' or
',' characters.  This name distinguishes all endpoints running on any
given compute node (coherent shared memory).  If the ``geopmendpoint``
executable is run with no arguments then a list of all policy
endpoints available on the system is printed to standard output.

OPTIONS
-------


* 
  ``-c``\ :
  Create an endpoint for an attaching agent.

* 
  ``-d``\ :
  Destroy the endpoint and send a signal to any attached agent that
  no more policies will be written or samples read from the
  endpoint.

* 
  ``-a``\ :
  Check if an agent has attached to the endpoint, and print the name
  of the agent if it has.  The return value will be non-zero if an
  agent has not yet attached to the endpoint.

* 
  ``-s``\ :
  Read a sample from the attached agent, if any.  The output has one
  line per signal.  On each line is written the signal name and
  signal value separated by a colon.  The last line of the output
  represents the SAMPLE_AGE signal which is the elapsed time since
  the last update to the endpoint by the GEOPM runtime.  The signal
  names provided by an agent can be determined with the
  `geopmagent(1) <geopmagent.1.html>`_ command line tool.  The return value will be
  non-zero if an agent has not yet attached to the endpoint.

* 
  ``-p`` *POLICY0*\ ,\ *POLICY1*\ ,...:
  Set policies for the attached agent.  The values to be set for
  each policy are given in a comma-separated list.  The order of
  these values corresponds to the ordering of the policy names by
  the `geopmagent(1) <geopmagent.1.html>`_ executable when the attached agent is
  specified.  The return value will be non-zero if an agent has not
  yet attached to the endpoint.

EXAMPLES
--------

List all endpoints on a system when none are open:

.. code-block::

   $ geopmendpoint


Create two endpoints called "job-123" and "job-321" for agents to
attach:

.. code-block::

   $ geopmendpoint -c job-123
   $ geopmendpoint
   job-123
   $ geopmendpoint -c job-321
   $ geopmendpoint
   job-123
   job-321


Check if agent has attached to endpoint "job-123", but no agent has
yet attached:

.. code-block::

   $ geopmendpoint -a job-123
   Error: <geopm> No agent has attached to endpoint.


Check if agent has attached to endpoint "job-321" after a
power_balancer agent has attached:

.. code-block::

   $ geopmendpoint -a job-321
   Agent: power_balancer
   Nodes: compute-node-4,compute-node-5,compute-node-7,compute-node-8


Set policy at endpoint for power_balancer agent with 250 Watt per
node power budget:

.. code-block::

   $ geopmendpoint -p 250 job-321


Sample from balancing agent with endpoint "job-321":

.. code-block::

   $ geopmendpoint -s job-321
   POWER: 247.2
   IS_CONVERGED: 1
   EPOCH_RUNTIME: 90.5
   SAMPLE_AGE: 1.234E-4


Destroy endpoints "job-123" and "job-321":

.. code-block::

   $ geopmendpoint -d job-321
   $ geopmendpoint
   job-123
   $ geopmendpoint -d job-123
   $ geopmendpoint



SEE ALSO
--------

`geopm(7) <geopm.7.html>`_\ ,
`geopm_agent_monitor(7) <geopm_agent_monitor.7.html>`_\ ,
`geopm_agent_power_balancer(7) <geopm_agent_power_balancer.7.html>`_\ ,
`geopm_agent_power_governor(7) <geopm_agent_power_governor.7.html>`_\ ,
`geopm_agent_c(3) <geopm_agent_c.3.html>`_\ ,
`geopm_endpoint_c(3) <geopm_endpoint_c.3.html>`_\ ,
`geopmagent(1) <geopmagent.1.html>`_
