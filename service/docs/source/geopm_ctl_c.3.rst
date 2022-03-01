.. role:: raw-html-m2r(raw)
   :format: html


geopm_ctl_c(3) -- geopm runtime control thread
==============================================






SYNOPSIS
--------

#include `<geopm_ctl.h> <https://github.com/geopm/geopm/blob/dev/src/geopm_ctl.h>`_\ 

Link with ``-lgeopm``


.. code-block:: c++

       int geopm_ctl_create(MPI_Comm comm,
                            struct geopm_ctl_c **ctl);

       int geopm_ctl_destroy(struct geopm_ctl_c *ctl);

       int geopm_ctl_run(struct geopm_ctl_c *ctl);

       int geopm_ctl_pthread(struct geopm_ctl_c *ctl,
                             const pthread_attr_t *attr,
                             pthread_t *thread);

DESCRIPTION
-----------

The ``geopm_ctl_c`` structure is used to launch the global extensible open
power manager algorithm.  There are several ways to enable control:
running the control algorithm as a distinct processes from the
application under control, or running the control algorithm as a
separate pthread owned by the application process under control.  Each
of these methods has different requirements and trade offs.


* 
  ``geopm_ctl_create()``:
  creates a ``geopm_ctl_c`` object, *ctl* which is an opaque structure
  that holds the state used to execute the control algorithm with
  one of the other functions described in this man page.  The
  control algorithm relies on feedback about the application
  profile.  The user provides an MPI communicator, *comm* which must
  have at least one process running on every compute node under
  control.

* 
  ``geopm_ctl_destroy()``:
  destroys all resources associated with the *ctl* structure which
  allocated by a previous call to ``geopm_ctl_create()``.

* 
  ``geopm_ctl_run()``:
  steps the control algorithm continuously until the application
  signals shutdown.

* 
  ``geopm_ctl_pthread()``:
  creates a POSIX thread running the control algorithm continuously
  until the application signals shutdown.  With this method of launch
  the supporting MPI implementation must be enabled for
  ``MPI_THREAD_MULTIPLE`` using ``MPI_Init_thread()``.

ERRORS
------

All functions described on this man page return an error code.  See
`geopm_error(3) <geopm_error.3.html>`_ for a full description of the error numbers and how
to convert them to strings.

SEE ALSO
--------

`geopm(7) <geopm.7.html>`_\ ,
`geopm_error(3) <geopm_error.3.html>`_\ ,
`geopm_sched(3) <geopm_sched.3.html>`_\ ,
`geopmctl(1) <geopmctl.1.html>`_
