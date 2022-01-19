.. role:: raw-html-m2r(raw)
   :format: html


geopm_agent_c(3) -- query information about available agents
============================================================






SYNOPSIS
--------

#include `<geopm_agent.h> <https://github.com/geopm/geopm/blob/dev/src/geopm_agent.h>`_\ 

``Link with -lgeopm``


* 
  ``int geopm_agent_supported(``\ :
  `const char *`_agent\ *name*\ ``);``

* 
  ``int geopm_agent_num_policy(``\ :
  `const char *`_agent\ *name*\ , :raw-html-m2r:`<br>`
  `int *`_num\ *policy*\ ``);``

* 
  ``int geopm_agent_policy_name(``\ :
  `const char *`_agent\ *name*\ , :raw-html-m2r:`<br>`
  ``int`` _policy\ *idx*\ , :raw-html-m2r:`<br>`
  ``size_t`` _policy_name\ *max*\ , :raw-html-m2r:`<br>`
  `char *`_policy\ *name*\ ``);``

* 
  ``int geopm_agent_policy_json(``\ :
  `const char *`_agent\ *name*\ , :raw-html-m2r:`<br>`
  `const double *`_policy\ *array*\ , :raw-html-m2r:`<br>`
  ``size_t`` _json_string\ *max*\ , :raw-html-m2r:`<br>`
  `const char *`_json\ *string*\ ``);``

* 
  ``int geopm_agent_policy_json_partial(``\ :
  `const char *`_agent\ *name*\ , :raw-html-m2r:`<br>`
  ``size_t`` _policy_array\ *size*\ , :raw-html-m2r:`<br>`
  `const double *`_policy\ *array*\ , :raw-html-m2r:`<br>`
  ``size_t`` _json_string\ *max*\ , :raw-html-m2r:`<br>`
  `const char *`_json\ *string*\ ``);``

* 
  ``int geopm_agent_num_sample(``\ :
  `const char *`_agent\ *name*\ , :raw-html-m2r:`<br>`
  `int *`_num\ *sample*\ ``);``

* 
  ``int geopm_agent_sample_name(``\ :
  `const char *`_agent\ *name*\ , :raw-html-m2r:`<br>`
  ``int`` _sample\ *idx*\ , :raw-html-m2r:`<br>`
  ``size_t`` _sample_name\ *max*\ , :raw-html-m2r:`<br>`
  `char *`_sample\ *name*\ ``);``

* 
  ``int geopm_agent_num_avail(``\ :
  `int *`_num\ *agent*\ ``);``

* 
  ``int geopm_agent_name(``\ :
  ``int`` _agent\ *idx*\ , :raw-html-m2r:`<br>`
  ``size_t`` _agent_name\ *max*\ , :raw-html-m2r:`<br>`
  `char *`_agent\ *name*\ ``);``

* 
  ``int geopm_agent_enforce_policy(``\ :
  ``void);``

DESCRIPTION
-----------

The _geopm_agent\ *c* interface is used to query GEOPM about agents on a
system and create static policies for those agents.  The interface
provides the ability to query about specific types of supported
agents, the policy parameters required by an agent, and the sample
parameters provided by an agent.  In addition, this interface provides
a way to create a JSON policy configuration file that will be parsed
by the GEOPM runtime for statically enforcing a job policy.

All functions described in this man page return an error code on failure and
zero upon success; see [ERRORS][] section below for details.


* 
  ``geopm_agent_supported``\ ():
  queries GEOPM to determine if _agent\ *name* is supported.  Returns zero if
  an agent is supported, otherwise an error code is returned.

* 
  ``geopm_agent_num_policy``\ ():
  queries GEOPM for the number of policy parameters required by _agent\ *name*.
  The result is placed in _num\ *policy*.  Returns zero if an agent is
  supported, otherwise an error code is returned.

* 
  ``geopm_agent_policy_name``\ ():
  queries GEOPM for the name of a policy parameter occurring at index
  _policy\ *idx* for _agent\ *name*.  The result is stored in _policy\ *name*.  The
  number of bytes reserved for the output string is specified in
  _policy_name\ *max*.  Returns zero if an agent is supported, the _policy\ *idx*
  is in range, and the policy name can be stored in the output string
  (_policy\ *name*\ ).  Otherwise an error code is returned.

* 
  ``geopm_agent_policy_json``\ ():
  creates a JSON file based on the provided _policy\ *array* that can be used
  for statically controlling the job policy for _agent\ *name*.  The output is
  stored in the _json\ *string* buffer.  The number of bytes reserved for the
  buffer is specified in _json_string\ *max*.

* 
  ``geopm_agent_policy_json_partial``\ ():
  creates a JSON file based on the provided _policy\ *array* of length
  _policy_array\ *size* that can be used for statically controlling
  the job policy for _agent\ *name*.  The output is stored in the
  _json\ *string* buffer.  The number of bytes reserved for the buffer
  is specified in _json_string\ *max*.  The _policy\ *array* may contain
  only a partial list of the policies accepted by the Agent.  Missing
  policy values will be automatically interpreted as NAN, indicating
  that the Agent should use a default value.

* 
  ``geopm_agent_num_sample``\ ():
  queries GEOPM for the number of sample parameters provided by _agent\ *name*.
  The result is stored in _num\ *sample*.  Returns zero on success, otherwise
  an error code is returned.

* 
  ``geopm_agent_sample_name``\ ():
  queries GEOPM for the name of a sample parameter occurring at index
  _sample\ *idx* for _agent\ *name*.  The output is stored in the _sample\ *name*
  buffer.  The number of bytes allocated for the output buffer is specified
  in _sample_name\ *max*.  Returns zero if an agent is supported, the
  _sample\ *idx* is in range, and the sample name can be stored in the output
  string (_sample\ *name*\ ).  Otherwise an error code is returned.

* 
  ``geopm_agent_num_avail``\ ():
  queries GEOPM for the number of currently available agents.  The output
  is stored in _num\ *agent*.  Returns zero on success, otherwise
  an error code is returned.

* 
  ``geopm_agent_name``\ ():
  queries GEOPM for the name of the agent occurring at index _agent\ *idx*.  The
  result is stored in _agent\ *name*.  The number of bytes reserved for the
  output string is specified in _agent_name\ *max*.  Returns zero if the
  _agent\ *idx* is in range, and the agent name can be stored in the output
  string (_agent\ *name*\ ).  Otherwise an error code is returned.

* 
  ``geopm_agent_enforce_policy``\ ():
  queries the environment for the ``GEOPM_AGENT`` and ``GEOPM_POLICY``
  and enforces the policy for the agent by writing controls to the
  platform (see `geopm_pio_c(3) <geopm_pio_c.3.html>`_\ ).  A resource manager can use
  this function to enforce the GEOPM configured policy (see
  `geopm(7) <geopm.7.html>`_\ ) prior to releasing compute nodes for a user
  allocation.  More generally this function allows one-time use of
  an Agent outside of the context of an MPI runtime or the use of a
  GEOPM Controller.  Note that the enforcement of the policy will
  not support any of the dynamic features of the agent.  For instance,
  calling ``geopm_agent_enforce_policy``\ () for the power_balancer will
  set all nodes to the same power limit, and those limits will not
  vary with time.

ERRORS
------

All functions described on this man page return an error code.  See
`geopm_error(3) <geopm_error.3.html>`_ for a full description of the error numbers and how
to convert them to strings.

SEE ALSO
--------

`geopm(7) <geopm.7.html>`_\ ,
`geopm_agent_energy_efficient(7) <geopm_agent_energy_efficient.7.html>`_\ ,
`geopm_agent_monitor(7) <geopm_agent_monitor.7.html>`_\ ,
`geopm_agent_power_balancer(7) <geopm_agent_power_balancer.7.html>`_\ ,
`geopm_agent_power_governor(7) <geopm_agent_power_governor.7.html>`_\ ,
`geopm_error(3) <geopm_error.3.html>`_\ ,
`geopmagent(1) <geopmagent.1.html>`_\ ,
`geopmlaunch(1) <geopmlaunch.1.html>`_
