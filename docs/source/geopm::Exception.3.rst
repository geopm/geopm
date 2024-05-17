
geopm::Exception(3) -- custom GEOPM exceptions
==============================================


Namespaces
----------

The ``Exception`` class and the ``exception_handler()`` function are members of
the ``namespace geopm``, but the full names, ``geopm::Exception`` and
``geopm::exception_handler()``, have been abbreviated in this manual.
Similarly, the ``std::`` namespace specifier has been omitted from the
interface definitions for the following standard types: ``std::vector``\ ,
``std::string``\ , ``std::exception_ptr``\ , ``std::runtime_error``\ , and ``std::function``\ , to enable
better rendering of this manual.

Note that the ``geopm::Exception`` class is derived from ``std::runtime_error`` class.

Synopsis
--------

#include `<geopm/Exception.hpp> <https://github.com/geopm/geopm/blob/dev/libgeopmd/include/geopm/Exception.hpp>`_

Link with ``-lgeopm`` **(MPI)** or ``-lgeopm`` **(non-MPI)**


.. code-block:: c++

       int exception_handler(exception_ptr eptr, bool do_print=false);

       Exception::Exception(void);

       Exception::Exception(const Exception &other);

       Exception::Exception(const string &what, int err, const char *file, int line);

       virtual Exception::~Exception(void) = default;

       int Exception::err_value(void) const;

Description
-----------

This class is used to format error messages for the GEOPM runtime
according to the error code.  The list of errors is described in
:doc:`geopm_error(3) <geopm_error.3>`.  Positive error codes are system errors (see
`errno(3) <https://man7.org/linux/man-pages/man3/errno.3.html>`_\ ), negative values are GEOPM errors.  If zero is specified
for the error code, ``GEOPM_ERROR_RUNTIME`` (-1) is assumed.

Functions
---------


* ``exception_handler()``:
  Handle a thrown exception pointed to by *eptr* and return an error
  value.  This exception handler is used by every GEOPM **C** interface
  to handle any exceptions that are thrown during execution of a **C++**
  implementation.  If GEOPM has been configured with debugging
  enabled and *do_print* is true, then this handler will print an
  explanatory message to standard error.  In all cases it will
  convert the **C++** exception into an error number which can be used
  with ``geopm_error_message()`` to obtain an error message.  Note that
  the error message printed when debugging is enabled has more
  specific information than the message produced by
  ``geopm_error_message()``.

Class Methods
-------------


*
  ``Exception()``:
  Empty constructor.  Uses ``errno`` to determine the error code.
  If errno is zero then ``GEOPM_ERROR_RUNTIME`` (-1) is used for the error code.
  Enables an abbreviated ``what()`` result.

*
  ``Exception(const string &what, int err, const char *file, int line)``:
  Message, error number, file and line constructor.  User provides
  message *what*\ , error code *err*\ , and location where the exception
  was thrown.  The *file* name and *line* number may come from
  preprocessor macros ``__FILE__`` and ``__LINE__`` respectively.  The
  ``what()`` method appends the user specified message, file name and
  line number to the abbreviated message.  This is the most verbose
  messaging available with the Exception class.

*
  ``err_value()``:
  Returns the non-zero error code associated with the
  exception.  Negative error codes are GEOPM-specific
  and documented in the :doc:`geopm_error(3) <geopm_error.3>` man page.
  Positive error codes are system errors and are
  documented in the system `errno(3) <https://man7.org/linux/man-pages/man3/errno.3.html>`_ man page.  A brief
  description of all error codes can be obtained with
  the ``geopm_error_message()`` interface.

See Also
--------

:doc:`geopm(7) <geopm.7>`\ ,
:doc:`geopm_error(3) <geopm_error.3>`\ ,
`errno(3) <https://man7.org/linux/man-pages/man3/errno.3.html>`_
