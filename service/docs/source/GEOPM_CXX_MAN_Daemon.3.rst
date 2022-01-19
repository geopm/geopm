.. role:: raw-html-m2r(raw)
   :format: html


geopm::Daemon(3) -- geopm daemon helper methods
===============================================






NAMESPACES
----------

The ``Daemon`` class is a member of the ``namespace geopm``\ , but the
full name, ``geopm::Daemon``\ , has been abbreviated in this manual.
Similarly, the ``std::`` namespace specifier has been omitted from the
interface definitions for the following standard types: ``std::vector``\ ,
``std::string``\ , and ``std::set``\ , to enable better rendering of this
manual.

Note that the ``Daemon`` class is an abstract base class.  There is one
concrete implementation, ``DaemonImp``.

SYNOPSIS
--------

#include `<geopm/Daemon.hpp> <https://github.com/geopm/geopm/blob/dev/src/Daemon.hpp>`_\ 

Link with ``-lgeopm`` **(MPI)** or ``-lgeopmpolicy`` **(non-MPI)**


.. code-block:: c++

       virtual void Daemon::update_endpoint_from_policystore(double timeout) = 0;

       virtual void Daemon::stop_wait_loop(void) = 0;

       virtual void Daemon::reset_wait_loop(void) = 0;

       static unique_ptr<Daemon> Daemon::make_unique(const string &endpoint_name,
                                                     const string &db_path);

DESCRIPTION
-----------

The ``Daemon`` class is the underlying **C++** implementation for the
`geopm_daemon_c(3) <geopm_daemon_c.3.html>`_ **C** interface.  Please refer to the
`geopm_daemon_c(3) <geopm_daemon_c.3.html>`_ man page for a general description of the
purpose, goals, and use cases for this interface.

FACTORY METHOD
--------------


* 
  ``make_unique()``:
  This method returns a ``unique_ptr<Daemon>`` to a concrete ``DaemonImp``
  object.  The shared memory prefix for the Endpoint should be given
  in *endpoint_name*.  The path to the PolicyStore should be given
  in *db_path*.

CLASS METHODS
-------------


* 
  ``update_endpoint_from_policystore()``:
  Looks up a policy in the Daemon's PolicyStore given the attached
  Controller's agent and profile name, and writes it back into the
  policy side of the Daemon's Endpoint.  If no policy is found, an
  error is returned.  If the Controller fails to attach within the
  *timeout*\ , or detaches while this function is running, no policy
  is written.

* 
  ``stop_wait_loop()``:
  Exits early from any ongoing wait loops in the Daemon, for example
  in a call to ``update_endpoint_from_policystore()``.

* 
  ``reset_wait_loop()``:
  Resets the Daemon's endpoint to prepare for a future wait loop.

ERRORS
------

All functions described on this man page throw `geopm::Exception(3) <GEOPM_CXX_MAN_Exception.3.html>`_
on error.

SEE ALSO
--------

`geopm(7) <geopm.7.html>`_\ ,
`geopm_daemon_c(3) <geopm_daemon_c.3.html>`_
