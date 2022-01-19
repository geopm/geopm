.. role:: raw-html-m2r(raw)
   :format: html


geopmagent(1) -- query agent information and create static policies
===================================================================






SYNOPSIS
--------

``geopmagent`` [\ ``-a`` *AGENT*\ ] [\ ``-p`` *POLICY0*\ [,\ *POLICY1*\ ,...] ]

DESCRIPTION
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
the string "Policy:".  The JSON-formatted string can be piped to a
file to be used as the ``GEOPM_POLICY`` as described in `geopm(7) <geopm.7.html>`_ to
provide a static input policy for the GEOPM controller.  If ``-p`` is
given but ``-a`` is not, an error is reported.

OPTIONS
-------


* 
  ``--help``\ :
  Print brief summary of the command line usage information,
  then exit.

* 
  ``--version``\ :
  Print version of `geopm(7) <geopm.7.html>`_ to standard output, then exit.

* 
  ``-a`` *AGENT*\ :
  Specify the name of the agent.

* 
  ``-p`` *POLICY0*\ ,\ *POLICY1*\ ,...:
  The values to be set for each policy in a comma-separated list.
  Values other than the first policy are optional and will be set to
  NAN if not provided, indicating that the Agent should use a
  default value.  If the agent does not require any policy values
  this option must be specified as "None" or "none".

EXAMPLES
--------

List all available agents on the system:

.. code-block::

   $ geopmagent
   energy_efficient
   frequency_map
   monitor
   power_balancer
   power_governor


Get power_balancer agent policy and sample names:

.. code-block::

   $ geopmagent -a power_balancer
   Policy: POWER_CAP,STEP_COUNT,MAX_EPOCH_RUNTIME,POWER_SLACK
   Sample: STEP_COUNT,MAX_EPOCH_RUNTIME,SUM_POWER_SLACK,MIN_POWER_HEADROOM


Create policy for power_governor agent with 250 watts per node power
budget:

.. code-block::

   $ geopmagent -a power_governor -p 250
   {"POWER_PACKAGE_LIMIT_TOTAL" : 250}


Create policy for power_balancer agent with 250 watts per node power
budget and other policies set to default:

.. code-block::

   $ geopmagent -a power_balancer -p 250
   {"POWER_CAP" : 250}


Create policy for energy_efficient agent with minimum frequency of 1.2
GHz and maximum frequency of 2.4 GHz:

.. code-block::

   $ geopmagent -a energy_efficient -p 1.2e9,2.4e9
   {"FREQ_MIN" : 1.2e+09, "FREQ_MAX" : 2.4e+09}


Create policy for energy_efficient agent with default minimum and
maximum frequency, with a performance margin of 5%:

.. code-block::

   $ geopmagent -a energy_efficient -p NAN,NAN,0.05
   {"FREQ_MIN" : "NAN", "FREQ_MAX" : "NAN", "PERF_MARGIN": 0.05}


Create policy for monitor agent which does not require any policies.
Note that GEOPM uses the monitor agent by default, in which case
specifying ``--geopm-agent`` and ``--geopm-policy`` are optional.

.. code-block::

   $ geopmagent -a monitor -p None
   {}



SEE ALSO
--------

`geopm(7) <geopm.7.html>`_\ ,
`geopm_agent_energy_efficient(7) <geopm_agent_energy_efficient.7.html>`_\ ,
`geopm_agent_frequency_map(7) <geopm_agent_frequency_map.7.html>`_\ ,
`geopm_agent_monitor(7) <geopm_agent_monitor.7.html>`_\ ,
`geopm_agent_power_balancer(7) <geopm_agent_power_balancer.7.html>`_\ ,
`geopm_agent_power_governor(7) <geopm_agent_power_governor.7.html>`_\ ,
`geopm_agent_c(3) <geopm_agent_c.3.html>`_
