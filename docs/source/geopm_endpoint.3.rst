
geopm_endpoint(3) -- dynamic policy control for resource management
=====================================================================


Synopsis
--------

#include `<geopm_endpoint.h> <https://github.com/geopm/geopm/blob/dev/libgeopm/include/geopm_endpoint.h>`_

Link with ``-lgeopm``


.. code-block:: c++

       int geopm_endpoint_create(const char *endpoint_name,
                                 struct geopm_endpoint_c **endpoint);

       int geopm_endpoint_destroy(struct geopm_endpoint_c *endpoint);

       int geopm_endpoint_open(struct geopm_endpoint_c *endpoint);

       int geopm_endpoint_close(struct geopm_endpoint_c *endpoint);

       int geopm_endpoint_agent(struct geopm_endpoint_c *endpoint,
                                size_t agent_name_max,
                                char *agent_name);

       int geopm_endpoint_wait_for_agent_attach(struct geopm_endpoint_c *endpoint,
                                                double timeout);

       int geopm_endpoint_stop_wait_loop(struct geopm_endpoint_c *endpoint);

       int geopm_endpoint_reset_wait_loop(struct geopm_endpoint_c *endpoint);

       int geopm_endpoint_profile_name(struct geopm_endpoint_c *endpoint,
                                       size_t profile_name_max,
                                       char *profile_name);

       int geopm_endpoint_num_node(struct geopm_endpoint_c *endpoint,
                                   int *num_node);

       int geopm_endpoint_node_name(struct geopm_endpoint_c *endpoint,
                                    int node_idx,
                                    size_t node_name_max,
                                    char *node_name);

       int geopm_endpoint_write_policy(struct geopm_endpoint_c *endpoint,
                                       size_t num_policy,
                                       const double *policy_array);

       int geopm_endpoint_read_sample(struct geopm_endpoint_c *endpoint,
                                      size_t num_sample,
                                      double *sample_array,
                                      double *sample_age_sec);

Description
-----------

The ``geopm_endpoint_c`` interface can be utilized by a system resource manager
or parallel job scheduler to create, inspect, and destroy a GEOPM endpoint.
These endpoints can also be used to dynamically adjust the policy over the
compute application runtime.

For dynamic control, a daemon can use this interface to create and modify an
inter-process shared memory region on the compute node hosting the root MPI
process of the compute application.  The shared memory region is monitored by
the GEOPM runtime to enforce the policy across the entire MPI job allocation.

The endpoints can also be used to extract sample telemetry from the runtime.
This interface works in concert with the :doc:`geopm_agent(3) <geopm_agent.3>` interface for
inspecting the policy and sample parameters that pertain to the current agent.
One must utilize the agent interface to determine the number of sample
parameters provided by the agent and the number of policy values required by the
agent.

All functions described in this man page return an error code on failure and
zero upon success; see `ERRORS <ERRORS_>`_ section below for details.


*
  ``geopm_endpoint_create()``:
  will create an endpoint object.  This object will hold the
  necessary state for interfacing with the shmem regions.  The
  endpoint is stored in *endpoint* and will be used with other
  functions in this interface.  The shared memory regions managed by
  this endpoint will have a substring of the shmem key that matches
  *endpoint_name*.  This will return zero on success indicating that
  the *endpoint* struct can now be used.  *endpoint* will
  be unmodified if an error occurs.

*
  ``geopm_endpoint_destroy()``:
  will release resources associated with *endpoint*.  This will return zero
  on success indicating that the shmem regions were removed.  Otherwise an
  error code is returned.  This function will not delete any shmem regions.
  Additionally will send a signal to the agent that the manager
  is detaching from the policy and will no longer send updates.

*
  ``geopm_endpoint_open()``:
  will create shared memory regions for passing policies to the
  Agent, and reading samples from the Agent.  These shmem regions
  are managed by the *endpoint*.  This will return zero on success
  indicating that the shmem regions were successfully created.  If
  the shmem key already exists, an error code of ``EEXIST`` is returned.

*
  ``geopm_endpoint_close()``:
  will release shmem regions containing the substring
  *endpoint_name*.  This will return zero on success indicating that
  the shmem regions were removed.  Otherwise an error code is
  returned.

*
  ``geopm_endpoint_agent()``:
  checks to see if an agent specified by *agent_name* has attached
  to the *endpoint*.  The number of bytes reserved for *agent_name*
  is specified in *agent_name_max*.  Returns zero if the endpoint
  has an agent attached and the agent's name can be stored in the
  *agent_name* buffer, or if no agent has attached.  Otherwise an
  error code is returned.  If no agent has attached, *agent_name*
  will be an unaltered string.  If no shmem region has been created with
  ``geopm_endpoint_open()``\ , an error code is returned.

*
  ``geopm_endpoint_wait_for_agent_attach()``:
  blocks until an agent has attached to the *endpoint* or the
  *timeout* in seconds is reached.  This will return zero on success
  indicating that the agent attached or the wait was cancelled.
  Otherwise an error code is returned.

*
  ``geopm_endpoint_stop_wait_loop()``:
  stops any current wait loops the *endpoint* is running.

*
  ``geopm_endpoint_reset_wait_loop()``:
  resets the *endpoint* to prepare for a subsequent call to
  ``geopm_endpoint_wait_for_agent_attach()``.  This only needs to be
  called after calling ``geopm_endpoint_stop_wait_loop()`` once to reuse
  the endpoint for another agent.

*
  ``geopm_endpoint_profile_name()``:
  provides the profile name of the attached agent in *profile_name*.
  The number of bytes reserved for *profile_name* is specified in
  *profile_name_max*.  Returns zero if the endpoint has an agent
  attached and the profile name can be stored in the *profile_name*
  buffer.  Otherwise an error code is returned.  If no agent has
  attached, *profile_name* will be an unaltered string.  If no shmem
  region has been created with ``geopm_endpoint_open()``\ , an error
  code is returned.

*
  ``geopm_endpoint_num_node()``:
  provides the number of nodes controlled by the agent attached to
  the *endpoint* in *num_node*.  Returns zero on success, otherwise
  an error code is returned.  If no shmem region has been created
  with ``geopm_endpoint_open()``\ , an error code is returned.

*
  ``geopm_endpoint_node_name()``:
  provides the hostname of the *endpoint* managed compute node in
  *node_name*.  The index is specified by *node_idx*.  The number of
  bytes reserved for *node_name* is specified in *node_name_max*.
  Returns zero if the node name can be stored in the *node_name*
  buffer, otherwise an error code is returned.  If no shmem region
  has been created with ``geopm_endpoint_open()``\ , an error code is
  returned.

*
  ``geopm_endpoint_write_policy()``:
  sets the policy values for the agent within *endpoint* to follow.
  These values provided in *policy_array* will be consumed by the
  GEOPM runtime at the next iteration of the control loop.  The size
  of the *policy_array* is given in *num_policy*.  Returns zero on
  success, otherwise an error code is returned.  Setting NAN for a
  policy value can be used to to indicate that the Agent should use
  an appropriate default value.  If no shmem region has been created
  with ``geopm_endpoint_open()``\ , an error code is returned.

*
  ``geopm_endpoint_read_sample()``:
  provides the sample telemetry from the *endpoint*\ 's agent in
  *sample_array* and the amount of time that has passed since the
  agent last provided an update in *sample_age_sec*.  The number of
  samples is given in *num_sample*.  Returns zero on success,
  otherwise an error code is returned.  If no shmem region has been
  created with ``geopm_endpoint_open()``\ , an error code is returned.

Errors
------

All functions described on this man page return an error code.  See
:doc:`geopm_error(3) <geopm_error.3>` for a full description of the error numbers and how
to convert them to strings.

Example
-------
The endpoint interface needs to be opened and attached to an agent. The
following pseudocode illustrates the order of function calls that would enable
an endpoint user to interact with an agent after a job has already started
executing, instead of writing a static policy to a file and waiting for job
completion to receive feedback.

Error-checking is omitted from this example for brevity. See
:doc:`geopm_error(3) <geopm_error.3>` for interpretation of the return values
from these functions.

.. code-block:: c

    char agent_name[128];
    char profile_name[128];
    char node_name[128];
    int num_node;
    struct geopm_endpoint_c* endpoint;
    geopm_endpoint_create("my_endpoint", &endpoint);
    geopm_endpoint_open(endpoint);
    geopm_endpoint_wait_for_agent_attach(endpoint, 1.0);
    geopm_endpoint_agent(endpoint, sizeof agent_name, agent_name);
    // Now you can look up agent properties by agent_name, such as the names of
    // policy and sample vector elements. See geopm_agent(3).
    geopm_endpoint_profile_name(endpoint, sizeof profile_name, profile_name);
    // Now you have the user-provided profile name for the job
    geopm_endpoint_num_node(endpoint, &num_node);
    geopm_endpoint_node_name(endpoint, num_node-1 /* The last node's name */,
                             sizeof node_name, node_name);
    double *policy = /* populate the array based on the agent policy */;
    geopm_endpoint_write_policy(endpoint, policy, num_policy);
    double *sample = /* Allocate large enough to hold agent's samples */;
    double sample_age_sec; // How old the sample is by the time you request it
    geopm_endpoint_read_sample(endpoint, num_sample, sample, &sample_age_sec);
    geopm_endpoint_close(endpoint);
    geopm_endpoint_destroy(endpoint);

See Also
--------

:doc:`geopm(7) <geopm.7>`,
:doc:`geopm_error(3) <geopm_error.3>`,
:doc:`geopm::Endpoint(3) <geopm::Endpoint.3>`,
:doc:`geopmendpoint(1) <geopmendpoint.1>`,
:doc:`geopm_agent(3) <geopm_agent.3>`
