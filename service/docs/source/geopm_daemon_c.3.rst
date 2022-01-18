.. role:: raw-html-m2r(raw)
   :format: html


geopm_daemon_c(3) -- helpers for geopm daemons
==============================================






SYNOPSIS
--------

#include `<geopm_daemon.h> <https://github.com/geopm/geopm/blob/dev/src/geopm_daemon.h>`_\ 

``Link with -lgeopmpolicy``


* 
  ``int geopm_daemon_create(``\ :
  `const char *`_endpoint\ *name*\ , :raw-html-m2r:`<br>`
  `const char *`_policystore\ *path*\ , :raw-html-m2r:`<br>`
  `struct geopm_daemon_c **`_daemon_\ ``);``

* 
  ``int geopm_daemon_destroy(``\ :
  `struct geopm_daemon_c *`_daemon_\ ``);``

* 
  ``int geopm_daemon_update_endpoint_from_policystore(``\ :
  `struct geopm_daemon_c *`_daemon_, :raw-html-m2r:`<br>`
  ``double`` timeout\ ``);``

* 
  ``int geopm_daemon_stop_wait_loop(``\ :
  `struct geopm_daemon_c *`_daemon_\ ``);``

* 
  ``int geopm_daemon_reset_wait_loop(``\ :
  `struct geopm_daemon_c *`_daemon_\ ``);``

DESCRIPTION
-----------

The geopm_daemon_c interface contains common high-level utility
functions for interacting with the GEOPM Endpoint.  Its main purpose
is to provide the functionality needed for system resource manager
plugins or daemon processes, as well as handling clean up tasks.  The
underlying objects used by the daemon can also be used separately;
refer to **geopm_endpoint(3)** and **geopm_policystore(3)** for more
information.

All functions described in this man page return an error code on failure and
zero upon success; see [ERRORS][] section below for details.


* 
  ``geopm_daemon_create``\ ():
  will create a daemon object.  This object will hold the necessary
  state for interfacing with the endpoint and policystore.  The
  daemon object is stored in *daemon* and will be used with other
  functions in this interface.  The shared memory regions managed by
  the internal endpoint will be created with the shmem key
  _endpoint\ *name*.  This will return zero on success indicating that
  the *daemon* struct can now be used.  *daemon* will be
  unmodified if an error occurs.

* 
  ``geopm_daemon_destroy``\ ():
  will release resources associated with *daemon*.  This will return
  zero on success indicating that the underlying endpoint and
  policystore connections were destroyed.  Otherwise an error code
  is returned.  This method removes any shared memory regions
  associated with the endpoint.

* 
  ``geopm_daemon_update_endpoint_from_policystore``\ ():
  looks up a policy in the PolicyStore given the attached
  Controller's agent and profile name, and writes it back into the
  policy side of the Endpoint.  The PolicyStore connection and
  Endpoint are owned by the given *daemon* object.  If no policy is
  found, an error is returned.  If the Controller fails to attach
  within the *timeout*\ , or detaches while this function is running,
  no policy is written.

* 
  ``geopm_daemon_stop_wait_loop``\ ():
  exits early from any ongoing wait loops in the *daemon*\ , for
  example in a call to
  ``geopm_daemon_update_endpoint_from_policystore()``.

* 
  ``geopm_daemon_reset_wait_loop``\ ():
  resets the *daemon*\ 's endpoint to prepare for a future wait loop.

ERRORS
------

All functions described on this man page return an error code.  See
`geopm_error(3) <geopm_error.3.html>`_ for a full description of the error numbers and how
to convert them to strings.

COPYRIGHT
---------

Copyright (c) 2015 - 2021, Intel Corporation. All rights reserved.

SEE ALSO
--------

`geopm(7) <geopm.7.html>`_\ ,
`geopm_error(3) <geopm_error.3.html>`_\ ,
**geopm_endpoint(3)**\ ,
**geopm_policystore(3)**\ ,
**geopm::Daemon(3)**\ ,
**geopm::Endpoint(3)**\ ,
**geopm::PolicyStore(3)**
