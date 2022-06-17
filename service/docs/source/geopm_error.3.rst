
geopm_error(3) -- error code descriptions
=========================================


Synopsis
--------

#include `<geopm_error.h> <https://github.com/geopm/geopm/blob/dev/service/src/geopm_error.h>`_

Link with ``-lgeopm`` **(MPI)** or ``-lgeopmpolicy`` **(non-MPI)**


.. code-block:: c

       void geopm_error_message(int err,
                                char *msg,
                                size_t size);

Description
-----------

All :doc:`geopm(7) <geopm.7>` C and Fortran APIs that can have an error condition
occur during runtime return an error number.  This is the way
:doc:`geopm(7) <geopm.7>` handles errors, and almost all of the interfaces behave
this way.  This man page describes how to interpret these returned
error codes.  A returned error number of zero by a GEOPM API indicates
success.  If the error number returned by a GEOPM API is positive then
this indicates a generic system error, and if the error number is
negative this indicates a :doc:`geopm(7) <geopm.7>` specific error has occurred.
The GEOPM specific error numbers are enumerated in the `geopm_error.h <https://github.com/geopm/geopm/blob/dev/service/src/geopm_error.h>`_
header file and they are described below.  The system error numbers
are documented in the `errno(3) <https://man7.org/linux/man-pages/man3/errno.3.html>`_ man page.

Any non-zero error number can be passed as the *err* parameter to the
``geopm_error_message()`` function and it will be converted into a
descriptive string *msg*.  The string *msg* is user allocated buffer
of length *size* bytes.  The result, *msg* will always be NULL
terminated even if the message is truncated to fit in the *msg*
buffer.

When a GEOPM C interface returns a non-zero value and this value is
subsequently passed as the *err* argument to
``geopm_error_message()``\ , the message may contain detailed
information about the failure that most recently occurred.  The
details may include the source file, line number, and a detailed
description of the error condition.  If possible, please provide this
message when reporting a bug to the GEOPM developers.

Error Numbers
-------------


*
  ``GEOPM_ERROR_RUNTIME = -1``\ :
  Runtime error

*
  ``GEOPM_ERROR_LOGIC = -2``\ :
  Logic error

*
  ``GEOPM_ERROR_INVALID = -3``\ :
  Invalid argument

*
  ``GEOPM_ERROR_FILE_PARSE = -4``\ :
  Unable to parse input file

*
  ``GEOPM_ERROR_LEVEL_RANGE = -5``\ :
  Control hierarchy level is out of range

*
  ``GEOPM_ERROR_NOT_IMPLEMENTED = -6``\ :
  Feature not yet implemented

*
  ``GEOPM_ERROR_PLATFORM_UNSUPPORTED = -7``\ :
  Current platform not supported or unrecognized

*
  ``GEOPM_ERROR_MSR_OPEN = -8``\ :
  Could not open MSR device

*
  ``GEOPM_ERROR_MSR_READ = -9``\ :
  Could not read from MSR device

*
  ``GEOPM_ERROR_MSR_WRITE = -10``\ :
  Could not write to MSR device

*
  ``GEOPM_ERROR_AGENT_UNSUPPORTED = -11``\ :
  Specified Agent not supported or unrecognized

*
  ``GEOPM_ERROR_AFFINITY = -12``\ :
  MPI ranks are not affinitized to distinct CPUs

*
  ``GEOPM_ERROR_NO_AGENT = -13``\ :
  Requested agent is unavailable or invalid

*
  ``GEOPM_ERROR_DATA_STORE = -14``\ :
  Encountered a data store error

See Also
--------

:doc:`geopm(7) <geopm.7>`\ ,
:doc:`geopm_agent(3) <geopm_agent.3>`\ ,
:doc:`geopm_ctl(3) <geopm_ctl.3>`\ ,
:doc:`geopm_fortran(3) <geopm_fortran.3>`\ ,
:doc:`geopm_prof(3) <geopm_prof.3>`\ ,
`errno(3) <https://man7.org/linux/man-pages/man3/errno.3.html>`_
