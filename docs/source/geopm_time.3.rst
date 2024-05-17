.. role:: raw-html-m2r(raw)
   :format: html


geopm_time(3) -- helper methods for time
==========================================






Synopsis
--------

#include `<geopm_time.h> <https://github.com/geopm/geopm/blob/dev/libgeopmd/include/geopm_time.h>`_

Link with ``-lgeopm`` **(MPI)** or ``-lgeopm`` **(non-MPI)**


.. code-block:: c

       static inline int geopm_time_string(int buf_size,
                                           char *buf);

       static inline int geopm_time(struct geopm_time_s *time);

       static inline double geopm_time_diff(const struct geopm_time_s *begin,
                                            const struct geopm_time_s *end);

       static inline bool geopm_time_comp(const struct geopm_time_s *aa,
                                          const struct geopm_time_s *bb);

       static inline void geopm_time_add(const struct geopm_time_s *begin,
                                         double elapsed,
                                         struct geopm_time_s *end);

       static inline double geopm_time_since(const struct geopm_time_s *begin);

       int geopm_time_zero(struct geopm_time_s *zero_time);

Description
-----------

The `geopm_time.h <https://github.com/geopm/geopm/blob/dev/libgeopmd/include/geopm_time.h>`_ header defines GEOPM interfaces for measuring time
in seconds relative to a fixed arbitrary reference point. The ``geopm_time_s``
structure is used to hold time values.


``geopm_time_string()``
  Fills *buf* with the current date and time as a string.  The
  string will be null-terminated and truncated to *buf_size*, which
  must be at least 26 characters as required by `asctime_r(3) <https://man7.org/linux/man-pages/man3/asctime_r.3.html>`_.

``geopm_time()``
  Sets *time* to the current time.

``geopm_time_diff()``
  Returns the difference in seconds between *begin* and *end*.

``geopm_time_comp()``
  Return true if *aa* is less than *bb*.

``geopm_time_add()``
  Sets *end* to *elapsed* seconds after *begin*.

``geopm_time_since()``
  Returns the number of seconds elapsed between the current time and *begin*.

Structure Type
--------------


This structure is part of the global **C** namespace.
This structure is used to abstract the ``timespec`` on Linux from other representations of time.

The field ``struct timespec t`` is a **POSIX.1b** structure for a time value.
This is like a ``struct timeval`` but has *nanoseconds* instead of *microseconds*.

.. code-block:: c

       struct geopm_time_s {
           struct timespec t;
       };

Singleton Accessor
------------------


``geopm_time_zero()``
  Is the accessor for the ``TimeZero`` singleton.
  It records the earliest time request for the process.

See Also
--------

:doc:`geopm(7) <geopm.7>`\ ,
`asctime_r(3) <https://man7.org/linux/man-pages/man3/asctime_r.3.html>`_
