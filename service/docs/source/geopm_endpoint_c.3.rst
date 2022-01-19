.. role:: raw-html-m2r(raw)
   :format: html


geopm_endpoint_c(3) -- dynamic policy control for resource management
=====================================================================






SYNOPSIS
--------

#include `<geopm_endpoint.h> <https://github.com/geopm/geopm/blob/dev/src/geopm_endpoint.h>`_\ 

``Link with -lgeopmpolicy``


* 
  ``int geopm_endpoint_create(``\ :
  `const char *`_endpoint\ *name*\ , :raw-html-m2r:`<br>`
  `struct geopm_endpoint_c **`_endpoint_\ ``);``

* 
  ``int geopm_endpoint_destroy(``\ :
  `struct geopm_endpoint_c *`_endpoint_\ ``);``

* 
  ``int geopm_endpoint_open(``\ :
  `struct geopm_endpoint_c *`_endpoint_\ ``);``

* 
  ``int geopm_endpoint_close(``\ :
  `struct geopm_endpoint_c *`_endpoint_\ ``);``

* 
  ``int geopm_endpoint_agent(``\ :
  `struct geopm_endpoint_c *`_endpoint_, :raw-html-m2r:`<br>`
  ``size_t`` _agent_name\ *max*\ , :raw-html-m2r:`<br>`
  `char *`_agent\ *name*\ ``);``

* 
  ``int geopm_endpoint_wait_for_agent_attach(``\ :
   `struct geopm_endpoint_c *`_endpoint_, :raw-html-m2r:`<br>`
   ``double`` timeout\ ``);``

* 
  ``int geopm_endpoint_stop_wait_loop(``\ :
  `struct geopm_endpoint_c *`_endpoint_\ ``);``

* 
  ``int geopm_endpoint_reset_wait_loop(``\ :
  `struct geopm_endpoint_c *`_endpoint_\ ``);``

* 
  ``int geopm_endpoint_profile_name(``\ :
  `struct geopm_endpoint_c *`_endpoint_, :raw-html-m2r:`<br>`
  ``size_t`` _profile_name\ *max*\ , :raw-html-m2r:`<br>`
  `char *`_profile\ *name*\ ``);``

* 
  ``int geopm_endpoint_num_node(``\ :
  `struct geopm_endpoint_c *`_endpoint_, :raw-html-m2r:`<br>`
  `int *`_num\ *node*\ ``);``

* 
  ``int geopm_endpoint_node_name(``\ :
  `struct geopm_endpoint_c *`_endpoint_, :raw-html-m2r:`<br>`
  ``int`` _node\ *idx*\ , :raw-html-m2r:`<br>`
  ``size_t`` _node_name\ *max*\ , :raw-html-m2r:`<br>`
  `char *`_node\ *name*\ ``);``

* 
  ``int geopm_endpoint_write_policy(``\ :
  `struct geopm_endpoint_c *`_endpoint_, :raw-html-m2r:`<br>`
  ``size_t`` _num\ *policy*\ , :raw-html-m2r:`<br>`
  `const double *`_policy\ *array*\ ``);``

* 
  ``int geopm_endpoint_read_sample(``\ :
  `struct geopm_endpoint_c *`_endpoint_, :raw-html-m2r:`<br>`
  ``size_t`` num_sample, :raw-html-m2r:`<br>`
  `double *`_sample\ *array*\ , :raw-html-m2r:`<br>`
  `double *`_sample_age\ *sec*\ ``);``

DESCRIPTION
-----------

The _geopm_endpoint\ *c* interface can be utilized by a system resource manager
or parallel job scheduler to create, inspect, and destroy a GEOPM endpoint.
These endpoints can also be used to dynamically adjust the policy over the
compute application runtime.

For dynamic control, a daemon can use this interface to create and modify an
inter-process shared memory region on the compute node hosting the root MPI
process of the compute application.  The shared memory region is monitored by
the GEOPM runtime to enforce the policy across the entire MPI job allocation.

The endpoints can also be used to extract sample telemetry from the runtime.
This interface works in concert with the `geopm_agent_c(3) <geopm_agent_c.3.html>`_ interface for
inspecting the policy and sample parameters that pertain to the current agent.
One must utilize the agent interface to determine the number of sample
parameters provided by the agent and the number of policy values required by the
agent.

All functions described in this man page return an error code on failure and
zero upon success; see [ERRORS][] section below for details.


* 
  ``geopm_endpoint_create``\ ():
  will create an endpoint object.  This object will hold the
  necessary state for interfacing with the shmem regions.  The
  endpoint is stored in *endpoint* and will be used with other
  functions in this interface.  The shared memory regions managed by
  this endpoint will have a substring of the shmem key that matches
  _endpoint\ *name*.  This will return zero on success indicating that
  the *endpoint* struct can now be used.  *endpoint* will
  be unmodified if an error occurs.

* 
  ``geopm_endpoint_destroy``\ ():
  will release resources associated with *endpoint*.  This will return zero
  on success indicating that the shmem regions were removed.  Otherwise an
  error code is returned.  This function will not delete any shmem regions.

* 
  ``geopm_endpoint_open``\ ():
  will create shared memory regions for passing policies to the
  Agent, and reading samples from the Agent.  These shmem regions
  are managed by the endpoint.  This will return zero on success
  indicating that the shmem regions were successfully created.  If
  the shmem key already exists, an error code of EEXIST is returned.

* 
  ``geopm_endpoint_close``\ ():
  will release shmem regions containing the substring
  _endpoint\ *name*.  This will return zero on success indicating that
  the shmem regions were removed.  Otherwise an error code is
  returned.

* 
  ``geopm_endpoint_agent``\ ():
  checks to see if an agent specified by _agent\ *name* has attached
  to the *endpoint*.  The number of bytes reserved for _agent\ *name*
  is specified in _agent_name\ *max*.  Returns zero if the endpoint
  has an agent attached and the agent's name can be stored in the
  _agent\ *name* buffer, or if no agent has attached.  Otherwise an
  error code is returned.  If no agent has attached, _agent\ *name*
  will be an empty string.  If no shmem region has been created with
  ``geopm_endpoint_open()``\ , an error code is returned.

* 
  ``geopm_endpoint_wait_for_agent_attach``\ ():
  blocks until an agent has attached to the *endpoint* or the
  *timeout* in seconds is reached.  This will return zero on success
  indicating that the agent attached or the wait was cancelled.
  Otherwise an error code is returned.

* 
  ``geopm_endpoint_stop_wait_loop``\ ():
  stops any current wait loops the *endpoint* is running.

* 
  ``geopm_endpoint_reset_wait_loop``\ ():
  resets the *endpoint* to prepare for a subsequent call to
  ``geopm_endpoint_wait_for_agent_attach()``.  This only needs to be
  called after calling ``geopm_endpoint_stop_wait_loop()`` once to reuse
  the endpoint for another agent.

* 
  ``geopm_endpoint_profile_name``\ ():
  provides the profile name of the attached agent in _profile\ *name*.
  The number of bytes reserved for _profile\ *name* is specified in
  _profile_name\ *max*.  Returns zero if the endpoint has an agent
  attached and the profile name can be stored in the _profile\ *name*
  buffer.  Otherwise an error code is returned.  If no agent has
  attached, _profile\ *name* will be an empty string.  If no shmem
  region has been created with ``geopm_endpoint_open()``\ , an error
  code is returned.

* 
  ``geopm_endpoint_num_node``\ ():
  provides the number of nodes controlled by the agent attached to
  the *endpoint* in _num\ *node*.  Returns zero on success, otherwise
  an error code is returned.  If no shmem region has been created
  with ``geopm_endpoint_open()``\ , an error code is returned.

* 
  ``geopm_endpoint_node_name``\ ():
  provides the hostname of the *endpoint* managed compute node in
  _node\ *name*.  The index is specified by _node\ *idx*.  The number of
  bytes reserved for _node\ *name* is specified in _node_name\ *max*.
  Returns zero if the node name can be stored in the _node\ *name*
  buffer, otherwise an error code is returned.  If no shmem region
  has been created with ``geopm_endpoint_open()``\ , an error code is
  returned.

* 
  ``geopm_endpoint_write_policy``\ ():
  sets the policy values for the agent within *endpoint* to follow.
  These values provided in _policy\ *array* will be consumed by the
  GEOPM runtime at the next iteration of the control loop.  The size
  of the _policy\ *array* is given in _num\ *policy*.  Returns zero on
  success, otherwise an error code is returned.  Setting NAN for a
  policy value can be used to to indicate that the Agent should use
  an appropriate default value.  If no shmem region has been created
  with ``geopm_endpoint_open()``\ , an error code is returned.

* 
  ``geopm_endpoint_read_sample``\ ():
  provides the sample telemetry from the *endpoint*\ 's agent in
  _sample\ *array* and the amount of time that has passed since the
  agent last provided an update in _sample_age\ *sec*.  The number of
  samples is given in _num\ *sample*.  Returns zero on success,
  otherwise an error code is returned.  If no shmem region has been
  created with ``geopm_endpoint_open()``\ , an error code is returned.

ERRORS
------

All functions described on this man page return an error code.  See
`geopm_error(3) <geopm_error.3.html>`_ for a full description of the error numbers and how
to convert them to strings.

SEE ALSO
--------

`geopm(7) <geopm.7.html>`_\ ,
`geopm_error(3) <geopm_error.3.html>`_\ ,
**geopm::Endpoint(3)**\ ,
`geopmendpoint(1) <geopmendpoint.1.html>`_
