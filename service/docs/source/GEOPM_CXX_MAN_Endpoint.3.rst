.. role:: raw-html-m2r(raw)
   :format: html


geopm::Endpoint(3) -- geopm endpoint interface
==============================================






NAMESPACES
----------

The ``Endpoint`` class is a member of the ``namespace geopm``\ , but the
full name, ``geopm::Endpoint``\ , has been abbreviated in this manual.
Similarly, the ``std::`` namespace specifier has been omitted from the
interface definitions for the following standard types: ``std::vector``\ ,
``std::string``\ , and ``std::set``\ , to enable better rendering of this
manual.

Note that the ``Endpoint`` class is an abstract base class.  There is one
concrete implementation, ``EndpointImp``\ , which uses shared memory to
implement the Endpoint interface functionality.

SYNOPSIS
--------

#include `<geopm/Endpoint.hpp> <https://github.com/geopm/geopm/blob/dev/src/Endpoint.hpp>`_\ 

Link with ``-lgeopm``


.. code-block:: c++

       static unique_ptr<Endpoint> Endpoint::make_unique(const string &data_path);

       virtual void Endpoint::open(void);

       virtual void Endpoint::close(void);

       virtual void Endpoint::write_policy(const vector<double> &policy);

       virtual double Endpoint::read_sample(vector<double> &sample);

       virtual string Endpoint::get_agent(void);

       virtual void Endpoint::wait_for_agent_attach(double timeout);

       virtual void Endpoint::wait_for_agent_detach(double timeout);

       virtual void Endpoint::stop_wait_loop(void);

       virtual void Endpoint::reset_wait_loop(void);

       virtual string Endpoint::get_profile_name(void);

       virtual set<string> Endpoint::get_hostnames(void);

DESCRIPTION
-----------

The ``Endpoint`` class is the underlying **C++** implementation for the
`geopm_endpoint_c(3) <geopm_endpoint_c.3.html>`_ **C** interface.  Please refer to the
`geopm_endpoint_c(3) <geopm_endpoint_c.3.html>`_ man page for a general description of the
purpose, goals, and use cases for this interface.

FACTORY METHOD
--------------


* ``make_unique()``:
  This method returns a ``unique_ptr<Endpoint>`` to a concrete
  ``EndpointImp`` object.  The shared memory prefix should be given in
  *data_path*.

CLASS METHODS
-------------


* 
  ``open()``:
  creates the underlying shared memory regions belonging to the
  Endpoint.

* 
  ``close()``:
  unlinks the shared memory regions belonging to the Endpoint.

* 
  ``write_policy()``:
  writes a set of policy values given in *policy* to the endpoint.
  The order of the values is determined by the currently attached
  agent; see `geopm::Agent(3) <GEOPM_CXX_MAN_Agent.3.html>`_.

* 
  ``read_sample()``:
  reads the most recent set of sample values from the endpoint into
  the output vector, *sample*\ , and returns the sample age in seconds.
  The order of the values is determined by the currently attached
  agent; see `geopm::Agent(3) <GEOPM_CXX_MAN_Agent.3.html>`_.

* 
  ``get_agent()``:
  returns the agent name associated with the Controller attached to
  this endpoint, or empty if no Controller is attached.

* 
  ``wait_for_agent_attach()``:
  Blocks until an agent attaches to the endpoint,
  a *timeout* is reached, or the operation is
  canceled with ``stop_wait_loop()``.  Throws an
  exception if the given *timeout* is reached
  before an agent attaches.  The name of the
  attached agent can be read with ``get_agent()``.

* 
  ``wait_for_agent_detach()``:
  Blocks as long as the same agent is still
  attached to the endpoint, a *timeout* is reached,
  or the operation is canceled with ``stop_wait_loop()``.
  The name of the attached agent can be read with ``get_agent()``.

* 
  ``stop_wait_loop()``:
  Cancels any current wait loops in this Endpoint.

* 
  ``reset_wait_loop()``:
  Re-enables wait loops occurring after this call.

* 
  ``get_profile_name()``:
  returns the profile name associated with the attached job, or
  empty if no Controller is attached.

* 
  ``get_hostnames()``:
  returns the set of hostnames used by the Controller attached to
  this endpoint, or empty if no Controller is attached.

ERRORS
------

All functions described on this man page throw `geopm::Exception(3) <GEOPM_CXX_MAN_Exception.3.html>`_
on error.

SEE ALSO
--------

`geopm(7) <geopm.7.html>`_\ ,
`geopm_endpoint_c(3) <geopm_endpoint_c.3.html>`_
