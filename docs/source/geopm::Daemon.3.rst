
geopm::Daemon(3) -- GEOPM daemon helper methods
===============================================


Namespaces
----------

The ``Daemon`` class is a member of the ``namespace geopm``\ , but the
full name, ``geopm::Daemon``\ , has been abbreviated in this manual.
Similarly, the ``std::`` namespace specifier has been omitted from the
interface definitions for the following standard types: ``std::vector``\ ,
``std::string``\ , and ``std::set``\ , to enable better rendering of this
manual.

Note that the ``Daemon`` class is an abstract base class.  There is one
concrete implementation, ``DaemonImp``.

Synopsis
--------

#include `<geopm/Daemon.hpp> <https://github.com/geopm/geopm/blob/dev/src/Daemon.hpp>`_

Link with ``-lgeopm`` **(MPI)** or ``-lgeopm`` **(non-MPI)**


.. code-block:: c++

       virtual void Daemon::update_endpoint_from_policystore(double timeout) = 0;

       virtual void Daemon::stop_wait_loop(void) = 0;

       virtual void Daemon::reset_wait_loop(void) = 0;

       static unique_ptr<Daemon> Daemon::make_unique(const string &endpoint_name,
                                                     const string &db_path);

Description
-----------

The ``Daemon`` class is the underlying **C++** implementation for the
:doc:`geopm_daemon(3) <geopm_daemon.3>` **C** interface.  Please refer to the
:doc:`geopm_daemon(3) <geopm_daemon.3>` man page for a general description of the
purpose, goals, and use cases for this interface.

Factory Method
--------------


*
  ``make_unique()``:
  This method returns a ``unique_ptr<Daemon>`` to a concrete ``DaemonImp``
  object.  The shared memory prefix for the Endpoint should be given
  in *endpoint_name*.  The path to the PolicyStore should be given
  in *db_path*.

Class Methods
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

Errors
------

All functions described on this man page throw :doc:`geopm::Exception(3) <geopm::Exception.3>`
on error.

See Also
--------

:doc:`geopm(7) <geopm.7>`\ ,
:doc:`geopm_daemon(3) <geopm_daemon.3>`
